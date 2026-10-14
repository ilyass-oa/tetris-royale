#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <connexion.hpp>
#include <cstdlib>
#include <database/database_manager.hpp>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <game/TetrisGame.hpp>
#include <game/network_board.hpp>
#include <gestion_serveur/serveur.hpp>
#include <iostream>
#include <packet_read.hpp>
#include <packets_include.hpp>
#include <poll.h>
#include <sys/stat.h>
#include <thread>
#include <transport.hpp>
#include <unistd.h>
#include <vector>
#include <write.hpp>
using namespace tetris;
using namespace std::chrono_literals;
namespace cb = tetris::clientbound_packets;
namespace sb = tetris::serverbound_packets;
using Clock = std::chrono::steady_clock;
int checks = 0;
void check(bool ok, const std::string &message) {
    ++checks;
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> void rejects(F f, const std::string &message) {
    bool threw = false;
    try {
        f();
    } catch (const std::exception &) {
        threw = true;
    }
    check(threw, message);
}
struct Temp {
    std::filesystem::path path;
    Temp() {
        char name[] = "/tmp/tetris-test-XXXXXX";
        auto dir = ::mkdtemp(name);
        if (!dir)
            throw std::runtime_error("mkdtemp");
        path = dir;
    }
    ~Temp() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};
struct Pair {
    int fd[2];
    Pair() {
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fd))
            throw std::runtime_error("socketpair");
    }
    ~Pair() {
        for (int s : fd) {
            transport::forget_socket(s);
            if (s >= 0)
                ::close(s);
        }
    }
    void closePeer() {
        transport::forget_socket(fd[1]);
        ::close(fd[1]);
        fd[1] = -1;
    }
};
void game() {
    Grid grid;
    grid.newGrid({{0, 8}});
    check(grid.isOccupied({0, 0}) && grid.getTileColor({0, 0}) == CYAN, "cyan cells are occupied");
    check(!grid.isOccupied({1, 0}), "empty cells remain empty");
    grid.setOccupied({0, 0}, true);
    grid.moveLinesDown(19);
    check(!grid.isOccupied({0, 0}) && grid.isOccupied({0, 1}), "line removal clears top row");
    rejects([&] { grid.isOccupied({-1, 0}); }, "negative coordinates rejected");
    rejects([&] { grid.getTileColor({10, 20}); }, "color bounds checked");
    grid.addGrayLines(-20);
    grid.addGrayLines(100);
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        int occupied = 0;
        for (int x = 0; x < BOARD_WIDTH; ++x)
            occupied += grid.isOccupied({x, y});
        check(occupied == 9, "garbage lines retain one empty cell");
    }
    Piece offBoard(I, {-20, -5});
    check(offBoard.collidesWith(grid), "out of board collision is safe");
    rejects([] { Piece invalid(static_cast<PieceType>(999), {0, 0}); }, "invalid piece rejected");
    TetrisGame state;
    state.getGrid()->setOccupied({0, 0}, true);
    state.energy = 10;
    state.score = 1000;
    state.totalLinesCleared = 25;
    state.end();
    state.reset();
    check(state.getRunning() && state.score == 0 && state.energy == 0 &&
              state.totalLinesCleared == 0 && !state.isOccupied({0, 0}),
          "reset clears complete game state");
    for (int x = 0; x < BOARD_WIDTH; ++x) {
        state.getGrid()->setOccupied({x, 19}, true);
        state.getGrid()->setTileColor({x, 19}, CYAN);
    }
    check(state.checkForLines() == 1 && state.getScore() == 100, "clearing line scores once");
    check(state.checkForLines() == 0 && !state.isOccupied({0, 0}), "line clearing is not repeated");
    for (int round = 0; round < 30; ++round) {
        state.reset();
        for (int i = 0; i < 100 && state.getRunning(); ++i) {
            state.moveCurrentPiece(-20, 0);
            state.rotateCurrentPieceClockwise();
            state.moveCurrentPiece(20, 0);
            state.rotateCurrentPieceCounterclockwise();
            state.rotateCurrentPiece180();
            state.hardDrop();
            state.checkForLines();
        }
        check(!state.getRunning(), "stack eventually ends game safely");
    }
}
void transportTests() {
    std::vector<std::vector<int>> board(20, std::vector<int>(10, 8));
    board[0][0] = board[0][1] = board[1][1] = board[1][2] = 4;
    check(!isEliminatedNetworkBoard(board), "red tetromino does not eliminate its live player");
    board.assign(20, std::vector<int>(10, 4));
    check(isEliminatedNetworkBoard(board), "server full-red elimination marker recognised");
    board.assign(20, std::vector<int>(10, 8));
    check(!isEliminatedNetworkBoard(board), "blackout board does not eliminate its player");

    Pair wire;
    sb::Login("demo_user", "demo-password").send(wire.fd[0]);
    std::vector<unsigned char> bytes(256);
    auto size = ::recv(wire.fd[1], bytes.data(), bytes.size(), 0);
    check(size > 0, "serialized credentials");
    bytes.resize(size);
    check(bytes[0] == 0 && bytes[3] == static_cast<int>(ServerboundPacket::Login),
          "wire integers use big endian");
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        transport::send_all(wire.fd[0], &bytes[i], 1);
        auto packet = read_serverbound_packet(wire.fd[1], 0);
        if (i + 1 < bytes.size())
            check(!packet, "partial packets are retained");
        else {
            auto login = std::dynamic_pointer_cast<sb::Login>(packet);
            check(login && login->username == "demo_user" && login->password == "demo-password",
                  "fragmented packet decoded exactly once");
        }
    }
    sb::Hello(false).send(wire.fd[0]);
    sb::Login("x", "y").send(wire.fd[0]);
    check(bool(std::dynamic_pointer_cast<sb::Hello>(read_serverbound_packet(wire.fd[1], 100))),
          "coalesced first packet");
    check(bool(std::dynamic_pointer_cast<sb::Login>(read_serverbound_packet(wire.fd[1], 100))),
          "coalesced second packet");
    {
        Pair closing;
        transport::send_all(closing.fd[0], bytes.data(), 3);
        check(!read_serverbound_packet(closing.fd[1], 0), "closing peer prefix retained");
        std::jthread sender([&] {
            std::this_thread::sleep_for(20ms);
            transport::send_all(closing.fd[0], bytes.data() + 3, bytes.size() - 3);
            ::shutdown(closing.fd[0], SHUT_RDWR);
        });
        auto last =
            std::dynamic_pointer_cast<sb::Login>(read_serverbound_packet(closing.fd[1], 500));
        check(last && last->username == "demo_user" && last->password == "demo-password",
              "POLLIN with POLLHUP delivers the final complete packet");
        rejects([&] { read_serverbound_packet(closing.fd[1], 100); },
                "EOF follows the final packet");
    }
    cb::Malus(3, 7).send(wire.fd[0]);
    auto effect = std::dynamic_pointer_cast<cb::Malus>(read_clientbound_packet(wire.fd[1], 100));
    check(effect && effect->malus == 3 && effect->player == 7,
          "effect and target survive round trip");
    cb::RoomInfos(2, "room", true, GameMode::Duel, "demo_user").send(wire.fd[0]);
    auto info = std::dynamic_pointer_cast<cb::RoomInfos>(read_clientbound_packet(wire.fd[1], 100));
    check(info && info->owner == "demo_user" && info->mode == GameMode::Duel,
          "room ownership and mode round trip");
    {
        Pair bad;
        write(bad.fd[0], static_cast<ServerboundPacket>(9999));
        rejects([&] { read_serverbound_packet(bad.fd[1], 100); }, "unknown packet rejected");
    }
    {
        Pair bad;
        write(bad.fd[0], ServerboundPacket::Hello);
        unsigned char invalid = 2;
        transport::send_all(bad.fd[0], &invalid, 1);
        rejects([&] { read_serverbound_packet(bad.fd[1], 100); }, "invalid boolean rejected");
    }
    {
        Pair bad;
        write(bad.fd[0], ServerboundPacket::Login);
        write(bad.fd[0], std::size_t(transport::kMaxStringBytes + 1));
        rejects([&] { read_serverbound_packet(bad.fd[1], 100); },
                "oversized string rejected before allocation");
    }
    {
        Pair bad;
        write(bad.fd[0], ServerboundPacket::Input);
        unsigned char byte = 0;
        transport::send_all(bad.fd[0], &byte, 1);
        auto before = Clock::now();
        rejects([&] { read_serverbound_packet(bad.fd[1], 50); },
                "partial packet read has deadline");
        check(Clock::now() - before < 500ms, "read deadline bounded");
    }
    {
        Pair gone;
        gone.closePeer();
        rejects([&] { sb::Hello(false).send(gone.fd[0]); },
                "closed socket raises instead of SIGPIPE");
    }
    {
        Pair blocked;
        int size = 4096;
        setsockopt(blocked.fd[0], SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
        std::vector<char> data(1024 * 1024, 'a');
        auto before = Clock::now();
        rejects([&] { transport::send_all(blocked.fd[0], data.data(), data.size(), 60); },
                "blocked writes stop");
        check(Clock::now() - before < 500ms, "write deadline bounded");
    }
    cb::ScoreResponse(123).send(wire.fd[0]);
    auto score =
        std::dynamic_pointer_cast<cb::ScoreResponse>(read_clientbound_packet(wire.fd[1], 100));
    check(score && score->score == 123, "score response consumes only payload");
    cb::GetMyRequests({"demo_friend"}).send(wire.fd[0]);
    auto requests =
        std::dynamic_pointer_cast<cb::GetMyRequests>(read_clientbound_packet(wire.fd[1], 100));
    check(requests && requests->friendRequests == std::vector<std::string>{"demo_friend"},
          "JSON responses carry their packet ID");
    check(!read_serverbound_packet(wire.fd[1], 0), "idle poll does not block");
}
void database() {
    Temp temp;
    auto file = temp.path / "accounts.json";
    DatabaseManager data(file.string());
    check(!data.addUser("../bad", "x") && !data.addUser("", "x") && !data.addUser("demo", ""),
          "invalid credentials rejected");
    check(data.addUser("demo_a", "unique-password") && data.addUser("demo_b", "unique-password"),
          "accounts created");
    check(!data.addUser("demo_a", "new") && !data.verifyUser("demo_a", "wrong") &&
              data.verifyUser("demo_a", "unique-password"),
          "duplicate and password validation");
    std::ifstream input(file);
    nlohmann::json stored;
    input >> stored;
    check(!stored["demo_a"].contains("mot_de_passe") && !stored["demo_a"].contains("password") &&
              stored["demo_a"]["password_hash"] != stored["demo_b"]["password_hash"],
          "passwords are salted and not plaintext");
    struct stat status{};
    ::stat(file.c_str(), &status);
    check((status.st_mode & 0777) == 0600, "account file permission 0600");
    std::string target = "demo_b";
    data.acceptFriendRequest("demo_b", "demo_a");
    check(data.getFriendsList("demo_b").empty(), "accept requires request");
    data.sendFriendRequest("demo_a", target);
    data.sendFriendRequest("demo_a", target);
    check(data.getFriendRequests(target) == std::vector<std::string>{"demo_a"},
          "requests de-duplicated");
    data.acceptFriendRequest(target, "demo_a");
    check(data.getFriendsList(target) == std::vector<std::string>{"demo_a"} &&
              data.getFriendRequests(target).empty(),
          "accept makes friendship");
    data.removeFriend("demo_a", target);
    check(data.getFriendsList(target).empty() && data.getFriendsList("demo_a").empty(),
          "remove bilateral");
    std::vector<std::jthread> tasks;
    std::atomic<bool> failed = false;
    for (int i = 0; i < 6; ++i)
        tasks.emplace_back([&, i] {
            try {
                auto name = "parallel_" + std::to_string(i);
                if (!data.addUser(name, "test-password"))
                    failed = true;
                data.saveScore(name, 100 + i);
                data.saveScore(name, 5);
            } catch (...) {
                failed = true;
            }
        });
    tasks.clear();
    check(!failed, "concurrent updates succeed");
    DatabaseManager reload(file.string());
    for (int i = 0; i < 6; ++i)
        check(reload.getUserScore("parallel_" + std::to_string(i)) == 100 + i,
              "concurrent changes persisted");
    check(reload.getTopScores().front() == "parallel_5 : 105", "score order deterministic");
    auto legacy = temp.path / "legacy.json";
    {
        std::ofstream out(legacy);
        out << R"({"demo":{"mot_de_passe":"secret"}})";
    }
    rejects([&] { DatabaseManager old(legacy.string()); },
            "legacy database refused without migration");
    check(std::filesystem::file_size(legacy) > 0, "legacy file preserved");
}
unsigned short freePort(int type = SOCK_STREAM) {
    int fd = ::socket(AF_INET, type, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)))
        throw std::runtime_error("free port bind");
    socklen_t size = sizeof(addr);
    ::getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &size);
    ::close(fd);
    return ntohs(addr.sin_port);
}
template <class T>
std::shared_ptr<T> await(Client &client, std::chrono::milliseconds timeout = 2000ms) {
    auto deadline = Clock::now() + timeout;
    while (Clock::now() < deadline) {
        auto packet = client.recepteur();
        if (auto expected = std::dynamic_pointer_cast<T>(packet))
            return expected;
        if (!client.isConnected())
            throw std::runtime_error("disconnected: " + client.getLastError());
        std::this_thread::sleep_for(2ms);
    }
    throw std::runtime_error("timed out waiting for response");
}
void drain(Client &client) {
    for (int i = 0; i < 256; ++i)
        if (!client.recepteur())
            break;
}
void integration() {
    Temp temp;
    tetris::serveur::ServerConfig config;
    config.port = freePort();
    config.discovery_port = freePort(SOCK_DGRAM);
    config.database_path = (temp.path / "accounts.json").string();
    tetris::serveur::Serveur server(config);
    std::atomic<bool> stop = false;
    std::exception_ptr failure;
    std::jthread runner([&] {
        try {
            server.run(stop);
        } catch (...) {
            failure = std::current_exception();
        }
    });
    struct Stop {
        std::atomic<bool> &flag;
        ~Stop() {
            flag = true;
        }
    } stopOnExit{stop};
    std::string port = std::to_string(config.port);
    ::setenv("TETRIS_PORT", port.c_str(), 1);
    Client a("127.0.0.1"), b("127.0.0.1"), c("127.0.0.1");
    check(a.connect_create("demo_a", "network-password") &&
              b.connect_create("demo_b", "network-password") &&
              c.connect_create("demo_c", "network-password"),
          "three real clients authenticate");
    Client wrong("127.0.0.1");
    check(!wrong.connect_log("demo_a", "bad"), "wrong password rejected");
    check(!wrong.connect_log("demo_a", "network-password"), "duplicate live session rejected");
    check(a.request_score() == 0 && a.isConnected(), "existing session survives duplicate login");
    check(!wrong.connect_create("demo_a", "network-password") &&
              wrong.connect_create("demo_d", "network-password"),
          "retry after rejected signup reconnects");
    a.sendRequest("demo_b");
    a.request_score();
    auto requests = b.handle_my_requests();
    check(requests == std::vector<std::string>{"demo_a"},
          "friend request round trip: " + b.getLastError());
    b.gestion_friend("demo_a", true);
    b.request_score();
    check(a.friends_list() == std::vector<std::string>{"demo_b"}, "friend response round trip");
    a.send_score(125);
    check(a.request_score() == 0 && a.isConnected() && !a.top_scores_list().empty(),
          "scores queried and forged client score ignored");
    for (auto mode : {GameMode::Endless, GameMode::Classique, GameMode::Duel, GameMode::BR}) {
        const auto room = "test_" + std::to_string(static_cast<int>(mode));
        int limit = mode == GameMode::Endless ? 1 : 2;
        a.create_room(limit, room, mode);
        auto created = await<cb::RoomInfos>(a);
        check(created->reponse && created->owner == "demo_a" && created->mode == mode,
              "room created with authoritative metadata");
        if (mode != GameMode::Endless) {
            a.start_game(mode);
            auto denied = await<cb::RoomInfos>(a);
            check(denied->reponse && denied->numb_player == 1,
                  "insufficient players do not remove membership");
            b.join_room(room);
            check(await<cb::RoomInfos>(b)->reponse, "second player joins");
            check(await<cb::RoomInfos>(a)->numb_player == 2, "owner gets membership update");
            c.join_room(room);
            check(!await<cb::RoomInfos>(c)->reponse, "full room rejects third player");
            b.start_game(mode);
            check(await<cb::RoomInfos>(b)->reponse,
                  "non-owner cannot start but retains membership");
        }
        a.start_game(mode);
        await<cb::GameStart>(a);
        if (mode != GameMode::Endless)
            await<cb::GameStart>(b);
        auto grid = await<cb::GridData>(a);
        check(grid->grid.size() == 20 && grid->grid.front().size() == 10,
              "mode streams valid board");
        for (int i = 0; i < 10; ++i) {
            a.send_input('x');
            a.send_input(' ');
        }
        a.send_input('q');
        await<cb::GameEnd>(a);
        if (mode != GameMode::Endless)
            await<cb::GameEnd>(b);
        a.change_status(GameMode::Menu);
        if (mode != GameMode::Endless)
            b.change_status(GameMode::Menu);
        std::this_thread::sleep_for(30ms);
        drain(a);
        drain(b);
    }
    a.create_room(2, "handover", GameMode::Duel);
    await<cb::RoomInfos>(a);
    b.join_room("handover");
    await<cb::RoomInfos>(b);
    await<cb::RoomInfos>(a);
    a.change_status(GameMode::Menu);
    check(await<cb::RoomInfos>(b)->owner == "demo_b", "owner departure transfers ownership");
    c.join_room("handover");
    await<cb::RoomInfos>(c);
    await<cb::RoomInfos>(b);
    b.start_game(GameMode::Duel);
    await<cb::GameStart>(b);
    await<cb::GameStart>(c);
    await<cb::GridData>(b);
    c.exit();
    await<cb::GameEnd>(b);
    check(b.isConnected(), "disconnect in match ends surviving player cleanly");
    b.change_status(GameMode::Menu);
    std::this_thread::sleep_for(30ms);
    drain(b);
    // Corrupt and fragmented sessions must not stop the listening server.
    {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(config.port);
        ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        check(!::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)),
              "malformed test connects");
        write(fd, static_cast<ServerboundPacket>(999));
        ::close(fd);
    }
    Client healthy("127.0.0.1");
    check(healthy.connect_create("after_bad", "network-password"),
          "server accepts after malformed peer");
    stop = true;
    auto before = Clock::now();
    runner.join();
    check(Clock::now() - before < 1500ms, "shutdown joins active sessions promptly");
    if (failure)
        std::rethrow_exception(failure);
}
int main(int argc, char **argv) {
    try {
        std::string suite = argc > 1 ? argv[1] : "";
        if (suite == "game")
            game();
        else if (suite == "transport")
            transportTests();
        else if (suite == "database")
            database();
        else if (suite == "integration")
            integration();
        else
            throw std::runtime_error("unknown suite");
        std::cout << suite << ": " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL after " << checks << " checks: " << e.what() << '\n';
        return 1;
    }
}
