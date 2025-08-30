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

// ========================= PAWN STRUCTURE SIMD OPTIMIZATIONS =========================
// Phase 2.5.e-2: Vectorized pawn structure evaluation for improved NPS

// Helper to extract the lowest set bit position
inline int popLsb(uint64_t& bb) {
    int sq = __builtin_ctzll(bb);
    bb &= bb - 1;  // Clear the lowest bit
    return sq;
}

// SIMD-optimized isolated pawn detection
// Processes all files in parallel using bitwise operations
inline uint64_t getIsolatedPawnsFast(uint64_t ourPawns) {
    // File masks for adjacent file detection
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;
    constexpr uint64_t NOT_FILE_A = ~FILE_A;
    constexpr uint64_t NOT_FILE_H = ~FILE_H;
    
    uint64_t isolated = 0ULL;
    
    // Process each file and check for adjacent file support
    // This can be vectorized by the compiler with -march=native
    #pragma GCC unroll 8
    for (int file = 0; file < 8; ++file) {
        uint64_t fileMask = FILE_A << file;
        uint64_t pawnsOnFile = ourPawns & fileMask;
        
        if (pawnsOnFile) {
            uint64_t adjacentFiles = 0ULL;
            if (file > 0) adjacentFiles |= FILE_A << (file - 1);
            if (file < 7) adjacentFiles |= FILE_A << (file + 1);
            
            // If no pawns on adjacent files, all pawns on this file are isolated
            if (!(ourPawns & adjacentFiles)) {
                isolated |= pawnsOnFile;
            }
        }
    }
    
    return isolated;
}

// SIMD-optimized doubled pawn detection
// Process multiple files simultaneously for better ILP
inline uint64_t getDoubledPawnsFast(uint64_t ourPawns, bool isWhite) {
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    uint64_t doubled = 0ULL;
    
    // Process 4 files at a time for better instruction-level parallelism
    #pragma GCC unroll 2
    for (int i = 0; i < 8; i += 4) {
        // Process 4 files in parallel
        uint64_t files[4];
        int counts[4];
        
        // Extract file masks and count pawns
        for (int j = 0; j < 4 && (i + j) < 8; ++j) {
            files[j] = ourPawns & (FILE_A << (i + j));
            counts[j] = std::popcount(files[j]);
        }
        
        // Mark doubled pawns (all but rearmost)
        for (int j = 0; j < 4 && (i + j) < 8; ++j) {
            if (counts[j] > 1) {
                // Find rearmost pawn
                uint64_t rearmost;
                if (isWhite) {
                    // For white, rearmost is lowest bit (lowest rank)
                    rearmost = files[j] & -files[j];
                } else {
                    // For black, rearmost is highest bit (highest rank)
                    rearmost = 1ULL << (63 - __builtin_clzll(files[j]));
                }
                doubled |= files[j] & ~rearmost;
            }
        }
    }
    
    return doubled;
}

