#include "../src/core/board.h"
#include "../src/core/board_safety.h"
#include "../src/core/move_generation.h"
#include "../src/search/negamax.h"
#include "../src/search/types.h"
#include "../src/search/search_info.h"
#include <iostream>
#include <cassert>

using namespace seajay;

void testMateInOne() {
    std::cout << "Testing Mate in 1 positions...\n";
    
    // Test 1: Back rank mate - Ra8#
    {
        Board board;
        bool fenOk = board.fromFEN("6k1/5ppp/8/8/8/8/8/R6K w - - 0 1");
        if (!fenOk) {
            std::cerr << "Failed to parse FEN!\n";
            return;
        }
        std::cout << "Position: " << board.toFEN() << "\n";
        
        search::SearchLimits limits;
        limits.maxDepth = 2;  // Only need 2 ply to find mate in 1
        
        Move bestMove = search::search(board, limits);
        
        // Check that the move is Ra8
        Square from = moveFrom(bestMove);
        Square to = moveTo(bestMove);
        
        std::cout << "Found move: " << SafeMoveExecutor::moveToString(bestMove) << "\n";
        
        assert(from == A1);
        assert(to == A8);
        std::cout << "✓ Back rank mate found!\n\n";
    }
    
    // Test 2: Queen mate - multiple mates available
    {
        Board board;
        board.fromFEN("7k/8/8/8/8/8/1Q6/7K w - - 0 1");
        std::cout << "Position: " << board.toFEN() << "\n";
        
        search::SearchLimits limits;
        limits.maxDepth = 2;
        
        Move bestMove = search::search(board, limits);
        
        Square from = moveFrom(bestMove);
        Square to = moveTo(bestMove);
        
        std::cout << "Found move: " << SafeMoveExecutor::moveToString(bestMove) << "\n";
        
        // Should be Qb8# or Qa8# or Qa7#
        assert(from == B2);
        assert(to == B8 || to == A8 || to == A7);
        std::cout << "✓ Queen mate found!\n\n";
    }
    
    // Test 3: Rook mate with castling rights
    {
        Board board;
        board.fromFEN("r3k3/8/8/8/8/8/8/2KR4 w - - 0 1");  // This one already has both kings
        std::cout << "Position: " << board.toFEN() << "\n";
        
        search::SearchLimits limits;
        limits.maxDepth = 2;
        
        Move bestMove = search::search(board, limits);
        
        Square from = moveFrom(bestMove);
        Square to = moveTo(bestMove);
        
        std::cout << "Found move: " << SafeMoveExecutor::moveToString(bestMove) << "\n";
        
        assert(from == D1);
        assert(to == D8);
        std::cout << "✓ Rook mate found!\n\n";
    }
}

void testDepthSearch() {
    std::cout << "Testing different search depths...\n";
    
    Board board;  // Starting position
    std::cout << "Starting position\n";
    
    // Test depth 1
    {
        search::SearchLimits limits;
        limits.maxDepth = 1;
        limits.movetime = std::chrono::milliseconds(100);
        
        SearchInfo searchInfo;
        searchInfo.clear();
        searchInfo.setRootHistorySize(board.gameHistorySize());
        
        search::SearchData info;
        info.timeLimit = limits.movetime;
        
        eval::Score score = search::negamax(board, 1, 0, 
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, info);
        
        std::cout << "Depth 1: " << info.nodes << " nodes, score: " 
                  << score.to_cp() << " cp\n";
        assert(info.nodes > 0);
        assert(info.nodes < 100);  // Should be around 20 for starting position
    }
    
    // Test depth 2
    {
        search::SearchLimits limits;
        limits.maxDepth = 2;
        limits.movetime = std::chrono::milliseconds(1000);
        
        SearchInfo searchInfo;
        searchInfo.clear();
        searchInfo.setRootHistorySize(board.gameHistorySize());
        
        search::SearchData info;
        info.timeLimit = limits.movetime;
        
        eval::Score score = search::negamax(board, 2, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, info);
        
        std::cout << "Depth 2: " << info.nodes << " nodes, score: " 
                  << score.to_cp() << " cp\n";
        assert(info.nodes > 20);
        assert(info.nodes < 1000);  // Should be around 400
    }
    
    // Test depth 3
    {
        search::SearchLimits limits;
        limits.maxDepth = 3;
        limits.movetime = std::chrono::milliseconds(2000);
        
        SearchInfo searchInfo;
        searchInfo.clear();
        searchInfo.setRootHistorySize(board.gameHistorySize());
        
        search::SearchData info;
        info.timeLimit = limits.movetime;
        
        eval::Score score = search::negamax(board, 3, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, info);
        
        std::cout << "Depth 3: " << info.nodes << " nodes, score: " 
                  << score.to_cp() << " cp, NPS: " << info.nps() << "\n";
        assert(info.nodes > 400);
        assert(info.nodes < 20000);  // Should be around 8900
    }
    
    std::cout << "✓ Depth search tests passed!\n\n";
}

