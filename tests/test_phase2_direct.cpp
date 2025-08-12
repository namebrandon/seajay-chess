/**
 * Direct test without linking
 */

#include <iostream>

// Include the implementation file directly
#include "../src/core/magic_bitboards.cpp"

int main() {
    std::cout << "Direct test starting...\n" << std::flush;
    
    seajay::magic::initMagics();
    
    std::cout << "Direct test complete!\n";
    
    return 0;
}