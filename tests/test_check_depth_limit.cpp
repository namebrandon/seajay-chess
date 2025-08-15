#include <iostream>
#include <chrono>
#include "../src/core/board.h"
#include "../src/search/negamax.h"
#include "../src/search/quiescence.h"
#include "../src/core/transposition_table.h"

using namespace seajay;

// Test positions with perpetual check potential
struct TestPosition {
    const char* fen;
    const char* description;
};

TestPosition perpetualCheckPositions[] = {
    // Positions that could lead to perpetual check
    {"6k1/5p2/6p1/8/7Q/8/5PPP/6K1 w - - 0 1", "Queen can give perpetual check"},
    {"4r1k1/5ppp/8/8/8/8/5PPP/4R1K1 w - - 0 1", "Rook endgame with perpetual check"},
    {"8/8/8/4k3/8/8/4Q3/4K3 w - - 0 1", "Queen vs lone king"},
    {"r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1", "Berlin Defense position"}
};

int main() {
    std::cout << "Testing Check Depth Limit (MAX_CHECK_PLY = " 
              << search::MAX_CHECK_PLY << ")\n\n";
    
    // Initialize transposition table
    TranspositionTable tt(16);  // 16 MB
    
    for (const auto& pos : perpetualCheckPositions) {
        std::cout << "Position: " << pos.description << "\n";
        std::cout << "FEN: " << pos.fen << "\n";
        
        Board board;
        board.parseFEN(pos.fen);
        SearchInfo searchInfo;
        search::SearchData data;
        data.timeLimit = std::chrono::milliseconds(1000);  // 1 second limit
        
        // Test quiescence search with check depth tracking
        auto start = std::chrono::high_resolution_clock::now();
        
        eval::Score score = search::quiescence(
            board, 0, 
            eval::Score(-10000), eval::Score(10000),
            searchInfo, data, tt, 0
        );
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Score: " << score.value() << "\n";
        std::cout << "Nodes: " << data.qsearchNodes << "\n";
        std::cout << "Time: " << duration.count() << " ms\n";
        
        // Check that we didn't explode in node count
        if (data.qsearchNodes > 100000) {
            std::cout << "WARNING: High node count - possible check extension explosion\n";
        }
        
        std::cout << "---\n";
    }
    
    // Test with a position at different check depths
    std::cout << "\nTesting different initial check depths:\n";
    Board board;
    board.parseFEN("6k1/5p2/6p1/8/7Q/8/5PPP/6K1 w - - 0 1");
    
    for (int checkPly = 0; checkPly <= search::MAX_CHECK_PLY + 2; checkPly++) {
        SearchInfo searchInfo;
        search::SearchData data;
        data.timeLimit = std::chrono::milliseconds(1000);
        
        eval::Score score = search::quiescence(
            board, 0,
            eval::Score(-10000), eval::Score(10000),
            searchInfo, data, tt, checkPly
        );
        
        std::cout << "CheckPly=" << checkPly << ": ";
        if (checkPly > search::MAX_CHECK_PLY) {
            std::cout << "Should return static eval immediately\n";
        } else {
            std::cout << "Score=" << score.value() 
                      << ", Nodes=" << data.qsearchNodes << "\n";
        }
    }
    
    std::cout << "\nCheck depth limit test complete!\n";
    return 0;
}