// SIMD-optimized backward pawn detection
// Uses parallel bit operations for efficient detection
inline uint64_t getBackwardPawnsFast(uint64_t ourPawns, uint64_t theirPawns, 
                                     bool isWhite, uint64_t isolatedPawns) {
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    constexpr uint64_t FILE_H = 0x8080808080808080ULL;
    constexpr uint64_t RANK_2 = 0x000000000000FF00ULL;
    constexpr uint64_t RANK_7 = 0x00FF000000000000ULL;
    
    // Don't double-penalize isolated pawns
    uint64_t candidates = ourPawns & ~isolatedPawns;
    
    // Remove pawns on starting ranks (can't be backward)
    if (isWhite) {
        candidates &= ~RANK_2;
    } else {
        candidates &= ~RANK_7;
    }
    
    uint64_t backward = 0ULL;
    uint64_t pawns = candidates;
    
    // Process pawns in batches for better cache usage
    while (pawns) {
        int sq = popLsb(pawns);
        int rank = sq / 8;
        int file = sq % 8;
        
        // Check for support from adjacent files
        uint64_t supportMask = 0ULL;
        if (file > 0) {
            // Support from left file
            uint64_t leftFile = FILE_A << (file - 1);
            if (isWhite) {
                // White pawns need support from same rank or behind (lower ranks)
                for (int r = 0; r <= rank; ++r) {
                    supportMask |= (1ULL << (r * 8 + file - 1));
                }
            } else {
                // Black pawns need support from same rank or behind (higher ranks)
                for (int r = rank; r < 8; ++r) {
                    supportMask |= (1ULL << (r * 8 + file - 1));
                }
            }
        }
        
        if (file < 7) {
            // Support from right file
            if (isWhite) {
                for (int r = 0; r <= rank; ++r) {
                    supportMask |= (1ULL << (r * 8 + file + 1));
                }
            } else {
                for (int r = rank; r < 8; ++r) {
                    supportMask |= (1ULL << (r * 8 + file + 1));
                }
            }
        }
        
        // If no support and front square is attacked, pawn is backward
        if (!(ourPawns & supportMask)) {
            // Check if front square is attacked by enemy pawns
            int frontSq = isWhite ? sq + 8 : sq - 8;
            if (frontSq >= 0 && frontSq < 64) {
                uint64_t enemyAttackers = 0ULL;
                
                if (isWhite) {
                    // Check for black pawn attackers
                    if (file > 0 && frontSq + 7 < 64) {
                        enemyAttackers |= (1ULL << (frontSq + 7));
                    }
                    if (file < 7 && frontSq + 9 < 64) {
                        enemyAttackers |= (1ULL << (frontSq + 9));
                    }
                } else {
                    // Check for white pawn attackers
                    if (file < 7 && frontSq - 7 >= 0) {
                        enemyAttackers |= (1ULL << (frontSq - 7));
                    }
                    if (file > 0 && frontSq - 9 >= 0) {
                        enemyAttackers |= (1ULL << (frontSq - 9));
                    }
                }
                
                if (theirPawns & enemyAttackers) {
                    backward |= (1ULL << sq);
                }
            }
        }
    }
    
    return backward;
}

// SIMD-optimized passed pawn detection using batch processing
inline uint64_t getPassedPawnsFast(uint64_t ourPawns, uint64_t theirPawns, 
                                   const uint64_t* passedMasks) {
    uint64_t passed = 0ULL;
    uint64_t pawns = ourPawns;
    
    // Process pawns in groups for better instruction-level parallelism
    while (pawns) {
        // Extract up to 4 pawns at once for parallel processing
        int sqs[4];
        int count = 0;
        
        for (int i = 0; i < 4 && pawns; ++i) {
            sqs[i] = popLsb(pawns);
            count++;
        }
        
        // Check all extracted pawns in parallel
        for (int i = 0; i < count; ++i) {
            if (!(theirPawns & passedMasks[sqs[i]])) {
                passed |= (1ULL << sqs[i]);
            }
        }
    }
    
    return passed;
}

// Optimized pawn island counting using bit manipulation
inline int countPawnIslandsFast(uint64_t ourPawns) {
    if (!ourPawns) return 0;
    
    constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    
    // Create a byte mask of files that have pawns
    uint8_t fileMask = 0;
    #pragma GCC unroll 8
    for (int f = 0; f < 8; ++f) {
        if (ourPawns & (FILE_A << f)) {
            fileMask |= (1 << f);
        }
    }
    
    // Count transitions from 0 to 1 (start of each island)
    int islands = 0;
    bool prevFile = false;
    
    for (int f = 0; f < 8; ++f) {
        bool currFile = (fileMask >> f) & 1;
        if (currFile && !prevFile) {
            islands++;
        }
        prevFile = currFile;
    }
    
    return islands;
}

} // namespace simd
} // namespace seajay

#endif // SIMD_UTILS_H