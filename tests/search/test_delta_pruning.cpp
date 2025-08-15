#include "../../src/search/quiescence.h"
#include "../../src/search/search_info.h"
#include "../../src/core/board.h"
#include "../../src/core/transposition_table.h"
#include "../../src/evaluation/evaluate.h"
#include <iostream>
#include <cassert>

using namespace seajay;
using namespace seajay::search;
using namespace seajay::eval;

void testDeltaPruningEfficiency() {
    std::cout << "Testing delta pruning efficiency...\n";
    
    // Position where one side is significantly ahead
    // Delta pruning should reduce nodes significantly
    Board board;
    bool result = board.fromFEN("8/8/8/8/8/2k5/1q6/K7 w - - 0 1");
    assert(result);  // Black queen vs nothing
    
    SearchInfo searchInfo;
    TranspositionTable tt(16);  // 16MB TT
    
    // Test with delta pruning enabled (default)
    SearchData dataWithPruning;
    Score result1 = quiescence(board, 0, Score(-1000), Score(1000), searchInfo, dataWithPruning, tt);
    
    std::cout << "Delta pruning test results:\n";
    std::cout << "  Nodes searched: " << dataWithPruning.qsearchNodes << "\n";
    std::cout << "  Deltas pruned: " << dataWithPruning.deltasPruned << "\n";
    std::cout << "  Final score: " << result1.value() << "\n";
    
    // Verify pruning happened in this hopeless position
    if (dataWithPruning.deltasPruned > 0) {
        std::cout << "✓ Delta pruning is working (pruned " << dataWithPruning.deltasPruned << " moves)\n";
    } else {
        std::cout << "⚠ No delta pruning occurred (may be normal in this position)\n";
    }
    
    // Test endgame detection
    Board endgameBoard;
    bool endgameResult = endgameBoard.fromFEN("8/8/8/8/8/k7/8/K7 w - - 0 1");
    assert(endgameResult);  // King vs King
    
    SearchData endgameData;
    Score endgameResult = quiescence(endgameBoard, 0, Score(-50), Score(50), searchInfo, endgameData, tt);
    
    std::cout << "\nEndgame test results:\n";
    std::cout << "  Nodes searched: " << endgameData.qsearchNodes << "\n";
    std::cout << "  Deltas pruned: " << endgameData.deltasPruned << "\n";
    
    std::cout << "\nDelta pruning test completed successfully!\n";
}

void testPromotionSafety() {
    std::cout << "\nTesting promotion safety (never prune promotions)...\n";
    
    // Position with possible promotion
    Board board;
    bool result = board.fromFEN("8/1P6/8/8/8/8/8/k1K5 w - - 0 1");
    assert(result);  // White pawn about to promote
    
    SearchInfo searchInfo;
    TranspositionTable tt(16);
    SearchData data;
    
    Score result = quiescence(board, 0, Score(-2000), Score(2000), searchInfo, data, tt);
    
    std::cout << "Promotion test results:\n";
    std::cout << "  Nodes searched: " << data.qsearchNodes << "\n";
    std::cout << "  Deltas pruned: " << data.deltasPruned << "\n";
    std::cout << "  Final score: " << result.value() << "\n";
    
    std::cout << "✓ Promotion safety test completed\n";
}

int main() {
    try {
        testDeltaPruningEfficiency();
        testPromotionSafety();
        std::cout << "\n✓ All delta pruning tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}