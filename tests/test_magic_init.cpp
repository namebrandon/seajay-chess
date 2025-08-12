/**
 * Test magic bitboards initialization
 */

#include <iostream>

// Forward declare the initialization function
namespace seajay {
namespace magic {
    void initMagics();
}
}

int main() {
    std::cout << "Testing magic bitboards initialization...\n";
    
    // Call initialization
    seajay::magic::initMagics();
    
    std::cout << "Done!\n";
    return 0;
}