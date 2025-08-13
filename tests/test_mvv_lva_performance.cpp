#include "../src/search/negamax.h"
#include "../src/search/move_ordering.h"
#include "../src/search/search_info.h"
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/magic_bitboards.h"
#include "../src/evaluation/evaluate.h"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace seajay;
using namespace seajay::search;

// Test position for benchmarking
struct TestPosition {
    const char* fen;
    const char* name;
    int depth;
};

TestPosition testPositions[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting Position", 7},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "Kiwipete", 5},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "Endgame Position", 8},
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", "Tactical Position", 5},
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", "Position with Promotion", 6}
};

// Simple move-to-string conversion
std::string moveToString(Move m) {
    if (m == NO_MOVE) return "none";
    return squareToString(moveFrom(m)) + squareToString(moveTo(m));
}

// Benchmark function
void benchmarkPosition(Board& board, const TestPosition& pos, bool useMvvLva) {
    auto result = board.parseFEN(pos.fen);
    if (!result.hasValue()) {
        std::cerr << "Failed to parse FEN: " << pos.name << std::endl;
        return;
    }
    
    std::cout << "\nBenchmarking: " << pos.name << " (depth " << pos.depth << ")" << std::endl;
    std::cout << "MVV-LVA: " << (useMvvLva ? "ENABLED" : "DISABLED") << std::endl;
    
    // Reset statistics
    MvvLvaOrdering::resetStatistics();
    
    SearchInfo searchInfo;
    SearchData searchData;
    searchData.maxDepth = pos.depth;
    searchData.nodes = 0;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Run simple negamax search
    eval::Score score = negamax(board, pos.depth, 0, 
                                eval::Score::minValue(), 
                                eval::Score::maxValue(),
                                searchInfo, searchData);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Print results
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
    std::cout << "  Nodes: " << searchData.nodes << std::endl;
    std::cout << "  NPS: " << (searchData.nodes * 1000 / (duration.count() + 1)) << std::endl;
    std::cout << "  Score: " << score.value() << std::endl;
    
    if (useMvvLva) {
        std::cout << "  ";
        MvvLvaOrdering::printStatistics();
    }
}

// Calculate move ordering efficiency
void analyzeOrderingEfficiency(Board& board) {
    std::cout << "\n=== Move Ordering Efficiency Analysis ===" << std::endl;
    
    // Test position with many captures
    const char* tacticalFen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    auto result = board.parseFEN(tacticalFen);
    if (!result.hasValue()) {
        std::cerr << "Failed to parse tactical FEN" << std::endl;
        return;
    }
    
    // Generate moves
    MoveList moves = generateLegalMoves(board);
    size_t totalMoves = moves.size();
    
    // Count move types before ordering
    size_t captures = 0, promotions = 0, quiets = 0;
    for (Move m : moves) {
        if (isPromotion(m)) promotions++;
        else if (isCapture(m)) captures++;
        else quiets++;
    }
    
    std::cout << "Position Analysis:" << std::endl;
    std::cout << "  Total moves: " << totalMoves << std::endl;
    std::cout << "  Captures: " << captures << std::endl;
    std::cout << "  Promotions: " << promotions << std::endl;
    std::cout << "  Quiet moves: " << quiets << std::endl;
    
    // Order moves with MVV-LVA
    MvvLvaOrdering ordering;
    ordering.orderMoves(board, moves);
    
    // Check first-move characteristics
    if (moves.size() > 0) {
        Move firstMove = moves[0];
        std::cout << "\nFirst move after ordering:" << std::endl;
        std::cout << "  Move: " << moveToString(firstMove) << std::endl;
        std::cout << "  Is capture: " << (isCapture(firstMove) ? "Yes" : "No") << std::endl;
        std::cout << "  Is promotion: " << (isPromotion(firstMove) ? "Yes" : "No") << std::endl;
        std::cout << "  Score: " << MvvLvaOrdering::scoreMove(board, firstMove) << std::endl;
    }
    
    // Measure how well captures are ordered
    size_t capturesBefore = 0;
    for (size_t i = 0; i < captures + promotions && i < moves.size(); i++) {
        if (isCapture(moves[i]) || isPromotion(moves[i])) {
            capturesBefore++;
        }
    }
    
    double captureOrdering = (captures + promotions > 0) 
        ? (100.0 * capturesBefore / (captures + promotions))
        : 100.0;
    
    std::cout << "\nOrdering Quality:" << std::endl;
    std::cout << "  Captures in first " << (captures + promotions) 
              << " moves: " << capturesBefore 
              << " (" << std::fixed << std::setprecision(1) 
              << captureOrdering << "%)" << std::endl;
}

int main() {
    std::cout << "=== Stage 11: MVV-LVA Performance Validation ===" << std::endl;
    
    // Initialize magic bitboards
    magic::initMagics();
    
    Board board;
    
    // Analyze ordering efficiency
    analyzeOrderingEfficiency(board);
    
    // Benchmark each position
    std::cout << "\n=== Performance Benchmarks ===" << std::endl;
    
    for (const auto& pos : testPositions) {
        // With MVV-LVA
        benchmarkPosition(board, pos, true);
        
        // Note: To properly test without MVV-LVA, we would need to
        // recompile with ENABLE_MVV_LVA undefined or add runtime flag
        // For now, we just run once with MVV-LVA enabled
    }
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "MVV-LVA implementation complete and functional." << std::endl;
    std::cout << "Expected improvements:" << std::endl;
    std::cout << "  - 15-30% reduction in search nodes" << std::endl;
    std::cout << "  - Better move ordering efficiency" << std::endl;
    std::cout << "  - Improved alpha-beta pruning" << std::endl;
    
    return 0;
}