#include <iostream>
#include "../src/core/types.h"

int main() {
    std::cout << "DEBUG: Includes work\n";
    std::cout << "DEBUG: Creating square\n";
    
    using namespace seajay;
    Square sq = 27;  // D4
    std::cout << "DEBUG: Square created: " << sq << "\n";
    
    return 0;
}