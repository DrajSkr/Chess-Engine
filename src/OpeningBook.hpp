#ifndef OPENINGBOOK_HPP
#define OPENINGBOOK_HPP

#include <string>
#include <vector>
#include <map>
#include <cstdlib>

inline std::string get_book_move(const std::string& fen) {
    // A tiny, hardcoded opening book to introduce variety from the start position.
    // FEN strings should match exactly as exported by the engine.
    static std::map<std::string, std::vector<std::string>> book = {
        // Initial position (White to move)
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", {"e2e4", "d2d4", "c2c4", "g1f3"}},
        
        // 1. e4 (Black to move)
        {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", {"e7e5", "c7c5", "e7e6", "c7c6"}},
        // 1. d4 (Black to move)
        {"rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 1", {"d7d5", "g8f6", "e7e6"}},
        // 1. c4 (Black to move)
        {"rnbqkbnr/pppppppp/8/8/2P5/8/PP1PPPPP/RNBQKBNR b KQkq c3 0 1", {"e7e5", "c7c5", "g8f6", "e7e6"}},
        // 1. Nf3 (Black to move)
        {"rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1", {"d7d5", "g8f6", "c7c5"}},

        // 1. e4 e5
        {"rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2", {"g1f3", "f2f4", "b1c3"}},
        // 1. e4 c5 (Sicilian)
        {"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2", {"g1f3", "b1c3"}},
        // 1. e4 e6 (French)
        {"rnbqkbnr/pppp1ppp/4p3/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", {"d2d4", "d2d3"}},
        
        // 1. d4 d5
        {"rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq d6 0 2", {"c2c4", "g1f3", "c1f4"}},
        // 1. d4 Nf6
        {"rnbqkb1r/pppppppp/5n2/8/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 1 2", {"c2c4", "g1f3"}},
        
        // 1. e4 e5 2. Nf3
        {"rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2", {"b8c6", "g8f6", "d7d6"}},
        // 1. e4 c5 2. Nf3
        {"rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2", {"d7d6", "e7e6", "b8c6"}},
        
        // 1. d4 d5 2. c4
        {"rnbqkbnr/ppp1pppp/8/3p4/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2", {"e7e6", "c7c6", "d5c4"}},
        // 1. d4 Nf6 2. c4
        {"rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2", {"e7e6", "g7g6"}},
    };

    auto it = book.find(fen);
    if (it != book.end()) {
        const auto& moves = it->second;
        if (!moves.empty()) {
            int random_index = rand() % moves.size();
            return moves[random_index];
        }
    }
    return ""; // No book move found
}

#endif
