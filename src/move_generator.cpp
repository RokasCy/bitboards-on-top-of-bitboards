#include"engine.h"
#include"move_generator.h"
#include<iostream>

//pop least significant bit
int pop_lsb(U64& bb){
    unsigned long index;
    _BitScanForward64(&index, bb); // find LSB
    bb &= bb - 1;             
    return static_cast<int>(index);
}

std::vector<Move> Board::generate_legal_moves(std::vector<Move> &pseudo_moves, int side, bool checkmate_check){

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
                    if(is_king && !to_attacked && !(m.flags & CASTLING))
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
                else if (!is_king){
                    legal_moves.push_back(m);
                }
            }

            if (is_king && !to_attacked && !(m.flags & CASTLING)) {
                legal_moves.push_back(m);
            }
            
            if(m.flags & CASTLING){
                if(m.to == 6){
                    if (!(attacks[BLACK] & king_side_gaps[WHITE])){
                        legal_moves.push_back(m);
                    }
                }
                else if(m.to == 1){
                    if (!(attacks[BLACK] & queen_side_gaps[WHITE])){
                        legal_moves.push_back(m);
                    }
                }
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
                    if(is_king && !to_attacked && !(m.flags & CASTLING))
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
                else if (!is_king){
                    legal_moves.push_back(m);
                }
            }

            if (is_king && !to_attacked && !(m.flags & CASTLING)) {
                legal_moves.push_back(m);
            }


            if(m.flags & CASTLING){
                if(m.to == 62){
                    if (!(attacks[WHITE] & king_side_gaps[BLACK])){
                        legal_moves.push_back(m);
                    }
                }
                else if(m.to == 57){
                    if (!(attacks[WHITE] & queen_side_gaps[BLACK])){
                        legal_moves.push_back(m);
                    }
                }
            }
        }
    }
    if(legal_moves.empty() && checkmate_check){
        CHECKMATED[side] = true;
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

        if (castling_rights[WHITE][0]){
            //king side castle
            if(castling_rights[WHITE][1]){
                if (!(all_pieces & king_side_gaps[WHITE])){
                    moves.emplace_back(from, 6, CASTLING);
                }
            }

            //queen side castle
            if(castling_rights[WHITE][2]){
                if (!(all_pieces & queen_side_gaps[WHITE])){
                    moves.emplace_back(from, 1, CASTLING);
                }
            }
        }

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

        if (castling_rights[BLACK][0]){
            //king side castle
            if(castling_rights[BLACK][1]){
                if (!(all_pieces & king_side_gaps[BLACK])){
                    moves.emplace_back(from, 62, CASTLING);
                }
            }

            //queen side castle
            if(castling_rights[BLACK][2]){
                if (!(all_pieces & queen_side_gaps[BLACK])){
                    moves.emplace_back(from, 57, CASTLING);
                }
            }
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

            U64 curr_attack = 0;

            if (r != 7){
                U64 left_attack = (1ULL << (from+UP+LEFT));
                U64 right_attack = (1ULL << (from+UP+RIGHT));

                if (f > fileA){
                    curr_attack |= left_attack;
                }
                if (f < fileH){
                    curr_attack |= right_attack;
                }
                
                if(bitboard[BLACK][KING] & curr_attack){
                    check_rays_bitboard[WHITE] |= (1ULL << from);
                    checking_bitboard[WHITE] |= (1ULL << from);
                }
            }

            attacks |= curr_attack;
        }
    }
    else {
        U64 bb = bitboard[BLACK][PAWN];

        while (bb){
            int from = pop_lsb(bb);
            r = from / 8;
            f = from % 8;

            U64 curr_attack = 0;

            if (r != 0){
                U64 left_attack = (1ULL << (from+DOWN+LEFT));
                U64 right_attack = (1ULL << (from+DOWN+RIGHT));

                if (f < fileH){
                    curr_attack |= right_attack;
                }
                if (f > fileA){
                    curr_attack |= left_attack;
                }

                if(bitboard[WHITE][KING] & curr_attack){
                    check_rays_bitboard[BLACK] |= (1ULL << from);
                    checking_bitboard[BLACK] |= (1ULL << from);
                }
            }
            attacks |= curr_attack;
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
                    check_rays_bitboard[colour] |= (1ULL << from);
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
            }
            break;
        }

        ray |= to_mask;
    }
}

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

