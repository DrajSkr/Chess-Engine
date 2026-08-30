/*##########################

Search Capture Utility
Wraps the engine's search_position() to capture
the bestmove output and score into strings
instead of printing to stdout.

Does NOT modify Search.cpp or Search.hpp.

#############################*/
#ifndef SEARCHCAPTURE_HPP
#define SEARCHCAPTURE_HPP

#include "config.hpp"
#include "Board.hpp"
#include "MoveGenerator.hpp"
#include "Search.hpp"
#include <string>
#include <sstream>

#include "UCI.hpp"

// Captures the bestmove string from the engine's search output.
// search_position() prints "bestmove e2e4\n" to cout.
// We redirect cout to a stringstream, run the search, then restore cout.
struct SearchResult
{
    std::string bestmove;  // e.g. "e2e4" or "e7e8q"
    int score;             // centipawn score from white's perspective
};

static inline SearchResult capture_search(int max_depth)
{
    SearchResult result;
    result.score = 0;
    result.bestmove = "";

    best_move = 0;
    ply = 0;
    int best_score = negamax(-50000, 50000, max_depth);

    // Score is from the side-to-move's perspective (negamax convention).
    // Convert to white's perspective for the eval bar.
    if (side == black)
        result.score = -best_score;
    else
        result.score = best_score;

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
