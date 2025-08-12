/**
 * Verify magic number for D4 works correctly
 */

#include <iostream>
#include <vector>
#include <cstdint>

using Bitboard = uint64_t;

int main() {
    // From ROOK_SHIFTS in magic_constants.h
    // D4 is square 27 (3 + 3*8)
    // Looking at the pattern, D4 should have shift 54
    
    Bitboard mask = 0x8080876080800ULL;  // D4 rook mask
    Bitboard magic = 0x140848010000802ULL;  // D4 rook magic
    int shift = 54;
    
    int maskBits = __builtin_popcountll(mask);
    std::cout << "D4 mask has " << maskBits << " bits\n";
    std::cout << "Shift = " << shift << " (64 - " << (64 - shift) << ")\n";
    
    // The table size should be 2^(64-shift) = 2^10 = 1024
    size_t tableSize = 1ULL << (64 - shift);
    std::cout << "Table size = " << tableSize << "\n";
    
    // Test if this magic produces unique indices
    std::vector<bool> used(tableSize, false);
    std::vector<Bitboard> occupancies(tableSize, 0);
    
    // Generate all possible subsets of the mask
    for (int pattern = 0; pattern < (1 << maskBits); ++pattern) {
        Bitboard occupancy = 0;
        Bitboard tempMask = mask;
        
        for (int bit = 0; bit < maskBits; ++bit) {
            int sq = __builtin_ctzll(tempMask);
            tempMask &= tempMask - 1;
            
            if (pattern & (1 << bit)) {
                occupancy |= (1ULL << sq);
            }
        }
        
        // Calculate magic index
        uint64_t index = (occupancy * magic) >> shift;
        
        if (index >= tableSize) {
            std::cerr << "ERROR: Index " << index << " out of bounds!\n";
            return 1;
        }
        
        if (used[index] && occupancies[index] != occupancy) {
            std::cerr << "COLLISION at index " << index << "\n";
            std::cerr << "  Pattern 1: 0x" << std::hex << occupancies[index] << "\n";
            std::cerr << "  Pattern 2: 0x" << std::hex << occupancy << "\n";
            std::cerr << std::dec;
            // Don't return, let's see all collisions
        } else {
            used[index] = true;
            occupancies[index] = occupancy;
        }
    }
    
    // Count unique indices
    int uniqueIndices = 0;
    for (bool u : used) {
        if (u) uniqueIndices++;
    }
    
    std::cout << "\nUnique indices used: " << uniqueIndices << " out of " << tableSize << "\n";
    std::cout << "Total patterns: " << (1 << maskBits) << "\n";
    
    if (uniqueIndices == (1 << maskBits)) {
        std::cout << "✓ Magic number produces unique indices for all patterns!\n";
    } else {
        std::cout << "✗ Magic number has collisions!\n";
    }
    
    return 0;
}