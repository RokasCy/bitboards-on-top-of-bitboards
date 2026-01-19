#pragma once
#include<array>
#include<cstdint>
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<stack>

#include"move_generator.h"

using U64 = uint64_t;

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

enum Color { WHITE=0, BLACK=1 };
enum UniquePiece { PAWN=0, KNIGHT=1, BISHOP=2, ROOK=3, QUEEN=4, KING=5 };

const int CHECKMATE = 1'000'000;

extern const int pst[2][6][64];
enum Phases {mid_game=0, end_game=1};


struct Board {
    U64 bitboard[2][6];
    U64 occupancy[2];
    U64 all_pieces;

    //check handling
    U64 attacks[2]{};
    U64 pinned_bitboard[2]{}, checking_bitboard[2]{}, check_rays_bitboard[2]{};
    std::unordered_map<int, U64> pin_ray_bitboard[2]{};

    bool white_in_check=false, black_in_check=false;
    std::array<bool, 2> CHECKMATED = {false, false};

    //special moves
    //[colour][king_moved, king_rook_moved, queen_rook_moved]
    std::array<std::array<bool, 3>, 2> castling_rights{ { {true, true, true}, {true, true, true} } };
    U64 king_side_gaps[2] = {U64((1ULL << 5) | (1ULL << 6)), U64((1ULL << 61) | (1ULL << 62))};
    U64 queen_side_gaps[2] = {U64((1ULL << 1) | (1ULL << 2) | (1ULL << 3)), U64((1ULL << 57) | (1ULL << 58) | (1ULL << 59))};

    std::array<int,64> squares{};
    std::stack<Undo> history{};

    std::unordered_map<int, U64> pawn_lookup;
    std::unordered_map<int, U64> knight_lookup;
    std::unordered_map<int, U64> king_lookup;

    Board();
    void set_up_board();
    void player_move(Move& move);
    void make_move(Move& move);
    void undo_move();
    std::vector<Move> generate_piece_moves(int piece, int from);
    std::vector<Move> generate_all_moves(int side);
    std::vector<Move> generate_legal_moves(std::vector<Move> &pseudo_moves, int side, bool checkmate_check);
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
    void get_flags(Move &move, int colour);
    void castle_update(Move &move, int colour, bool undo);
    void castle_right_update(Move &move, int colour);

    void promotion(Move &move, int colour, bool undo);

    void knight_move_init();
    void king_move_init();
};


