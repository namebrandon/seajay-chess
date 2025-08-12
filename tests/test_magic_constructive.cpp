/**
 * Verify magic collisions are constructive (same attacks)
 * This is the key to understanding magic bitboards!
 */

#include <iostream>
#include <vector>
#include <cstdint>
#include <map>

using Bitboard = uint64_t;
using Square = uint8_t;

constexpr Square D4 = 27;

// Helper functions
int popCount(Bitboard bb) { return __builtin_popcountll(bb); }
Square popLsb(Bitboard& bb) {
    Square s = __builtin_ctzll(bb);
    bb &= bb - 1;
    return s;
}

// Ray-based rook attacks (simplified)
Bitboard slowRookAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    int f = sq & 7;
    int r = sq >> 3;
    
    // North
    for (int r2 = r + 1; r2 < 8; ++r2) {
        Square s = (r2 << 3) | f;
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    // South
    for (int r2 = r - 1; r2 >= 0; --r2) {
        Square s = (r2 << 3) | f;
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    // East
    for (int f2 = f + 1; f2 < 8; ++f2) {
        Square s = (r << 3) | f2;
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    // West
    for (int f2 = f - 1; f2 >= 0; --f2) {
        Square s = (r << 3) | f2;
        attacks |= (1ULL << s);
        if (occupied & (1ULL << s)) break;
    }
    
    return attacks;
}

int main() {
    Bitboard mask = 0x8080876080800ULL;  // D4 rook mask
    Bitboard magic = 0x140848010000802ULL;  // D4 rook magic
    int shift = 54;
    int maskBits = popCount(mask);
    size_t tableSize = 1ULL << (64 - shift);
    
    std::cout << "Testing magic bitboard collisions for D4...\n\n";
    
    // Map from index to attack pattern
    std::map<uint64_t, Bitboard> indexToAttacks;
    
    // Statistics
    int totalPatterns = 0;
    int constructiveCollisions = 0;
    int destructiveCollisions = 0;
    
    // Generate all possible subsets of the mask
    for (int pattern = 0; pattern < (1 << maskBits); ++pattern) {
        totalPatterns++;
        
        // Generate occupancy from pattern
        Bitboard occupancy = 0;
        Bitboard tempMask = mask;
        
        for (int bit = 0; bit < maskBits; ++bit) {
            int sq = __builtin_ctzll(tempMask);
            tempMask &= tempMask - 1;
            
            if (pattern & (1 << bit)) {
                occupancy |= (1ULL << sq);
            }
        }
        
        // Calculate attacks for this occupancy
        Bitboard attacks = slowRookAttacks(D4, occupancy);
        
        // Calculate magic index
        uint64_t index = (occupancy * magic) >> shift;
        
        // Check if this index has been used before
        auto it = indexToAttacks.find(index);
        if (it != indexToAttacks.end()) {
            // Collision detected
            if (it->second == attacks) {
                // Constructive collision - same attacks
                constructiveCollisions++;
            } else {
                // Destructive collision - different attacks!
                destructiveCollisions++;
                if (destructiveCollisions <= 5) {
                    std::cerr << "DESTRUCTIVE collision at index " << index << ":\n";
                    std::cerr << "  Occupancy 1: produces attacks 0x" << std::hex << it->second << "\n";
                    std::cerr << "  Occupancy 2: produces attacks 0x" << std::hex << attacks << "\n";
                    std::cerr << std::dec;
                }
            }
        } else {
            // New index
            indexToAttacks[index] = attacks;
        }
    }
    
    std::cout << "=== RESULTS ===\n";
    std::cout << "Total patterns: " << totalPatterns << "\n";
    std::cout << "Unique indices used: " << indexToAttacks.size() << " out of " << tableSize << "\n";
    std::cout << "Constructive collisions: " << constructiveCollisions << " (GOOD - same attacks)\n";
    std::cout << "Destructive collisions: " << destructiveCollisions << " (BAD - different attacks)\n";
    
    if (destructiveCollisions == 0) {
        std::cout << "\n✓ Magic number is PERFECT for D4!\n";
        std::cout << "All collisions are constructive (produce same attacks).\n";
        std::cout << "This is exactly how magic bitboards work!\n";
    } else {
        std::cout << "\n✗ Magic number is INVALID for D4!\n";
        std::cout << "There are destructive collisions.\n";
    }
    
    // Calculate space efficiency
    double efficiency = (double)indexToAttacks.size() / tableSize * 100.0;
    std::cout << "\nSpace efficiency: " << efficiency << "%\n";
    std::cout << "(" << indexToAttacks.size() << " unique attack patterns in " 
              << tableSize << " table entries)\n";
    
    return destructiveCollisions == 0 ? 0 : 1;
}