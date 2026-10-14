#include "commun_class.hpp"
#include "../gestion_serveur/player.hpp"
#include <algorithm>
#include <chrono>
#include <key_codes.hpp>
#include <random>
#include <thread>
namespace tetris::serveur {
namespace {
using Clock = std::chrono::steady_clock;
struct Participant {
    std::string name;
    std::shared_ptr<Player> session;
    TetrisGame game;
    bool alive = true;
    Clock::time_point fall = Clock::now(), inverse{}, blocked{}, blackout{}, fast{}, slow{};
    int energy = 0;
};
std::vector<std::vector<int>> board(const Participant &player) {
    std::vector<std::vector<int>> result(BOARD_HEIGHT, std::vector<int>(BOARD_WIDTH, 8));
    if (!player.alive)
        return std::vector<std::vector<int>>(BOARD_HEIGHT, std::vector<int>(BOARD_WIDTH, 4));
    if (Clock::now() < player.blackout)
        return result;
    for (int y = 0; y < BOARD_HEIGHT; ++y)
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            Coordinate cell(x, y);
            if (player.game.isOccupied(cell))
                result[y][x] = player.game.getColor(cell);
            else if (player.game.containsCurrentPiece(cell))
                result[y][x] = player.game.getCurrentPieceColor();
        }
    return result;
}
void applyEffect(Participant &target, Malus effect) {
    auto until = Clock::now() + std::chrono::seconds(5);
    switch (effect) {
    case Malus::ONE_LINE:
        target.game.applyGrayLines(1);
        break;
    case Malus::TWO_LINES:
        target.game.applyGrayLines(2);
        break;
    case Malus::FOUR_LINES:
        target.game.applyGrayLines(4);
        break;
    case Malus::INVERT_CONTROLS:
        target.inverse = until;
        break;
    case Malus::BLOCK_CONTROLS:
        target.blocked = until;
        break;
    case Malus::SPEED_UP:
        target.fast = until;
        break;
    case Malus::SLOW_DOWN:
        target.slow = until;
        break;
    case Malus::SHUT_DOWN:
        target.blackout = until;
        break;
    case Malus::ONE_BY_ONE_BLOCK:
        target.game.oneByOneBlocks();
        break;
    case Malus::STRIKE:
        target.game.clearTopLines(4);
        break;
    default:
        break;
    }
}
int price(Malus effect) {
    switch (effect) {
    case Malus::INVERT_CONTROLS:
        return 3;
    case Malus::BLOCK_CONTROLS:
        return 2;
    case Malus::SPEED_UP:
        return 2;
    case Malus::SHUT_DOWN:
        return 4;
    case Malus::STRIKE:
        return 3;
    case Malus::SLOW_DOWN:
        return 2;
    case Malus::ONE_BY_ONE_BLOCK:
        return 2;
    default:
        return 0;
    }
}
} // namespace
void run_match(Room &room, const std::shared_ptr<PlayersManager> &players, GameMode mode) {
    std::vector<std::unique_ptr<Participant>> participants;
    auto names = room.get_deep_cpy_list();
    std::vector<std::string> ordered(names.begin(), names.end());
    std::sort(ordered.begin(), ordered.end());
    const auto roomName = room.get_name();
    for (const auto &name : ordered)
        if (auto session = players->get_object(name)) {
            auto entry = std::make_unique<Participant>();
            entry->name = name;
            entry->session = session;
            participants.push_back(std::move(entry));
        }
    std::mt19937 random(std::random_device{}());
    auto lastSend = Clock::now() - std::chrono::seconds(1);
    while (!room.is_cancelled()) {
        int survivors = 0;
        for (auto &ptr : participants) {
            auto &player = *ptr;
            if (!player.alive)
                continue;
            if (!player.session->is_playing_in(roomName)) {
                player.alive = false;
                continue;
            }
            auto now = Clock::now();
            for (int n = 0; n < 32; ++n) {
                int input = player.session->input_pop();
                if (input == '+')
                    break;
                if (input == 'q') {
                    player.game.end();
                    break;
                }
                if (now < player.blocked)
                    continue;
                int direction = now < player.inverse ? -1 : 1;
                switch (input) {
                case input::left:
                    player.game.moveCurrentPiece(-direction, 0);
                    break;
                case input::right:
                    player.game.moveCurrentPiece(direction, 0);
                    break;
                case input::down:
                    player.game.moveCurrentPiece(0, 1);
                    break;
                case 'z':
                    player.game.rotateCurrentPieceCounterclockwise();
                    break;
                case 'x':
                    player.game.rotateCurrentPieceClockwise();
                    break;
                case 'c':
                    player.game.rotateCurrentPiece180();
                    break;
                case ' ':
                    player.game.hardDrop();
                    break;
                }
            }
            int delay = std::max(100, 500 - (player.game.totalLinesCleared / 10) * 50);
            if (now < player.fast)
                delay = std::max(100, delay - 150);
            if (now < player.slow)
                delay += 150;
            if (now - player.fall >= std::chrono::milliseconds(delay)) {
                player.game.update();
                player.fall = now;
            }
            int lines = player.game.checkForLines();
            if (mode == GameMode::BR)
                player.energy += lines == 4 ? 5 : lines;
            if ((mode == GameMode::Classique || mode == GameMode::Duel) && lines >= 2) {
                std::vector<std::size_t> targets;
                for (std::size_t i = 0; i < participants.size(); ++i)
                    if (participants[i].get() != &player && participants[i]->alive)
                        targets.push_back(i);
                if (!targets.empty())
                    participants[targets[random() % targets.size()]]->game.applyGrayLines(
                        lines == 4 ? 4 : lines - 1);
            }
            if (mode == GameMode::BR) {
                auto [index, effect] = player.session->malus_to_other_pop();
                int cost = price(effect);
                bool bonus = effect == Malus::SLOW_DOWN || effect == Malus::ONE_BY_ONE_BLOCK ||
                             effect == Malus::STRIKE;
                if (cost > 0 && player.energy >= cost &&
                    ((bonus && index == 0) ||
                     (!bonus && index > 0 &&
                      static_cast<std::size_t>(index) <= participants.size()))) {
                    auto &target = index == 0 ? player : *participants[index - 1];
                    if (target.alive && (bonus || &target != &player)) {
                        applyEffect(target, effect);
                        player.energy -= cost;
                    }
                }
            }
            player.alive = player.game.getRunning();
            if (player.alive)
                ++survivors;
        }
        if (survivors == 0 || (mode != GameMode::Endless && survivors <= 1))
            break;
        if (Clock::now() - lastSend >= std::chrono::milliseconds(50)) {
            for (std::size_t index = 0; index < participants.size(); ++index) {
                auto &recipient = *participants[index];
                if (!recipient.session->is_playing_in(roomName))
                    continue;
                try {
                    recipient.session->send_packet<clientbound_packets::GridData>(0,
                                                                                  board(recipient));
                    recipient.session->send_packet<clientbound_packets::NextTile>(
                        recipient.game.getPieceFactory()->peekNextPiece());
                    recipient.session->send_packet<clientbound_packets::ScorePacket>(
                        recipient.game.getScore(), recipient.energy);
                    if (mode != GameMode::Endless)
                        for (std::size_t other = 0; other < participants.size(); ++other)
                            if (other != index)
                                recipient.session->send_packet<clientbound_packets::GridData>(
                                    static_cast<int>(other + 1), board(*participants[other]));
                } catch (const std::exception &) {
                    recipient.alive = false;
                }
            }
            lastSend = Clock::now();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    for (auto &ptr : participants) {
        auto &participant = *ptr;
        try {
            participant.session->save_score(participant.game.getScore());
        } catch (const std::exception &) {
        }
        if (participant.session->is_playing_in(roomName)) {
            participant.session->change_status(GameMode::Menu);
            try {
                participant.session->send_packet<clientbound_packets::GameEnd>(true);
            } catch (const std::exception &) {
            }
        }
    }
}
} // namespace tetris::serveur
