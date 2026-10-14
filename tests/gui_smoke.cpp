#include <GlobalConnexion.hpp>
#include <atomic>
#include <chrono>
#include <connexion.hpp>
#include <engine/SFMLEngine.hpp>
#include <filesystem>
#include <functional>
#include <game/modes/ClassicGUI.hpp>
#include <game/modes/DuelGUI.hpp>
#include <game/modes/EndlessGUI.hpp>
#include <game/modes/RoyalCompetGUI.hpp>
#include <gestion_serveur/serveur.hpp>
#include <iostream>
#include <menu/Menu.hpp>
#include <menu/Room.hpp>
#include <thread>
#include <unistd.h>
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
void spin(const std::function<bool()> &done, const std::function<void()> &update) {
    auto until = Clock::now() + 3s;
    while (!done() && Clock::now() < until) {
        update();
        std::this_thread::sleep_for(2ms);
    }
    require(done(), "GUI response timed out");
}
void drain(tetris::Client &c) {
    for (int i = 0; i < 300; ++i)
        if (!c.recepteur())
            break;
}
int main() {
    try {
        char name[] = "/tmp/tetris-gui-XXXXXX";
        auto dir = ::mkdtemp(name);
        require(dir, "temporary directory");
        struct Cleanup {
            std::filesystem::path path;
            ~Cleanup() {
                std::error_code e;
                std::filesystem::remove_all(path, e);
            }
        } cleanup{dir};
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        require(::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0,
                "bind test port");
        socklen_t len = sizeof(addr);
        ::getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len);
        ::close(fd);
        tetris::serveur::ServerConfig config;
        config.port = ntohs(addr.sin_port);
        config.discovery_port = 0;
        config.database_path = (cleanup.path / "accounts.json").string();
        tetris::serveur::Serveur server(config);
        std::atomic<bool> stop = false;
        std::jthread thread([&] { server.run(stop); });
        struct Stop {
            std::atomic<bool> &flag;
            ~Stop() {
                flag = true;
            }
        } stopOnExit{stop};
        auto port = std::to_string(config.port);
        ::setenv("TETRIS_PORT", port.c_str(), 1);
        ::setenv("TETRIS_SERVER", "127.0.0.1", 1);
        tetris::gui::SFMLEngine engine(800, 600, 60, "Tetris automated verification");
        Menu menu(engine, engine.getWindow());
        engine.setMenu(&menu);
        for (int i = 0; i < 5; ++i) {
            engine.clear();
            menu.update(0.016f);
            menu.render();
            engine.display();
        }
        tetris::gui::EndlessGUI solo(engine, engine.getWindow());
        solo.start();
        for (int i = 0; i < 5; ++i) {
            solo.update(0.016f);
            solo.render();
            engine.display();
        }
        solo.stop();
        require(solo.wantsToReturnToMenu(), "solo stop");
        auto &client = tetris::GlobalConnection::getInstance();
        tetris::Client peer("127.0.0.1");
        require(client.connect_create("gui_a", "synthetic-only-password") &&
                    peer.connect_create("gui_b", "synthetic-only-password"),
                "GUI client auth");
        std::vector<std::unique_ptr<tetris::Client>> extraPlayers;
        for (int i = 0; i < 7; ++i) {
            auto extra = std::make_unique<tetris::Client>("127.0.0.1");
            require(
                extra->connect_create("gui_extra_" + std::to_string(i), "synthetic-only-password"),
                "additional GUI client auth");
            extraPlayers.push_back(std::move(extra));
        }
        auto verify = [&]<class View>(tetris::GameMode mode, const char *roomName) {
            tetris::gui::Room room;
            require(room.createRoom(roomName, mode), "GUI create room");
            spin([&] { return room.getPlayerCount() == 1; }, [&] { room.update(); });
            peer.join_room(roomName);
            spin([&] { return room.getPlayerCount() == 2; },
                 [&] {
                     room.update();
                     peer.recepteur();
                 });
            const bool fullRoom = mode == tetris::GameMode::Classique;
            if (fullRoom) {
                for (int i = 0; i < 7; ++i) {
                    extraPlayers[i]->join_room(roomName);
                    spin([&] { return room.getPlayerCount() == i + 3; },
                         [&] {
                             room.update();
                             peer.recepteur();
                             for (auto &extra : extraPlayers)
                                 extra->recepteur();
                         });
                }
                require(room.getPlayerCount() == 9, "GUI room supports all nine players");
            }
            require(room.getGameMode() == mode && room.isHost("gui_a"), "GUI room mode and owner");
            require(room.startGame(mode), "GUI start request");
            spin([&] { return room.shouldStartGame(); },
                 [&] {
                     room.update();
                     peer.recepteur();
                 });
            View view(engine, engine.getWindow());
            view.start();
            for (int i = 0; i < 20; ++i) {
                view.update(0.016f);
                view.render();
                engine.display();
                peer.recepteur();
                if (fullRoom)
                    for (auto &extra : extraPlayers)
                        extra->recepteur();
                require(!view.wantsToReturnToMenu(), "GUI remains active until match end");
            }
            if (const char *output = std::getenv("TETRIS_TEST_SCREENSHOT")) {
                sf::Texture shot(engine.getWindow()->getSize());
                shot.update(*engine.getWindow());
                require(
                    shot.copyToImage().saveToFile(std::string(output) + "-" + roomName + ".png"),
                    "save test screenshot");
            }
            if (fullRoom)
                for (auto &extra : extraPlayers)
                    extra->send_input('q');
            peer.send_input('q');
            spin([&] { return view.wantsToReturnToMenu(); },
                 [&] {
                     view.update(0.016f);
                     peer.recepteur();
                 });
            if (fullRoom)
                for (auto &extra : extraPlayers) {
                    extra->change_status(tetris::GameMode::Menu);
                    drain(*extra);
                }
            room.leaveRoom();
            peer.change_status(tetris::GameMode::Menu);
            std::this_thread::sleep_for(50ms);
            drain(client);
            drain(peer);
        };
        verify.template operator()<tetris::gui::ClassicGUI>(tetris::GameMode::Classique, "classic");
        verify.template operator()<tetris::gui::DuelGUI>(tetris::GameMode::Duel, "duel");
        verify.template operator()<tetris::gui::RoyalCompetitionGUI>(tetris::GameMode::BR, "royal");
        client.exit();
        client.send_message("gui_b", "synthetic disconnected-send regression");
        require(!client.isConnected(), "sending after disconnect stays contained");
        stop = true;
        thread.join();
        engine.getWindow()->close();
        std::cout << "GUI smoke: menu, solo, three network modes and match returns passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "GUI FAIL: " << e.what() << '\n';
        return 1;
    }
}
