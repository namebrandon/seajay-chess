// Test program to identify SEE evaluation bug
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/evaluation/evaluate.h"
#include "src/core/see.h"
#include "src/core/fen.h"

using namespace seajay;

struct TestPosition {
    std::string fen;
    std::string description;
    std::vector<std::string> moves;
};

int main() {
    std::cout << "SEE Evaluation Bug Investigation\n";
    std::cout << "=================================\n\n";
    
    // Test positions where bug might manifest
    std::vector<TestPosition> positions = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position", {}},
        {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", "After 1.e4", {"e2e4"}},
        {"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2", "After 1.e4 c5", {"e2e4", "c7c5"}},
        {"rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2", "After 1.e4 Nf6", {"e2e4", "g8f6"}},
        {"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", "Italian-like position", {"e2e4", "e7e5", "g1f3", "b8c6"}},
    };
    
    // Initialize SEE calculator
    SEECalculator seeCalc;
    
    for (const auto& test : positions) {
        Board board;
        if (!parseFEN(test.fen, board)) {
            std::cerr << "Failed to parse FEN: " << test.fen << "\n";
            continue;
        }
        
        // Apply moves if any
        for (const auto& moveStr : test.moves) {
            Move move = parseMove(board, moveStr);
            if (move == NO_MOVE) {
                std::cerr << "Failed to parse move: " << moveStr << "\n";
                break;
            }
            board.makeMove(move);
        }
        
        std::cout << "Position: " << test.description << "\n";
        std::cout << "FEN: " << board.toFEN() << "\n";
        std::cout << "Side to move: " << (board.sideToMove() == WHITE ? "White" : "Black") << "\n";
        
        // Get static evaluation
        eval::Score staticEval = eval::evaluate(board);
        std::cout << "Static eval: " << staticEval.value() << " cp\n";
        
        // Generate all moves and check SEE values
        MoveList moves;
        generateMoves(board, moves);
        
        int captureCount = 0;
        int totalSEE = 0;
        int maxSEE = -10000;
        int minSEE = 10000;
        
        std::cout << "\nCapture SEE values:\n";
        for (Move move : moves) {
            if (isCapture(move)) {
                SEEValue see = seeCalc.see(board, move);
                captureCount++;
                totalSEE += see;
                maxSEE = std::max(maxSEE, (int)see);
                minSEE = std::min(minSEE, (int)see);
                
                if (std::abs(see) == 290 || std::abs(see) == 289 || std::abs(see) == 291) {
                    std::cout << "  SUSPICIOUS: " << moveToString(move) 
                              << " SEE=" << see << " cp ***\n";
                }
            }
        }
        
        if (captureCount > 0) {
            std::cout << "  Captures: " << captureCount << "\n";
            std::cout << "  Average SEE: " << (totalSEE / captureCount) << " cp\n";
            std::cout << "  Max SEE: " << maxSEE << " cp\n";
            std::cout << "  Min SEE: " << minSEE << " cp\n";
            
            // Check if average is near 290
            int avgSEE = totalSEE / captureCount;
            if (std::abs(avgSEE - 290) < 10 || std::abs(avgSEE + 290) < 10) {
                std::cout << "  *** SUSPICIOUS AVERAGE NEAR ±290! ***\n";
            }
        }
        
        // Check for any hardcoded 290 value
        if (std::abs(staticEval.value() - 290) < 10 || 
            std::abs(staticEval.value() + 290) < 10) {
            std::cout << "\n*** EVALUATION NEAR ±290 DETECTED! ***\n";
        }
        
        std::cout << "\n" << std::string(50, '-') << "\n\n";
    }
    
    // Test SEE cache behavior
    std::cout << "Testing SEE Cache Behavior:\n";
    std::cout << "===========================\n";
    
    Board board;
    parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", board);
    board.makeMove(parseMove(board, "e2e4"));
    
    // Test the same capture multiple times
    Move testCapture = parseMove(board, "d7d5"); // Not a capture, but test
    if (testCapture != NO_MOVE) {
        board.makeMove(testCapture);
        Move capture = parseMove(board, "e4d5"); // This would be a capture
        
        if (capture != NO_MOVE && isCapture(capture)) {
            std::cout << "Testing repeated SEE calls for " << moveToString(capture) << ":\n";
            for (int i = 0; i < 5; i++) {
                SEEValue see = seeCalc.see(board, capture);
                std::cout << "  Call " << (i+1) << ": SEE = " << see << " cp\n";
                if (std::abs(see) == 290) {
                    std::cout << "    *** FOUND 290 VALUE! ***\n";
                }
            }
        }
    }
    
    // Check cache statistics
    auto stats = seeCalc.getStats();
    std::cout << "\nSEE Cache Statistics:\n";
    std::cout << "  Total calls: " << stats.totalCalls << "\n";
    std::cout << "  Cache hits: " << stats.cacheHits << "\n";
    std::cout << "  Cache misses: " << stats.cacheMisses << "\n";
    std::cout << "  Hit rate: " << std::fixed << std::setprecision(1) 
              << stats.hitRate() << "%\n";
    
    return 0;
}