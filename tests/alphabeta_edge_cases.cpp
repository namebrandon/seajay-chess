// Additional edge case tests for alpha-beta
#include "../src/core/board.h"
#include "../src/search/negamax.h"
#include "../src/search/types.h"
#include <iostream>
#include <cassert>

using namespace seajay;
using namespace seajay::search;

void testZugzwang() {
    // Position where any move worsens the position
    std::string fen = "8/8/p1p5/1p5p/1P5p/8/PPP2K1p/4R1rk w - - 0 1";
    Board board;
    board.fromFEN(fen);
    
    SearchInfo info;
    info.timeLimit = std::chrono::milliseconds::max();
    
    eval::Score score = negamax(board, 6, 0,
                                eval::Score::minus_infinity(),
                                eval::Score::infinity(),
                                info);
    
    std::cout << "Zugzwang test: score=" << score.to_cp() 
              << " nodes=" << info.nodes << std::endl;
}

void testRepetition() {
    // Position where repetition draw is best
    Board board;
    board.fromFEN("4k3/8/8/8/8/8/4P3/4K2R w K - 0 1");
    
    // Make some moves to set up potential repetition
    // This would need repetition detection to work properly
    SearchInfo info;
    info.timeLimit = std::chrono::milliseconds::max();
    
    eval::Score score = negamax(board, 8, 0,
                                eval::Score::minus_infinity(),
                                eval::Score::infinity(),
                                info);
    
    std::cout << "Repetition scenario: score=" << score.to_cp()
              << " nodes=" << info.nodes << std::endl;
}

void testFortress() {
    // Fortress position - material imbalance but drawn
    std::string fen = "8/8/3k4/8/2BK4/8/8/8 w - - 0 1";
    Board board;
    board.fromFEN(fen);
    
    SearchInfo info;
    info.timeLimit = std::chrono::milliseconds::max();
    
    eval::Score score = negamax(board, 10, 0,
                                eval::Score::minus_infinity(),
                                eval::Score::infinity(),
                                info);
    
    std::cout << "Fortress test: score=" << score.to_cp()
              << " nodes=" << info.nodes << std::endl;
}

void testPromotionRace() {
    // Both sides racing to promote
    std::string fen = "8/2P5/8/8/8/8/2p5/8 w - - 0 1";
    Board board;
    board.fromFEN(fen);
    
    SearchInfo info;
    info.timeLimit = std::chrono::milliseconds::max();
    
    eval::Score score = negamax(board, 8, 0,
                                eval::Score::minus_infinity(),
                                eval::Score::infinity(),
                                info);
    
    std::cout << "Promotion race: score=" << score.to_cp()
              << " nodes=" << info.nodes 
              << " (white promotes first, should be winning)" << std::endl;
    assert(score.to_cp() > 500); // Should be clearly winning for white
}

void testQuietPosition() {
    // Very quiet position with few captures
    std::string fen = "r1bqk2r/pp2bppp/2n1pn2/3p4/2PP4/2N1PN2/PP2BPPP/R1BQK2R w KQkq - 0 8";
    Board board;
    board.fromFEN(fen);
    
    SearchInfo info;
    info.timeLimit = std::chrono::milliseconds::max();
    
    eval::Score score = negamax(board, 5, 0,
                                eval::Score::minus_infinity(),
                                eval::Score::infinity(),
                                info);
    
    std::cout << "Quiet position: score=" << score.to_cp()
              << " nodes=" << info.nodes
              << " efficiency=" << info.moveOrderingEfficiency() << "%"
              << std::endl;
    
    // In quiet positions, move ordering efficiency might be lower
    // since there are fewer captures to order first
}

int main() {
    std::cout << "=== Alpha-Beta Edge Case Tests ===" << std::endl << std::endl;
    
    testPromotionRace();
    testZugzwang();
    testFortress();
    testQuietPosition();
    testRepetition();
    
    std::cout << "\nAll edge case tests completed!" << std::endl;
    return 0;
}