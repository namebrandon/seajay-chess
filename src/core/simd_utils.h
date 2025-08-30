#ifndef SIMD_UTILS_H
#define SIMD_UTILS_H

#include <cstdint>
#include <bit>
#include <array>

// CPU feature detection
#ifdef __x86_64__
#include <cpuid.h>
#include <immintrin.h>
#endif

namespace seajay {
namespace simd {

// Runtime CPU feature detection
class CpuFeatures {
public:
    static CpuFeatures& getInstance() {
        static CpuFeatures instance;
        return instance;
    }
    
    bool hasSSE42() const { return m_hasSSE42; }
    bool hasAVX2() const { return m_hasAVX2; }
    bool hasPOPCNT() const { return m_hasPOPCNT; }
    
private:
    CpuFeatures() {
        #ifdef __x86_64__
        unsigned int eax, ebx, ecx, edx;
        
        // Check for SSE4.2 and POPCNT
        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
            m_hasSSE42 = (ecx & bit_SSE4_2) != 0;
            m_hasPOPCNT = (ecx & bit_POPCNT) != 0;
        }
        
        // Check for AVX2
        if (__get_cpuid_max(0, nullptr) >= 7) {
            __cpuid_count(7, 0, eax, ebx, ecx, edx);
            m_hasAVX2 = (ebx & bit_AVX2) != 0;
        }
        #endif
    }
    
    bool m_hasSSE42 = false;
    bool m_hasAVX2 = false;
    bool m_hasPOPCNT = false;
};

// Batch popcount for multiple bitboards
// This function processes multiple bitboards in parallel where possible
template<size_t N>
inline void batchPopcount(const uint64_t* bitboards, uint32_t* results) {
    const auto& cpu = CpuFeatures::getInstance();
    
    // For now, use scalar popcount with potential compiler vectorization
    // The compiler with -march=native -O3 may auto-vectorize this loop
    #pragma GCC unroll 8
    for (size_t i = 0; i < N; ++i) {
        results[i] = std::popcount(bitboards[i]);
    }
}

// Specialized version for material counting (4 piece types)
inline void popcountMaterial(
    uint64_t knights, uint64_t bishops, 
    uint64_t rooks, uint64_t queens,
    uint32_t& knightCount, uint32_t& bishopCount,
    uint32_t& rookCount, uint32_t& queenCount) {
    
    // Process all four popcounts together for better ILP
    knightCount = std::popcount(knights);
    bishopCount = std::popcount(bishops);
    rookCount = std::popcount(rooks);
    queenCount = std::popcount(queens);
}

// Specialized version for insufficient material detection (10 piece counts)
inline void popcountAllPieces(
    const uint64_t whitePieces[6], const uint64_t blackPieces[6],
    uint32_t whiteCounts[6], uint32_t blackCounts[6]) {
    
    // Unroll and interleave for better instruction-level parallelism
    // Modern CPUs can execute multiple popcnt instructions in parallel
    whiteCounts[0] = std::popcount(whitePieces[0]); // PAWN
    blackCounts[0] = std::popcount(blackPieces[0]);
    whiteCounts[1] = std::popcount(whitePieces[1]); // KNIGHT
    blackCounts[1] = std::popcount(blackPieces[1]);
    whiteCounts[2] = std::popcount(whitePieces[2]); // BISHOP
    blackCounts[2] = std::popcount(blackPieces[2]);
    whiteCounts[3] = std::popcount(whitePieces[3]); // ROOK
    blackCounts[3] = std::popcount(blackPieces[3]);
    whiteCounts[4] = std::popcount(whitePieces[4]); // QUEEN
    blackCounts[4] = std::popcount(blackPieces[4]);
    whiteCounts[5] = std::popcount(whitePieces[5]); // KING
    blackCounts[5] = std::popcount(blackPieces[5]);
}

} // namespace simd
} // namespace seajay

#endif // SIMD_UTILS_H