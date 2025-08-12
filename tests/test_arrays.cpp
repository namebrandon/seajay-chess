#include <iostream>
#include "../src/core/magic_constants.h"

using namespace seajay;

int main() {
    std::cout << "Testing access to magic constants...\n";
    
    // Test accessing ROOK_SHIFTS
    for (int i = 0; i < 64; ++i) {
        std::cout << "ROOK_SHIFTS[" << i << "] = " << (int)magic::ROOK_SHIFTS[i] << "\n";
    }
    
    std::cout << "\nFirst ROOK_MAGIC = 0x" << std::hex << magic::ROOK_MAGICS[0] << std::dec << "\n";
    
    return 0;
}