// Test program for Stage 15 Day 5: SEE Integration
#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/search/move_ordering.h"

using namespace seajay;
using namespace seajay::search;

void testParallelScoring(const std::string& fen) {
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Invalid FEN: " << fen << "\n";
        return;
    }
    
    std::cout << "\n=== Testing position: " << fen << " ===\n";
    
    // Generate all legal moves
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Generated " << moves.size() << " legal moves\n";
    
    // Test parallel scoring in different modes
    for (auto mode : {SEEMode::OFF, SEEMode::TESTING, SEEMode::SHADOW, SEEMode::PRODUCTION}) {
        std::cout << "\n--- Mode: " << seeModeToString(mode) << " ---\n";
        
        // Set the mode
        g_seeMoveOrdering.setMode(mode);
        
        // Reset statistics
        SEEMoveOrdering::getStats().reset();
        
        // Get parallel scores for captures only
        MoveList captures;
        for (Move move : moves) {
            if (isCapture(move) || isPromotion(move) || isEnPassant(move)) {
                captures.push_back(move);
            }
        }
        
        if (captures.empty()) {
            std::cout << "No captures in this position\n";
            continue;
        }
        
        // Score moves in parallel
        auto scores = g_seeMoveOrdering.scoreMovesParallel(board, captures);
        
        // Display results
        std::cout << "Scored " << scores.size() << " captures:\n";
        
        // Show top 5 moves
        int shown = 0;
        for (const auto& ps : scores) {
            if (shown++ >= 5) break;
            
            Square from = moveFrom(ps.move);
            Square to = moveTo(ps.move);
            
            std::cout << "  " << squareToString(from) << squareToString(to);
            if (isPromotion(ps.move)) {
                const char promoChars[] = "nbrq";
                PieceType pt = promotionType(ps.move);
                if (pt >= KNIGHT && pt <= QUEEN) {
                    std::cout << promoChars[pt - KNIGHT];
                }
            }
            
            std::cout << std::fixed << std::setw(12)
                      << " MVV=" << std::setw(4) << ps.mvvLvaScore
                      << " SEE=" << std::setw(5) << ps.seeValue
                      << " " << (ps.agree ? "AGREE" : "DIFFER") << "\n";
        }
        
        // Print statistics
        auto& stats = SEEMoveOrdering::getStats();
        std::cout << "\nStatistics:\n";
        std::cout << "  Total comparisons: " << stats.totalComparisons << "\n";
        std::cout << "  Agreement rate: " << std::fixed << std::setprecision(1) 
                  << stats.agreementRate() << "%\n";
        std::cout << "  SEE preferred: " << stats.seePreferred << "\n";
        std::cout << "  MVV-LVA preferred: " << stats.mvvLvaPreferred << "\n";
        std::cout << "  Equal scores: " << stats.equalScores << "\n";
    }
}

void testMoveOrdering(const std::string& fen) {
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Invalid FEN: " << fen << "\n";
        return;
    }
    
    std::cout << "\n=== Testing move ordering: " << fen << " ===\n";
    
    // Generate all legal moves
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    // Order with MVV-LVA (OFF mode)
    g_seeMoveOrdering.setMode(SEEMode::OFF);
    MoveList mvvMoves = moves;
    g_seeMoveOrdering.orderMoves(board, mvvMoves);
    
    // Order with SEE (PRODUCTION mode)
    g_seeMoveOrdering.setMode(SEEMode::PRODUCTION);
    MoveList seeMoves = moves;
    g_seeMoveOrdering.orderMoves(board, seeMoves);
    
    // Compare top 10 moves
    std::cout << "\nTop 10 moves comparison:\n";
    std::cout << std::setw(5) << "#" << std::setw(15) << "MVV-LVA" 
              << std::setw(15) << "SEE" << std::setw(10) << "Same?\n";
    std::cout << std::string(45, '-') << "\n";
    
    for (size_t i = 0; i < std::min(size_t(10), moves.size()); ++i) {
        Move mvvMove = mvvMoves[i];
        Move seeMove = seeMoves[i];
        
        std::string mvvStr = squareToString(moveFrom(mvvMove)) + squareToString(moveTo(mvvMove));
        std::string seeStr = squareToString(moveFrom(seeMove)) + squareToString(moveTo(seeMove));
        
        if (isPromotion(mvvMove)) {
            const char promoChars[] = "nbrq";
            PieceType pt = promotionType(mvvMove);
            if (pt >= KNIGHT && pt <= QUEEN) mvvStr += promoChars[pt - KNIGHT];
        }
        
        if (isPromotion(seeMove)) {
            const char promoChars[] = "nbrq";
            PieceType pt = promotionType(seeMove);
            if (pt >= KNIGHT && pt <= QUEEN) seeStr += promoChars[pt - KNIGHT];
        }
        
        std::cout << std::setw(5) << (i+1) 
                  << std::setw(15) << mvvStr
                  << std::setw(15) << seeStr
                  << std::setw(10) << (mvvMove == seeMove ? "YES" : "NO")
                  << "\n";
    }
}

int main() {
    std::cout << "Stage 15 Day 5: SEE Integration Test\n";
    std::cout << "=====================================\n";
    
    // Test positions with interesting captures
    std::vector<std::string> testPositions = {
        // Starting position (no captures)
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        
        // Position with multiple captures
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        
        // Position with hanging pieces
        "rnbqk1nr/pppp1ppp/8/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
        
        // Complex tactical position
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
        
        // Endgame with pawn captures
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
    };
    
    for (const auto& fen : testPositions) {
        testParallelScoring(fen);
    }
    
    std::cout << "\n\n=== Move Ordering Comparison ===\n";
    
    // Test move ordering differences
    for (const auto& fen : testPositions) {
        testMoveOrdering(fen);
    }
    
    // Final statistics
    std::cout << "\n\n=== Final Statistics ===\n";
    g_seeMoveOrdering.setMode(SEEMode::PRODUCTION);
    SEEMoveOrdering::getStats().reset();
    
    // Run through all positions in production mode
    for (const auto& fen : testPositions) {
        Board board;
        if (board.fromFEN(fen)) {
            MoveList moves;
            MoveGenerator::generateLegalMoves(board, moves);
            g_seeMoveOrdering.orderMoves(board, moves);
        }
    }
    
    SEEMoveOrdering::getStats().print(std::cout);
    
    return 0;
}