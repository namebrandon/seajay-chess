#include <iostream>
#include "../src/search/quiescence.h"

int main() {
    std::cout << "Check Depth Limit Test\n";
    std::cout << "MAX_CHECK_PLY = " << seajay::search::MAX_CHECK_PLY << "\n";
    
    // This simple test just verifies the constant is defined
    if (seajay::search::MAX_CHECK_PLY == 8) {
        std::cout << "PASS: MAX_CHECK_PLY is correctly set to 8\n";
        return 0;
    } else {
        std::cout << "FAIL: MAX_CHECK_PLY is not 8\n";
        return 1;
    }
}