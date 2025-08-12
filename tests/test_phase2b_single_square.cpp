/**
 * Phase 2B: Single Square Table Generation
 * Generate and validate attack table for ONE rook square (D4)
 * 
 * METHODICAL VALIDATION: Test every single pattern!
 */

#include <iostream>
#include <memory>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <vector>

using Bitboard = uint64_t;
using Square = uint8_t;
using File = uint8_t;
using Rank = uint8_t;

// Square constants
constexpr Square D4 = 27;  // d4 = 3 + 3*8
constexpr Square D3 = 19;  // d3 = 3 + 2*8
constexpr Square D5 = 35;  // d5 = 3 + 4*8
constexpr Square C4 = 26;  // c4 = 2 + 3*8
constexpr Square E4 = 28;  // e4 = 4 + 3*8

// Helper functions
constexpr File fileOf(Square s) { return s & 7; }
constexpr Rank rankOf(Square s) { return s >> 3; }
constexpr Square makeSquare(File f, Rank r) { return (r << 3) | f; }
constexpr Bitboard squareBB(Square s) { return 1ULL << s; }

// Bit manipulation
int popCount(Bitboard bb) {
    return __builtin_popcountll(bb);
}

Square popLsb(Bitboard& bb) {
    Square s = __builtin_ctzll(bb);
    bb &= bb - 1;
    return s;
}

// Compute blocker mask for rook (excludes edges)
Bitboard computeRookMask(Square sq) {
    Bitboard mask = 0;
    int f = fileOf(sq);
    int r = rankOf(sq);
    
    // North ray (exclude rank 8)
    for (int r2 = r + 1; r2 < 7; ++r2) {
        mask |= squareBB(makeSquare(f, r2));
    }
    
    // South ray (exclude rank 1)
    for (int r2 = r - 1; r2 > 0; --r2) {
        mask |= squareBB(makeSquare(f, r2));
    }
    
    // East ray (exclude file H)
    for (int f2 = f + 1; f2 < 7; ++f2) {
        mask |= squareBB(makeSquare(f2, r));
    }
    
    // West ray (exclude file A)
    for (int f2 = f - 1; f2 > 0; --f2) {
        mask |= squareBB(makeSquare(f2, r));
    }
    
    return mask;
}

// Convert index to occupancy pattern
Bitboard indexToOccupancy(int index, Bitboard mask) {
    Bitboard occupancy = 0;
    Bitboard maskCopy = mask;
    int bitCount = popCount(mask);
    
    for (int i = 0; i < bitCount; ++i) {
        Square sq = popLsb(maskCopy);
        if (index & (1 << i)) {
            occupancy |= squareBB(sq);
        }
    }
    
    return occupancy;
}

// Ray-based rook attacks (reference implementation)
Bitboard slowRookAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    int f = fileOf(sq);
    int r = rankOf(sq);
    
    // North ray
    for (int r2 = r + 1; r2 < 8; ++r2) {
        Square s = makeSquare(f, r2);
        attacks |= squareBB(s);
        if (occupied & squareBB(s)) break;
    }
    
    // South ray
    for (int r2 = r - 1; r2 >= 0; --r2) {
        Square s = makeSquare(f, r2);
        attacks |= squareBB(s);
        if (occupied & squareBB(s)) break;
    }
    
    // East ray
    for (int f2 = f + 1; f2 < 8; ++f2) {
        Square s = makeSquare(f2, r);
        attacks |= squareBB(s);
        if (occupied & squareBB(s)) break;
    }
    
    // West ray
    for (int f2 = f - 1; f2 >= 0; --f2) {
        Square s = makeSquare(f2, r);
        attacks |= squareBB(s);
        if (occupied & squareBB(s)) break;
    }
    
    return attacks;
}

