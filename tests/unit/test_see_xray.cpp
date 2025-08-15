#include "../../src/core/see.h"
#include "../../src/core/board.h"
#include "../../src/core/move_generation.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace seajay;

struct XRayTestCase {
    std::string fen;
    Move move;
    SEEValue expected;
    std::string description;
};

void runXRayTest(const XRayTestCase& test) {
    Board board;
    if (!board.fromFEN(test.fen)) {
        std::cerr << "FAILED to parse FEN: " << test.fen << "\n";
        assert(false);
    }
    
    SEEValue result = see(board, test.move);
    
    if (result != test.expected) {
        std::cerr << "FAILED: " << test.description << "\n";
        std::cerr << "  FEN: " << test.fen << "\n";
        std::cerr << "  Move: " << squareToString(moveFrom(test.move)) 
                  << squareToString(moveTo(test.move)) << "\n";
        std::cerr << "  Expected: " << test.expected << ", Got: " << result << "\n";
        assert(false);
    } else {
        std::cout << "PASSED: " << test.description << " (SEE = " << result << ")\n";
    }
}

int main() {
    std::cout << "=== Day 3 X-Ray SEE Tests ===\n\n";
    
    std::vector<XRayTestCase> tests = {
        // Test 1: Simple rook x-ray
        {
            "1k2r3/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - 0 1",  // Black rook on e8
            makeMove(E1, E5),
            -400,
            "Rook takes pawn, rook x-ray recaptures"
        },
        
        // Test 2: Bishop x-ray through pawn
        {
            "4k3/8/4p3/3b4/4P3/8/4B3/4K3 w - - 0 1",
            makeMove(E2, D3),
            0,
            "Bishop move with x-ray defense"
        },
        
        // Test 3: Queen x-ray (diagonal)
        {
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            makeMove(E5, F7),
            -225,  // Knight(325) - Pawn(100) - Queen x-ray recapture
            "Knight takes pawn with queen x-ray"
        },
        
        // Test 4: Queen takes rook
        {
            "4k3/8/4r3/4P3/8/8/4Q3/4K3 w - - 0 1",
            makeMove(E2, E6),  // Queen takes rook
            500,  // Win a rook
            "Queen takes undefended rook"
        },
        
        // Test 5: Rook x-ray on file
        {
            "R6r/8/8/R1r5/8/8/8/K6k b - - 0 1",  // White rooks on a8 and a5
            makeMove(C5, A5),
            0,
            "Rook takes rook with x-ray recapture"
        },
        
        // Test 6: Bishop x-ray through piece
        {
            "4k3/8/4b3/3p4/2B5/8/8/4K3 w - - 0 1",  // Bishop takes pawn, bishop x-ray
            makeMove(C4, D5),
            -225,  // Bishop(325) - Pawn(100) - Bishop recaptures = -225
            "Bishop takes pawn with x-ray recapture"
        },
        
        // Test 7: No x-ray - piece not on ray
        {
            "4k3/8/4p3/8/2n5/4P3/8/R3K2R w - - 0 1",
            makeMove(A1, A7),
            0,
            "Rook move, knight not on ray (no x-ray)"
        },
        
        // Test 8: Knight takes defended pawn
        {
            "3r2k1/p2r1p1p/1p2p1p1/q4n2/3P4/PQ5P/1P1RNPP1/3R2K1 b - - 0 1",
            makeMove(F5, D4),
            -225,  // Knight for pawn is bad when queen recaptures
            "Knight takes pawn, queen recaptures"
        },
        
        // Test 9: Rook takes undefended rook
        {
            "r3k3/8/8/8/R7/8/8/4K3 w - - 0 1",
            makeMove(A4, A8),
            500,  // Win a rook
            "Rook takes undefended rook"
        },
        
        // Test 10: Queen takes undefended queen
        {
            "r3k2r/8/8/3q4/3Q4/8/8/R3K2R w - - 0 1",
            makeMove(D4, D5),
            975,  // Win a queen
            "Queen takes undefended queen"
        },
        
        // Test 11: Discovered attack (not x-ray)
        {
            "4k3/8/2n5/8/2P5/8/8/R3K3 w - - 0 1",
            makeMove(C4, C5),
            0,
            "Pawn advance (no x-ray effect)"
        },
        
        // Test 12: X-ray in endgame
        {
            "8/8/4k3/8/2r5/4P3/2R5/4K3 w - - 0 1",  // Rook on c2
            makeMove(C2, C4),
            0,  // Equal trade
            "Rook takes rook (equal trade)"
        },
        
        // Test 13: Bishop battery x-ray
        {
            "4k3/8/4p3/3b4/2B5/1B6/8/4K3 w - - 0 1",
            makeMove(C4, D5),
            -325 + 100,  // Bishop for pawn, bishop x-ray
            "Bishop takes bishop with x-ray"
        },
        
        // Test 14: Rook battery x-ray
        {
            "4k3/8/8/3r4/8/3R4/3R4/4K3 w - - 0 1",
            makeMove(D3, D5),
            0,
            "Rook takes rook with x-ray backup"
        },
        
        // Test 15: Complex position with multiple x-rays
        {
            "r2qk2r/pp2bppp/2n1pn2/3p4/2PP4/2N1PN2/PP2BPPP/R2QK2R w KQkq - 0 1",
            makeMove(C4, D5),
            0,
            "Central pawn exchange with pieces behind"
        },
        
        // Test 16: X-ray only counts if on same ray
        {
            "4k3/8/8/3p4/3P4/8/1B6/4K3 w - - 0 1",
            makeMove(D4, D5),
            -100,
            "Pawn takes pawn, bishop not on ray"
        },
        
        // Test 17: Queen x-rays as both bishop and rook
        {
            "r3k3/8/8/3p4/3Q4/8/8/4K2R w - - 0 1",
            makeMove(D4, D5),
            -875,  // Queen for pawn
            "Queen takes pawn (bad trade)"
        }
    };
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        try {
            runXRayTest(test);
            passed++;
        } catch (...) {
            failed++;
        }
    }
    
    std::cout << "\n=== X-Ray Test Summary ===\n";
    std::cout << "Passed: " << passed << "/" << tests.size() << "\n";
    if (failed > 0) {
        std::cout << "Failed: " << failed << "\n";
        return 1;
    }
    
    std::cout << "All x-ray tests passed!\n";
    return 0;
}