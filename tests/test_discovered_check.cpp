#include <iostream>
#include "../src/search/discovered_check.h"
#include "../src/core/board.h"
#include "../src/core/move_generation.h"

using namespace seajay;

// Test positions with potential discovered checks
struct TestPosition {
    const char* fen;
    const char* moveStr;  // In format like "e2e4"
    bool expectedDiscovered;
    const char* description;
};

// Helper to parse move string (simplified)
Move parseMove(const std::string& str) {
    if (str.length() < 4) return NO_MOVE;
    
    File fromFile = File(str[0] - 'a');
    Rank fromRank = Rank(str[1] - '1');
    File toFile = File(str[2] - 'a');
    Rank toRank = Rank(str[3] - '1');
    
    Square from = makeSquare(fromFile, fromRank);
    Square to = makeSquare(toFile, toRank);
    
    return makeMove(from, to);
}

int main() {
    std::cout << "Testing Discovered Check Detection\n\n";
    
    TestPosition positions[] = {
        // Position where moving a piece uncovers check
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1", 
         "f3g5", true, "Knight move uncovers bishop check"},
        
        {"8/8/8/3k4/3n4/3R4/3K4/8 w - - 0 1",
         "d3d4", false, "Rook captures knight (direct check, not discovered)"},
         
        {"8/3k4/8/3n4/8/3R4/3K4/8 w - - 0 1",
         "d3h3", true, "Rook moves horizontally (might uncover vertical check)"},
         
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "e2e4", false, "Opening move (no discovered check)"}
    };
    
    int passed = 0;
    int total = 0;
    
    for (const auto& test : positions) {
        Board board;
        auto result = board.parseFEN(test.fen);
        if (!result.hasValue()) {
            std::cout << "Failed to parse FEN: " << test.fen << "\n";
            continue;
        }
        
        Move move = parseMove(test.moveStr);
        if (move == NO_MOVE) {
            std::cout << "Failed to parse move: " << test.moveStr << "\n";
            continue;
        }
        
        bool isDiscovered = search::isDiscoveredCheck(board, move);
        
        std::cout << "Position: " << test.description << "\n";
        std::cout << "Move: " << test.moveStr << "\n";
        std::cout << "Expected discovered check: " << (test.expectedDiscovered ? "YES" : "NO") << "\n";
        std::cout << "Detected discovered check: " << (isDiscovered ? "YES" : "NO") << "\n";
        
        total++;
        if (isDiscovered == test.expectedDiscovered) {
            std::cout << "PASS\n";
            passed++;
        } else {
            std::cout << "FAIL\n";
        }
        std::cout << "---\n";
    }
    
    std::cout << "\nResults: " << passed << "/" << total << " tests passed\n";
    
    if (passed == total) {
        std::cout << "All tests passed! Discovered check detection working.\n";
        return 0;
    } else {
        std::cout << "Some tests failed. Detection needs refinement.\n";
        return 1;
    }
}