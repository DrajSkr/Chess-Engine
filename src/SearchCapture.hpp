/*##########################

Search Capture Utility
Wraps the engine's search_position() to capture
the bestmove output and score into strings
instead of printing to stdout.

Does NOT modify Search.cpp or Search.hpp.

#############################*/
#ifndef SEARCHCAPTURE_HPP
#define SEARCHCAPTURE_HPP

class ChessEngine; // Stops Clangd preamble
#include "ChessEngine.hpp"
#include <string>
#include <sstream>

#include "UCI.hpp"
#include "MoveGenerator.hpp"
#include "Search.hpp"

// Captures the bestmove string from the engine's search output.
// search_position() prints "bestmove e2e4\n" to cout.
// We redirect cout to a stringstream, run the search, then restore cout.

inline SearchResult ChessEngine::capture_search(int max_depth)
{
    SearchResult result;
    result.score = 0;
    result.bestmove = "";

    // Start time and reset time management variables
    search_start_time = get_time_ms();
    time_stopped = false;
    search_nodes = 0;
    
    // Clear heuristics
    clear_heuristics();

    int current_best_move = 0;
    int current_best_score = 0;

    // Use Iterative Deepening so time limit works properly
    for (int depth = 1; depth <= max_depth; depth++) {
        ply = 0;
        int score = negamax(-50000, 50000, depth);
        
        if (time_stopped) {
            break;
        }
        
        current_best_score = score;
        current_best_move = best_move;
    }

    best_move = current_best_move;

    // Fallback: If best_move is still 0 (e.g. search aborted on depth 1), pick the first legal move
    if (best_move == 0) {
        MoveList fallback_list;
        generate_moves(fallback_list);
        for (int i = 0; i < fallback_list.index; i++) {
            copy_board();
            if (make_move(fallback_list.moves[i], all_moves)) {
                take_back();
                best_move = fallback_list.moves[i];
                break;
            }
            take_back();
        }
    }

    // Score is from the side-to-move's perspective (negamax convention).
    // Convert to white's perspective for the eval bar.
    if (side == black)
        result.score = -current_best_score;
    else
        result.score = current_best_score;

    // Extract the bestmove string from the best_move integer
    if (best_move)
    {
        std::ostringstream move_ss;
        move_ss << square_to_board[decode_move_source(best_move)]
                << square_to_board[decode_move_target(best_move)];
        if (promoted_pieces[decode_move_promo_piece(best_move)])
            move_ss << promoted_pieces[decode_move_promo_piece(best_move)];
        result.bestmove = move_ss.str();
    }

    return result;
}

#endif
