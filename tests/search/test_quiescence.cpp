#include "../../src/search/quiescence.h"
#include "../../src/search/types.h"
#include "../../src/core/board.h"
#include "../../src/core/transposition_table.h"
#include "../../src/evaluation/evaluate.h"
#include <iostream>
#include <cassert>

using namespace seajay;

void testStandPatBehavior() {
    std::cout << "Testing stand-pat behavior... ";
    
    Board board;
    board.setStartingPosition();
    SearchInfo searchInfo;
    search::SearchData data;
    TranspositionTable tt(16); // 16MB TT for testing
    
    int ply = 0;
    eval::Score alpha = eval::Score(-1000);
    eval::Score beta = eval::Score(1000);
    
    eval::Score result = search::quiescence(board, ply, alpha, beta, searchInfo, data, tt);
    
    // In starting position, quiescence should return a value within bounds
    assert(result >= alpha);
    assert(result <= beta);
    
    // Should have counted this node
    assert(data.qsearchNodes == 1);
    
    std::cout << "PASSED\n";
}

void testStandPatBetaCutoff() {
    std::cout << "Testing stand-pat beta cutoff... ";
    
    Board board;
    board.setStartingPosition();
    SearchInfo searchInfo;
    search::SearchData data;
    TranspositionTable tt(16);
    
    int ply = 0;
    eval::Score staticEval = eval::evaluate(board);
    eval::Score alpha = eval::Score(-1000);
    eval::Score beta = staticEval - eval::Score(100); // Beta below static eval
    
    eval::Score result = search::quiescence(board, ply, alpha, beta, searchInfo, data, tt);
    
    // Should cutoff and return static eval
    assert(result == staticEval);
    assert(data.standPatCutoffs == 1);
    
    std::cout << "PASSED\n";
}

void testMaxPlyReturnsEval() {
    std::cout << "Testing max ply returns eval... ";
    
    Board board;
    board.setStartingPosition();
    SearchInfo searchInfo;
    search::SearchData data;
    TranspositionTable tt(16);
    
    int ply = search::TOTAL_MAX_PLY;
    eval::Score alpha = eval::Score(-1000);
    eval::Score beta = eval::Score(1000);
    
    eval::Score result = search::quiescence(board, ply, alpha, beta, searchInfo, data, tt);
    eval::Score staticEval = eval::evaluate(board);
    
    assert(result == staticEval);
    
    std::cout << "PASSED\n";
}

void testSelectiveDepthTracking() {
    std::cout << "Testing selective depth tracking... ";
    
    Board board;
    board.setStartingPosition();
    SearchInfo searchInfo;
    search::SearchData data;
    TranspositionTable tt(16);
    
    int ply = 5;
    eval::Score alpha = eval::Score(-1000);
    eval::Score beta = eval::Score(1000);
    
    data.seldepth = 3; // Previous max depth
    search::quiescence(board, ply, alpha, beta, searchInfo, data, tt);
    
    // Should update seldepth to current ply
    assert(data.seldepth == ply);
    
    std::cout << "PASSED\n";
}

void testRepetitionDetection() {
    std::cout << "Testing repetition detection... ";
    
    Board board;
    board.setStartingPosition();
    SearchInfo searchInfo;
    search::SearchData data;
    TranspositionTable tt(16);
    
    // Simulate a position already in search history
    Hash currentZobrist = board.zobristKey();
    searchInfo.pushSearchPosition(currentZobrist, Move(), 0);
    searchInfo.pushSearchPosition(Hash(123456), Move(), 1);  // Different position
    searchInfo.pushSearchPosition(Hash(789012), Move(), 2);  // Different position
    searchInfo.pushSearchPosition(Hash(345678), Move(), 3);  // Different position
    searchInfo.pushSearchPosition(currentZobrist, Move(), 4); // Same position again
    
    int ply = 6;  // Current ply
    eval::Score alpha = eval::Score(-1000);
    eval::Score beta = eval::Score(1000);
    
    // Now search should detect repetition and return draw score
    eval::Score result = search::quiescence(board, ply, alpha, beta, searchInfo, data, tt);
    
    // Should return draw score (0)
    assert(result == eval::Score::zero());
    
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "\n=== Quiescence Search Unit Tests ===\n\n";
    
    testStandPatBehavior();
    testStandPatBetaCutoff();
    testMaxPlyReturnsEval();
    testSelectiveDepthTracking();
    testRepetitionDetection();
    
    std::cout << "\nAll tests PASSED!\n";
    return 0;
}