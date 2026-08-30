#include "Zobrist.hpp"
#include "Board.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>

// Zobrist keys
U64 piece_keys[12][64];
U64 enpassant_keys[64];
U64 castle_keys[16];
U64 side_key;

U64 hash_key = 0ULL;

TTEntry* tt_table = nullptr;
int tt_size = 0;

// Simple 64-bit PRNG
U64 random_u64() {
    U64 u1, u2, u3, u4;
    u1 = (U64)(rand()) & 0xFFFF;
    u2 = (U64)(rand()) & 0xFFFF;
    u3 = (U64)(rand()) & 0xFFFF;
    u4 = (U64)(rand()) & 0xFFFF;
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

void init_zobrist() {
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 64; j++) {
            piece_keys[i][j] = random_u64();
        }
    }
    for (int i = 0; i < 64; i++) {
        enpassant_keys[i] = random_u64();
    }
    for (int i = 0; i < 16; i++) {
        castle_keys[i] = random_u64();
    }
    side_key = random_u64();
}

U64 generate_hash_key() {
    U64 final_key = 0;
    
    // Pieces
    for (int piece = P; piece <= k; piece++) {
        U64 bitboard = bitboards[piece];
        while (bitboard) {
            int square = get_fsb(bitboard);
            final_key ^= piece_keys[piece][square];
            reset_bit(bitboard, square);
        }
    }
    
    // En-passant
    if (enpassant != no_square) {
        final_key ^= enpassant_keys[enpassant];
    }
    
    // Castling
    final_key ^= castle_keys[castle];
    
    // Side
    if (side == black) {
        final_key ^= side_key;
    }
    
    return final_key;
}

void init_tt(int megabytes) {
    int hash_size = 0x100000 * megabytes; // MB to bytes
    tt_size = hash_size / sizeof(TTEntry);
    
    if (tt_table != nullptr) {
        free(tt_table);
    }
    
    tt_table = (TTEntry*)malloc(tt_size * sizeof(TTEntry));
    if (tt_table == nullptr) {
        std::cout << "Couldn't allocate memory for TT!\n";
        tt_size = 0;
    } else {
        clear_tt();
    }
}

void free_tt() {
    if (tt_table) {
        free(tt_table);
        tt_table = nullptr;
    }
}

void clear_tt() {
    if (tt_table) {
        memset(tt_table, 0, tt_size * sizeof(TTEntry));
    }
}

int read_tt(int depth, int alpha, int beta, int *best_move) {
    if (tt_size == 0) return 100000;
    
    TTEntry *entry = &tt_table[hash_key % tt_size];
    
    if (entry->key == hash_key) {
        // We found a match, so extract the best move (useful for move sorting even if depth is lower)
        *best_move = entry->best_move;
        
        // If the entry depth is sufficient, we can use the score
        if (entry->depth >= depth) {
            int score = entry->score;
            
            // Adjust mate scores for distance to root
            if (score > 49000) score -= ply;
            if (score < -49000) score += ply;
            
            if (entry->flag == hash_exact) {
                return score;
            }
            if ((entry->flag == hash_alpha) && (score <= alpha)) {
                return alpha;
            }
            if ((entry->flag == hash_beta) && (score >= beta)) {
                return beta;
            }
        }
    }
    
    return 100000; // Value indicating failure to find a usable score
}

void write_tt(int depth, int score, int flag, int best_move) {
    if (tt_size == 0) return;
    
    TTEntry *entry = &tt_table[hash_key % tt_size];
    
    // Simple replacement scheme: always replace
    // Adjust mate scores to independent of ply
    if (score > 49000) score += ply;
    if (score < -49000) score -= ply;
    
    entry->key = hash_key;
    entry->depth = depth;
    entry->flag = flag;
    entry->score = score;
    entry->best_move = best_move;
}
