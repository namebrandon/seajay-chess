#include <iostream>

int main() {
    std::cout << "DEBUG: Starting test\n";
    std::cout.flush();
    
    std::cout << "DEBUG: About to include header\n";
    std::cout.flush();
    
    // Test will continue after includes work
    std::cout << "DEBUG: Test complete\n";
    return 0;
}