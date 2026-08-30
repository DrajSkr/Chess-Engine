/*##########################

Search

#############################*/
#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "config.hpp"
#include "Board.hpp"
#include "Evaluate.hpp"
#include "MoveGenerator.hpp"
#include "Zobrist.hpp"

//best move variable
extern int best_move;

#include <cmath> // for std::abs
#include "UCI.hpp"

// Score move for move ordering (MVV-LVA)
static inline int score_move(int move, int pv_move) {
    if (move == pv_move) {
        return 20000; // Best move from TT searched first
    }
    // If it's a capture move
    if (decode_move_capture(move)) {
        int target_square = decode_move_target(move);
        int attacker_piece = decode_move_piece(move);
        int victim_piece = P; // Default
        
        // Find the victim piece
        int start_piece = (side == white) ? p : P;
        int end_piece = (side == white) ? k : K;
        
        for (int p_piece = start_piece; p_piece <= end_piece; p_piece++) {
            if (get_bit(bitboards[p_piece], target_square)) {
                victim_piece = p_piece;
                break;
            }
        }
        
        // MVV-LVA score: prioritize capturing high-value pieces with low-value pieces
        // Add 10000 to ensure captures are searched before quiet moves (which get score 0)
        return std::abs(material_score[victim_piece]) - std::abs(material_score[attacker_piece]) + 10000;
    }
    
    // Quiet moves get a base score of 0 for now (can be enhanced with history heuristic later)
    return 0;
}

// Sort moves based on their score
static inline void sort_moves(MoveList& move_list, int pv_move) {
    int scores[256];
    for (int i = 0; i < move_list.index; i++) {
        scores[i] = score_move(move_list.moves[i], pv_move);
    }
    
    // Selection sort for small arrays
    for (int i = 0; i < move_list.index - 1; i++) {
        int max_idx = i;
        for (int j = i + 1; j < move_list.index; j++) {
            if (scores[j] > scores[max_idx]) {
                max_idx = j;
            }
        }
        // Swap scores
        int temp_score = scores[i];
        scores[i] = scores[max_idx];
        scores[max_idx] = temp_score;
        // Swap moves
        int temp_move = move_list.moves[i];
        move_list.moves[i] = move_list.moves[max_idx];
        move_list.moves[max_idx] = temp_move;
    }
}

// Quiescence search to avoid the horizon effect
static inline int quiescence(int alpha, int beta)
{
    // Evaluate the position (stand pat score)
    int evaluation = evaluate();

    // Fail-hard beta cutoff
    if (evaluation >= beta)
        return beta;

    // Update alpha if stand pat score is better
    if (evaluation > alpha)
        alpha = evaluation;

    MoveList move_list;
    generate_moves(move_list);
    sort_moves(move_list, 0);

    // Iterate over all possible moves, but only evaluate captures
    for (int i = 0; i < move_list.index; i++)
    {
        // Skip quiet moves in Quiescence Search
        if (!decode_move_capture(move_list.moves[i]))
            continue;

        copy_board();
        ply++;

        // Make the capture move
        if (make_move(move_list.moves[i], all_moves))
        {
            // Recursively search the resulting position
            int score = -1 * quiescence(-1 * beta, -1 * alpha);

            ply--;
            take_back();

            // Fail-hard beta cutoff
            if (score >= beta)
                return beta;

            // Update alpha
            if (score > alpha)
                alpha = score;
        }
        else
        {
            ply--;
        }
    }

    return alpha;
}