void Board::castle_update(Move &move, int colour, bool undo){
    
    if(undo){
        castling_rights[colour][0] = true;

        if (colour == WHITE){
            if(move.to == 6){ 
                bitboard[colour][ROOK] = (bitboard[colour][ROOK] & ~(1ULL << 5)) | (1ULL << 7);
                squares[5] = EMPTY; squares[7] = (colour == WHITE ? WHITE_ROOK : BLACK_ROOK);
            }
            else if(move.to == 1){
                bitboard[colour][ROOK] = (bitboard[colour][ROOK] & ~(1ULL << 2)) | (1ULL << 0);
                squares[2] = EMPTY; squares[0] = (colour == WHITE ? WHITE_ROOK : BLACK_ROOK);
            }
        }
        else {
            if(move.to == 57){ 
                bitboard[colour][ROOK] = (bitboard[colour][ROOK] & ~(1ULL << 58)) | (1ULL << 56);
                squares[58] = EMPTY; squares[56] = (colour == WHITE ? WHITE_ROOK : BLACK_ROOK);
            }

            if(move.to == 62){  
                bitboard[colour][ROOK] = (bitboard[colour][ROOK] & ~(1ULL << 61)) | (1ULL << 63);
                squares[61] = EMPTY; squares[63] = (colour == WHITE ? WHITE_ROOK : BLACK_ROOK);
            }
        }
        return ;
    }

    castling_rights[colour][0] = false;

    if (colour == WHITE){
        if(move.to == 6){
            bitboard[colour][ROOK] = (bitboard[colour][ROOK] & ~(1ULL << 7)) | (1ULL << 5);
            squares[7] = EMPTY; squares[5] = (colour == WHITE ? WHITE_ROOK : BLACK_ROOK);
            castling_rights[colour][1] = false;
        }
        //queen castle
        else if(move.to == 1){
            bitboard[colour][ROOK] = (bitboard[colour][ROOK] & ~(1ULL << 0)) | (1ULL << 2);
            squares[0] = EMPTY; squares[2] = (colour == WHITE ? WHITE_ROOK : BLACK_ROOK);
            castling_rights[colour][2] = false;
        }
    }
    else {
        if(move.to == 57){
            bitboard[colour][ROOK] = (bitboard[colour][ROOK] & ~(1ULL << 56)) | (1ULL << 58);
            squares[56] = EMPTY; squares[58] = (colour == WHITE ? WHITE_ROOK : BLACK_ROOK);
            castling_rights[colour][2] = false;
        }

        if(move.to == 62){
            bitboard[colour][ROOK] = (bitboard[colour][ROOK] & ~(1ULL << 63)) | (1ULL << 61);
            squares[63] = EMPTY; squares[61] = (colour == WHITE ? WHITE_ROOK : BLACK_ROOK);
            castling_rights[colour][2] = false;
        }
    }
}

void Board::castle_right_update(Move& move, int colour, bool undo){
    int opponent = (colour == WHITE ? BLACK : WHITE);
    U64 from_mask = (1ULL << move.from);

    if(bitboard[colour][KING] & from_mask){
        castling_rights[colour][0] = false;
        return ;
    }

    //rook being captured
    if (move.flags & CAPTURE){
        if(move.to  == 0) castling_rights[opponent][2] = false;
        else if (move.to == 7) castling_rights[opponent][1] = false;
        else if (move.to == 56) castling_rights[opponent][2] = false;
        else if (move.to == 63) castling_rights[opponent][1] = false;
    }

    if (bitboard[colour][ROOK] & from_mask){
        if(move.from  == 0) castling_rights[colour][2] = false;
        else if (move.from == 7) castling_rights[colour][1] = false;
        else if (move.from == 56) castling_rights[colour][2] = false;
        else if (move.from == 63) castling_rights[colour][1] = false;
    }
}