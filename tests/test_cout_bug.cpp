/**
 * Test for cout issue
 */

#include <iostream>

void testFunction() {
    std::cout << "Line 1\n";
    std::cout << "Line 2\n";
    std::cout << "Line 3\n";
    
    for (int i = 0; i < 3; ++i) {
        std::cout << "Loop " << i << "\n";
    }
    
    std::cout << "After loop\n";
}

int main() {
    std::cout << "Main starting\n";
    testFunction();
    std::cout << "Main ending\n";
    return 0;
}