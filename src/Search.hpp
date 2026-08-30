/*##########################

Search

#############################*/
#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "ChessEngine.hpp"
#include "config.hpp"
#include "Board.hpp"
#include "Evaluate.hpp"
#include "MoveGenerator.hpp"
#include "Zobrist.hpp"

#include <cmath> // for std::abs
#include "UCI.hpp"

// Clear heuristics before search
inline void ChessEngine::clear_heuristics() {
    for (int i = 0; i < MAX_PLY; i++) {
        killer_moves[0][i] = 0;
        killer_moves[1][i] = 0;
    }
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 64; j++) {
            history_moves[i][j] = 0;
        }
    }
}

// Score move for move ordering
inline int ChessEngine::score_move(int move, int pv_move) {
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
        
        // MVV-LVA score
        return std::abs(mg_value[victim_piece]) - std::abs(mg_value[attacker_piece]) + 10000;
    }
    else {
        // Quiet moves
        if (ply < MAX_PLY) {
            if (killer_moves[0][ply] == move) return 9000;
            if (killer_moves[1][ply] == move) return 8000;
        }
        
        // History heuristic
        return history_moves[decode_move_piece(move)][decode_move_target(move)];
    }
}

// Sort moves based on their score
inline void ChessEngine::sort_moves(MoveList& move_list, int pv_move) {
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
inline int ChessEngine::quiescence(int alpha, int beta)
{
    if (time_stopped) return 0;
    search_nodes++;
    if ((search_nodes & 2047) == 0) {
        if (get_time_ms() - search_start_time > search_time_limit) {
            time_stopped = true;
            return 0;
        }
    }

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
inline int ChessEngine::negamax(int alpha, int beta, int depth)
{
    if (time_stopped) return 0;
    search_nodes++;
    if ((search_nodes & 2047) == 0) {
        if (get_time_ms() - search_start_time > search_time_limit) {
            time_stopped = true;
            return 0;
        }
    }

    int pv_move = 0;
    
    // Check Transposition Table
    best_move = 0;
    int tt_score = read_tt(alpha, beta, depth);
    pv_move = best_move;
    if (tt_score != 100000) {
        // We can immediately return the cached score
        if (ply == 0 && pv_move != 0) {
            best_move = pv_move;
            return tt_score;
        } else if (ply != 0) {
            return tt_score;
        }
    }
    
    // Check Extension
    int king_square = ((side==white)? get_fsb(bitboards[K]) : get_fsb(bitboards[k]));
    int king_in_check = is_square_attacked(king_square, (1-side));
    
    if (king_in_check) {
        depth++;
    }

    //base case of recursion: use quiescence search instead of static evaluate
    if (depth<=0)
        return quiescence(alpha, beta);

    // Reverse Futility Pruning (Static Null Move Pruning)
    if (depth <= 2 && !king_in_check) {
        int static_eval = evaluate();
        int margin = depth * 200;
        if (static_eval - margin >= beta) {
            return static_eval; 
        }
    }

    // Null Move Pruning (NMP)
    // Zugzwang safety: only allow NMP if we have non-pawn material
    U64 non_pawn_king = occupancies[side] & ~(bitboards[K] | bitboards[k] | bitboards[P] | bitboards[p]);
    if (depth >= 3 && ply > 0 && !king_in_check && non_pawn_king)
    {
        // Make a null move (pass turn)
        int ep_copy = enpassant;
        U64 hash_copy = hash_key;
        
        side ^= 1;
        enpassant = no_square;
        ply++;
        hash_key = generate_hash_key(); // MUST regenerate hash for TT accuracy
        
        // Search with reduced depth (R=2) and zero-width window
        int null_score = -negamax(-beta, -beta + 1, depth - 1 - 2);
        
        // Unmake null move
        ply--;
        side ^= 1;
        enpassant = ep_copy;
        hash_key = hash_copy;
        
        if (null_score >= beta)
            return beta;
    }

    MoveList move_list;
    generate_moves(move_list);
    sort_moves(move_list, pv_move);

    int legal_moves=0;
    int moves_searched=0;
    int hash_flag = hash_alpha;
    int local_best_move = 0;

    for (int i=0;i<move_list.index;i++)
    {
        copy_board();
        ply++;

        if (make_move(move_list.moves[i], all_moves))
        {            
            legal_moves++;
            int childscore;

            // Late Move Reductions (LMR)
            // If we've searched a few moves (meaning they were likely best), and depth is high enough,
            // and this is a quiet move (not a capture, not in check), search it shallower.
            bool is_capture = decode_move_capture(move_list.moves[i]);
            
            // Note: king_in_check is for the CURRENT node, we should check if the new move gives check
            // For simplicity, we just check if it's not a capture, not a promotion, and depth > 2
            bool is_promotion = decode_move_promo_piece(move_list.moves[i]) != 0;
            
            if (moves_searched >= 4 && depth >= 3 && !king_in_check && !is_capture && !is_promotion) {
                // Reduced depth search
                childscore = -1 * negamax(-1*alpha - 1, -1*alpha, depth-2);
                
                // If it fails high, do a full depth search
                if (childscore > alpha) {
                    childscore = -1 * negamax(-1*alpha - 1, -1*alpha, depth-1);
                    if (childscore > alpha && childscore < beta) {
                        childscore = -1 * negamax(-1*beta, -1*alpha, depth-1);
                    }
                }
            } else {
                // Principal Variation Search (PVS)
                if (moves_searched == 0)
                {
                    childscore = -1 * negamax(-1*beta, -1*alpha, depth-1);
                }
                else
                {
                    childscore = -1 * negamax(-1*alpha - 1, -1*alpha, depth-1);
                    if (childscore > alpha && childscore < beta)
                    {
                        childscore = -1 * negamax(-1*beta, -1*alpha, depth-1);
                    }
                }
            }
            
            moves_searched++;
            ply--;
            take_back();

            if (childscore>=beta)
            {
                best_move = move_list.moves[i];
                write_tt(beta, depth, hash_beta);
                
                // Quiet move caused a beta cutoff
                if (!decode_move_capture(move_list.moves[i])) {
                    if (ply < MAX_PLY) {
                        killer_moves[1][ply] = killer_moves[0][ply];
                        killer_moves[0][ply] = move_list.moves[i];
                    }
                    history_moves[decode_move_piece(move_list.moves[i])][decode_move_target(move_list.moves[i])] += depth * depth;
                }
                
                return beta;
            }

            if (childscore>alpha) 
            {
                hash_flag = hash_exact;
                local_best_move = move_list.moves[i];
                
                if (ply==0) 
                {
                    best_move = move_list.moves[i];
                }
            }
            alpha = max(alpha, childscore);
        }
        else
        {            
            ply--;
        }            
    }
    
    if (legal_moves==0)
    {
        if (king_in_check)
            return -49000+ply;
        else 
            return 0; 
    }

    best_move = local_best_move;
    write_tt(alpha, depth, hash_flag);
    return alpha;
}

//search position for best move
void search_position(int depth);

#endif