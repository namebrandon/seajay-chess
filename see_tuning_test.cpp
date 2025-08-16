// Stage 15 Day 8.1-8.3: SEE Parameter Tuning Test Program
// This program facilitates testing different SEE parameters

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/see.h"
#include "src/search/negamax.h"
#include "src/evaluation/evaluate.h"
#include "src/uci/uci.h"  // For move parsing

using namespace seajay;

// Test positions for SEE tuning
struct TestPosition {
    std::string fen;
    std::string move;
    std::string description;
    int expectedSEE;  // Expected SEE value with current parameters
};

// Collection of test positions that stress different aspects of SEE
std::vector<TestPosition> testPositions = {
    // Pawn captures
    {"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", "e4d5", "PxP undefended", 100},
    {"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", "f3e5", "NxP defended by N", -225},
    
    // Knight exchanges
    {"r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 4 5", "f3e5", "NxP defended", -225},
    {"r1bqkb1r/pppp1ppp/2n5/4p3/3nP3/2N5/PPPP1PPP/R1BQKB1R w KQkq - 0 5", "c3d5", "NxN equal", 0},
    
    // Bishop exchanges
    {"r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 6", "c4f7", "BxP check", 100},
    
    // Rook captures
    {"r3k2r/ppp2ppp/2n2n2/2bpp3/2B1P3/3P1N2/PPP2PPP/R3K2R w KQkq - 0 9", "a1a7", "RxP undefended", 100},
    {"r3k2r/Ppp2ppp/2n2n2/2bpp3/2B1P3/3P1N2/1PP2PPP/R3K2R b KQkq - 0 9", "a8a7", "RxP promotion", 100},
    
    // Queen captures
    {"r2qk2r/ppp2ppp/2n2n2/2bpp3/2B1P3/3P1N2/PPP2PPP/R2QK2R w KQkq - 0 9", "d1a4", "Q attacks", 0},
    {"r2q1rk1/ppp2ppp/2n2n2/2bpp3/2B1P3/3P1N2/PPP2PPP/R2Q1RK1 w - - 0 10", "d1d5", "QxP defended", -875},
    
    // Complex exchanges
    {"r1bq1rk1/ppp2ppp/2n2n2/2bpp3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 0 10", "f3e5", "Complex exchange", -225},
    {"2r2rk1/p4ppp/1p2pn2/3p4/1b1P4/2NBPN2/PP3PPP/2R2RK1 w - - 0 15", "c3b5", "N forks", 100},
    
    // X-ray situations
    {"r3k2r/p1p2ppp/2n5/3p4/1b1P4/2N1PN2/PP3PPP/R3KB1R w KQkq - 0 12", "c3b5", "X-ray on rook", -225},
    {"r3kb1r/p1p2ppp/2n2n2/3p4/3P4/2N1PN2/PP3PPP/R3KB1R w KQkq - 0 12", "c3d5", "N defended by bishop x-ray", -325},
};

// Helper to parse UCI move for a given board position
Move parseMove(const Board& board, const std::string& moveStr) {
    if (moveStr.length() < 4 || moveStr.length() > 5) {
        return NO_MOVE;
    }
    
    // Parse from and to squares
    Square from = static_cast<Square>((moveStr[1] - '1') * 8 + (moveStr[0] - 'a'));
    Square to = static_cast<Square>((moveStr[3] - '1') * 8 + (moveStr[2] - 'a'));
    
    // Check for promotion
    PieceType promType = NO_PIECE_TYPE;
    if (moveStr.length() == 5) {
        switch (moveStr[4]) {
            case 'q': promType = QUEEN; break;
            case 'r': promType = ROOK; break;
            case 'b': promType = BISHOP; break;
            case 'n': promType = KNIGHT; break;
        }
    }
    
    // Generate legal moves to find the matching one
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    for (Move move : moves) {
        if (moveFrom(move) == from && moveTo(move) == to) {
            if (promType != NO_PIECE_TYPE) {
                if (isPromotion(move) && promotionType(move) == promType) {
                    return move;
                }
            } else {
                return move;
            }
        }
    }
    
    return NO_MOVE;
}

// Function to test SEE with different parameters
void testSEEParameters(int pawnValue, int knightValue, int bishopValue, 
                       int rookValue, int queenValue) {
    std::cout << "\n=== Testing SEE with piece values ===" << std::endl;
    std::cout << "P=" << pawnValue << " N=" << knightValue << " B=" << bishopValue 
              << " R=" << rookValue << " Q=" << queenValue << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    SEECalculator see;
    int totalError = 0;
    int correctCount = 0;
    
    for (const auto& test : testPositions) {
        Board board;
        if (!board.fromFEN(test.fen)) {
            std::cout << "ERROR: Invalid FEN " << test.fen << std::endl;
            continue;
        }
        
        Move move = parseMove(board, test.move);
        
        if (move == NO_MOVE) {
            std::cout << "ERROR: Invalid move " << test.move << " in position " << test.fen << std::endl;
            continue;
        }
        
        SEEValue seeValue = see.see(board, move);
        int error = std::abs(seeValue - test.expectedSEE);
        totalError += error;
        
        if (error == 0) correctCount++;
        
        std::cout << std::setw(6) << test.move << " : " 
                  << std::setw(6) << seeValue 
                  << " (expected " << std::setw(6) << test.expectedSEE << ")"
                  << " | " << test.description;
        
        if (error > 0) {
            std::cout << " [ERROR: " << error << "]";
        } else {
            std::cout << " [OK]";
        }
        std::cout << std::endl;
    }
    
    std::cout << std::string(60, '-') << std::endl;
    std::cout << "Results: " << correctCount << "/" << testPositions.size() 
              << " correct, Total error: " << totalError << std::endl;
}

