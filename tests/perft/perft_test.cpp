#include <iostream>
#include <chrono>
#include <iomanip>
#include "../../src/core/board.h"
#include "../../src/core/move_generation.h"
#include "../../src/core/move_list.h"

using namespace seajay;
using namespace std::chrono;

// Perft function - counts nodes at given depth
uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    if (depth == 1) return moves.size();
    
    uint64_t nodes = 0;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        nodes += perft(board, depth - 1);
        board.unmakeMove(move, undo);
    }
    
    return nodes;
}

// Perft divide - shows node count for each root move
void perftDivide(Board& board, int depth) {
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    uint64_t total = 0;
    std::cout << "\nPerft divide at depth " << depth << ":\n";
    std::cout << "--------------------------------\n";
    
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        uint64_t nodes = (depth > 1) ? perft(board, depth - 1) : 1;
        board.unmakeMove(move, undo);
        
        // Format move string
        std::string moveStr = squareToString(moveFrom(move)) + squareToString(moveTo(move));
        if (isPromotion(move)) {
            PieceType promo = promotionType(move);
            const char promoChar[] = {'?', 'n', 'b', 'r', 'q'};
            moveStr += promoChar[promo];
        }
        
        std::cout << std::left << std::setw(8) << moveStr << ": " << nodes << "\n";
        total += nodes;
    }
    
    std::cout << "--------------------------------\n";
    std::cout << "Total: " << total << "\n\n";
}

// Test position structure
struct PerftTest {
    const char* name;
    const char* fen;
    int depth;
    uint64_t expected;
};

int main() {
    std::cout << "SeaJay Chess Engine - Perft Test Suite\n";
    std::cout << "=======================================\n\n";
    
    // Standard perft test positions
    PerftTest tests[] = {
        // Starting position
        {"Starting Position - Depth 1", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20},
        {"Starting Position - Depth 2", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400},
        {"Starting Position - Depth 3", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902},
        {"Starting Position - Depth 4", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281},
        {"Starting Position - Depth 5", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609},
        
        // Kiwipete position - tests many special moves
        {"Kiwipete - Depth 1", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48},
        {"Kiwipete - Depth 2", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039},
        {"Kiwipete - Depth 3", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862},
        {"Kiwipete - Depth 4", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603},
        
        // Position 3 - Promotions and checks
        {"Position 3 - Depth 1", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1, 14},
        {"Position 3 - Depth 2", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2, 191},
        {"Position 3 - Depth 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 3, 2812},
        {"Position 3 - Depth 4", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238},
        
        // Position 4 - Castling and en passant
        {"Position 4 - Depth 1", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 1, 6},
        {"Position 4 - Depth 2", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2, 264},
        {"Position 4 - Depth 3", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467},
        {"Position 4 - Depth 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333},
        
        // Position 5 - Complex middle game
        {"Position 5 - Depth 1", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 1, 44},
        {"Position 5 - Depth 2", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 2, 1486},
        {"Position 5 - Depth 3", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379},
        {"Position 5 - Depth 4", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487},
        
        // Position 6 - Complex middlegame (from master project plan)
        {"Position 6 - Depth 1", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 1, 46},
        {"Position 6 - Depth 2", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 2, 2079},
        {"Position 6 - Depth 3", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890},
        {"Position 6 - Depth 4", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594}
    };
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        Board board;
        bool result = board.fromFEN(test.fen);
        
        if (!result) {
            std::cout << "❌ " << test.name << "\n";
            std::cout << "   Failed to parse FEN\n\n";
            failed++;
            continue;
        }
        
        auto start = high_resolution_clock::now();
        uint64_t nodes = perft(board, test.depth);
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);
        
        bool success = (nodes == test.expected);
        
        std::cout << (success ? "✅ " : "❌ ") << test.name << "\n";
        std::cout << "   Expected: " << test.expected << "\n";
        std::cout << "   Got:      " << nodes << "\n";
        std::cout << "   Time:     " << duration.count() << " ms\n";
        
        if (!success) {
            std::cout << "   Diff:     " << static_cast<int64_t>(nodes - test.expected) << "\n";
            
            // Show perft divide for failed low-depth tests
            if (test.depth <= 2) {
                board.fromFEN(test.fen);
                perftDivide(board, test.depth);
            }
            failed++;
        } else {
            passed++;
        }
        std::cout << "\n";
    }
    
    std::cout << "=======================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    
    if (failed > 0) {
        std::cout << "\n⚠️  Move generation has errors that need to be fixed.\n";
    } else {
        std::cout << "\n✅ All perft tests passed! Move generation is correct.\n";
    }
    
    return failed > 0 ? 1 : 0;
}