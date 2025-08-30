#include <iostream>
#include <chrono>
#include <random>
#include <bit>
#include <cstdint>
#include <vector>

// Simple benchmark to verify SIMD optimization impact
void benchmarkPopcountBatch() {
    const int ITERATIONS = 10000000;
    const int BATCH_SIZE = 12;  // Typical number of piece bitboards
    
    // Generate random bitboards
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    
    std::vector<uint64_t> bitboards(BATCH_SIZE);
    for (auto& bb : bitboards) {
        bb = dis(gen);
    }
    
    // Test 1: Sequential popcount (old method)
    auto start = std::chrono::high_resolution_clock::now();
    uint32_t sum1 = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        for (int j = 0; j < BATCH_SIZE; ++j) {
            sum1 += std::popcount(bitboards[j]);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto sequential_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Test 2: Batched popcount (simulating ILP benefits)
    start = std::chrono::high_resolution_clock::now();
    uint32_t sum2 = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        // Unrolled loop for better ILP
        uint32_t c0 = std::popcount(bitboards[0]);
        uint32_t c1 = std::popcount(bitboards[1]);
        uint32_t c2 = std::popcount(bitboards[2]);
        uint32_t c3 = std::popcount(bitboards[3]);
        uint32_t c4 = std::popcount(bitboards[4]);
        uint32_t c5 = std::popcount(bitboards[5]);
        uint32_t c6 = std::popcount(bitboards[6]);
        uint32_t c7 = std::popcount(bitboards[7]);
        uint32_t c8 = std::popcount(bitboards[8]);
        uint32_t c9 = std::popcount(bitboards[9]);
        uint32_t c10 = std::popcount(bitboards[10]);
        uint32_t c11 = std::popcount(bitboards[11]);
        
        sum2 += c0 + c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9 + c10 + c11;
    }
    end = std::chrono::high_resolution_clock::now();
    auto batched_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "SIMD Popcount Optimization Analysis\n";
    std::cout << "====================================\n";
    std::cout << "Sequential time: " << sequential_time << " ms\n";
    std::cout << "Batched time:    " << batched_time << " ms\n";
    std::cout << "Speedup:         " << (double)sequential_time / batched_time << "x\n";
    std::cout << "Checksums match: " << (sum1 == sum2 ? "YES" : "NO") << "\n";
}

int main() {
    std::cout << "Testing SIMD optimizations for SeaJay\n\n";
    
    // Check CPU capabilities
    std::cout << "CPU Capabilities:\n";
    std::cout << "-----------------\n";
    
    #ifdef __SSE4_2__
    std::cout << "SSE4.2:  AVAILABLE\n";
    #else
    std::cout << "SSE4.2:  NOT AVAILABLE\n";
    #endif
    
    #ifdef __POPCNT__
    std::cout << "POPCNT:  AVAILABLE\n";
    #else
    std::cout << "POPCNT:  NOT AVAILABLE\n";
    #endif
    
    #ifdef __AVX2__
    std::cout << "AVX2:    AVAILABLE\n";
    #else
    std::cout << "AVX2:    NOT AVAILABLE\n";
    #endif
    
    #ifdef __BMI2__
    std::cout << "BMI2:    AVAILABLE\n";
    #else
    std::cout << "BMI2:    NOT AVAILABLE\n";
    #endif
    
    std::cout << "\n";
    
    benchmarkPopcountBatch();
    
    return 0;
}