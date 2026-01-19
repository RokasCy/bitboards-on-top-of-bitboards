#include"engine.h"
#include"move_generator.h"
#include"utils.h"

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
//redo update_attack_info
//pawn lookup table


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

    knight_move_init();
    king_move_init();
}

void Board::player_move(Move& move){
    U64 from_mask = 1ULL << move.from;
    U64 to_mask = 1ULL << move.to;

    int piece = squares[move.from], piece_index, colour;
    if (piece <= 5 && piece >= 0) {
        colour = WHITE; piece_index = piece;
    }
    else {
        colour = BLACK;
        piece_index = piece - 6;
    }

        // adding/removing piece from bitboards
    bitboard[colour][piece_index] = (bitboard[colour][piece_index] & ~from_mask) | to_mask;

    if (move.flags & CAPTURE){
        int captured = squares[move.to];
        int opponent = (colour == WHITE ? BLACK : WHITE);
        int captured_index = (opponent == BLACK ? captured - 6 : captured);

        bitboard[opponent][captured_index] &= ~to_mask;
    }
    squares[move.from] = EMPTY;
    squares[move.to] = piece;

    if (move.flags & CASTLING){
        castle_update(move, colour, false);
    }
    if (move.flags & PROMOTION){
        promotion(move, colour, false);
    }
    castle_right_update(move, colour);


    occupancy[WHITE] = occupancy[BLACK] = 0;
    for(int pt=0; pt<6; pt++){
        occupancy[WHITE] |= bitboard[WHITE][pt];
        occupancy[BLACK] |= bitboard[BLACK][pt];
    }

    all_pieces = occupancy[WHITE] | occupancy[BLACK];
    update_attack_info();
}

void Board::make_move(Move& move){
    Undo u(move, squares[move.to], castling_rights);
    history.push(u);

    U64 from_mask = 1ULL << move.from;
    U64 to_mask = 1ULL << move.to;

    int piece = squares[move.from], piece_index, colour;

    if (piece <= 5 && piece >= 0) {
        colour = WHITE; piece_index = piece;
    }
    else {
        colour = BLACK; piece_index = piece - 6;
    }

    
    // adding/removing piece from bitboards
    bitboard[colour][piece_index] = (bitboard[colour][piece_index] & ~from_mask) | to_mask;

    if (move.flags & CAPTURE){
        int captured = squares[move.to];
        int opponent = (colour == WHITE ? BLACK : WHITE);
        int captured_index = (opponent == BLACK ? captured - 6 : captured);

        bitboard[opponent][captured_index] &= ~to_mask;
    }

    squares[move.from] = EMPTY;
    squares[move.to] = piece;

    if (move.flags & CASTLING){
        castle_update(move, colour, false);
    }
    if (move.flags & PROMOTION){
        promotion(move, colour, false);
    }
    castle_right_update(move, colour);

    occupancy[WHITE] = occupancy[BLACK] = 0;
    for(int pt=0; pt<6; pt++){
        occupancy[WHITE] |= bitboard[WHITE][pt];
        occupancy[BLACK] |= bitboard[BLACK][pt];
    }

    all_pieces = occupancy[WHITE] | occupancy[BLACK];

    update_attack_info();
}

void Board::undo_move(){
    if(history.empty()) {return ;}

    Undo u = history.top();
    history.pop();

    Move move = u.move;

    U64 from_mask = 1ULL << move.from;
    U64 to_mask = 1ULL << move.to;

    int piece = squares[move.to], colour, piece_index;
    squares[move.from] = piece;

    if (piece <= 5 && piece >= 0) {
        colour = WHITE; piece_index = piece;
    }
    else {
        colour = BLACK; piece_index = piece - 6;
    }

    // adding/removing piece from bitboards
    bitboard[colour][piece_index] = (bitboard[colour][piece_index] & ~to_mask) | from_mask;


    if (move.flags & CAPTURE){
        int captured = u.captured_piece;
        int opponent = (colour == WHITE ? BLACK : WHITE);
        int captured_index = (opponent == BLACK ? captured - 6 : captured);

        bitboard[opponent][captured_index] |= to_mask;
	    squares[move.to] = u.captured_piece;
    } else {
        squares[move.to] = EMPTY;
    }

    all_pieces = occupancy[WHITE] | occupancy[BLACK];

    if (move.flags & CASTLING){
        castle_update(move, colour, true);
    }
    if (move.flags & PROMOTION){
        promotion(move, colour, true);
    }
    //revert back to old castling rights
    castling_rights = u.castling_rights;

    occupancy[WHITE] = 0;
    occupancy[BLACK] = 0;
    for (int pt = 0; pt < 6; pt++) {
        occupancy[WHITE] |= bitboard[WHITE][pt];
        occupancy[BLACK] |= bitboard[BLACK][pt];
    }


    update_attack_info();
}

int Board::evaluation(){
    static int pieceValue[6] = {100, 300, 300, 500, 900, 0};
    static int c=0;

    int WHITE_VALUE = 0, BLACK_VALUE = 0;
    //material calc
    for (int piece=PAWN; piece<=KING; piece++){
        WHITE_VALUE += __popcnt64(bitboard[WHITE][piece]) * pieceValue[piece];
        BLACK_VALUE += __popcnt64(bitboard[BLACK][piece]) * pieceValue[piece];
    }
    //piece square tables
    int phase = ((WHITE_VALUE+BLACK_VALUE)>3800 ? mid_game : end_game);

    for (int piece=PAWN; piece<=KING; piece++){
        
        U64 bb = bitboard[WHITE][piece];
        while(bb){
            int index = pop_lsb(bb);
            WHITE_VALUE += pst[phase][piece][index ^ 56]; //rank inversion (56 = 0b111000) RRRFFF
        }

        bb = bitboard[BLACK][piece];
        while(bb){
            int index = pop_lsb(bb);
            BLACK_VALUE += pst[phase][piece][index];
        }
    }
   
    //number of squares attacked
    WHITE_VALUE += __popcnt64(attacks[WHITE]) * 5;
    BLACK_VALUE += __popcnt64(attacks[BLACK]) * 5;

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

        auto moves = generate_legal_moves(generate_all_moves(WHITE), WHITE, true);
        if (moves.empty())
            return {Move{}, -CHECKMATE};

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
        auto moves = generate_legal_moves(generate_all_moves(BLACK), BLACK, true);
        if (moves.empty()){
            return {Move{}, CHECKMATE};
        }


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
        auto moves = generate_legal_moves(generate_all_moves(WHITE), WHITE, false);
        if (moves.empty())
            return -CHECKMATE;

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
        auto moves = generate_legal_moves(generate_all_moves(BLACK), BLACK, false);
        if (moves.empty())
            return CHECKMATE;

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


void Board::get_flags(Move &move, int colour){
    U64 from_mask = (1ULL << move.from);
    U64 to_mask = (1ULL << move.to);

    int opponent = (colour == WHITE ? BLACK : WHITE);

    if (occupancy[opponent] & to_mask){
        move.flags |= CAPTURE;
    }
    if(castling_rights[colour][0]){
        if(bitboard[colour][KING] & from_mask){
            //valid castle coordinates
            if(move.to == 6 || move.to == 1 || move.to == 62 || move.to == 57){
                move.flags |= CASTLING;
            }
        }
    }
    int to_rank = move.to / 8;

    if((bitboard[WHITE][PAWN] & from_mask) && to_rank==7) {
        move.flags |= PROMOTION;
    }
    if((bitboard[BLACK][PAWN] & from_mask) && to_rank==0) {
        move.flags |= PROMOTION;
    }
}
