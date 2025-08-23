#include <iostream>
#include <chrono>
#include <bit>
#include <cstdint>
#include <random>

using Bitboard = uint64_t;
using Square = int;

enum Color { WHITE, BLACK };

constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;

inline Square lsb(Bitboard bb) {
    return static_cast<Square>(std::countr_zero(bb));
}

inline Square msb(Bitboard bb) {
    return static_cast<Square>(63 - std::countl_zero(bb));
}

Bitboard getDoubledPawns(Color c, Bitboard ourPawns) {
    Bitboard doubled = 0ULL;
    
    for (int file = 0; file < 8; file++) {
        Bitboard fileMask = FILE_A_BB << file;
        Bitboard pawnsOnFile = ourPawns & fileMask;
        
        int pawnCount = std::popcount(pawnsOnFile);
        
        if (pawnCount > 1) {
            Bitboard doublePawns = pawnsOnFile;
            
            if (c == WHITE) {
                Square rearmost = lsb(pawnsOnFile);
                doublePawns &= ~(1ULL << rearmost);
            } else {
                Square rearmost = msb(pawnsOnFile);
                doublePawns &= ~(1ULL << rearmost);
            }
            
            doubled |= doublePawns;
        }
    }
    
    return doubled;
}

int main() {
    // Generate random pawn structures
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<uint64_t> dist;
    
    constexpr int NUM_POSITIONS = 100000;
    std::vector<Bitboard> positions;
    positions.reserve(NUM_POSITIONS);
    
    // Generate realistic pawn structures (pawns only on ranks 2-7)
    for (int i = 0; i < NUM_POSITIONS; i++) {
        Bitboard pawns = dist(rng) & 0x00FFFFFFFFFFFF00ULL;
        // Ensure reasonable number of pawns (4-8)
        while (std::popcount(pawns) < 4 || std::popcount(pawns) > 8) {
            pawns = dist(rng) & 0x00FFFFFFFFFFFF00ULL;
        }
        positions.push_back(pawns);
    }
    
    // Test 1: Time doubled pawn detection
    auto start = std::chrono::high_resolution_clock::now();
    
    Bitboard totalDoubled = 0;
    for (const auto& pawns : positions) {
        // Call for both colors like in evaluation
        Bitboard whiteDoubled = getDoubledPawns(WHITE, pawns);
        Bitboard blackDoubled = getDoubledPawns(BLACK, pawns);
        totalDoubled ^= whiteDoubled ^ blackDoubled;  // Prevent optimization
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Doubled pawn detection performance test:\n";
    std::cout << "Positions tested: " << NUM_POSITIONS << "\n";
    std::cout << "Total time: " << duration.count() << " microseconds\n";
    std::cout << "Time per position: " << (double)duration.count() / NUM_POSITIONS << " microseconds\n";
    std::cout << "Positions per second: " << (NUM_POSITIONS * 1000000.0) / duration.count() << "\n";
    
    // Test 2: Baseline (just iterating)
    start = std::chrono::high_resolution_clock::now();
    
    Bitboard baseline = 0;
    for (const auto& pawns : positions) {
        baseline ^= pawns;  // Just access the data
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto baselineDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nBaseline (just iteration):\n";
    std::cout << "Total time: " << baselineDuration.count() << " microseconds\n";
    std::cout << "Overhead of doubled pawn detection: " << 
              (duration.count() - baselineDuration.count()) << " microseconds\n";
    std::cout << "Overhead per position: " << 
              (double)(duration.count() - baselineDuration.count()) / NUM_POSITIONS << " microseconds\n";
    
    // Prevent optimization
    std::cout << "\n(Checksum: " << totalDoubled << " " << baseline << ")\n";
    
    return 0;
}