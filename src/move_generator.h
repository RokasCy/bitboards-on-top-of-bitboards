#pragma once
#include<cstdint>
#include<array>

#include"engine.h"

enum MoveFlags : uint8_t {
    QUIET        = 0,
    CAPTURE      = 1 << 0,
    EN_PASSANT   = 1 << 1,
    CASTLING     = 1 << 2,
    PROMOTION    = 1 << 3
};

enum Directions {
    UP = 8,
    DOWN = -8,
    LEFT = -1,
    RIGHT = 1,

};

struct Move {
    int from, to, flags;

    Move() = default;
    Move(int f, int t, int fl)
        : from(f), to(t), flags(fl) {}
};

struct Undo {
    Move move;
    int captured_piece;
    std::array<std::array<bool, 3>, 2> castling_rights;

    Undo(Move m, int cap, std::array<std::array<bool, 3>, 2>& cr) 
    :  move(m), captured_piece(cap), castling_rights(cr){}
};