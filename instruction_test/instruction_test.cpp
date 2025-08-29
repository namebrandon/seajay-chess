#include <iostream>
#include <chrono>
#include <immintrin.h>
#include <cstdint>

// Test basic operations
uint64_t basic_test() {
    uint64_t result = 0;
    for (int i = 0; i < 1000000; ++i) {
        result += i * i;
    }
    return result;
}

// Test POPCNT instruction
#ifdef __POPCNT__
uint64_t popcnt_test() {
    uint64_t result = 0;
    for (uint64_t i = 0; i < 1000000; ++i) {
        result += __builtin_popcountll(i);
    }
    return result;
}
#endif

// Test SSE4.2 instructions
#ifdef __SSE4_2__
uint64_t sse42_test() {
    __m128i a = _mm_set_epi32(1, 2, 3, 4);
    __m128i b = _mm_set_epi32(5, 6, 7, 8);
    __m128i c = _mm_add_epi32(a, b);
    
    uint64_t result = 0;
    for (int i = 0; i < 1000000; ++i) {
        c = _mm_add_epi32(c, a);
        result += _mm_extract_epi32(c, 0);
    }
    return result;
}
#endif

// Test BMI1 instructions (TZCNT, LZCNT)
#ifdef __BMI__
uint64_t bmi1_test() {
    uint64_t result = 0;
    for (uint64_t i = 1; i < 1000000; ++i) {
        result += __builtin_ctzll(i);  // Count trailing zeros
    }
    return result;
}
#endif

// Test BMI2 instructions (PEXT, PDEP)
#ifdef __BMI2__
#include <x86intrin.h>
uint64_t bmi2_test() {
    uint64_t result = 0;
    uint64_t mask = 0x0F0F0F0F0F0F0F0F;
    for (uint64_t i = 0; i < 1000000; ++i) {
        result += _pext_u64(i, mask);
    }
    return result;
}
#endif

// Test AVX instructions
#ifdef __AVX__
uint64_t avx_test() {
    __m256d a = _mm256_set_pd(1.0, 2.0, 3.0, 4.0);
    __m256d b = _mm256_set_pd(5.0, 6.0, 7.0, 8.0);
    __m256d c = _mm256_add_pd(a, b);
    
    uint64_t result = 0;
    for (int i = 0; i < 1000000; ++i) {
        c = _mm256_add_pd(c, a);
        result += static_cast<uint64_t>(c[0]);
    }
    return result;
}
#endif

// Test AVX2 instructions
#ifdef __AVX2__
uint64_t avx2_test() {
    __m256i a = _mm256_set_epi32(1, 2, 3, 4, 5, 6, 7, 8);
    __m256i b = _mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1);
    __m256i c = _mm256_add_epi32(a, b);
    
    uint64_t result = 0;
    for (int i = 0; i < 1000000; ++i) {
        c = _mm256_add_epi32(c, a);
        result += _mm256_extract_epi32(c, 0);
    }
    return result;
}
#endif

int main() {
    std::cout << "Instruction Set Test Program\n";
    std::cout << "============================\n\n";
    
    std::cout << "Compiler flags detected:\n";
    
    #ifdef __SSE__
    std::cout << "  SSE:     YES\n";
    #else
    std::cout << "  SSE:     NO\n";
    #endif
    
    #ifdef __SSE2__
    std::cout << "  SSE2:    YES\n";
    #else
    std::cout << "  SSE2:    NO\n";
    #endif
    
    #ifdef __SSE3__
    std::cout << "  SSE3:    YES\n";
    #else
    std::cout << "  SSE3:    NO\n";
    #endif
    
    #ifdef __SSSE3__
    std::cout << "  SSSE3:   YES\n";
    #else
    std::cout << "  SSSE3:   NO\n";
    #endif
    
    #ifdef __SSE4_1__
    std::cout << "  SSE4.1:  YES\n";
    #else
    std::cout << "  SSE4.1:  NO\n";
    #endif
    
    #ifdef __SSE4_2__
    std::cout << "  SSE4.2:  YES\n";
    #else
    std::cout << "  SSE4.2:  NO\n";
    #endif
    
    #ifdef __POPCNT__
    std::cout << "  POPCNT:  YES\n";
    #else
    std::cout << "  POPCNT:  NO\n";
    #endif
    
    #ifdef __BMI__
    std::cout << "  BMI1:    YES\n";
    #else
    std::cout << "  BMI1:    NO\n";
    #endif
    
    #ifdef __BMI2__
    std::cout << "  BMI2:    YES\n";
    #else
    std::cout << "  BMI2:    NO\n";
    #endif
    
    #ifdef __AVX__
    std::cout << "  AVX:     YES\n";
    #else
    std::cout << "  AVX:     NO\n";
    #endif
    
    #ifdef __AVX2__
    std::cout << "  AVX2:    YES\n";
    #else
    std::cout << "  AVX2:    NO\n";
    #endif
    
    std::cout << "\nRunning tests...\n\n";
    
    // Basic test
    {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t result = basic_test();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Basic test:   " << duration.count() << " us (result: " << result << ")\n";
    }
    
    #ifdef __POPCNT__
    {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t result = popcnt_test();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "POPCNT test:  " << duration.count() << " us (result: " << result << ")\n";
    }
    #endif
    
    #ifdef __SSE4_2__
    {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t result = sse42_test();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "SSE4.2 test:  " << duration.count() << " us (result: " << result << ")\n";
    }
    #endif
    
    #ifdef __BMI__
    {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t result = bmi1_test();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "BMI1 test:    " << duration.count() << " us (result: " << result << ")\n";
    }
    #endif
    
    #ifdef __BMI2__
    {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t result = bmi2_test();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "BMI2 test:    " << duration.count() << " us (result: " << result << ")\n";
    }
    #endif
    
    #ifdef __AVX__
    {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t result = avx_test();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "AVX test:     " << duration.count() << " us (result: " << result << ")\n";
    }
    #endif
    
    #ifdef __AVX2__
    {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t result = avx2_test();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "AVX2 test:    " << duration.count() << " us (result: " << result << ")\n";
    }
    #endif
    
    std::cout << "\nTest complete!\n";
    return 0;
}