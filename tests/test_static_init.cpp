/**
 * Test static initialization issues
 */

#include "../src/core/types.h"
#include "../src/core/bitboard.h"
#include <iostream>

using namespace seajay;

// Global variable that calls bitboard functions during static init
struct StaticTest {
    Bitboard mask;
    
    StaticTest() {
        std::cout << "StaticTest constructor\n";
        Square sq = D4;
        int f = static_cast<int>(fileOf(sq));
        int r = static_cast<int>(rankOf(sq));
        std::cout << "File: " << f << ", Rank: " << r << "\n";
        
        mask = squareBB(sq);
        std::cout << "Mask set\n";
    }
};

// This will be initialized before main()
StaticTest globalTest;

int main() {
    std::cout << "Main starting\n";
    std::cout << "Global mask: " << std::hex << globalTest.mask << std::dec << "\n";
    return 0;
}