#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;

struct TestPosition {
    std::string fen;
    std::string description;
    int expected_eval_range_min;
    int expected_eval_range_max;
};

int main() {
    std::cout << "Testing Various Positions for Evaluation Correctness\n";
    std::cout << "====================================================\n\n";
    
    std::vector<TestPosition> positions = {
        // Symmetric positions - should be 0 or very close
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 
         "Starting position", -10, 10},
        
        {"rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", 
         "After 1.e4 e5", -10, 10},
         
        {"rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4",
         "After 1.e4 e5 2.Nf3 Nf6", -10, 10},
         
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 6 5",
         "Four Knights position", -10, 10},
        
        // Material imbalance tests
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN1 w Qkq - 0 1",
         "White missing rook on h1", -550, -450},
         
        {"rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQq - 0 1",
         "Black missing rook on h8", 450, 550},
         
        {"rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "Black missing e-pawn", 90, 110},
         
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1",
         "White missing e-pawn", -110, -90},
         
        // Endgame positions
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
         "Rook endgame (Tarrasch)", -100, 100},
         
        {"8/8/8/3k4/8/3K4/8/8 w - - 0 1",
         "Bare kings (draw)", 0, 0},
         
        {"8/8/8/3k4/8/3K4/4P3/8 w - - 0 1",
         "K+P vs K", 150, 250},
         
        {"8/4k3/8/8/8/8/4P3/4K3 w - - 0 1",
         "K+P vs K (centered)", 150, 250}
    };
    
    std::cout << std::fixed << std::setprecision(0);
    
    bool all_passed = true;
    for (const auto& test : positions) {
        Board board;
        board.fromFEN(test.fen);
        
        // Get raw evaluation from White's perspective
        eval::Score eval = eval::evaluate(board);
        int eval_cp = eval.value();
        
        bool passed = (eval_cp >= test.expected_eval_range_min && 
                      eval_cp <= test.expected_eval_range_max);
        
        std::cout << (passed ? "✓" : "✗") << " " << test.description << "\n";
        std::cout << "  FEN: " << test.fen << "\n";
        std::cout << "  Evaluation: " << eval_cp << " cp\n";
        std::cout << "  Expected range: [" << test.expected_eval_range_min 
                  << ", " << test.expected_eval_range_max << "] cp\n";
        
        if (!passed) {
            std::cout << "  ERROR: Evaluation outside expected range!\n";
            all_passed = false;
        }
        std::cout << "\n";
    }
    
    std::cout << "========================================\n";
    if (all_passed) {
        std::cout << "✓ All tests PASSED!\n";
    } else {
        std::cout << "✗ Some tests FAILED - further investigation needed\n";
    }
    
    return all_passed ? 0 : 1;
}