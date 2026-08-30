#ifndef CHESS_ENGINE_HPP
#define CHESS_ENGINE_HPP

#include "config.hpp"
#include "Zobrist.hpp"
#include "Board.hpp"

struct SearchResult {
    string bestmove;
    int score;
};

class ChessEngine {
public:
    U64 bitboards[12];
    U64 occupancies[3];
    int side;
    int enpassant;
    int castle;
    int fifty;
    int ply;
    
    U64 hash_key;
    TTEntry* tt_table;
    int tt_size;

    int best_move;
    U64 search_nodes;
    bool time_stopped;
    long long search_time_limit;
    long long search_start_time;
    int killer_moves[2][MAX_PLY];
    int history_moves[12][64];

    ChessEngine();
    ~ChessEngine();
    void init_tt(int megabytes);
    void clear_tt();

    // Board / FEN
    void print_board();
    void parse_FEN_string(const string& fen);
    string export_fen();

    // Zobrist
    U64 generate_hash_key();

    // Move Generator
    inline int is_square_attacked(int square, int attacking_side);
    void print_attacked_squares(int attacking_side);
    inline void generate_moves(struct MoveList& move_list);
    inline bool make_move(int move, int move_flag);
    int parse_move_string(const string &move_string);

    // Evaluate
    inline int evaluate();

    // Perft
    U64 perft(int depth);
    void perft_test(int depth);

    // Search
    inline void clear_heuristics();
    inline int score_move(int move, int pv_move);
    inline void sort_moves(struct MoveList& move_list, int pv_move);
    int read_tt(int alpha, int beta, int depth, int* pv_move);
    void write_tt(int score, int depth, int hash_flag, int move);
    inline int quiescence(int alpha, int beta);
    inline int negamax(int alpha, int beta, int depth);
    void search_position(int depth);

    // Search Capture
    struct SearchResult capture_search(int depth);
};

// Implementations of the methods declared above
#include "MoveGenerator.hpp"
#include "Evaluate.hpp"
#include "Search.hpp"
#include "SearchCapture.hpp"

#endif
