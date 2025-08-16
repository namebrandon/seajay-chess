#pragma once

#include "types.h"
#include "board.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <atomic>
#include <memory>

namespace seajay {

// SEE-specific piece values in centipawns
// These differ from regular evaluation values for better capture ordering
namespace SEEValues {
    // Stage 15 Day 8.2: Tuned piece values for SEE
    constexpr int PAWN_VALUE = 100;
    constexpr int KNIGHT_VALUE = 320;   // Slightly lower (was 325)
    constexpr int BISHOP_VALUE = 330;   // Slightly higher (was 325)
    constexpr int ROOK_VALUE = 500;
    constexpr int QUEEN_VALUE = 950;    // Reduced (was 975)
    constexpr int KING_VALUE = 10000;   // King cannot be captured
    
    // Array indexed by PieceType
    constexpr std::array<int, 7> PIECE_VALUES = {
        PAWN_VALUE,    // PAWN
        KNIGHT_VALUE,  // KNIGHT
        BISHOP_VALUE,  // BISHOP
        ROOK_VALUE,    // ROOK
        QUEEN_VALUE,   // QUEEN
        KING_VALUE,    // KING
        0              // NO_PIECE_TYPE
    };
    
    // Binary fingerprint for validation (Stage 15 signature)
    constexpr uint32_t SEE_FINGERPRINT = 0x5EE15000;  // "SEE 15.0"
    constexpr uint32_t SEE_VERSION = 1;
}

// SEE result type
using SEEValue = int;

// Special SEE values
constexpr SEEValue SEE_INVALID = -32768;
constexpr SEEValue SEE_UNKNOWN = -32767;

// Maximum depth for SEE calculation (prevents stack overflow)
constexpr int MAX_SEE_DEPTH = 32;

// Day 4.3: SEE Cache configuration
constexpr size_t SEE_CACHE_SIZE = 16384;  // Must be power of 2
constexpr uint64_t SEE_CACHE_MASK = SEE_CACHE_SIZE - 1;

// Cache entry for SEE results
struct SEECacheEntry {
    std::atomic<uint64_t> key{0};      // Zobrist key of position + move
    std::atomic<SEEValue> value{0};    // SEE result
    std::atomic<uint8_t> age{0};       // Age for replacement policy
};

// Day 4.4: SEE Statistics for debugging
struct SEEStatistics {
    std::atomic<uint64_t> calls{0};
    std::atomic<uint64_t> cacheHits{0};
    std::atomic<uint64_t> cacheMisses{0};
    std::atomic<uint64_t> earlyExits{0};
    std::atomic<uint64_t> lazyEvals{0};
    std::atomic<uint64_t> xrayChecks{0};
    
    void reset() noexcept {
        calls = 0;
        cacheHits = 0;
        cacheMisses = 0;
        earlyExits = 0;
        lazyEvals = 0;
        xrayChecks = 0;
    }
    
    double hitRate() const noexcept {
        uint64_t total = cacheHits + cacheMisses;
        return total > 0 ? (100.0 * cacheHits) / total : 0.0;
    }
};

// Forward declaration
class SEECalculator {
public:
    // Day 4.3: Constructor initializes cache
    SEECalculator();
    ~SEECalculator() = default;
    
    // Main SEE interface
    [[nodiscard]] SEEValue see(const Board& board, Move move) const noexcept;
    [[nodiscard]] SEEValue seeSign(const Board& board, Move move) const noexcept;
    
    // Threshold-based SEE (early exit optimization)
    [[nodiscard]] bool seeGE(const Board& board, Move move, SEEValue threshold) const noexcept;
    
    // Debug interface
    [[nodiscard]] uint32_t fingerprint() const noexcept { return SEEValues::SEE_FINGERPRINT; }
    [[nodiscard]] uint32_t version() const noexcept { return SEEValues::SEE_VERSION; }
    
    // Day 4.3: Cache management
    void clearCache() noexcept;
    void ageCache() noexcept;  // Age entries for replacement
    
    // Day 4.4: Statistics and debugging
    [[nodiscard]] const SEEStatistics& statistics() const noexcept { return m_stats; }
    void resetStatistics() noexcept { m_stats.reset(); }
    void enableDebugOutput(bool enable) noexcept { m_debugOutput = enable; }
    [[nodiscard]] bool isDebugEnabled() const noexcept { return m_debugOutput; }
    
private:
    // Attack detection wrappers (Day 1.2)
    // Day 4.2: Marked inline for hot path optimization
    [[nodiscard]] inline Bitboard attackersTo(const Board& board, Square sq, Bitboard occupied) const noexcept;
    [[nodiscard]] inline Bitboard leastValuableAttacker(const Board& board, Bitboard attackers, 
                                                        Color side, PieceType& attacker) const noexcept;
    
    // Swap algorithm implementation (Day 1.3)
    [[nodiscard]] SEEValue computeSEE(const Board& board, Square to, Color stm, 
                                      Bitboard attackers, Bitboard occupied) const noexcept;
    
    // Helper to get piece value
    // Day 4.2: Force inline for hot path
    [[nodiscard]] static constexpr int pieceValue(PieceType pt) noexcept {
        return pt < 7 ? SEEValues::PIECE_VALUES[pt] : 0;
    }
    
    // Day 4.3: SEE result cache
    std::unique_ptr<SEECacheEntry[]> m_cache;
    mutable uint8_t m_currentAge = 0;
    
    // Day 4.4: Statistics tracking
    mutable SEEStatistics m_stats;
    bool m_debugOutput = false;
    
    // Cache key generation
    [[nodiscard]] uint64_t makeCacheKey(const Board& board, Move move) const noexcept;
    [[nodiscard]] SEEValue probeCache(uint64_t key) const noexcept;
    void storeCache(uint64_t key, SEEValue value) const noexcept;
    
    // Day 3.1: X-ray detection
    // Detects sliding attackers that are revealed when a piece moves off a ray
    [[nodiscard]] Bitboard getXrayAttackers(const Board& board, Square sq, 
                                            Bitboard occupied, Bitboard removedPiece) const noexcept;
    
    // Thread-local storage for swap arrays (Day 1.3)
    struct SwapList {
        std::array<int, MAX_SEE_DEPTH> gains{};
        int depth = 0;
        
        void clear() noexcept { depth = 0; }
        void push(int value) noexcept { 
            assert(depth < MAX_SEE_DEPTH);
            gains[depth++] = value; 
        }
    };
    
    // Thread-local storage (declared static to work with const methods)
    static thread_local SwapList m_swapList;
};

// Global SEE calculator instance
inline SEECalculator g_seeCalculator;

// Convenience functions
[[nodiscard]] inline SEEValue see(const Board& board, Move move) noexcept {
    return g_seeCalculator.see(board, move);
}

[[nodiscard]] inline SEEValue seeSign(const Board& board, Move move) noexcept {
    return g_seeCalculator.seeSign(board, move);
}

[[nodiscard]] inline bool seeGE(const Board& board, Move move, SEEValue threshold) noexcept {
    return g_seeCalculator.seeGE(board, move, threshold);
}

} // namespace seajay