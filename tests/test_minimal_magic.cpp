#include <iostream>
#include <cstdint>

using Bitboard = uint64_t;
using Square = uint8_t;

// Minimal magic test without any complex includes
int main() {
    std::cout << "Testing minimal magic functionality\n";
    
    // Test basic magic multiplication
    Bitboard occupancy = 0x1234567890ABCDEFULL;
    Bitboard magic = 0x8a80104000800020ULL;  // First rook magic
    uint8_t shift = 52;  // Typical shift value
    
    uint64_t index = (occupancy * magic) >> shift;
    
    std::cout << "Occupancy: 0x" << std::hex << occupancy << std::dec << "\n";
    std::cout << "Magic:     0x" << std::hex << magic << std::dec << "\n";
    std::cout << "Shift:     " << (int)shift << "\n";
    std::cout << "Index:     " << index << "\n";
    
    // Create a simple attack table
    const size_t tableSize = 4096;  // 2^12 for corner square
    Bitboard* attacks = new Bitboard[tableSize];
    
    // Fill with dummy values
    for (size_t i = 0; i < tableSize; ++i) {
        attacks[i] = i * 0x123;
    }
    
    // Lookup
    Bitboard result = attacks[index];
    std::cout << "Attack lookup result: 0x" << std::hex << result << std::dec << "\n";
    
    delete[] attacks;
    
    std::cout << "\nMinimal test completed successfully\n";
    return 0;
}