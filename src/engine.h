#pragma once
#include<array>
#include<cstdint>
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<stack>

using U64 = uint64_t;

enum MoveFlags : uint8_t {
    QUIET        = 0,
    CAPTURE      = 1 << 0,
    EN_PASSANT   = 1 << 1,
    CASTLING     = 1 << 2,
    PROMOTION    = 1 << 3
};

enum Piece {
    WHITE_PAWN = 0,
    WHITE_KNIGHT = 1,
    WHITE_BISHOP = 2,
    WHITE_ROOK = 3,
    WHITE_QUEEN = 4,
    WHITE_KING = 5,
    BLACK_PAWN = 6,
    BLACK_KNIGHT = 7,
    BLACK_BISHOP = 8,
    BLACK_ROOK = 9,
    BLACK_QUEEN = 10,
    BLACK_KING = 11,
    EMPTY = 12
};

enum Directions {
    UP = 8,
    DOWN = -8,
    LEFT = -1,
    RIGHT = 1,

};

enum Color { WHITE=0, BLACK=1 };
enum UniquePiece { PAWN=0, KNIGHT=1, BISHOP=2, ROOK=3, QUEEN=4, KING=5 };


struct Move {
    int from, to, flags;

    Move() = default;
    Move(int f, int t, int fl)
        : from(f), to(t), flags(fl) {}
};

struct Undo {
    Move move;
    int captured_piece;

    Undo(Move m, int cap) : move(m), captured_piece(cap) {}
};

struct Board {
    U64 bitboard[2][6];
    U64 occupancy[2];
    U64 all_pieces;

    U64 attacks[2]{};
    U64 pinned_bitboard[2]{}, checking_bitboard[2]{}, check_rays_bitboard[2]{};
    std::unordered_map<int, U64> pin_ray_bitboard[2]{};
    bool white_in_check=false, black_in_check=false;

    std::array<int,64> squares{};
    std::stack<Undo> history{};

    Board();
    void set_up_board();
    void player_move(Move& move);
    void make_move(Move& move);
    void undo_move();
    std::vector<Move> generate_piece_moves(int piece, int from);
    std::vector<Move> generate_all_moves(int side);
    std::vector<Move> generate_legal_moves(std::vector<Move> &pseudo_moves, int side);
    int evaluation();
    std::pair<Move, int> get_minimax_move(int side);
    int minimax_search(int depth, bool maximizing_player, int alpha, int beta);

    U64 pawn_attacks(int colour);
    U64 knight_attacks(int colour);
    U64 bishop_attacks(int colour);
    U64 rook_attacks(int colour);
    U64 queen_attacks(int colour);
    U64 king_attacks(int colour);
    void check_rays(int colour, int from, int r, int f, int dr, int df, U64 ray);

    void update_attack_info();
};







