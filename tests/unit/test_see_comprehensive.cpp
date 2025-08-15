#include "../../src/core/see.h"
#include "../../src/core/board.h"
#include "../../src/core/move_generation.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

using namespace seajay;

struct SEETestCase {
    std::string fen;
    std::string moveStr;
    SEEValue expected;
    std::string description;
};

// Parse a move string like "e2e4" 
Move parseMove(const std::string& moveStr) {
    if (moveStr.length() < 4) return 0;
    
    Square from = static_cast<Square>((moveStr[1] - '1') * 8 + (moveStr[0] - 'a'));
    Square to = static_cast<Square>((moveStr[3] - '1') * 8 + (moveStr[2] - 'a'));
    
    // Handle promotion
    if (moveStr.length() == 5) {
        uint8_t flags = PROMOTION;
        switch (moveStr[4]) {
            case 'q': flags |= (QUEEN << 2); break;
            case 'r': flags |= (ROOK << 2); break;
            case 'b': flags |= (BISHOP << 2); break;
            case 'n': flags |= (KNIGHT << 2); break;
        }
        return makeMove(from, to, flags);
    }
    
    return makeMove(from, to);
}

bool runTest(const SEETestCase& test) {
    Board board;
    if (!board.fromFEN(test.fen)) {
        std::cerr << "Failed to parse FEN: " << test.fen << "\n";
        return false;
    }
    
    Move move = parseMove(test.moveStr);
    if (move == 0) {
        std::cerr << "Failed to parse move: " << test.moveStr << "\n";
        return false;
    }
    
    SEEValue result = see(board, move);
    
    if (result != test.expected) {
        std::cerr << "FAILED: " << test.description << "\n";
        std::cerr << "  FEN: " << test.fen << "\n";
        std::cerr << "  Move: " << test.moveStr << "\n";
        std::cerr << "  Expected: " << test.expected << ", Got: " << result << "\n";
        return false;
    }
    
    std::cout << "PASSED: " << test.description << " (SEE = " << result << ")\n";
    return true;
}

std::vector<SEETestCase> loadTestsFromEPD(const std::string& filename) {
    std::vector<SEETestCase> tests;
    std::ifstream file(filename);
    std::string line;
    
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        // Parse EPD line: FEN move expected "description"
        std::istringstream iss(line);
        std::string fen_part1, fen_part2, fen_part3, fen_part4, fen_part5, fen_part6;
        std::string moveStr;
        int expected;
        
        // Read FEN components
        if (!(iss >> fen_part1 >> fen_part2 >> fen_part3 >> fen_part4 >> fen_part5 >> fen_part6)) {
            continue;
        }
        
        // Reconstruct FEN
        std::string fen = fen_part1 + " " + fen_part2 + " " + fen_part3 + " " + 
                          fen_part4 + " " + fen_part5 + " " + fen_part6;
        
        // Read move and expected value
        if (!(iss >> moveStr >> expected)) {
            continue;
        }
        
        // Read description (rest of line)
        std::string description;
        std::getline(iss, description);
        
        // Remove quotes from description
        size_t firstQuote = description.find('"');
        size_t lastQuote = description.rfind('"');
        if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
            description = description.substr(firstQuote + 1, lastQuote - firstQuote - 1);
        }
        
        tests.push_back({fen, moveStr, static_cast<SEEValue>(expected), description});
    }
    
    return tests;
}

int main() {
    std::cout << "=== SeaJay SEE Comprehensive Test Suite ===\n\n";
    
    // Load tests from EPD file
    auto tests = loadTestsFromEPD("/workspace/tests/positions/see_stockfish.epd");
    
    if (tests.empty()) {
        std::cerr << "No tests loaded from EPD file\n";
        return 1;
    }
    
    std::cout << "Loaded " << tests.size() << " test positions\n\n";
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        if (runTest(test)) {
            passed++;
        } else {
            failed++;
        }
    }
    
    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Total: " << tests.size() << "\n";
    std::cout << "Passed: " << passed << "\n";
    std::cout << "Failed: " << failed << "\n";
    
    if (failed > 0) {
        std::cout << "\nSome tests failed. Please review the output above.\n";
        return 1;
    }
    
    std::cout << "\n✓ All SEE tests passed!\n";
    return 0;
}