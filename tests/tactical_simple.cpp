/**
 * Simplified Tactical Test for Quiescence Search
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include "core/board.h"
#include "core/types.h"
#include "evaluation/evaluate.h"
#include "search/negamax.h"
#include "search/quiescence.h"
#include "search/types.h"

using namespace seajay;
using namespace seajay::search;

std::string formatMove(Move move) {
    if (move == NO_MOVE) return "none";
    
    Square from = moveFrom(move);
    Square to = moveTo(move);
    
    std::string result;
    result += static_cast<char>('a' + fileOf(from));
    result += static_cast<char>('1' + rankOf(from));
    result += static_cast<char>('a' + fileOf(to));
    result += static_cast<char>('1' + rankOf(to));
    
    return result;
}

void testQuiescenceDirectly() {
    std::cout << "\n=== Direct Quiescence Test ===" << std::endl;
    
    // Simple position with hanging piece
    std::string fen = "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3";
    std::cout << "FEN: " << fen << std::endl;
    
    Board board;
    std::cout << "Loading FEN..." << std::endl;
    if (!board.fromFEN(fen)) {
        std::cout << "ERROR: Invalid FEN" << std::endl;
        return;
    }
    std::cout << "FEN loaded successfully" << std::endl;
    
    // Static eval
    eval::Score staticEval = eval::evaluate(board);
    std::cout << "Static eval: " << staticEval.to_cp() << " cp" << std::endl;
    
    // Call quiescence directly
    SearchInfo searchInfo;
    searchInfo.clear();
    
    SearchData searchData;
    searchData.useQuiescence = true;
    searchData.startTime = std::chrono::steady_clock::now();
    
    // Need a TT for quiescence
    TranspositionTable tt(16);
    
    std::cout << "Calling quiescence search..." << std::endl;
    eval::Score qScore = quiescence(board, 0, 
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, searchData, tt);
    
    std::cout << "Quiescence score: " << qScore.to_cp() << " cp" << std::endl;
    std::cout << "Q-nodes: " << searchData.qsearchNodes << std::endl;
    std::cout << "Stand-pats: " << searchData.standPatCutoffs << std::endl;
    std::cout << "Q-cutoffs: " << searchData.qsearchCutoffs << std::endl;
    
    if (searchData.qsearchNodesLimited > 0) {
        std::cout << "Hit node limit: " << searchData.qsearchNodesLimited << " times" << std::endl;
    }
}

void testSearchWithQuiescence() {
    std::cout << "\n=== Search with Quiescence Test ===" << std::endl;
    
    // Back rank mate position
    std::string fen = "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1";
    std::cout << "FEN: " << fen << std::endl;
    std::cout << "Expected: White should find Rd8# (back rank mate)" << std::endl;
    
    Board board;
    if (!board.fromFEN(fen)) {
        std::cout << "ERROR: Invalid FEN" << std::endl;
        return;
    }
    
    // Search with quiescence
    SearchLimits limits;
    limits.maxDepth = 6;
    limits.movetime = std::chrono::milliseconds(1000);
    limits.useQuiescence = true;
    
    std::cout << "Searching with depth " << limits.maxDepth << "..." << std::endl;
    
    TranspositionTable tt(16);
    Move bestMove = seajay::search::search(board, limits, &tt);
    
    std::cout << "Best move: " << formatMove(bestMove) << std::endl;
    
    // Verify it's the right move
    if (moveFrom(bestMove) == D1 && moveTo(bestMove) == D8) {
        std::cout << "SUCCESS: Found back rank mate!" << std::endl;
    } else {
        std::cout << "WARNING: Did not find expected mate move" << std::endl;
    }
}

void testPromotionRace() {
    std::cout << "\n=== Promotion Race Test ===" << std::endl;
    
    std::string fen = "8/1P6/8/8/8/8/1p6/R6K b - - 0 1";
    std::cout << "FEN: " << fen << std::endl;
    std::cout << "Black to move - should promote with b1=Q" << std::endl;
    
    Board board;
    if (!board.fromFEN(fen)) {
        std::cout << "ERROR: Invalid FEN" << std::endl;
        return;
    }
    
    // Search
    SearchLimits limits;
    limits.maxDepth = 8;
    limits.movetime = std::chrono::milliseconds(1000);
    limits.useQuiescence = true;
    
    TranspositionTable tt(16);
    Move bestMove = seajay::search::search(board, limits, &tt);
    
    std::cout << "Best move: " << formatMove(bestMove) << std::endl;
    
    // Check if it's a promotion
    if (moveFlags(bestMove) & PROMOTION) {
        std::cout << "SUCCESS: Found promotion!" << std::endl;
    } else {
        std::cout << "WARNING: Did not find promotion" << std::endl;
    }
}

int main() {
    std::cout << "SeaJay Quiescence Search Validation" << std::endl;
    std::cout << "Stage 14 - Phase 1.11: Tactical Testing" << std::endl;
    
    #ifdef QSEARCH_TESTING
    std::cout << "Mode: TESTING (10K node limit per position)" << std::endl;
    #elif defined(QSEARCH_TUNING)
    std::cout << "Mode: TUNING (100K node limit per position)" << std::endl;
    #else
    std::cout << "Mode: PRODUCTION (no limits)" << std::endl;
    #endif
    
    std::cout << std::string(50, '=') << std::endl;
    
    // Run tests
    testQuiescenceDirectly();
    testSearchWithQuiescence();
    testPromotionRace();
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "Tactical validation complete!" << std::endl;
    
    return 0;
}