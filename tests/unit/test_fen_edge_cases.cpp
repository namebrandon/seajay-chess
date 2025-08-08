#include "../../src/core/board.h"
#include "../../src/core/bitboard.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace seajay;

// Test cases for known problematic positions from chess programming community
struct ProblematicFEN {
    std::string name;
    std::string fen;
    std::string description;
};

int main() {
    std::cout << "\n=== Testing Known Problematic FEN Positions ===\n\n";
    
    std::vector<ProblematicFEN> problematicPositions = {
        {
            "Kiwipete",
            "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
            "Famous perft test position with complex piece interactions"
        },
        {
            "Position 3 (from perft suite)",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            "Endgame position with potential en passant complications"
        },
        {
            "Position 4 (from perft suite)", 
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            "Complex middlegame with all piece types"
        },
        {
            "Position 5 (from perft suite)",
            "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            "Position with advanced pawn and knight on f2"
        },
        {
            "Position 6 (from perft suite)",
            "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
            "Symmetrical position testing edge cases"
        },
        {
            "Tricky castling position",
            "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
            "Both sides can castle, black to move"
        },
        {
            "En passant corner case",
            "rnbqkb1r/pppppppp/5n2/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 2",
            "En passant available but no capturing pawn"
        },
        {
            "Maximum material",
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "Starting position with all pieces"
        },
        {
            "Minimal material",
            "4k3/8/8/8/8/8/8/4K3 w - - 0 1",
            "Only kings on board"
        },
        {
            "Stalemate position",
            "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",
            "Black king is stalemated"
        },
        {
            "Complex en passant",
            "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
            "Double en passant opportunity"
        },
        {
            "Promotion squares edge case",
            "4k3/P6P/8/8/8/8/p6p/4K3 w - - 0 1",
            "Pawns one square from promotion (legal)"
        }
    };
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& pos : problematicPositions) {
        std::cout << "Testing: " << pos.name << "\n";
        std::cout << "Description: " << pos.description << "\n";
        std::cout << "FEN: " << pos.fen << "\n";
        
        Board board;
        bool parseResult = board.fromFEN(pos.fen);
        
        if (parseResult) {
            // Test round trip
            std::string regenerated = board.toFEN();
            if (regenerated == pos.fen) {
                std::cout << "✓ Passed - Round trip successful\n";
                passed++;
            } else {
                std::cout << "❌ Failed - Round trip mismatch\n";
                std::cout << "   Original:    " << pos.fen << "\n";
                std::cout << "   Regenerated: " << regenerated << "\n";
                failed++;
            }
        } else {
            std::cout << "❌ Failed - Could not parse FEN\n";
            failed++;
        }
        std::cout << "\n";
    }
    
    std::cout << "=== Summary ===\n";
    std::cout << "✅ Passed: " << passed << "\n";
    std::cout << "❌ Failed: " << failed << "\n\n";
    
    if (failed == 0) {
        std::cout << "🎉 All problematic positions handled correctly!\n";
        std::cout << "FEN implementation is robust and ready for production use.\n\n";
        return 0;
    } else {
        std::cerr << "⚠️  Some positions failed - FEN implementation needs attention.\n";
        return 1;
    }
}
