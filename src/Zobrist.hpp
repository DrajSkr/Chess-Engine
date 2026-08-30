#ifndef ZOBRIST_HPP
#define ZOBRIST_HPP

#include "config.hpp"
#include <cstdint>

// PRNG for 64-bit numbers (Xorshift64)
U64 random_u64();

// Zobrist keys
extern U64 piece_keys[12][64];
extern U64 enpassant_keys[64];
extern U64 castle_keys[16];
extern U64 side_key;

// The global hash key representing the current board state
extern U64 hash_key;

// Initialize the Zobrist keys with random 64-bit numbers
void init_zobrist();

// Generate the hash key from scratch (used when parsing FEN)
U64 generate_hash_key();

/*##########################
  Transposition Table
#############################*/

// Flags for the TT entry
enum { hash_exact, hash_alpha, hash_beta };

struct TTEntry {
    U64 key;         // Zobrist hash key
    int depth;       // Depth of the search
    int flag;        // Type of node (exact, alpha, beta)
    int score;       // Evaluation score
    int best_move;   // Best move found in this position
};

// Global TT table
extern TTEntry* tt_table;
extern int tt_size;

// Initialize TT memory
void init_tt(int megabytes);

// Free TT memory
void free_tt();

// Clear TT entries
void clear_tt();

// Read from TT
int read_tt(int depth, int alpha, int beta, int *best_move);

// Write to TT
void write_tt(int depth, int score, int flag, int best_move);

#endif
