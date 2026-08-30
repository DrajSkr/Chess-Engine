/*##########################

Evaluate

#############################*/
#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "config.hpp"
#include "Board.hpp"
#include <algorithm>

// Piece values for game phase calculation (not evaluation)
constexpr int game_phase_inc[12] = {
    0, 1, 1, 2, 4, 0,
    0, 1, 1, 2, 4, 0
};

// PeSTO's Evaluation Tables
// Contains material value + piece square table value
// Middlegame (MG) and Endgame (EG) tables

constexpr int mg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  62,  11,  8, -24,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0
};

constexpr int eg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0
};

constexpr int mg_knight_table[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23
};

constexpr int eg_knight_table[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,   2,   7,  22,  -4, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,   2, -20, -23, -44,
    -29, -51, -23, -38, -29, -27, -43, -74
};

constexpr int mg_bishop_table[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21
};

constexpr int eg_bishop_table[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17
};

constexpr int mg_rook_table[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26
};

constexpr int eg_rook_table[64] = {
    13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20
};

constexpr int mg_queen_table[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
      -1, -18,  -9,  10, -15, -25, -31, -50
};

constexpr int eg_queen_table[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  12,  11,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41
};

constexpr int mg_king_table[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49, -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14
};

constexpr int eg_king_table[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43
};

// Mirror square for opposite side
constexpr int mirror_square[64] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

// PeSTO's material values (used for base score, already included in the tables above)
// We add them here just in case they are needed elsewhere, but the tables below add material + pos
constexpr int mg_value[12] = { 82, 337, 365, 477, 1025, 0, 82, 337, 365, 477, 1025, 0};
constexpr int eg_value[12] = { 94, 281, 297, 512,  936, 0, 94, 281, 297, 512, 936, 0};

inline int ChessEngine::evaluate()
{
    int mg_score = 0;
    int eg_score = 0;
    int game_phase = 0;

    U64 bitboard;
    int square;
    
    // Feature bonuses
    int white_bishops = 0, black_bishops = 0;

    for (int piece = P; piece <= k; piece++)
    {
        bitboard = bitboards[piece];
        while (bitboard)
        {
            square = get_fsb(bitboard);
            
            // Calculate game phase
            game_phase += game_phase_inc[piece];

            switch(piece)
            {
                case P: 
                    mg_score += mg_pawn_table[square] + mg_value[P]; 
                    eg_score += eg_pawn_table[square] + eg_value[P]; 
                    break;
                case N: 
                    mg_score += mg_knight_table[square] + mg_value[N]; 
                    eg_score += eg_knight_table[square] + eg_value[N]; 
                    break;
                case B: 
                    mg_score += mg_bishop_table[square] + mg_value[B]; 
                    eg_score += eg_bishop_table[square] + eg_value[B]; 
                    white_bishops++;
                    break;
                case R: 
                    mg_score += mg_rook_table[square] + mg_value[R]; 
                    eg_score += eg_rook_table[square] + eg_value[R]; 
                    break;
                case Q: 
                    mg_score += mg_queen_table[square] + mg_value[Q]; 
                    eg_score += eg_queen_table[square] + eg_value[Q]; 
                    break;
                case K: 
                    mg_score += mg_king_table[square]; 
                    eg_score += eg_king_table[square]; 
                    break;

                case p: 
                    mg_score -= (mg_pawn_table[mirror_square[square]] + mg_value[p]); 
                    eg_score -= (eg_pawn_table[mirror_square[square]] + eg_value[p]); 
                    break;
                case n: 
                    mg_score -= (mg_knight_table[mirror_square[square]] + mg_value[n]); 
                    eg_score -= (eg_knight_table[mirror_square[square]] + eg_value[n]); 
                    break;
                case b: 
                    mg_score -= (mg_bishop_table[mirror_square[square]] + mg_value[b]); 
                    eg_score -= (eg_bishop_table[mirror_square[square]] + eg_value[b]); 
                    black_bishops++;
                    break;
                case r: 
                    mg_score -= (mg_rook_table[mirror_square[square]] + mg_value[r]); 
                    eg_score -= (eg_rook_table[mirror_square[square]] + eg_value[r]); 
                    break;
                case q: 
                    mg_score -= (mg_queen_table[mirror_square[square]] + mg_value[q]); 
                    eg_score -= (eg_queen_table[mirror_square[square]] + eg_value[q]); 
                    break;
                case k: 
                    mg_score -= mg_king_table[mirror_square[square]]; 
                    eg_score -= eg_king_table[mirror_square[square]]; 
                    break;
            }
            
            reset_bit(bitboard, square);
        }
    }

    // Bishop pair bonus
    if (white_bishops >= 2) {
        mg_score += 30;
        eg_score += 40;
    }
    if (black_bishops >= 2) {
        mg_score -= 30;
        eg_score -= 40;
    }

    // Tapered evaluation
    // Max phase is 24 (4 knights=4, 4 bishops=4, 4 rooks=8, 2 queens=8)
    if (game_phase > 24) game_phase = 24; 
    
    int mg_weight = game_phase;
    int eg_weight = 24 - game_phase;
    
    int score = (mg_score * mg_weight + eg_score * eg_weight) / 24;

    // Tempo bonus
    if (side == white) {
        score += 15;
    } else {
        score -= 15;
    }

    return (side == white) ? score : -score;
}

#endif