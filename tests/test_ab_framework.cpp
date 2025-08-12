/**
 * A/B Testing Framework Test
 * 
 * This test verifies that we can switch between ray-based and magic bitboard
 * implementations using compile flags.
 */

#include "core/attack_wrapper.h"
#include "core/board.h"
#include <iostream>
#include <chrono>

using namespace seajay;

int main() {
    std::cout << "A/B Testing Framework Verification" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Check which implementation is being used
    #ifdef USE_MAGIC_BITBOARDS
        std::cout << "Configuration: MAGIC BITBOARDS" << std::endl;
    #else
        std::cout << "Configuration: RAY-BASED (default)" << std::endl;
    #endif
    
    #ifdef DEBUG_MAGIC
        std::cout << "Debug Mode: ENABLED (validation on every call)" << std::endl;
    #else
        std::cout << "Debug Mode: DISABLED" << std::endl;
    #endif
    
    std::cout << std::endl;
    
    // Test that the wrapper functions work
    Square testSquare = D4;
    Bitboard testOccupied = 0x0000001818000000ULL;
    
    std::cout << "Testing wrapper functions..." << std::endl;
    
    // Test rook attacks
    Bitboard rookResult = getRookAttacks(testSquare, testOccupied);
    std::cout << "Rook attacks from D4: 0x" << std::hex << rookResult << std::dec << std::endl;
    
    // Test bishop attacks
    Bitboard bishopResult = getBishopAttacks(testSquare, testOccupied);
    std::cout << "Bishop attacks from D4: 0x" << std::hex << bishopResult << std::dec << std::endl;
    
    // Test queen attacks
    Bitboard queenResult = getQueenAttacks(testSquare, testOccupied);
    std::cout << "Queen attacks from D4: 0x" << std::hex << queenResult << std::dec << std::endl;
    
    // Verify consistency
    std::cout << "\nVerifying consistency..." << std::endl;
    
    bool consistent = true;
    
    // Queen should be rook + bishop
    if (queenResult != (rookResult | bishopResult)) {
        std::cerr << "ERROR: Queen attacks != Rook | Bishop!" << std::endl;
        consistent = false;
    }
    
    // Test empty board
    Bitboard emptyRook = getRookAttacks(D4, 0);
    Bitboard emptyBishop = getBishopAttacks(D4, 0);
    
    // On empty board, rook from D4 should attack 14 squares (full rank + file minus D4)
    int rookCount = popCount(emptyRook);
    if (rookCount != 14) {
        std::cerr << "ERROR: Rook on empty board from D4 should attack 14 squares, got " 
                  << rookCount << std::endl;
        consistent = false;
    }
    
    // On empty board, bishop from D4 should attack 13 squares
    int bishopCount = popCount(emptyBishop);
    if (bishopCount != 13) {
        std::cerr << "ERROR: Bishop on empty board from D4 should attack 13 squares, got " 
                  << bishopCount << std::endl;
        consistent = false;
    }
    
    if (consistent) {
        std::cout << "✓ All consistency checks PASSED!" << std::endl;
    } else {
        std::cerr << "✗ Some consistency checks FAILED!" << std::endl;
        return 1;
    }
    
    // Performance benchmarking removed for production build
    
    // Test switching capability (informational)
    std::cout << "\n=== Configuration Test ===" << std::endl;
    std::cout << "To test magic bitboards, recompile with:" << std::endl;
    std::cout << "  cmake -DUSE_MAGIC_BITBOARDS=ON .." << std::endl;
    std::cout << "To enable debug validation, add:" << std::endl;
    std::cout << "  cmake -DUSE_MAGIC_BITBOARDS=ON -DDEBUG_MAGIC=ON .." << std::endl;
    
    std::cout << "\n✓ A/B Testing Framework is working correctly!" << std::endl;
    
    return 0;
}