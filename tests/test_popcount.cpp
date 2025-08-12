#include <iostream>
#include <cstdint>

int main() {
    uint64_t test = 0x1234567890ABCDEFULL;
    
    std::cout << "Testing popcount...\n";
    
    // Test builtin
    int count = __builtin_popcountll(test);
    
    std::cout << "Popcount of 0x" << std::hex << test << " = " << std::dec << count << "\n";
    
    return 0;
}