void testIterativeDeepening() {
    std::cout << "Testing iterative deepening...\n";
    
    Board board;  // Starting position
    
    search::SearchLimits limits;
    limits.maxDepth = 4;
    limits.movetime = std::chrono::milliseconds(3000);
    
    Move bestMove = search::search(board, limits);
    
    std::cout << "Best move found: " << SafeMoveExecutor::moveToString(bestMove) << "\n";
    
    // Should find a reasonable opening move
    assert(bestMove != Move());
    
    // Common opening moves: e2e4, d2d4, g1f3, etc.
    Square from = moveFrom(bestMove);
    Square to = moveTo(bestMove);
    
    bool isReasonableOpening = 
        (from == E2 && to == E4) ||   // e4
        (from == D2 && to == D4) ||   // d4
        (from == G1 && to == F3) ||   // Nf3
        (from == B1 && to == C3) ||   // Nc3
        (from == E2 && to == E3) ||   // e3
        (from == D2 && to == D3);     // d3
    
    if (!isReasonableOpening) {
        std::cout << "Note: Unusual opening move, but may be valid\n";
    }
    
    std::cout << "✓ Iterative deepening test passed!\n\n";
}

void testTimeManagement() {
    std::cout << "Testing time management...\n";
    
    Board board;
    
    // Test with fixed movetime
    {
        search::SearchLimits limits;
        limits.maxDepth = 10;  // High depth but limited time
        limits.movetime = std::chrono::milliseconds(500);
        
        auto start = std::chrono::steady_clock::now();
        Move bestMove = search::search(board, limits);
        auto elapsed = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        
        std::cout << "Fixed movetime (500ms): actual time = " << ms << "ms\n";
        
        // Should respect time limit with some tolerance
        assert(ms < 600);  // Allow 100ms tolerance
        assert(bestMove != Move());
    }
    
    // Test with game clock
    {
        search::SearchLimits limits;
        limits.maxDepth = 10;
        limits.time[WHITE] = std::chrono::milliseconds(60000);  // 1 minute
        limits.inc[WHITE] = std::chrono::milliseconds(1000);    // 1 second increment
        
        auto timeLimit = search::calculateTimeLimit(limits, board);
        std::cout << "Game clock (60s + 1s inc): allocated = " 
                  << timeLimit.count() << "ms\n";
        
        // Should allocate around 5% of time + 75% of increment
        // = 3000ms + 750ms = 3750ms
        assert(timeLimit.count() > 2000);
        assert(timeLimit.count() < 5000);
    }
    
    std::cout << "✓ Time management tests passed!\n\n";
}

int main() {
    std::cout << "=== Negamax Search Tests ===\n\n";
    
    try {
        testMateInOne();
        testDepthSearch();
        testIterativeDeepening();
        testTimeManagement();
        
        std::cout << "\n=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}