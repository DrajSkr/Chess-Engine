#include "ChessEngine.hpp"
#include <cstring>
#include <cstdlib>

ChessEngine::ChessEngine() {
    memset(bitboards, 0, sizeof(bitboards));
    memset(occupancies, 0, sizeof(occupancies));
    side = 0;
    enpassant = no_square;
    castle = 0;
    fifty = 0;
    ply = 0;
    
    hash_key = 0;
    tt_table = nullptr;
    tt_size = 0;

    best_move = 0;
    search_nodes = 0;
    time_stopped = false;
    search_time_limit = 4500;
    search_start_time = 0;
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));

    init_tt(16);
}

ChessEngine::~ChessEngine() {
    if (tt_table) free(tt_table);
}

void ChessEngine::init_tt(int megabytes) {
    int hash_size = 0x100000 * megabytes;
    tt_size = hash_size / sizeof(TTEntry);
    if (tt_table) free(tt_table);
    tt_table = (TTEntry*)malloc(hash_size);
    clear_tt();
}

void ChessEngine::clear_tt() {
    if (tt_table) memset(tt_table, 0, tt_size * sizeof(TTEntry));
}
