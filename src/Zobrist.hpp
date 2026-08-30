#ifndef ZOBRIST_HPP
#define ZOBRIST_HPP

#include "config.hpp" // IWYU pragma: keep
#include <cstdint> // IWYU pragma: keep

// PRNG for 64-bit numbers (Xorshift64)
U64 random_u64();

// Zobrist keys
extern U64 piece_keys[12][64];
extern U64 enpassant_keys[64];
extern U64 castle_keys[16];
extern U64 side_key;

// Initialize the Zobrist keys with random 64-bit numbers
void init_zobrist();

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

#endif
