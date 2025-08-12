/**
 * Debug test for Phase 2 initialization
 */

#include "../src/core/magic_bitboards.h"
#include <iostream>

using namespace seajay;
using namespace seajay::magic;

int main() {
    std::cout << "Starting Phase 2 debug test...\n";
    std::cout << "About to call initMagics()...\n" << std::flush;
    
    initMagics();
    
    std::cout << "initMagics() returned!\n";
    std::cout << "Magics initialized: " << (areMagicsInitialized() ? "YES" : "NO") << "\n";
    
    return 0;
}