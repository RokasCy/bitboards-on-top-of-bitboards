#include"engine.h"
#include<iostream>
#include<unordered_set>
#include<cmath>
#include<cstdint>
#include<bitset>
#include<climits>
#include<algorithm>
#include<cstdlib>
#include<chrono>
#include<intrin.h>
#include<cassert>

//to do list: 
//add checkmate
//castling
//more moves
//more advanced evaluator

void print_bb(U64 &bb){
    for(int y=7; y>=0; y--){
        for(int x=0; x<8; x++){
            if (bb & (1ULL << x+y*8)) std::cout<<1;
            else std::cout<<0;
        }
        std::cout<<'\n';
    }
    std::cout<<'\n';
}

Board::Board() {

}

void Board::set_up_board(){

    bitboard[WHITE][PAWN] = 0b11111111ULL << 8;
    bitboard[WHITE][ROOK] = 0b10000001ULL;
    bitboard[WHITE][KNIGHT] = 0b01000010ULL;
    bitboard[WHITE][BISHOP] = 0b00100100ULL;
    bitboard[WHITE][QUEEN] = 0b00001000ULL;
    bitboard[WHITE][KING] = 0b00010000ULL;

    // Black pieces
    bitboard[BLACK][PAWN] = 0b11111111ULL << 48;
    bitboard[BLACK][ROOK] = 0b10000001ULL << 56;
    bitboard[BLACK][KNIGHT] = 0b01000010ULL << 56;
    bitboard[BLACK][BISHOP] = 0b00100100ULL << 56;
    bitboard[BLACK][QUEEN] = 0b00001000ULL << 56;
    bitboard[BLACK][KING] = 0b00010000ULL << 56;

    occupancy[WHITE] =
        bitboard[WHITE][PAWN]
        | bitboard[WHITE][KNIGHT]
        | bitboard[WHITE][BISHOP]
        | bitboard[WHITE][ROOK]
        | bitboard[WHITE][QUEEN]
        | bitboard[WHITE][KING];

    occupancy[BLACK] =
        bitboard[BLACK][PAWN]
        | bitboard[BLACK][KNIGHT]
        | bitboard[BLACK][BISHOP]
        | bitboard[BLACK][ROOK]
        | bitboard[BLACK][QUEEN]
        | bitboard[BLACK][KING];

    all_pieces = occupancy[WHITE] | occupancy[BLACK];
    squares.fill(12);
    for(int i=0; i<64; i++){

        for(int colour = WHITE; colour <= BLACK; colour++){
            for(int piece = PAWN; piece <= KING; piece++){
                if(bitboard[colour][piece] & (1ULL << i)){
                    
                    if (colour==WHITE) squares[i] = piece;
                    else squares[i] = piece+6;
                }
            }
        }
    }
    
}

void Board::player_move(Move& move){

    U64 from_mask = 1ULL << move.from;
    U64 to_mask = 1ULL << move.to;

    if (move.flags == CAPTURE){
        int captured = squares[move.to];
        int cap_colour;
        if (captured <= 5) cap_colour = WHITE; 
        else {cap_colour = BLACK; captured -= 6;}

        bitboard[cap_colour][captured] &= ~to_mask;
        occupancy[cap_colour] = (occupancy[cap_colour] & ~to_mask);
    }

    int piece = squares[move.from], colour;
    squares[move.from] = EMPTY;
    squares[move.to] = piece;

    if (piece <= 5 && piece >= 0) colour = WHITE; 
    else {colour = BLACK; piece -= 6;}

    // adding/removing piece from bitboards
    bitboard[colour][piece] = (bitboard[colour][piece] & ~from_mask) | to_mask;
    occupancy[colour] = (occupancy[colour] & ~from_mask) | to_mask;
    all_pieces = (all_pieces & ~from_mask) | to_mask; 

    update_attack_info();
}

