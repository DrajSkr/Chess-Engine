/*##########################

Search

#############################*/

#include "config.hpp"
#include "config.hpp"
#include "ChessEngine.hpp"
#include "UCI.hpp"

// No global variables; they are now members of ChessEngine

//search position for best move
void ChessEngine::search_position(int max_depth)
{
    // Start timing
    search_start_time = get_time_ms();
    time_stopped = false;
    search_nodes = 0;
    
    // Clear heuristics
    clear_heuristics();
    
    int current_best_move = 0;
    int previous_score = 0;
    int alpha = -50000;
    int beta = 50000;

    // Iterative Deepening loop
    for (int depth = 1; depth <= max_depth; depth++) {
        int score = negamax(alpha, beta, depth);
        
        // Aspiration window failure: if score fell outside our narrow window, re-search with full window
        if ((score <= alpha || score >= beta) && depth > 1) {
            alpha = -50000;
            beta = 50000;
            score = negamax(alpha, beta, depth);
        }
        
        // If time ran out during the search, break out immediately
        if (time_stopped) {
            break;
        }

        // Successfully completed this depth
        current_best_move = best_move;
        previous_score = score;
        
        // Set up narrow window for next depth
        alpha = previous_score - 50;
        beta = previous_score + 50;
        
        cout<<"info depth "<<depth<<" score cp "<<score<<" nodes "<<search_nodes<<"\n";
    }

    // Restore the best move from the deepest COMPLETED search
    best_move = current_best_move;

    //best move placeholder (bestmove is UCI command)
    cout<<"bestmove ";
    print_move(best_move);
    cout<<"\n";
}