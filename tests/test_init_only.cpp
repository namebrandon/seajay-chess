/**
 * Test only initialization
 */

#include <iostream>

// Forward declarations
namespace seajay {
namespace magic {
    void initMagics();
    bool areMagicsInitialized();
}
}

int main() {
    std::cout << "Calling initMagics()...\n" << std::flush;
    seajay::magic::initMagics();
    std::cout << "initMagics() returned!\n";
    std::cout << "Initialized: " << seajay::magic::areMagicsInitialized() << "\n";
    return 0;
}