void Board::make_move(Move& move){
    Undo u(move, squares[move.to]);
    history.push(u);

    U64 from_mask = 1ULL << move.from;
    U64 to_mask = 1ULL << move.to;

    if (move.flags & CAPTURE){
        int captured = squares[move.to];
        int cap_colour;
        if (captured <= 5) cap_colour = WHITE; 
        else {cap_colour = BLACK; captured -= 6;}

        bitboard[cap_colour][captured] &= ~to_mask;
        occupancy[cap_colour] = (occupancy[cap_colour] & ~to_mask);
    }

    int piece = squares[move.from], colour;
    squares[move.from] = EMPTY;
    squares[move.to] = piece;

    if (piece <= 5 && piece >= 0) colour = WHITE; 
    else {colour = BLACK; piece -= 6;}

    // adding/removing piece from bitboards
    bitboard[colour][piece] = (bitboard[colour][piece] & ~from_mask) | to_mask;
    occupancy[colour] = (occupancy[colour] & ~from_mask) | to_mask;
    all_pieces = (all_pieces & ~from_mask) | to_mask; 

    update_attack_info();
}

void Board::undo_move(){
    if(history.empty()) {return ;}

    Undo u = history.top();
    history.pop();

    Move move = u.move;

    U64 from_mask = 1ULL << move.from;
    U64 to_mask = 1ULL << move.to;

    int piece = squares[move.to], colour;
    squares[move.from] = piece;

    if (piece <= 5 && piece >= 0) {colour = WHITE;}
    else {colour = BLACK; piece -= 6;}

    // adding/removing piece from bitboards
    bitboard[colour][piece] = (bitboard[colour][piece] & ~to_mask) | from_mask;
    occupancy[colour] = (occupancy[colour] & ~to_mask) | from_mask;
    all_pieces = (all_pieces & ~to_mask) | from_mask; 
    

    if (move.flags & CAPTURE){
        int captured = u.captured_piece, cap_colour;
        if (captured <= 5) {cap_colour = WHITE;}
        else {cap_colour = BLACK; captured -= 6;}

        bitboard[cap_colour][captured] |= to_mask;
        occupancy[cap_colour] |= to_mask;
	    all_pieces |= to_mask;

	    squares[move.to] = u.captured_piece;
    } else {
        squares[move.to] = EMPTY;

    }

    update_attack_info();

}

int Board::evaluation(){
    static int pieceValue[5] = {100, 300, 300, 500, 900};

    int WHITE_VALUE = 0, BLACK_VALUE = 0;
    for (int piece=PAWN; piece<=QUEEN; piece++){
            WHITE_VALUE += __popcnt64(bitboard[WHITE][piece]) * pieceValue[piece];
            BLACK_VALUE += __popcnt64(bitboard[BLACK][piece]) * pieceValue[piece];
    }

    return WHITE_VALUE-BLACK_VALUE;
}

long long nodes=0;

std::pair<Move, int> Board::get_minimax_move(int side){
    using namespace std::chrono;

    nodes++;
    auto start = high_resolution_clock::now();

    int alpha = INT_MIN, beta = INT_MAX;

    int best_eval;
    Move best_move;

    if (side == WHITE){
        best_eval = INT_MIN;

        auto moves = generate_legal_moves(generate_all_moves(WHITE), WHITE);
        if (moves.empty())
            return {Move{}, evaluation()};

        for(auto m : moves){
            make_move(m);
            int eval = minimax_search(3, false, alpha, beta);

            if (best_eval < eval){
                best_eval = eval;
                best_move = m;
            }
            alpha = std::max(alpha, eval);
            undo_move();
        }
    }
    else {
        best_eval = INT_MAX;

        auto moves = generate_legal_moves(generate_all_moves(BLACK), BLACK);
        if (moves.empty())
            return {Move{}, evaluation()};

        for(auto m : moves){
            make_move(m);
            int eval = minimax_search(3, true, alpha, beta);

            if (best_eval > eval){
                best_eval = eval;
                best_move = m;
            }
            beta = std::min(beta, eval);
            undo_move();
        }
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    std::cout<<"nodes:"<<nodes<<'\n';
    std::cout<<"duration:"<<duration.count()<<"ms"<<"\n";
    std::cout<<nodes / (duration.count()/1000.0)<<" nodes/s"<<"\n\n";

    nodes=0;
    return std::make_pair(best_move, best_eval);
}

int Board::minimax_search(int depth, bool maximizing_player, int alpha, int beta){
    nodes++;
    if(depth == 0) 
        return evaluation();

    if (maximizing_player){
        auto moves = generate_legal_moves(generate_all_moves(WHITE), WHITE);
        if (moves.empty())
            return evaluation();

        int max_eval = INT_MIN;
        for(auto m : moves){
            make_move(m);

            int eval = minimax_search(depth - 1, false, alpha, beta);

            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            undo_move();

            //black already has a better move
            if (beta <= alpha)
                break;
        }
        return max_eval;
    }
    else {
        auto moves = generate_legal_moves(generate_all_moves(BLACK), BLACK);
        if (moves.empty())
            return evaluation();

        int min_eval = INT_MAX;
        for(auto m : moves){
            make_move(m);
            int eval = minimax_search(depth - 1, true, alpha, beta);

            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, eval);
            undo_move();

            //white already has a better move
            if (alpha >= beta)
                break;
        }
        return min_eval;
    }
}


