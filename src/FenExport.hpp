/*##########################

FEN Export Utility
Reads global engine state and produces a FEN string.
This is a READ-ONLY utility — it does NOT modify any engine globals.

#############################*/
#ifndef FENEXPORT_HPP
#define FENEXPORT_HPP

#include "config.hpp"
#include "Board.hpp"
#include <string>
#include <sstream>

inline std::string ChessEngine::export_fen()
{
    std::ostringstream fen;

    // 1. Piece placement
    for (int rank = 0; rank < 8; rank++)
    {
        int empty_count = 0;
        for (int file = 0; file < 8; file++)
        {
            int square = 8 * rank + file;
            int piece_found = -1;
            for (int piece = P; piece <= k; piece++)
            {
                if (get_bit(bitboards[piece], square))
                {
                    piece_found = piece;
                    break;
                }
            }
            if (piece_found == -1)
            {
                empty_count++;
            }
            else
            {
                if (empty_count > 0)
                {
                    fen << empty_count;
                    empty_count = 0;
                }
                fen << ascii_pieces[piece_found];
            }
        }
        if (empty_count > 0)
            fen << empty_count;
        if (rank < 7)
            fen << '/';
    }

    // 2. Side to move
    fen << ' ' << (side == white ? 'w' : 'b');

    // 3. Castling availability
    fen << ' ';
    if (castle == 0)
    {
        fen << '-';
    }
    else
    {
        if (castle & wk) fen << 'K';
        if (castle & wq) fen << 'Q';
        if (castle & bk) fen << 'k';
        if (castle & bq) fen << 'q';
    }

    // 4. En passant target square
    fen << ' ';
    if (enpassant == no_square)
    {
        fen << '-';
    }
    else
    {
        fen << square_to_board[enpassant];
    }

    // 5. Halfmove clock (fifty-move rule)
    fen << ' ' << fifty;

    // 6. Fullmove number (we don't track this, default to 1)
    fen << ' ' << 1;

    return fen.str();
}

#endif
