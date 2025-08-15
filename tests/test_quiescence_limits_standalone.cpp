#include <iostream>
#include <cassert>
#include "../src/core/board.h"
#include "../src/search/quiescence.h"
#include "../src/search/search_info.h"
#include "../src/search/types.h"
#include "../src/core/transposition_table.h"

using namespace seajay;
using namespace seajay::search;

void testNodeLimitEnforcement() {
    std::cout << "Testing node limit enforcement..." << std::endl;
    
    Board board;
    SearchInfo searchInfo;
    SearchData searchData;
    TranspositionTable tt(16); // 16MB TT
    
    // Position with many captures available - should hit node limit in testing mode
    board.fromFEN("r1bqk2r/pp2nppp/2n1p3/3p4/1bPP4/2N1PN2/PP3PPP/R1BQKB1R w KQkq - 0 8");
    
    searchData.reset();
    
    // Run quiescence search
    eval::Score score = quiescence(board, 0, 
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, searchData, tt);
    
    std::cout << "  Nodes searched: " << searchData.qsearchNodes << std::endl;
    std::cout << "  Times limited: " << searchData.qsearchNodesLimited << std::endl;
    
#ifdef QSEARCH_TESTING
    std::cout << "  Mode: TESTING (10,000 node limit)" << std::endl;
    assert(searchData.qsearchNodes <= 10001u);
    std::cout << "  ✓ Node limit enforced correctly" << std::endl;
#elif defined(QSEARCH_TUNING)
    std::cout << "  Mode: TUNING (100,000 node limit)" << std::endl;
    assert(searchData.qsearchNodes <= 100001u);
    std::cout << "  ✓ Node limit enforced correctly" << std::endl;
#else
    std::cout << "  Mode: PRODUCTION (no artificial limit)" << std::endl;
    std::cout << "  ✓ Search completed without artificial limits" << std::endl;
#endif
    
    // Score should be reasonable
    assert(score.value() > -30000 && score.value() < 30000);
    std::cout << "  ✓ Score is reasonable: " << score.value() << std::endl;
}

void testSimplePosition() {
    std::cout << "\nTesting simple position (should not hit limits)..." << std::endl;
    
    Board board;
    SearchInfo searchInfo;
    SearchData searchData;
    TranspositionTable tt(16);
    
    // Simple position with few captures
    board.fromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    
    searchData.reset();
    
    eval::Score score = quiescence(board, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, searchData, tt);
    
    std::cout << "  Nodes searched: " << searchData.qsearchNodes << std::endl;
    std::cout << "  Times limited: " << searchData.qsearchNodesLimited << std::endl;
    
    assert(searchData.qsearchNodesLimited == 0u);
    std::cout << "  ✓ Simple position did not hit limits" << std::endl;
    
    assert(searchData.qsearchNodes < 100u);
    std::cout << "  ✓ Simple position searched few nodes" << std::endl;
}

void testProgressiveLimits() {
    std::cout << "\nTesting progressive limit system..." << std::endl;
    
#ifdef QSEARCH_TESTING
    std::cout << "  Mode: QSEARCH_TESTING" << std::endl;
    assert(NODE_LIMIT_PER_POSITION == 10000u);
    std::cout << "  ✓ Testing mode has 10,000 node limit" << std::endl;
#elif defined(QSEARCH_TUNING)
    std::cout << "  Mode: QSEARCH_TUNING" << std::endl;
    assert(NODE_LIMIT_PER_POSITION == 100000u);
    std::cout << "  ✓ Tuning mode has 100,000 node limit" << std::endl;
#else
    std::cout << "  Mode: PRODUCTION" << std::endl;
    assert(NODE_LIMIT_PER_POSITION == UINT64_MAX);
    std::cout << "  ✓ Production mode has no artificial limit" << std::endl;
#endif
}

int main() {
    std::cout << "=== Quiescence Search Limit Tests ===" << std::endl;
    std::cout << "Testing the progressive node limit system" << std::endl;
    
    try {
        testProgressiveLimits();
        testNodeLimitEnforcement();
        testSimplePosition();
        
        std::cout << "\n✅ All tests passed!" << std::endl;
        std::cout << "\nThe progressive limit system is working correctly:" << std::endl;
        std::cout << "- Compile-time mode detection works" << std::endl;
        std::cout << "- Per-position node limits are enforced" << std::endl;
        std::cout << "- Tracking of limited positions works" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}