//pop least significant bit
int pop_lsb(U64& bb){
    unsigned long index;
    _BitScanForward64(&index, bb); // find LSB
    bb &= bb - 1;             
    return static_cast<int>(index);
}

std::vector<Move> Board::generate_legal_moves(std::vector<Move> &pseudo_moves, int side){

    std::vector<Move> legal_moves;
    

    if (side == WHITE) {
        bool double_check = (__popcnt64(checking_bitboard[BLACK]) >= 2);

        for (auto m : pseudo_moves) {
            U64 from_mask = (1ULL << m.from);
            U64 to_mask = (1ULL << m.to);

            bool is_king = (bitboard[WHITE][KING] & from_mask);
            bool block_check = (check_rays_bitboard[BLACK] & to_mask);
            bool to_attacked = (attacks[BLACK] & to_mask);

            bool is_pinned = (pinned_bitboard[WHITE] & from_mask);
            bool in_pin_ray = (pin_ray_bitboard[WHITE][m.from] & to_mask);
            
            if (white_in_check) {
                if(double_check){
                    if(is_king && !to_attacked)
                        legal_moves.push_back(m);
                    continue;
                }

                // if move blocks check or captures checking piece
                if (!is_king && block_check) {
                    legal_moves.push_back(m);
                }

            }
            else {
                //for pinned pieces
                if (is_pinned) {
                    if (in_pin_ray){
                        legal_moves.push_back(m);
                    }
                }
                else {
                    legal_moves.push_back(m);
                }
            }

            if (is_king && !to_attacked) {
                legal_moves.push_back(m);
            }
        }
    }
    else if (side == BLACK) {
        bool double_check = (__popcnt64(checking_bitboard[WHITE]) >= 2);
        for (auto m : pseudo_moves) {
            U64 from_mask = (1ULL << m.from);
            U64 to_mask = (1ULL << m.to);

            bool is_king = (bitboard[BLACK][KING] & from_mask);
            bool block_check = (check_rays_bitboard[WHITE] & to_mask);
            bool to_attacked = (attacks[WHITE] & to_mask);

            bool is_pinned = (pinned_bitboard[BLACK] & from_mask);
            bool in_pin_ray = (pin_ray_bitboard[BLACK][m.from] & to_mask);
            
            if (black_in_check) {
                if(double_check){
                    if(is_king && !to_attacked)
                        legal_moves.push_back(m);
                    continue;
                }

                if (!is_king && !is_pinned && block_check) {
                    legal_moves.push_back(m);
                }

            }
            else {
                //for pinned pieces
                if (is_pinned) {
                    if (in_pin_ray){
                        legal_moves.push_back(m);
                    }
                }
                else {
                    legal_moves.push_back(m);
                }
            }
        

            if (is_king && !to_attacked) {
                legal_moves.push_back(m);
            }
        }
    }
    return legal_moves;
}

std::vector<Move> Board::generate_all_moves(int side) {
    std::vector<Move> moves;

    for(int piece=PAWN; piece<=KING; piece++){
        uint64_t bb = bitboard[side][piece]; 

        while(bb){
            int from = pop_lsb(bb);

            if(side == WHITE){
                for(auto m : generate_piece_moves(piece, from)){
                    moves.push_back(m);
                }
            }
            else {
                for(auto m : generate_piece_moves(piece+6, from)){
                    moves.push_back(m);
                }
            }

        }
    }

    return moves;
}

