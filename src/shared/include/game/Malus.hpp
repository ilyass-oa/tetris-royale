#ifndef MALUS_HPP
#define MALUS_HPP

namespace tetris {
enum class Malus {
    NO,
    ONE_LINE,
    TWO_LINES,
    FOUR_LINES,
    INVERT_CONTROLS,
    BLOCK_CONTROLS,
    SLOW_DOWN,
    SPEED_UP,
    ONE_BY_ONE_BLOCK,
    SHUT_DOWN,
    STRIKE
};

}

#endif