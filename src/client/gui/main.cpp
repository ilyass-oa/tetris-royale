#include "engine/SFMLEngine.hpp"
#include "game/GameInterface.hpp"
#include "menu/Menu.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    try {
        int smokeFrames = 0;
        bool solo = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << "TetrisRoyale [--server IPv4] [--port PORT] [--smoke-test] [--solo]\n";
                return 0;
            }
            if (arg == "--smoke-test")
                smokeFrames = 30;
            else if (arg == "--solo")
                solo = true;
            else if ((arg == "--server" || arg == "--port") && i + 1 < argc) {
                auto value = argv[++i];
                ::setenv(arg == "--server" ? "TETRIS_SERVER" : "TETRIS_PORT", value, 1);
            } else
                throw std::runtime_error("Unknown or incomplete option: " + arg);
        }
        tetris::gui::SFMLEngine engine(800, 600, 60, "Tetris Royale");
        Menu menu(engine, engine.getWindow());
        tetris::gui::GameInterface gameInterface(engine, engine.getWindow());
        engine.setMenu(&menu);
        engine.setGameInterface(&gameInterface);
        engine.setAppMode(solo ? tetris::gui::AppMode::Game : tetris::gui::AppMode::Menu);
        sf::Clock clock;
        while (engine.isWindowOpen()) {
            float dt = clock.restart().asSeconds();
            engine.handleEvents();
            if (!engine.isWindowOpen())
                break;
            engine.clear();
            if (engine.getAppMode() == tetris::gui::AppMode::Menu) {
                menu.update(dt);
                menu.render();
                if (menu.wantsToStartGame()) {
                    engine.setAppMode(tetris::gui::AppMode::Game);
                    menu.setWantsToStartGame(false);
                }
            } else {
                gameInterface.update(dt);
                gameInterface.render();
                if (gameInterface.wantsToReturnToMenu()) {
                    engine.setAppMode(tetris::gui::AppMode::Menu);
                    gameInterface.reset();
                }
            }
            engine.display();
            if (smokeFrames > 0 && --smokeFrames == 0)
                engine.getWindow()->close();
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