int fileA = 0, fileH = 7;
const std::array<int, 8> KNIGHT_MOVES = 
{2*UP+LEFT, 2*UP+RIGHT, 2*LEFT+UP, 2*RIGHT+UP, 
    2*LEFT+DOWN, 2*RIGHT+DOWN, 2*DOWN+LEFT, 2*DOWN+RIGHT};

const std::array<std::pair<int,int>,4> DIAGONALS = {
    std::make_pair(1,1),
    std::make_pair(-1,-1),  
    std::make_pair(1,-1), 
    std::make_pair(-1,1)   
};

const std::array<std::pair<int,int>,4> STRAIGHTS = {
    std::make_pair(1,0),
    std::make_pair(-1,0),  
    std::make_pair(0,-1), 
    std::make_pair(0,1)   
};

const std::array<std::pair<int,int>,8> STRAIGHTS_DIAGONALS = {
    std::make_pair(1,1),
    std::make_pair(-1,-1),  
    std::make_pair(1,-1), 
    std::make_pair(-1,1),
    std::make_pair(1,0),
    std::make_pair(-1,0),  
    std::make_pair(0,-1), 
    std::make_pair(0,1)   
};

std::vector<Move> Board::generate_piece_moves(int piece, int from){

    int rank = from / 8;
    int file = from % 8;

    std::vector<Move> moves;

    if(piece == WHITE_PAWN){
        int to = from+UP;


        //if not last rank and not blocked
        if(rank != 7 && !(all_pieces & (1ULL << to))){
            moves.emplace_back(from, to, QUIET);
            
            if(rank == 1 && !(all_pieces & (1ULL << (to+UP)))){
                moves.emplace_back(from, to+UP, QUIET);
            }

        }

        if (rank != 7){
            
            if(file > fileA && (occupancy[BLACK] & (1ULL << (to+LEFT)))){
                moves.emplace_back(from, to+LEFT, CAPTURE);
            }
            if(file < fileH && (occupancy[BLACK] & (1ULL << (to+RIGHT)))){
                moves.emplace_back(from, to+RIGHT, CAPTURE);
            }
        }
    }
    else if(piece == BLACK_PAWN){
        int to = from+DOWN;

        //if not last rank and not blocked
        if(rank != 0 && !(all_pieces & (1ULL << to))){
            moves.emplace_back(from, to, QUIET);
            
            if(rank == 6 && !(all_pieces & (1ULL << (to+DOWN)))){
                moves.emplace_back(from, to+DOWN, QUIET);
            }
        }

        if(rank != 7){
            if(file < fileH && (occupancy[WHITE] & (1ULL << (to+RIGHT)))){
                moves.emplace_back(from, to+RIGHT, CAPTURE);
            }
            if(file > fileA && (occupancy[WHITE] & (1ULL << (to+LEFT)))){
                moves.emplace_back(from, to+LEFT, CAPTURE);
            }
        }
    }

    else if(piece == WHITE_KNIGHT){
        
        for(auto m : KNIGHT_MOVES){
            int to = from+m;
            if(to < 0 || to >= 64) continue;
            
            int drank = abs(rank - to/8);
            int dfile = abs(file - to%8);
            if(drank + dfile == 3){
                if (occupancy[BLACK] & (1ULL << (to))){
                    moves.emplace_back(from, to, CAPTURE);
                }
                else if (!(occupancy[WHITE] & (1ULL << (to)))) 
                    moves.emplace_back(from, to, QUIET);
            }
        }
    }
    else if(piece == BLACK_KNIGHT){
        
        for(auto m : KNIGHT_MOVES){
            int to = from+m;
            if(to < 0 || to >= 64) continue;

            int drank = abs(rank - to/8);
            int dfile = abs(file - to%8);
            if(drank + dfile == 3){
                if (occupancy[WHITE] & (1ULL << (to)))
                    moves.emplace_back(from, to, CAPTURE);
                else if (!(occupancy[BLACK] & (1ULL << (to)))) 
                    moves.emplace_back(from, to, QUIET);
            }
        }
    }

    else if(piece == WHITE_BISHOP){
        for (auto [dr, df] : DIAGONALS){
            int r = rank;
            int f = file;
            
            while(true){
                r += dr;
                f += df;

                if(r < 0 || r > 7 || f < 0 || f > 7) 
                    break;

                int to = r*8 + f;
    
                if(all_pieces & (1ULL << to)){
                    if(occupancy[BLACK] & (1ULL << to))
                        moves.emplace_back(from, to, CAPTURE);

                    break;
                }
                moves.emplace_back(from, to, QUIET);
            } 
        } 
    }
    else if(piece == BLACK_BISHOP){
        for (auto [dr, df] : DIAGONALS){
            int r = rank;
            int f = file;
            
            while(true){
                r += dr;
                f += df;

                if(r < 0 || r > 7 || f < 0 || f > 7) 
                    break;

                int to = r*8 + f;
    
                if(all_pieces & (1ULL << to)){
                    if(occupancy[WHITE] & (1ULL << to))
                        moves.emplace_back(from, to, CAPTURE);

                    break;
                }
                moves.emplace_back(from, to, QUIET);
            } 
        } 
    }

    else if(piece == WHITE_ROOK){
        for (auto [dr, df] : STRAIGHTS){
            int to;
            int r = rank;
            int f = file;
                
            while(true){
                r += dr;
                f+= df;

                if(r < 0 || r > 7 || f < 0 || f > 7) 
                    break;

                int to = r*8 + f;
    
                if(all_pieces & (1ULL << to)){
                    if(occupancy[BLACK] & (1ULL << to))
                        moves.emplace_back(from, to, CAPTURE);

                    break;
                }
                moves.emplace_back(from, to, QUIET);
            }
        }
    }
    else if(piece == BLACK_ROOK){
        for (auto [dr, df] : STRAIGHTS){
            int r = rank;
            int f = file;
                
            while(true){
                r += dr;
                f+= df;

                if(r < 0 || r > 7 || f < 0 || f > 7) 
                    break;

                int to = r*8 + f;
    
                if(all_pieces & (1ULL << to)){
                    if(occupancy[WHITE] & (1ULL << to))
                        moves.emplace_back(from, to, CAPTURE);

                    break;
                }
                moves.emplace_back(from, to, QUIET);
            }
        }
    }

    else if(piece == WHITE_QUEEN){
        for (auto [dr, df] : STRAIGHTS_DIAGONALS){
            int r = rank;
            int f = file;
                
            while(true){
                r += dr;
                f += df;

                if(r < 0 || r > 7 || f < 0 || f > 7) 
                    break;

                int to = r*8 + f;
    
                if(all_pieces & (1ULL << to)){
                    if(occupancy[BLACK] & (1ULL << to))
                        moves.emplace_back(from, to, CAPTURE);

                    break;
                }
                moves.emplace_back(from, to, QUIET);
            }
        }
    }
    else if(piece == BLACK_QUEEN){
        for (auto [dr, df] : STRAIGHTS_DIAGONALS){
            int r = rank;
            int f = file;
                
            while(true){
                r += dr;
                f += df;

                if(r < 0 || r > 7 || f < 0 || f > 7) 
                    break;

                int to = r*8 + f;
    
                if(all_pieces & (1ULL << to)){
                    if(occupancy[WHITE] & (1ULL << to))
                        moves.emplace_back(from, to, CAPTURE);

                    break;
                }
                moves.emplace_back(from, to, QUIET);
            }
        }
    }
    else if(piece == WHITE_KING){
        for (auto [dr, df] : STRAIGHTS_DIAGONALS){
            int r = rank+dr;
            int f = file+df;

            if(r < 0 || r > 7 || f < 0 || f > 7) 
                continue;

            int to = r*8 + f;

            if(occupancy[BLACK] & (1ULL << to)) {moves.emplace_back(from, to, CAPTURE);}
            else if(!(occupancy[WHITE] & (1ULL << to))){moves.emplace_back(from, to, QUIET);}
        }
        
        //castling

    }
    else if(piece == BLACK_KING){
        for (auto [dr, df] : STRAIGHTS_DIAGONALS){
            int r = rank+dr;
            int f = file+df;

            if(r < 0 || r > 7 || f < 0 || f > 7) 
                continue;

            int to = r*8 + f;

            if(occupancy[WHITE] & (1ULL << to)) {moves.emplace_back(from, to, CAPTURE);}
            else if(!(occupancy[BLACK] & (1ULL << to))){moves.emplace_back(from, to, QUIET);}
        }
    }

    return moves;
}

