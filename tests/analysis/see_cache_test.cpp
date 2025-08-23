// Test to demonstrate the expensive fallback hash generation in SEE cache
// when zobrist key is not initialized

#include <chrono>
#include <iostream>
#include <iomanip>
#include "../../src/core/types.h"

using namespace seajay;

// Simulating the expensive fallback hash from see.cpp lines 48-62
uint64_t expensiveFallbackHash() {
    uint64_t boardKey = 0;
    
    // Simulate piece positions
    Piece pieces[64];
    for (int i = 0; i < 64; ++i) {
        pieces[i] = (i % 8 == 0) ? WHITE_PAWN : NO_PIECE;
    }
    
    // This is the expensive fallback from SEE
    for (Square sq = A1; sq <= H8; ++sq) {
        Piece p = pieces[sq];
        if (p != NO_PIECE) {
            boardKey ^= (uint64_t(p) << (sq % 32)) * 0x9E3779B97F4A7C15ULL;
            boardKey = (boardKey << 13) | (boardKey >> 51);  // Expensive rotate
        }
    }
    
    // Add side to move
    boardKey ^= 0x1234567890ABCDEFULL;
    
    return boardKey;
}

// Optimized simple hash
uint64_t simpleHash() {
    uint64_t boardKey = 0;
    
    // Simulate piece positions
    Piece pieces[64];
    for (int i = 0; i < 64; ++i) {
        pieces[i] = (i % 8 == 0) ? WHITE_PAWN : NO_PIECE;
    }
    
    // Simple hash without expensive operations
    for (Square sq = A1; sq <= H8; ++sq) {
        Piece p = pieces[sq];
        if (p != NO_PIECE) {
            // Just XOR with piece and square - no multiplication or rotation
            boardKey ^= (uint64_t(p) << 4) | uint64_t(sq);
            boardKey = boardKey * 0x9E3779B97F4A7C15ULL;  // Single multiply at end
        }
    }
    
    return boardKey;
}

// Even better: Just return a constant for uninitialized boards
uint64_t constantFallback() {
    // For uninitialized boards, we could just return a constant
    // since they shouldn't be used in real games anyway
    return 0xDEADBEEFCAFEBABEULL;
}

void testSEECacheKeyGeneration() {
    std::cout << "=== SEE Cache Key Generation Performance Test ===\n\n";
    std::cout << "This test demonstrates the cost of the fallback hash generation\n";
    std::cout << "in SEE when zobrist keys are not initialized.\n\n";
    
    const int ITERATIONS = 1000000;
    
    // Test expensive fallback (current implementation)
    std::cout << "Testing expensive fallback hash (current)...\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    volatile uint64_t sum = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        sum ^= expensiveFallbackHash();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto expensive_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Test simple hash
    std::cout << "Testing simple hash (proposed)...\n";
    start = std::chrono::high_resolution_clock::now();
    
    sum = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        sum ^= simpleHash();
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto simple_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Test constant fallback
    std::cout << "Testing constant fallback (best)...\n";
    start = std::chrono::high_resolution_clock::now();
    
    sum = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        sum ^= constantFallback();
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto constant_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Results
    std::cout << "\n=== Results for " << ITERATIONS << " iterations ===\n\n";
    
    std::cout << "Expensive fallback (current):\n";
    std::cout << "  Total time: " << expensive_time.count() << " µs\n";
    std::cout << "  Per operation: " << std::fixed << std::setprecision(3)
              << (double)expensive_time.count() / ITERATIONS * 1000 << " ns\n\n";
    
    std::cout << "Simple hash (proposed):\n";
    std::cout << "  Total time: " << simple_time.count() << " µs\n";
    std::cout << "  Per operation: " << std::fixed << std::setprecision(3)
              << (double)simple_time.count() / ITERATIONS * 1000 << " ns\n";
    std::cout << "  Speedup: " << std::fixed << std::setprecision(2)
              << (double)expensive_time.count() / simple_time.count() << "x\n\n";
    
    std::cout << "Constant fallback (best):\n";
    std::cout << "  Total time: " << constant_time.count() << " µs\n";
    std::cout << "  Per operation: " << std::fixed << std::setprecision(3)
              << (double)constant_time.count() / ITERATIONS * 1000 << " ns\n";
    std::cout << "  Speedup: " << std::fixed << std::setprecision(2)
              << (double)expensive_time.count() / constant_time.count() << "x\n\n";
    
    std::cout << "Analysis:\n";
    std::cout << "- The fallback hash should rarely be needed (zobrist should be initialized)\n";
    std::cout << "- When it is needed, the current implementation is very expensive\n";
    std::cout << "- Modulo operations (sq % 32) in a loop are particularly costly\n";
    std::cout << "- Rotation operations add unnecessary overhead\n\n";
    
    std::cout << "Recommendations:\n";
    std::cout << "1. Ensure zobrist keys are always initialized (best solution)\n";
    std::cout << "2. If fallback needed, use simple hash without modulo/rotation\n";
    std::cout << "3. Consider just returning a constant for uninitialized boards\n";
}

int main() {
    testSEECacheKeyGeneration();
    return 0;
}