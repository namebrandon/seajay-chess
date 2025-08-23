// Test to demonstrate performance difference between modulo (prime) and mask (power of 2)
// for pawn hash table indexing

#include <chrono>
#include <iostream>
#include <random>
#include <iomanip>

void testPawnHashPerformance() {
    std::cout << "=== Pawn Hash Table Indexing Performance Test ===\n\n";
    
    // Current implementation uses prime number
    const size_t PRIME_SIZE = 16381;
    
    // Optimized would use power of 2
    const size_t POWER2_SIZE = 16384;  // 2^14
    const size_t MASK = POWER2_SIZE - 1;
    
    // Generate random hash keys (simulating pawn zobrist keys)
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    
    const int NUM_KEYS = 1000;
    uint64_t* keys = new uint64_t[NUM_KEYS];
    for (int i = 0; i < NUM_KEYS; ++i) {
        keys[i] = dist(gen);
    }
    
    const int ITERATIONS = 1000000;
    
    // Test modulo with prime (current implementation)
    std::cout << "Testing modulo with prime number (16381)...\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    volatile size_t sum = 0;  // volatile to prevent optimization
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        for (int i = 0; i < NUM_KEYS; ++i) {
            sum += keys[i] % PRIME_SIZE;  // Expensive modulo operation
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto modulo_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Test mask with power of 2 (proposed optimization)
    std::cout << "Testing mask with power of 2 (16384)...\n";
    start = std::chrono::high_resolution_clock::now();
    
    sum = 0;
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        for (int i = 0; i < NUM_KEYS; ++i) {
            sum += keys[i] & MASK;  // Fast bitwise AND
        }
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto mask_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate and display results
    std::cout << "\n=== Results ===\n";
    std::cout << "Operations performed: " << (long long)ITERATIONS * NUM_KEYS << " hash lookups\n\n";
    
    std::cout << "Modulo (prime 16381):\n";
    std::cout << "  Total time: " << modulo_time.count() << " µs\n";
    std::cout << "  Time per lookup: " << std::fixed << std::setprecision(3) 
              << (double)modulo_time.count() / (ITERATIONS * NUM_KEYS) * 1000 << " ns\n\n";
    
    std::cout << "Mask (power of 2 - 16384):\n";
    std::cout << "  Total time: " << mask_time.count() << " µs\n";
    std::cout << "  Time per lookup: " << std::fixed << std::setprecision(3)
              << (double)mask_time.count() / (ITERATIONS * NUM_KEYS) * 1000 << " ns\n\n";
    
    double speedup = (double)modulo_time.count() / mask_time.count();
    std::cout << "SPEEDUP: " << std::fixed << std::setprecision(2) << speedup << "x faster with power of 2\n\n";
    
    // Distribution analysis
    std::cout << "=== Distribution Analysis ===\n";
    int* prime_dist = new int[PRIME_SIZE]();
    int* power2_dist = new int[POWER2_SIZE]();
    
    for (int i = 0; i < NUM_KEYS * 10; ++i) {
        uint64_t key = dist(gen);
        prime_dist[key % PRIME_SIZE]++;
        power2_dist[key & MASK]++;
    }
    
    // Calculate distribution statistics
    double prime_avg = (double)(NUM_KEYS * 10) / PRIME_SIZE;
    double power2_avg = (double)(NUM_KEYS * 10) / POWER2_SIZE;
    
    int prime_min = NUM_KEYS * 10, prime_max = 0;
    int power2_min = NUM_KEYS * 10, power2_max = 0;
    
    for (size_t i = 0; i < PRIME_SIZE; ++i) {
        if (prime_dist[i] < prime_min) prime_min = prime_dist[i];
        if (prime_dist[i] > prime_max) prime_max = prime_dist[i];
    }
    
    for (size_t i = 0; i < POWER2_SIZE; ++i) {
        if (power2_dist[i] < power2_min) power2_min = power2_dist[i];
        if (power2_dist[i] > power2_max) power2_max = power2_dist[i];
    }
    
    std::cout << "Prime (16381) distribution:\n";
    std::cout << "  Expected per bucket: " << std::fixed << std::setprecision(2) << prime_avg << "\n";
    std::cout << "  Min/Max: " << prime_min << "/" << prime_max << "\n\n";
    
    std::cout << "Power of 2 (16384) distribution:\n";
    std::cout << "  Expected per bucket: " << std::fixed << std::setprecision(2) << power2_avg << "\n";
    std::cout << "  Min/Max: " << power2_min << "/" << power2_max << "\n\n";
    
    std::cout << "Conclusion:\n";
    std::cout << "- Power of 2 is " << std::fixed << std::setprecision(1) << speedup << "x faster\n";
    std::cout << "- Distribution quality is nearly identical\n";
    std::cout << "- Only 3 extra hash slots used (16384 vs 16381)\n";
    std::cout << "- Simple fix: Change PAWN_HASH_SIZE from 16381 to 16384\n";
    
    delete[] keys;
    delete[] prime_dist;
    delete[] power2_dist;
}

int main() {
    testPawnHashPerformance();
    return 0;
}