U64 Board::pawn_attacks(int colour){
    int r, f;

    U64 attacks = 0;

    if (colour == WHITE){
        U64 bb = bitboard[WHITE][PAWN];

        while (bb){
            int from = pop_lsb(bb);
            r = from / 8;
            f = from % 8;

            if (r != 7){
                U64 left_attack = (1ULL << (from+UP+LEFT));
                U64 right_attack = (1ULL << (from+UP+RIGHT));

                if (f > fileA){
                    attacks |= left_attack;
                }
                if (f < fileH){
                    attacks |= right_attack;
                }
                
                if(bitboard[BLACK][KING] & (left_attack | right_attack)){
                    checking_bitboard[WHITE] |= (1ULL << from);
                }
            }
        }

    }
    else {
        U64 bb = bitboard[BLACK][PAWN];

        while (bb){
            int from = pop_lsb(bb);
            r = from / 8;
            f = from % 8;

            if (r != 0){
                U64 left_attack = (1ULL << (from+DOWN+LEFT));
                U64 right_attack = (1ULL << (from+DOWN+RIGHT));

                if (f < fileH){
                    attacks |= right_attack;
                }
                if (f > fileA){
                    attacks |= left_attack;
                }

                if(bitboard[WHITE][KING] & (left_attack | right_attack)){
                    checking_bitboard[BLACK] |= (1ULL << from);
                }
            }
        }
    }
    return attacks;
}