//negamax function
static inline int negamax(int alpha, int beta, int depth)
{
    int pv_move = 0;
    
    // Check Transposition Table
    int tt_score = read_tt(depth, alpha, beta, &pv_move);
    if (tt_score != 100000) {
        // We can immediately return the cached score
        if (ply == 0) best_move = pv_move;
        return tt_score;
    }
    
    //base case of recursion: use quiescence search instead of static evaluate
    if (depth<=0)
        return quiescence(alpha, beta);

    // Null Move Pruning (NMP)
    // Only attempt if we are not at the root (ply > 0), depth is high enough, 
    // and we are not in check (null move in check is illegal).
    int king_sq = (side == white) ? get_fsb(bitboards[K]) : get_fsb(bitboards[k]);
    if (depth >= 3 && ply > 0 && !is_square_attacked(king_sq, 1 - side))
    {
        // Make a null move (pass turn)
        int ep_copy = enpassant;
        side ^= 1;
        enpassant = no_square;
        ply++;
        
        // Search with reduced depth (R=2) and zero-width window
        int null_score = -negamax(-beta, -beta + 1, depth - 1 - 2);
        
        // Unmake null move
        ply--;
        side ^= 1;
        enpassant = ep_copy;
        
        // If the score after skipping a turn is still >= beta, opponent won't allow this branch
        if (null_score >= beta)
            return beta;
    }

    //move list
    MoveList move_list;

    //generate moves
    generate_moves(move_list);

    //sort moves to improve alpha-beta pruning (Move Ordering)
    sort_moves(move_list, pv_move);

    //legal moves count
    int legal_moves=0;
    int moves_searched=0;
    int hash_flag = hash_alpha;
    int local_best_move = 0;

    //king in check flag
    //check if king is attacked by the other side
    int king_square = ((side==white)? get_fsb(bitboards[K]) : get_fsb(bitboards[k]));
    int king_in_check = is_square_attacked(king_square, (1-side));

    //iterate over all possible moves
    for (int i=0;i<move_list.index;i++)
    {
        //preserve game state
        copy_board();

        //increment half move counter
        ply++;

        //make only legal moves
        if (make_move(move_list.moves[i], all_moves))
        {            
            //increment legal moves count
            legal_moves++;

            int childscore;

            // Principal Variation Search (PVS)
            if (moves_searched == 0)
            {
                // First move: full window search
                childscore = -1 * negamax(-1*beta, -1*alpha, depth-1);
            }
            else
            {
                // Subsequent moves: zero-width window to prove they are worse
                childscore = -1 * negamax(-1*alpha - 1, -1*alpha, depth-1);

                // If it failed high, we need a full re-search
                if (childscore > alpha && childscore < beta)
                {
                    childscore = -1 * negamax(-1*beta, -1*alpha, depth-1);
                }
            }
            
            moves_searched++;

            //decrement half move counter
            ply--;
            //take back the move
            take_back();

            //fail hard
            if (childscore>=beta)
            {
                //node fails high (terminology)
                write_tt(depth, beta, hash_beta, move_list.moves[i]);
                return beta;
            }

            //if this child score is better then current best (which is alpha) 
            if (childscore>alpha) 
            {
                hash_flag = hash_exact;
                local_best_move = move_list.moves[i];
                
                //principle variation node (terminology)
                if (ply==0) //root node
                {
                    // cout<<"hi "; 
                    //associating best move with best score
                    best_move = move_list.moves[i];
                    // print_move(current_best_move);cout<<" "<<depth<<"\n";
                }
            }
            //update alpha
            alpha = max(alpha, childscore);
        }
        else
        {            
            //decrement half move counter
            ply--;
        }            
    }
    
    //check if we have 0 legal moves
    if (legal_moves==0)
    {
        //if in check and no legal moves, checkmate
        if (king_in_check)
            //return mating score
            return -49000+ply; //slightly less than -INF to keep it within alpha beta bounds
            //ply is necessary to distinguish between different mates (M1/M4), more detail in notion doc
    
        //else stalemate
        else 
            //return stalemate score;
            return 0; //stalemate is draw
    }

    //return the best score
    //node fails low (terminology)
    write_tt(depth, alpha, hash_flag, local_best_move);
    return alpha;
}

//search position for best move
void search_position(int depth);

#endif