// Function to test pruning margins
void testPruningMargins(int conservativeMargin, int aggressiveMargin, int endgameMargin) {
    std::cout << "\n=== Testing Pruning Margins ===" << std::endl;
    std::cout << "Conservative=" << conservativeMargin 
              << " Aggressive=" << aggressiveMargin 
              << " Endgame=" << endgameMargin << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // Test how many captures would be pruned with these margins
    SEECalculator see;
    int conservativePruned = 0;
    int aggressivePruned = 0;
    int endgamePruned = 0;
    int totalCaptures = 0;
    
    for (const auto& test : testPositions) {
        Board board;
        if (!board.fromFEN(test.fen)) {
            continue;
        }
        
        // Generate all captures
        MoveList moves;
        MoveGenerator::generateCaptures(board, moves);
        
        for (Move move : moves) {
            SEEValue seeValue = see.see(board, move);
            totalCaptures++;
            
            if (seeValue < conservativeMargin) conservativePruned++;
            if (seeValue < aggressiveMargin) aggressivePruned++;
            if (seeValue < endgameMargin) endgamePruned++;
        }
    }
    
    std::cout << "Total captures analyzed: " << totalCaptures << std::endl;
    std::cout << "Conservative (" << conservativeMargin << "): " 
              << conservativePruned << " pruned ("
              << std::fixed << std::setprecision(1) 
              << (100.0 * conservativePruned / totalCaptures) << "%)" << std::endl;
    std::cout << "Aggressive (" << aggressiveMargin << "): " 
              << aggressivePruned << " pruned ("
              << std::fixed << std::setprecision(1) 
              << (100.0 * aggressivePruned / totalCaptures) << "%)" << std::endl;
    std::cout << "Endgame (" << endgameMargin << "): " 
              << endgamePruned << " pruned ("
              << std::fixed << std::setprecision(1) 
              << (100.0 * endgamePruned / totalCaptures) << "%)" << std::endl;
}

// Function to run performance benchmark with current parameters
void runPerformanceBenchmark() {
    std::cout << "\n=== Performance Benchmark ===" << std::endl;
    std::cout << "Evaluating 1M random captures..." << std::endl;
    
    SEECalculator see;
    Board board;
    
    // Use a more complex middlegame position for realistic testing
    if (!board.fromFEN("r1bq1rk1/ppp2ppp/2n2n2/2bpp3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 0 10")) {
        std::cout << "ERROR: Failed to set up benchmark position!" << std::endl;
        return;
    }
    
    // Generate captures
    MoveList moves;
    MoveGenerator::generateCaptures(board, moves);
    
    if (moves.size() == 0) {
        std::cout << "ERROR: No captures in test position!" << std::endl;
        return;
    }
    
    const int iterations = 1000000 / moves.size();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    SEEValue totalValue = 0;
    for (int i = 0; i < iterations; i++) {
        for (Move move : moves) {
            totalValue += see.see(board, move);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Time for " << (iterations * moves.size()) << " evaluations: " 
              << duration.count() << " ms" << std::endl;
    std::cout << "Evaluations per second: " 
              << (iterations * moves.size() * 1000) / duration.count() << std::endl;
    std::cout << "Checksum: " << totalValue << std::endl;
    
    // Get cache statistics
    const auto& stats = see.statistics();
    std::cout << "\nCache Statistics:" << std::endl;
    std::cout << "  Hits: " << stats.cacheHits << std::endl;
    std::cout << "  Misses: " << stats.cacheMisses << std::endl;
    std::cout << "  Hit rate: " << std::fixed << std::setprecision(1) 
              << stats.hitRate() << "%" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== SeaJay Stage 15 SEE Parameter Tuning ===" << std::endl;
    std::cout << "Day 8.1-8.3: Systematic parameter optimization" << std::endl;
    
    // Test different configurations
    if (argc > 1) {
        std::string mode = argv[1];
        
        if (mode == "margins") {
            // Test different margin values
            std::cout << "\n== Testing Margin Values ==" << std::endl;
            
            // Current baseline
            testPruningMargins(-100, -50, -25);
            
            // More conservative
            testPruningMargins(-150, -75, -50);
            
            // More aggressive  
            testPruningMargins(-75, -25, 0);
            
            // Very aggressive
            testPruningMargins(-50, 0, 25);
            
        } else if (mode == "pieces") {
            // Test different piece values
            std::cout << "\n== Testing Piece Values ==" << std::endl;
            
            // Current baseline
            testSEEParameters(100, 325, 325, 500, 975);
            
            // Stockfish-like values
            testSEEParameters(100, 320, 330, 500, 900);
            
            // Classical values
            testSEEParameters(100, 300, 300, 500, 900);
            
            // Compressed scale
            testSEEParameters(90, 293, 293, 450, 878);
            
            // Expanded scale
            testSEEParameters(110, 358, 358, 550, 1073);
            
        } else if (mode == "perf") {
            runPerformanceBenchmark();
        }
    } else {
        // Default: Run all tests
        std::cout << "\nUsage: " << argv[0] << " [margins|pieces|perf]" << std::endl;
        std::cout << "  margins - Test pruning margin values" << std::endl;
        std::cout << "  pieces  - Test piece values" << std::endl;
        std::cout << "  perf    - Run performance benchmark" << std::endl;
        std::cout << "\nRunning default test suite..." << std::endl;
        
        // Test current configuration
        testSEEParameters(100, 325, 325, 500, 975);
        testPruningMargins(-100, -50, -25);
        runPerformanceBenchmark();
    }
    
    return 0;
}