U64 Board::knight_attacks(int colour){
    U64 attacks = 0;
    
    U64 bb = bitboard[colour][KNIGHT];
    int opponent = (colour == WHITE ? BLACK : WHITE);

    while(bb){
        int from = pop_lsb(bb);
        int rank = from / 8;
        int file = from % 8;

        for(auto m : KNIGHT_MOVES){
            int to = from+m;
            if(to < 0 || to >= 64) continue;
            
            int drank = abs(rank - to/8);
            int dfile = abs(file - to%8);
            if(drank + dfile == 3){
                attacks |= (1ULL << to);
                
                if (bitboard[opponent][KING] & (1ULL << to)){
                    checking_bitboard[colour] |= (1ULL << from);
                }
            }
        }
    }

    return attacks;
}

U64 Board::bishop_attacks(int colour){
    U64 attacks = 0;
    int opponent = (colour == WHITE ? BLACK : WHITE);
    U64 bb = bitboard[colour][BISHOP];

    while(bb){
        int from = pop_lsb(bb);

        int rank = from / 8;
        int file = from % 8;

        for (auto [dr, df] : DIAGONALS){
            int r = rank;
            int f = file;

            U64 diagonal_attack = 0;
            
            while(true){
                r += dr;
                f += df;

                if(r < 0 || r > 7 || f < 0 || f > 7) 
                    break;

                int to = r*8 + f;

                diagonal_attack |= (1ULL << to);
                
                if(all_pieces & (1ULL << to)){
                    if(occupancy[opponent] & (1ULL << to)){
                        check_rays(colour, from, r, f, dr, df, diagonal_attack);

                        //one extra for king
                        if(bitboard[opponent][KING] & (1ULL << to)){
                           to = (r+dr)*8 + (f+df);
                           diagonal_attack |= (1ULL << to);
                        }
                    }
                    break;
                }
            } 

            attacks |= diagonal_attack;
        }
    }

    return attacks;
}