// Print bitboard for debugging
void printBitboard(Bitboard bb) {
    for (int r = 7; r >= 0; --r) {
        std::cout << (r + 1) << " ";
        for (int f = 0; f < 8; ++f) {
            Square s = makeSquare(f, r);
            std::cout << ((bb & squareBB(s)) ? "X " : ". ");
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
}

// Magic number for D4 rook (from Stockfish)
constexpr Bitboard ROOK_MAGIC_D4 = 0x140848010000802ULL;
constexpr uint8_t ROOK_SHIFT_D4 = 54;  // From ROOK_SHIFTS array for D4

bool testSingleSquareD4() {
    std::cout << "\n=== Phase 2B: Single Square Table Generation (D4) ===\n";
    
    // Step 1: Compute mask for D4
    Bitboard mask = computeRookMask(D4);
    int maskBits = popCount(mask);
    
    std::cout << "Square D4 (index " << (int)D4 << ")\n";
    std::cout << "Mask has " << maskBits << " bits (expected: 10)\n";
    std::cout << "Mask visualization:\n";
    printBitboard(mask);
    
    if (maskBits != 10) {
        std::cerr << "ERROR: D4 mask should have 10 bits!\n";
        return false;
    }
    
    // Step 2: Calculate table size for D4
    size_t tableSize = 1ULL << maskBits;  // 2^10 = 1024 entries
    std::cout << "\nTable size for D4: " << tableSize << " entries\n";
    
    // Step 3: Allocate attack table for D4
    std::unique_ptr<Bitboard[]> attackTable = std::make_unique<Bitboard[]>(tableSize);
    std::memset(attackTable.get(), 0, tableSize * sizeof(Bitboard));
    
    std::cout << "Allocated " << (tableSize * sizeof(Bitboard)) << " bytes for D4 table\n";
    
    // Step 4: Generate all occupancy patterns and compute attacks
    std::cout << "\nGenerating " << tableSize << " attack patterns for D4...\n";
    
    for (size_t i = 0; i < tableSize; ++i) {
        // Generate occupancy pattern from index
        // This gives us the blockers ONLY within the mask
        Bitboard occupancy = indexToOccupancy(i, mask);
        
        // Debug first few patterns
        if (i < 5) {
            std::cout << "Pattern " << i << ": occupancy=0x" << std::hex << occupancy 
                      << ", mask=0x" << mask << std::dec << "\n";
        }
        
        // Compute attacks using ray-based method
        // Note: slowRookAttacks needs the full occupancy
        Bitboard attacks = slowRookAttacks(D4, occupancy);
        
        // Calculate magic index
        // occupancy already contains only relevant blockers
        uint64_t magicIndex = (occupancy * ROOK_MAGIC_D4) >> ROOK_SHIFT_D4;
        
        // Store in table
        if (magicIndex >= tableSize) {
            std::cerr << "ERROR: Magic index " << magicIndex << " out of bounds!\n";
            std::cerr << "Table size: " << tableSize << ", Index: " << magicIndex << "\n";
            return false;
        }
        
        // Check for destructive collisions (same index, different attacks)
        if (attackTable[magicIndex] != 0 && attackTable[magicIndex] != attacks) {
            // This is a real collision - the magic number doesn't work!
            std::cerr << "ERROR: Destructive collision detected at index " << magicIndex << "!\n";
            std::cerr << "Pattern " << i << " maps to same index as previous pattern\n";
            std::cerr << "Occupancy: 0x" << std::hex << occupancy << std::dec << "\n";
            std::cerr << "Magic index: " << magicIndex << "\n";
            std::cerr << "Current attacks: 0x" << std::hex << attacks << std::dec << "\n";
            std::cerr << "Previous attacks: 0x" << std::hex << attackTable[magicIndex] << std::dec << "\n";
            return false;
        }
        
        // Store attacks (will overwrite with same value if constructive collision)
        attackTable[magicIndex] = attacks;
        
        // Show progress every 256 patterns
        if (i % 256 == 0) {
            std::cout << "  Generated " << i << "/" << tableSize << " patterns...\n";
        }
    }
    
    std::cout << "✓ All " << tableSize << " patterns generated successfully!\n";
    
    // Step 5: Validate every pattern matches ray-based
    std::cout << "\nValidating all patterns against ray-based implementation...\n";
    
    int mismatches = 0;
    for (size_t i = 0; i < tableSize; ++i) {
        Bitboard occupancy = indexToOccupancy(i, mask);
        Bitboard expectedAttacks = slowRookAttacks(D4, occupancy);
        
        // Look up using magic
        uint64_t magicIndex = (occupancy * ROOK_MAGIC_D4) >> ROOK_SHIFT_D4;
        Bitboard magicAttacks = attackTable[magicIndex];
        
        if (magicAttacks != expectedAttacks) {
            mismatches++;
            if (mismatches <= 5) {  // Show first 5 mismatches
                std::cerr << "MISMATCH at pattern " << i << ":\n";
                std::cerr << "Occupancy:\n";
                printBitboard(occupancy);
                std::cerr << "Expected attacks:\n";
                printBitboard(expectedAttacks);
                std::cerr << "Magic attacks:\n";
                printBitboard(magicAttacks);
            }
        }
    }
    
    if (mismatches > 0) {
        std::cerr << "ERROR: " << mismatches << " patterns don't match!\n";
        return false;
    }
    
    std::cout << "✓ All " << tableSize << " patterns validated successfully!\n";
    
    // Step 6: Test a few specific patterns
    std::cout << "\nTesting specific patterns:\n";
    
    // Empty board
    {
        Bitboard occupied = 0;
        Bitboard expected = slowRookAttacks(D4, occupied);
        uint64_t index = ((occupied & mask) * ROOK_MAGIC_D4) >> ROOK_SHIFT_D4;
        Bitboard magic = attackTable[index];
        
        std::cout << "Empty board: ";
        if (magic == expected) {
            std::cout << "✓ PASSED\n";
        } else {
            std::cout << "✗ FAILED\n";
            return false;
        }
    }
    
    // Blocked on all sides
    {
        Bitboard occupied = squareBB(D5) | squareBB(D3) | squareBB(E4) | squareBB(C4);
        Bitboard expected = slowRookAttacks(D4, occupied);
        uint64_t index = ((occupied & mask) * ROOK_MAGIC_D4) >> ROOK_SHIFT_D4;
        Bitboard magic = attackTable[index];
        
        std::cout << "Blocked adjacent: ";
        if (magic == expected) {
            std::cout << "✓ PASSED\n";
        } else {
            std::cout << "✗ FAILED\n";
            return false;
        }
    }
    
    return true;
}

int main() {
    std::cout << "=== METHODICAL VALIDATION: Phase 2B ===\n";
    
    if (testSingleSquareD4()) {
        std::cout << "\n=== PHASE 2B: COMPLETE AND VALIDATED ===\n";
        std::cout << "✓ Generated attack table for rook on D4\n";
        std::cout << "✓ All 1024 occupancy patterns validated\n";
        std::cout << "✓ Magic index calculation working correctly\n";
        std::cout << "✓ No collisions detected\n";
        std::cout << "✓ Ready to proceed to Step 2C: All Rook Tables\n";
        return 0;
    } else {
        std::cout << "\n✗ PHASE 2B: FAILED!\n";
        return 1;
    }
}