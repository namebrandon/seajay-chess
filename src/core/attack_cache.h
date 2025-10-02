#pragma once

#include "types.h"
#include <array>
#include <cstdint>
#include <utility>  // for std::pair

namespace seajay {

/**
 * Thread-safe attack cache for position-based attack detection
 * Phase 2.1.b: Optimizes isSquareAttacked performance with caching
 * 
 * Revised Design:
 * - Thread-local storage for zero synchronization overhead
 * - Caches individual square attack queries (not full bitboards)
 * - Larger cache (256 entries) for better hit rate
 * - Compact entries for cache efficiency
 * - Simple hash replacement policy
 */
class AttackCache {
public:
    static constexpr size_t CACHE_SIZE = 256;  // Power of 2 for fast modulo
    static constexpr size_t CACHE_MASK = CACHE_SIZE - 1;
    
    // Compact cache entry for individual square queries
    struct CacheEntry {
        Hash zobristKey = 0;
        Square square = NO_SQUARE;
        uint8_t attackedByWhite : 1;  // Single bit for result
        uint8_t attackedByBlack : 1;  // Single bit for result
        uint8_t validWhite : 1;        // Is white result valid?
        uint8_t validBlack : 1;        // Is black result valid?
        uint8_t padding : 4;           // Align to byte
    };
    
    AttackCache() = default;
    ~AttackCache() = default;
    
    // Non-copyable, non-movable (thread-local)
    AttackCache(const AttackCache&) = delete;
    AttackCache& operator=(const AttackCache&) = delete;
    AttackCache(AttackCache&&) = delete;
    AttackCache& operator=(AttackCache&&) = delete;
    
    /**
     * Look up cached attack information for a specific square
     * @param zobristKey The position's zobrist hash
     * @param square The square to check
     * @param attackingColor The attacking color
     * @return pair<bool, bool> - first=cache hit, second=is attacked
     */
    std::pair<bool, bool> probe(Hash zobristKey, Square square, Color attackingColor, bool recordStats = false) const {
        // Combine zobrist and square for unique cache key
        const size_t index = (zobristKey ^ (square * 0x9e3779b97f4a7c15ULL)) & CACHE_MASK;
        const CacheEntry& entry = m_entries[index];
        
        if (entry.zobristKey == zobristKey && entry.square == square) {
            if (attackingColor == Color::WHITE && entry.validWhite) {
                if (recordStats) {
                    ++m_hits;
                }
                return {true, entry.attackedByWhite};
            }
            if (attackingColor == Color::BLACK && entry.validBlack) {
                if (recordStats) {
                    ++m_hits;
                }
                return {true, entry.attackedByBlack};
            }
        }
        
        if (recordStats) {
            ++m_misses;
        }
        return {false, false};
    }
    
    /**
     * Store attack information for a specific square
     * @param zobristKey The position's zobrist hash
     * @param square The square that was checked
     * @param attackingColor The attacking color
     * @param isAttacked Whether the square is attacked
     */
    void store(Hash zobristKey, Square square, Color attackingColor, bool isAttacked, bool recordStats = false) {
        const size_t index = (zobristKey ^ (square * 0x9e3779b97f4a7c15ULL)) & CACHE_MASK;
        CacheEntry& entry = m_entries[index];
        
        // Check if we can update existing entry
        if (entry.zobristKey == zobristKey && entry.square == square) {
            // Update existing entry
            if (attackingColor == Color::WHITE) {
                entry.attackedByWhite = isAttacked;
                entry.validWhite = 1;
            } else {
                entry.attackedByBlack = isAttacked;
                entry.validBlack = 1;
            }
        } else {
            if (recordStats && entry.square != NO_SQUARE) {
                ++m_evictions;
            }
            // Replace with new entry
            entry.zobristKey = zobristKey;
            entry.square = square;
            entry.validWhite = (attackingColor == Color::WHITE) ? 1 : 0;
            entry.validBlack = (attackingColor == Color::BLACK) ? 1 : 0;
            entry.attackedByWhite = (attackingColor == Color::WHITE) ? isAttacked : 0;
            entry.attackedByBlack = (attackingColor == Color::BLACK) ? isAttacked : 0;
        }
    }
    
    /**
     * Clear the entire cache
     */
    void clear() {
        for (auto& entry : m_entries) {
            entry.zobristKey = 0;
            entry.square = NO_SQUARE;
            entry.attackedByWhite = 0;
            entry.attackedByBlack = 0;
            entry.validWhite = 0;
            entry.validBlack = 0;
        }
        
        m_hits = 0;
        m_misses = 0;
        m_evictions = 0;
    }

    struct Stats {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;
        double hitRate() const {
            const uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / total : 0.0;
        }
    };

    Stats getStats() const {
        return {m_hits, m_misses, m_evictions};
    }

    void resetStats() {
        m_hits = 0;
        m_misses = 0;
        m_evictions = 0;
    }

private:
    std::array<CacheEntry, CACHE_SIZE> m_entries{};
    mutable uint64_t m_hits = 0;
    mutable uint64_t m_misses = 0;
    mutable uint64_t m_evictions = 0;
};

// Thread-local attack cache instance
// Each thread gets its own cache with zero synchronization overhead
extern thread_local AttackCache t_attackCache;

} // namespace seajay