U64 Board::rook_attacks(int colour){
    U64 attacks = 0;

    int opponent = (colour == WHITE ? BLACK : WHITE);
    U64 bb = bitboard[colour][ROOK];

    while(bb){
        int from = pop_lsb(bb);
        int rank = from / 8;
        int file = from % 8;

        for (auto [dr, df] : STRAIGHTS){
            int r = rank;
            int f = file;

            U64 straight_attack = 0;
                
            while(true){
                r += dr;
                f += df;

                if(r < 0 || r > 7 || f < 0 || f > 7) 
                    break;

                int to = r*8 + f;
                straight_attack |= (1ULL << to);
    
                if(all_pieces & (1ULL << to)){
                    if(occupancy[opponent] & (1ULL << to)){
                        check_rays(colour, from, r, f, dr, df, straight_attack);

                        //one extra for king
                        if(bitboard[opponent][KING] & (1ULL << to)){
                           to = (r+dr)*8 + (f+df);
                           straight_attack |= (1ULL << to);
                        }
                    }
                    break;
                }
            }

            attacks |= straight_attack;
        }
    }

    return attacks;
}

U64 Board::queen_attacks(int colour){
    U64 attacks = 0;
    U64 bb = bitboard[colour][QUEEN];

    int opponent = (colour == WHITE ? BLACK : WHITE);
    while(bb){
        int from = pop_lsb(bb);

        int rank = from / 8;
        int file = from % 8;

        for (auto [dr, df] : STRAIGHTS_DIAGONALS){
            int r = rank;
            int f = file;

            U64 dir_attack = 0;
                
            while(true){
                r += dr;
                f += df;

                if(r < 0 || r > 7 || f < 0 || f > 7) 
                    break;

                int to = r*8 + f;
                
                dir_attack |= (1ULL << to);

                if(all_pieces & (1ULL << to)){
                    if(occupancy[opponent] & (1ULL << to)){
                        check_rays(colour, from, r, f, dr, df, dir_attack);

                        //one extra for king
                        if(bitboard[opponent][KING] & (1ULL << to)){
                           to = (r+dr)*8 + (f+df);
                           dir_attack |= (1ULL << to);
                        }
                    }
                    break;
                }
            }

            attacks |= dir_attack;
        }
    }
    return attacks;
}

U64 Board::king_attacks(int colour){
    U64 attacks = 0;
    U64 bb = bitboard[colour][KING];

    while(bb){
        int from = pop_lsb(bb);
        int rank = from / 8;
        int file = from % 8;

        for (auto [dr, df] : STRAIGHTS_DIAGONALS){
            int r = rank+dr;
            int f = file+df;

            if(r < 0 || r > 7 || f < 0 || f > 7) 
                continue;

            int to = r*8 + f;

            attacks |= (1ULL << to);
        }       
    }
    return attacks;
}

//finds pinned pieces and check rays
void Board::check_rays(int colour, int from, int r, int f, int dr, int df, U64 ray){
    int opponent = (colour == WHITE ? BLACK : WHITE);
    int block_index = r*8 + f;
    U64 block_mask = 1ULL << block_index;

    ray |= 1ULL << from;
    //kings in check
    if(bitboard[opponent][KING] & block_mask){
        checking_bitboard[colour] |= (1ULL << from);
        check_rays_bitboard[colour] |= (ray & ~block_mask);
        return ;
    }
    while(true){
        r += dr;
        f += df;

        if(r < 0 || r > 7 || f < 0 || f > 7) 
            break;

        U64 to_mask = 1ULL <<  (r*8 + f);

        if(all_pieces & to_mask){
            if(bitboard[opponent][KING] & to_mask){
                pinned_bitboard[opponent] |= block_mask;
                pin_ray_bitboard[opponent][block_index] = ray;
                break;
            }
        }

        ray |= to_mask;
    }
}

// 

void Board::update_attack_info(){
    white_in_check = black_in_check = false;
    for(int colour=WHITE; colour<=BLACK; colour++){
        attacks[colour] = 0;
        pinned_bitboard[(colour == WHITE ? BLACK : WHITE)] = 0;
        checking_bitboard[colour] = 0;
        check_rays_bitboard[colour] = 0;

        attacks[colour] |= pawn_attacks(colour)
                | knight_attacks(colour)
                | bishop_attacks(colour)
                | rook_attacks(colour)
                | queen_attacks(colour)
                | king_attacks(colour);
    }

    if(checking_bitboard[WHITE]) black_in_check = true;
    if(checking_bitboard[BLACK]) white_in_check = true;
}