/*##########################

Search

#############################*/

#include "config.hpp"
#include "Board.hpp"
#include "MoveGenerator.hpp"
#include "Search.hpp"
#include "UCI.hpp"

//best move variable (will be removed when we start storing principal variation)
int best_move;

// Time management variables
int search_time_limit = 4500; // default 4.5 seconds
int search_start_time = 0;
bool time_stopped = false;
U64 search_nodes = 0;

//search position for best move
void search_position(int max_depth)
{
    // Start timing
    search_start_time = get_time_ms();
    time_stopped = false;
    search_nodes = 0;
    
    int current_best_move = 0;

    // Iterative Deepening loop
    for (int depth = 1; depth <= max_depth; depth++) {
        // Run search
        negamax(-50000, 50000, depth);
        
        // If time ran out during the search, break out immediately
        // and DO NOT use the partial result (best_move from this aborted depth)
        if (time_stopped) {
            break;
        }

        // Successfully completed this depth
        current_best_move = best_move;
        
        // cout<<"info depth "<<depth<<" score cp "<<score<<" nodes "<<search_nodes<<"\n";
    }

    // Restore the best move from the deepest COMPLETED search
    best_move = current_best_move;

    //best move placeholder (bestmove is UCI command)
    cout<<"bestmove ";
    print_move(best_move);
    cout<<"\n";
}