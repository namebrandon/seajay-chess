#pragma once

// Phase 2.3 - Missing Item 4: Memory and Cache Optimization for Quiescence Search
// This file contains optimized versions of quiescence functions focused on:
// - Minimizing stack usage
// - Efficient move list handling 
// - Reducing function call overhead

#include "../core/types.h"
#include "../core/board.h"
#include "../evaluation/types.h"
#include "types.h"
#include <array>

namespace seajay::search {

// Memory-optimized constants for quiescence
namespace qsearch_opt {
    // Reduce maximum captures per node for better cache behavior
    constexpr int MAX_CAPTURES_OPTIMIZED = 16;  // Instead of 32
    
    // Use smaller, stack-friendly move arrays
    constexpr int QSEARCH_MOVE_BUFFER_SIZE = 32;  // Fixed-size stack array
    
    // Cache-friendly move generation batch size
    constexpr int MOVE_GEN_BATCH_SIZE = 8;
}

// Stack-optimized move container for quiescence
// Uses fixed-size array instead of dynamic MoveList to reduce allocations
class QSearchMoveBuffer {
private:
    std::array<Move, qsearch_opt::QSEARCH_MOVE_BUFFER_SIZE> m_moves;
    int m_size = 0;
    
public:
    // Iterator support for compatibility with existing code
    using iterator = std::array<Move, qsearch_opt::QSEARCH_MOVE_BUFFER_SIZE>::iterator;
    using const_iterator = std::array<Move, qsearch_opt::QSEARCH_MOVE_BUFFER_SIZE>::const_iterator;
    
    iterator begin() noexcept { return m_moves.begin(); }
    iterator end() noexcept { return m_moves.begin() + m_size; }
    const_iterator begin() const noexcept { return m_moves.begin(); }
    const_iterator end() const noexcept { return m_moves.begin() + m_size; }
    
    // Core operations
    void clear() noexcept { m_size = 0; }
    int size() const noexcept { return m_size; }
    bool empty() const noexcept { return m_size == 0; }
    bool full() const noexcept { return m_size >= qsearch_opt::QSEARCH_MOVE_BUFFER_SIZE; }
    
    // Add move (with overflow protection)
    bool push_back(Move move) noexcept {
        if (full()) return false;
        m_moves[m_size++] = move;
        return true;
    }
    
    // Access operators
    Move& operator[](int index) noexcept { return m_moves[index]; }
    const Move& operator[](int index) const noexcept { return m_moves[index]; }
    
    // Stack usage: ~128 bytes (32 moves * 4 bytes each) + metadata
    static constexpr size_t stack_usage() { return sizeof(QSearchMoveBuffer); }
};

// Optimized move generation specifically for quiescence
class OptimizedQSearchMoveGen {
public:
    // Generate captures directly into stack buffer (avoids MoveList allocation)
    static int generateCapturesOptimized(const Board& board, QSearchMoveBuffer& buffer);
    
    // Generate legal moves for check evasion (limited to buffer size)
    static int generateCheckEvasionsOptimized(const Board& board, QSearchMoveBuffer& buffer);
    
    // In-place move ordering with minimal memory movement
    static void orderMovesInPlace(const Board& board, QSearchMoveBuffer& buffer);
    
private:
    // Specialized generators for common piece types (reduces function call overhead)
    static void generatePawnCapturesOptimized(const Board& board, QSearchMoveBuffer& buffer);
    static void generatePieceCapturesOptimized(const Board& board, QSearchMoveBuffer& buffer, PieceType pt);
};

// Cache-friendly move scoring (avoids repeated board access)
struct CachedMoveScore {
    Move move;              // 2 bytes
    int16_t score;          // 2 bytes  
    uint8_t moveType;       // 1 byte - Cached move type to avoid repeated checks
    uint8_t padding1;       // 1 byte padding
    uint16_t padding2;      // 2 bytes padding - Align to 8 bytes total
    
    enum MoveTypeFlags : uint8_t {
        CAPTURE_FLAG = 1,
        PROMOTION_FLAG = 2,
        QUEEN_PROMOTION_FLAG = 4,
        EN_PASSANT_FLAG = 8
    };
    
    CachedMoveScore() = default;
    CachedMoveScore(Move m, int s, uint8_t type) : move(m), score(static_cast<int16_t>(s)), moveType(type), padding1(0), padding2(0) {}
    
    bool isCapture() const noexcept { return moveType & CAPTURE_FLAG; }
    bool isPromotion() const noexcept { return moveType & PROMOTION_FLAG; }
    bool isQueenPromotion() const noexcept { return moveType & QUEEN_PROMOTION_FLAG; }
    bool isEnPassant() const noexcept { return moveType & EN_PASSANT_FLAG; }
};

// Memory-optimized quiescence search implementation
class OptimizedQuiescence {
public:
    // Main optimized quiescence function
    static eval::Score quiescenceOptimized(
        Board& board,
        int ply,
        eval::Score alpha,
        eval::Score beta,
        SearchInfo& searchInfo,
        SearchData& data,
        TranspositionTable& tt
    );
    
    // Specialized version for positions in check (avoids branching)
    static eval::Score quiescenceInCheckOptimized(
        Board& board,
        int ply,
        eval::Score alpha,
        eval::Score beta,
        SearchInfo& searchInfo,
        SearchData& data,
        TranspositionTable& tt
    );
    
    // Performance comparison with standard implementation
    static void benchmarkOptimizations();
    
private:
    // Hot path optimization: inline capture generation and scoring
    static inline int generateAndScoreCaptures(
        const Board& board,
        QSearchMoveBuffer& buffer,
        std::array<CachedMoveScore, qsearch_opt::MAX_CAPTURES_OPTIMIZED>& scores
    );
    
    // Fast move ordering using cached scores
    static void fastMoveOrdering(
        std::array<CachedMoveScore, qsearch_opt::MAX_CAPTURES_OPTIMIZED>& scores,
        int count
    );
    
    // Minimal stack frame overhead search loop
    static eval::Score searchMovesOptimized(
        Board& board,
        const std::array<CachedMoveScore, qsearch_opt::MAX_CAPTURES_OPTIMIZED>& scores,
        int moveCount,
        int ply,
        eval::Score alpha,
        eval::Score beta,
        SearchInfo& searchInfo,
        SearchData& data,
        TranspositionTable& tt
    );
};

// Memory usage analysis tools
struct QSearchMemoryAnalysis {
    size_t standardStackUsage;     // Original implementation stack usage
    size_t optimizedStackUsage;    // Optimized implementation stack usage
    size_t memoryReduction;        // Bytes saved per call
    double cacheEfficiencyGain;    // Estimated cache performance improvement
    
    static QSearchMemoryAnalysis analyze();
    static void printAnalysis();
};

// Compile-time memory usage validation
static_assert(QSearchMoveBuffer::stack_usage() < 256, 
              "QSearchMoveBuffer should use less than 256 bytes stack space");

static_assert(sizeof(CachedMoveScore) == 8, 
              "CachedMoveScore should be exactly 8 bytes for cache alignment");

} // namespace seajay::search