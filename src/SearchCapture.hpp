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

// Captures the bestmove string from the engine's search output.
// search_position() prints "bestmove e2e4\n" to cout.
// We redirect cout to a stringstream, run the search, then restore cout.
struct SearchResult
{
    std::string bestmove;  // e.g. "e2e4" or "e7e8q"
    int score;             // centipawn score from white's perspective
};

static inline SearchResult capture_search(int depth)
{
    SearchResult result;
    result.score = 0;
    result.bestmove = "";

    // Run negamax directly to get the score
    // We need to call it the same way search_position does
    // but also capture the score return value
    ply = 0;
    int score = negamax(-50000, 50000, depth);

    // Score is from the side-to-move's perspective (negamax convention).
    // Convert to white's perspective for the eval bar.
    if (side == black)
        result.score = score;   // negamax returns positive=good for side to move
    else
        result.score = score;
    // Actually: negamax returns score from current side's perspective.
    // If white just moved, it's now black's turn. The score returned is black's perspective.
    // We want white-relative. So if side==black (black to move after white moved), 
    // the score from negamax is black's perspective → negate for white.
    // If side==white (white to move after black moved), score is white's perspective → keep.
    // But wait — we call capture_search AFTER making the player's move and BEFORE
    // the engine makes its move. So side == black (engine is black).
    // negamax returns score from black's perspective. Negate for white's eval bar.
    // Let's just always convert: if side is black, negate.
    if (side == black)
        result.score = -score;
    else
        result.score = score;

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
