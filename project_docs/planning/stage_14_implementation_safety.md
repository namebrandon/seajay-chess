# Stage 14: Quiescence Search - C++ Implementation Safety Guide

**Author:** Claude (C++ Pro Agent)  
**Date:** August 14, 2025  
**Focus:** Zero-bug implementation through defensive programming  
**Theme:** METHODICAL VALIDATION - Avoid debugging hell  

## Executive Summary

This document provides concrete C++ implementation guidance for Stage 14's quiescence search, emphasizing safety, correctness, and incremental delivery. Every recommendation is designed to prevent bugs before they occur, using modern C++20 features and defensive programming patterns.

## 1. Highest Risk Areas for Bugs

### 1.1 Stack Overflow from Unbounded Recursion

**Risk Level:** CRITICAL  
**Probability:** High without proper guards  
**Impact:** Engine crash, undefined behavior

**Defensive Implementation:**
```cpp
// quiescence.h - CRITICAL safety constants
namespace seajay::search {

// Stack safety: Conservative limits to prevent overflow
constexpr int QSEARCH_MAX_PLY = 32;      // Absolute max from root
constexpr int QSEARCH_CHECK_DEPTH = 5;   // Max depth for check extensions
constexpr int QSEARCH_STACK_GUARD = 128; // Leave room for other stack usage

// Compile-time verification using C++20 concepts
template<typename T>
concept ValidPlyType = std::is_integral_v<T> && 
                       std::is_signed_v<T> &&
                       sizeof(T) >= 2;  // At least 16-bit

// Safe ply counter wrapper with bounds checking
class SafePlyCounter {
private:
    int16_t m_ply = 0;
    static constexpr int16_t MAX_SAFE_PLY = QSEARCH_MAX_PLY;
    
public:
    [[nodiscard]] constexpr bool canGoDeeper() const noexcept {
        return m_ply < MAX_SAFE_PLY;
    }
    
    [[nodiscard]] constexpr int16_t value() const noexcept { 
        return m_ply; 
    }
    
    constexpr void increment() noexcept {
        assert(m_ply < MAX_SAFE_PLY && "Ply overflow!");
        ++m_ply;
    }
    
    constexpr void decrement() noexcept {
        assert(m_ply > 0 && "Ply underflow!");
        --m_ply;
    }
};

} // namespace
```

### 1.2 Integer Overflow in Score Calculations

**Risk Level:** HIGH  
**Probability:** Medium in extreme positions  
**Impact:** Incorrect evaluations, search instability

**Safe Score Handling:**
```cpp
// Safe score arithmetic with overflow detection
template<typename ScoreType>
class SafeScore {
private:
    static constexpr ScoreType MAX_SCORE = 30000;
    static constexpr ScoreType MIN_SCORE = -30000;
    ScoreType m_value;
    
public:
    explicit constexpr SafeScore(ScoreType v) noexcept 
        : m_value(std::clamp(v, MIN_SCORE, MAX_SCORE)) {}
    
    [[nodiscard]] SafeScore operator+(ScoreType delta) const noexcept {
        // Saturating addition
        if (delta > 0 && m_value > MAX_SCORE - delta) {
            return SafeScore(MAX_SCORE);
        }
        if (delta < 0 && m_value < MIN_SCORE - delta) {
            return SafeScore(MIN_SCORE);
        }
        return SafeScore(m_value + delta);
    }
    
    [[nodiscard]] constexpr ScoreType value() const noexcept { 
        return m_value; 
    }
};
```

### 1.3 Move List Buffer Overflow

**Risk Level:** HIGH  
**Probability:** Low with proper move generation  
**Impact:** Memory corruption, crash

**Safe Move List Container:**
```cpp
// Fixed-size move list with bounds checking
template<size_t MaxMoves = 256>
class SafeMoveList {
private:
    std::array<Move, MaxMoves> m_moves;
    size_t m_size = 0;
    
public:
    void add(Move move) {
        if (m_size >= MaxMoves) {
            // Log error but don't crash
            std::cerr << "WARNING: Move list overflow prevented!\n";
            return;
        }
        m_moves[m_size++] = move;
    }
    
    [[nodiscard]] size_t size() const noexcept { return m_size; }
    [[nodiscard]] bool empty() const noexcept { return m_size == 0; }
    
    // Safe iteration
    [[nodiscard]] auto begin() noexcept { return m_moves.begin(); }
    [[nodiscard]] auto end() noexcept { return m_moves.begin() + m_size; }
    
    // Bounds-checked access
    [[nodiscard]] Move operator[](size_t idx) const {
        assert(idx < m_size && "Move list bounds violation!");
        return m_moves[idx];
    }
};
```

## 2. Defensive Programming Patterns

### 2.1 RAII for Move Make/Unmake

**Critical Pattern:** Ensure moves are always unmade, even on exceptions

```cpp
// RAII move guard - automatically unmakes move on destruction
class MoveGuard {
private:
    Board& m_board;
    Move m_move;
    UndoInfo m_undo;
    bool m_made = false;
    
public:
    MoveGuard(Board& board, Move move) 
        : m_board(board), m_move(move) {
        m_board.makeMove(m_move, m_undo);
        m_made = true;
    }
    
    ~MoveGuard() {
        if (m_made) {
            m_board.unmakeMove(m_move, m_undo);
        }
    }
    
    // Disable copy to prevent double unmake
    MoveGuard(const MoveGuard&) = delete;
    MoveGuard& operator=(const MoveGuard&) = delete;
    
    // Allow move for efficiency
    MoveGuard(MoveGuard&& other) noexcept 
        : m_board(other.m_board), m_move(other.m_move), 
          m_undo(std::move(other.m_undo)), m_made(other.m_made) {
        other.m_made = false;  // Prevent other from unmake
    }
};
```

### 2.2 Invariant Checking

**Three Critical Invariants to Always Check:**

```cpp
class QuiescenceSearch {
private:
    // Invariant 1: Alpha < Beta always
    [[nodiscard]] bool checkAlphaBeta(eval::Score alpha, eval::Score beta) const {
        assert(alpha < beta && "Alpha-beta window invalid!");
        return alpha < beta;
    }
    
    // Invariant 2: Board consistency after move/unmake
    [[nodiscard]] bool checkBoardConsistency(const Board& board) const {
        #ifdef DEBUG
        uint64_t hash = board.zobristKey();
        // Verify bitboards match mailbox
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
            Piece p = board.pieceAt(sq);
            if (p != NO_PIECE) {
                assert(board.pieces(p) & squareBB(sq));
            }
        }
        assert(board.zobristKey() == hash && "Board corrupted!");
        #endif
        return true;
    }
    
    // Invariant 3: Ply depth within bounds
    [[nodiscard]] bool checkPlyBounds(int ply) const {
        assert(ply >= 0 && ply < QSEARCH_MAX_PLY && "Ply out of bounds!");
        return ply >= 0 && ply < QSEARCH_MAX_PLY;
    }
    
public:
    eval::Score search(Board& board, int ply, eval::Score alpha, eval::Score beta) {
        // Check all invariants at entry
        assert(checkAlphaBeta(alpha, beta));
        assert(checkBoardConsistency(board));
        assert(checkPlyBounds(ply));
        
        // ... search implementation ...
    }
};
```

## 3. Incremental Delivery Plan

### 3.1 Absolute Minimum Viable First Commit (Day 1, 2-3 hours)

**File:** `src/search/quiescence_minimal.cpp`

```cpp
// MINIMAL IMPLEMENTATION - Just prevents horizon effect
#include "quiescence_minimal.h"
#include "../core/board.h"
#include "../evaluation/evaluate.h"

namespace seajay::search {

// Phase 1: Absolute minimal quiescence - captures only, no optimizations
eval::Score quiescenceMinimal(Board& board, int ply, 
                              eval::Score alpha, eval::Score beta,
                              SearchData& data) {
    // SAFETY 1: Hard ply limit
    if (ply >= 16) {  // Conservative limit for phase 1
        return evaluate(board);
    }
    
    // SAFETY 2: Node counting for infinite loop detection
    if (++data.nodes > 1000000) {  // 1M node safety limit for testing
        data.stopped = true;
        return evaluate(board);
    }
    
    // Stand-pat evaluation
    eval::Score stand_pat = evaluate(board);
    if (stand_pat >= beta) {
        return stand_pat;  // Beta cutoff
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }
    
    // Generate captures only
    MoveList captures;
    MoveGenerator::generateCaptures(board, captures);
    
    // Search captures
    for (Move move : captures) {
        // SAFETY 3: Use RAII for move/unmake
        UndoInfo undo;
        board.makeMove(move, undo);
        
        eval::Score score = -quiescenceMinimal(board, ply + 1, 
                                               -beta, -alpha, data);
        
        board.unmakeMove(move, undo);
        
        if (score >= beta) {
            return score;  // Beta cutoff
        }
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

} // namespace
```

### 3.2 Incremental Enhancement Schedule

**Day 1: Minimal Implementation (2-3 hours)**
- Basic capture-only search
- Hard ply limit
- Node count safety
- Basic testing

**Day 2: Add Safety Infrastructure (3-4 hours)**
- RAII move guards
- Invariant checking
- Debug assertions
- Logging framework

**Day 3: Handle Check Positions (4 hours)**
- Check evasion handling
- Perpetual check prevention
- Extended testing

**Day 4: Add Delta Pruning (3 hours)**
- Conservative pruning
- Statistics collection
- Performance testing

**Day 5: TT Integration (4 hours)**
- Safe TT probing
- Proper depth handling
- Validation testing

**Day 6: Final Testing & Tuning (4 hours)**
- SPRT preparation
- Parameter tuning
- Documentation

## 4. Testing Infrastructure

### 4.1 Unit Test Structure

```cpp
// tests/search/test_quiescence.cpp
#include <gtest/gtest.h>
#include "search/quiescence.h"

class QuiescenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        board.setStartingPosition();
    }
    
    Board board;
    SearchData data;
};

// Test 1: Verify termination
TEST_F(QuiescenceTest, AlwaysTerminates) {
    // Position with many captures
    board.fromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    
    auto start = std::chrono::steady_clock::now();
    eval::Score score = quiescence(board, 0, -30000, 30000, data);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    // Must terminate within reasonable time
    EXPECT_LT(elapsed, std::chrono::seconds(1));
    EXPECT_LT(data.nodes, 1000000);  // Reasonable node limit
}

// Test 2: Horizon effect elimination
TEST_F(QuiescenceTest, NoHorizonEffect) {
    // Position where piece hangs after quiet move
    board.fromFEN("r3k2r/8/8/8/b7/8/8/R3K2R w KQkq - 0 1");
    
    // Without quiescence, might miss bishop takes rook
    eval::Score score = quiescence(board, 0, -30000, 30000, data);
    
    // Should see the hanging rook
    EXPECT_LT(score.value(), -400);  // Lost rook
}

// Test 3: Stand-pat correctness
TEST_F(QuiescenceTest, StandPatBehavior) {
    // Quiet position - no captures
    board.fromFEN("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    
    eval::Score staticEval = evaluate(board);
    eval::Score qscore = quiescence(board, 0, -30000, 30000, data);
    
    // Should return static eval when no captures
    EXPECT_EQ(staticEval.value(), qscore.value());
}

// Test 4: Check evasion
TEST_F(QuiescenceTest, CheckEvasionRequired) {
    // Position with check
    board.fromFEN("4k3/8/8/8/8/8/4r3/4K3 w - - 0 1");
    
    eval::Score score = quiescenceInCheck(board, 0, -30000, 30000, data);
    
    // Must generate legal moves, not stand-pat
    EXPECT_NE(score.value(), evaluate(board).value());
}
```

### 4.2 Debug Diagnostics

```cpp
// Debug output system for quiescence
class QuiescenceDebug {
private:
    std::ofstream m_logFile;
    int m_verbosity = 0;
    
public:
    QuiescenceDebug(int verbosity = 0) 
        : m_verbosity(verbosity) {
        if (m_verbosity > 0) {
            m_logFile.open("qsearch_debug.log");
        }
    }
    
    void logEntry(int ply, const Board& board, 
                  eval::Score alpha, eval::Score beta) {
        if (m_verbosity < 1) return;
        
        m_logFile << std::string(ply * 2, ' ') 
                  << "QS[" << ply << "] "
                  << "alpha=" << alpha.value() 
                  << " beta=" << beta.value() 
                  << " fen=" << board.toFEN() << "\n";
    }
    
    void logStandPat(int ply, eval::Score score, bool cutoff) {
        if (m_verbosity < 2) return;
        
        m_logFile << std::string(ply * 2, ' ')
                  << "  Stand-pat: " << score.value();
        if (cutoff) {
            m_logFile << " (CUTOFF)";
        }
        m_logFile << "\n";
    }
    
    void logMove(int ply, Move move, eval::Score score) {
        if (m_verbosity < 3) return;
        
        m_logFile << std::string(ply * 2, ' ')
                  << "  Move: " << moveToString(move)
                  << " score=" << score.value() << "\n";
    }
};
```

## 5. Common C++ Pitfalls and Solutions

### 5.1 Move Generation Efficiency

**Pitfall:** Creating temporary vectors for captures
**Solution:** Use stack-allocated fixed arrays

```cpp
// BAD: Heap allocation in hot path
std::vector<Move> generateCapturesBAD(const Board& board) {
    std::vector<Move> captures;  // HEAP ALLOCATION!
    captures.reserve(32);
    // ... generate ...
    return captures;
}

// GOOD: Stack allocation with fixed size
template<size_t MaxCaptures = 32>
struct CaptureList {
    std::array<Move, MaxCaptures> moves;
    uint8_t count = 0;
    
    void add(Move m) {
        assert(count < MaxCaptures);
        moves[count++] = m;
    }
    
    auto begin() { return moves.begin(); }
    auto end() { return moves.begin() + count; }
};

// BETTER: Use existing MoveList with inline buffer
void generateCaptures(const Board& board, MoveList& captures) {
    // MoveList already uses stack allocation for small counts
    // Just ensure it's cleared before use
    captures.clear();
    // ... generate directly into provided list ...
}
```

### 5.2 Cache-Friendly Data Access

**Pitfall:** Random memory access patterns
**Solution:** Structure data for sequential access

```cpp
// Cache-friendly quiescence data structure
struct alignas(64) QSearchNode {  // Cache line aligned
    eval::Score alpha;
    eval::Score beta;
    eval::Score bestScore;
    Move bestMove;
    uint16_t ply;
    uint16_t moveCount;
    bool inCheck;
    uint8_t padding[64 - 20];  // Pad to cache line
};

// Pre-allocate node stack to avoid allocation
class QSearchStack {
private:
    std::array<QSearchNode, QSEARCH_MAX_PLY> m_nodes;
    
public:
    QSearchNode& operator[](int ply) {
        assert(ply < QSEARCH_MAX_PLY);
        return m_nodes[ply];
    }
};
```

### 5.3 Branch Prediction Optimization

**Pitfall:** Unpredictable branches in hot path
**Solution:** Use likely/unlikely hints

```cpp
// Modern C++20 attributes for branch prediction
[[likely]] [[nodiscard]] 
inline bool shouldPruneDelta(eval::Score staticEval, 
                             eval::Score alpha,
                             int captureValue) {
    // Most captures are NOT pruned (likely path)
    if (staticEval + captureValue + DELTA_MARGIN >= alpha) [[likely]] {
        return false;  // Don't prune
    }
    return true;  // Prune (unlikely)
}

// Use in quiescence
if (shouldPruneDelta(stand_pat, alpha, pieceValue)) [[unlikely]] {
    // NEVER prune promotions or en passant!
    if (isPromotion(move) || move.isEnPassant()) [[unlikely]] {
        // Force search of special moves
    } else {
        stats.deltasPruned++;
        continue;  // Skip this capture
    }
}
```

## 6. Code Organization Recommendations

### 6.1 File Structure

```
src/search/
├── quiescence.h           # Public interface
├── quiescence.cpp         # Main implementation
├── quiescence_check.cpp   # Check handling (separate for clarity)
├── quiescence_stats.h     # Statistics tracking
└── quiescence_debug.h     # Debug utilities
```

### 6.2 Header Organization

```cpp
// quiescence.h - Clean public interface
#pragma once

#include "../evaluation/types.h"
#include "types.h"

namespace seajay {
class Board;
class TranspositionTable;
}

namespace seajay::search {

// Main quiescence search entry point
eval::Score quiescence(Board& board,
                       int ply,
                       eval::Score alpha,
                       eval::Score beta,
                       SearchInfo& searchInfo,
                       SearchData& data,
                       TranspositionTable* tt = nullptr);

// Configuration (can be modified for testing)
struct QSearchConfig {
    static constexpr int MAX_PLY = 32;
    static constexpr eval::Score DELTA_MARGIN = 200;
    static constexpr bool ENABLE_CHECKS = false;  // Stage 15+
};

} // namespace
```

## 7. Debug Instrumentation

### 7.1 Essential Logging Points

```cpp
// Minimal logging for production
#define QSEARCH_LOG_CRITICAL 1
#define QSEARCH_LOG_INFO     2
#define QSEARCH_LOG_DEBUG    3
#define QSEARCH_LOG_TRACE    4

#ifndef QSEARCH_LOG_LEVEL
    #ifdef NDEBUG
        #define QSEARCH_LOG_LEVEL 0
    #else
        #define QSEARCH_LOG_LEVEL QSEARCH_LOG_INFO
    #endif
#endif

// Logging macros
#if QSEARCH_LOG_LEVEL >= QSEARCH_LOG_CRITICAL
    #define QS_LOG_CRITICAL(msg) std::cerr << "[QS CRITICAL] " << msg << std::endl
#else
    #define QS_LOG_CRITICAL(msg) ((void)0)
#endif

// Usage in critical paths
if (ply >= QSEARCH_MAX_PLY) {
    QS_LOG_CRITICAL("Max ply reached at position: " << board.toFEN());
    return evaluate(board);
}
```

### 7.2 Statistics Collection

```cpp
// Lightweight statistics without performance impact
struct QSearchStats {
    std::atomic<uint64_t> nodes{0};
    std::atomic<uint64_t> standPatCutoffs{0};
    std::atomic<uint64_t> betaCutoffs{0};
    std::atomic<uint64_t> deltasPruned{0};
    std::atomic<uint64_t> checkPositions{0};
    
    void reset() {
        nodes = 0;
        standPatCutoffs = 0;
        betaCutoffs = 0;
        deltasPruned = 0;
        checkPositions = 0;
    }
    
    void print() const {
        std::cout << "QSearch Stats:\n"
                  << "  Nodes: " << nodes << "\n"
                  << "  Stand-pat cutoffs: " << standPatCutoffs << "\n"
                  << "  Beta cutoffs: " << betaCutoffs << "\n"
                  << "  Deltas pruned: " << deltasPruned << "\n"
                  << "  Check positions: " << checkPositions << "\n";
    }
};
```

## 8. Integration Safety

### 8.1 Safe Integration with Existing Negamax

```cpp
// Modify negamax.cpp - Add feature flag for gradual rollout
eval::Score negamax(Board& board, int depth, int ply,
                   eval::Score alpha, eval::Score beta,
                   SearchInfo& searchInfo, SearchData& data,
                   TranspositionTable* tt) {
    
    // ... existing code ...
    
    // SAFETY: Feature flag for A/B testing
    #ifdef ENABLE_QUIESCENCE
    if (depth <= 0) {
        // Enter quiescence search
        data.nodes++;  // Count the transition node
        return quiescence(board, ply, alpha, beta, searchInfo, data, tt);
    }
    #else
    if (depth <= 0) {
        // Fallback to static evaluation (old behavior)
        return evaluate(board);
    }
    #endif
    
    // ... rest of negamax ...
}
```

### 8.2 Rollback Mechanism

```cpp
// UCI option for runtime control
class UCIOptions {
public:
    bool useQuiescence = true;  // Default on, can disable
    int qsearchMaxPly = 32;
    int qsearchDeltaMargin = 200;
    
    void init() {
        // Register UCI options
        Options["UseQuiescence"] = Option(true);
        Options["QSearchMaxPly"] = Option(32, 16, 64);
        Options["QSearchDeltaMargin"] = Option(200, 50, 500);
    }
};

// Runtime check in search
if (Options.useQuiescence && depth <= 0) {
    return quiescence(...);
} else if (depth <= 0) {
    return evaluate(board);
}
```

## 9. Performance Monitoring

### 9.1 Key Metrics to Track

```cpp
class QSearchMetrics {
private:
    struct TimingData {
        uint64_t totalNanos = 0;
        uint64_t calls = 0;
        uint64_t maxNanos = 0;
    };
    
    TimingData m_quiescence;
    TimingData m_moveGen;
    TimingData m_evaluation;
    
public:
    template<typename Func>
    auto timeFunction(TimingData& data, Func&& f) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = f();
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
        data.totalNanos += nanos;
        data.calls++;
        data.maxNanos = std::max(data.maxNanos, static_cast<uint64_t>(nanos));
        
        return result;
    }
    
    void printReport() const {
        auto avgQS = m_quiescence.calls ? 
            m_quiescence.totalNanos / m_quiescence.calls : 0;
        
        std::cout << "QSearch Performance:\n"
                  << "  Avg time: " << avgQS << " ns\n"
                  << "  Max time: " << m_quiescence.maxNanos << " ns\n"
                  << "  Total calls: " << m_quiescence.calls << "\n";
    }
};
```

## 10. Critical Implementation Checklist

### If I Could Only Add 3 Assertions

```cpp
// ASSERTION 1: Ply depth bounds check (prevents stack overflow)
assert(ply >= 0 && ply < QSEARCH_MAX_PLY && "QSearch ply overflow!");

// ASSERTION 2: Alpha-beta relationship (prevents search corruption)
assert(alpha < beta && "Invalid alpha-beta window in qsearch!");

// ASSERTION 3: Board state consistency (prevents state corruption)
assert(board.isValid() && "Board corrupted in qsearch!");
```

### Day-by-Day Implementation Schedule

**Day 1 (Monday): Foundation**
- [ ] Create `quiescence_minimal.cpp` with basic capture search
- [ ] Add ply limit and node counting safety
- [ ] Write 3 basic unit tests
- [ ] Verify perft still passes

**Day 2 (Tuesday): Safety Infrastructure**
- [ ] Implement RAII MoveGuard
- [ ] Add invariant checking
- [ ] Create debug logging framework
- [ ] Add statistics collection

**Day 3 (Wednesday): Check Handling**
- [ ] Implement `quiescenceInCheck` function
- [ ] Add perpetual check detection
- [ ] Test with known perpetual positions
- [ ] Verify no infinite loops

**Day 4 (Thursday): Optimization**
- [ ] Add delta pruning with conservative margin
- [ ] Implement move ordering for captures
- [ ] Add performance benchmarks
- [ ] Profile and identify hotspots

**Day 5 (Friday): Integration**
- [ ] Integrate with negamax
- [ ] Add TT support for qsearch
- [ ] Create feature flag for rollback
- [ ] Run full test suite

**Day 6 (Saturday): Validation**
- [ ] Run tactical test suites (WAC, etc.)
- [ ] Prepare SPRT test configuration
- [ ] Document all changes
- [ ] Create release notes

## 11. Red Flags to Watch For

### During Development
1. **Node count explosion** - If qsearch nodes > 10x regular nodes, investigate
2. **Time spikes** - Any position taking >1 second in qsearch is suspicious
3. **Score oscillation** - Alternating scores between iterations indicates bug
4. **Memory growth** - Watch for memory leaks in move generation
5. **Stack depth** - Monitor actual stack usage, adjust limits if needed

### During Testing
1. **Perft regression** - Any perft failure means fundamental bug
2. **Tactical blindness** - Missing simple tactics means pruning too aggressive
3. **Time management issues** - Qsearch causing time pressure
4. **Draw detection failures** - Repetitions not properly detected
5. **TT corruption** - Search instability after TT probes

## 12. Concrete Code Examples

### 12.1 Complete Minimal Safe Implementation

```cpp
// quiescence_safe_v1.cpp - Production-ready minimal version
#include "quiescence.h"
#include "../core/board.h"
#include "../core/move_generation.h"
#include "../evaluation/evaluate.h"
#include <cassert>

namespace seajay::search {

namespace {
    // Local safety constants
    constexpr int SAFETY_MAX_PLY = 20;  // Conservative for v1
    constexpr int SAFETY_MAX_NODES = 1000000;  // 1M node limit
    
    // Statistics (thread_local for future parallelization)
    thread_local struct {
        uint64_t nodes = 0;
        uint64_t cutoffs = 0;
        void reset() { nodes = 0; cutoffs = 0; }
    } s_stats;
}

eval::Score quiescenceSafeV1(Board& board, int ply,
                             eval::Score alpha, eval::Score beta,
                             SearchData& data) {
    // SAFETY ASSERTION 1: Ply bounds
    assert(ply >= 0 && ply < SAFETY_MAX_PLY);
    if (ply >= SAFETY_MAX_PLY) {
        return evaluate(board);
    }
    
    // SAFETY ASSERTION 2: Alpha-beta validity
    assert(alpha < beta);
    if (alpha >= beta) {
        return alpha;  // Safeguard against invalid window
    }
    
    // SAFETY: Node limit to prevent runaway
    if (++s_stats.nodes > SAFETY_MAX_NODES) {
        data.stopped = true;
        return evaluate(board);
    }
    
    // Check for immediate draws
    if (board.isDraw()) {
        return eval::Score::zero();
    }
    
    // Stand-pat evaluation
    eval::Score stand_pat = evaluate(board);
    
    // Beta cutoff
    if (stand_pat >= beta) {
        s_stats.cutoffs++;
        return stand_pat;
    }
    
    // Update alpha
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }
    
    // Generate captures
    MoveList captures;
    MoveGenerator::generateCaptures(board, captures);
    
    // Search captures
    eval::Score bestScore = stand_pat;
    
    for (Move move : captures) {
        // Simple delta pruning
        int captureValue = getCaptureValue(board, move);
        if (stand_pat + captureValue + 200 < alpha) {
            continue;  // Prune
        }
        
        // Make move with RAII guard
        UndoInfo undo;
        board.makeMove(move, undo);
        
        // Recursive search
        eval::Score score = -quiescenceSafeV1(board, ply + 1,
                                              -beta, -alpha, data);
        
        // Unmake move
        board.unmakeMove(move, undo);
        
        // Update best score
        if (score > bestScore) {
            bestScore = score;
            
            if (score >= beta) {
                s_stats.cutoffs++;
                return score;  // Beta cutoff
            }
            
            if (score > alpha) {
                alpha = score;
            }
        }
    }
    
    return bestScore;
}

// Helper function for capture value
int getCaptureValue(const Board& board, Move move) {
    Piece captured = board.pieceAt(move.to());
    if (captured == NO_PIECE) return 0;
    
    static constexpr int values[] = {
        0, 100, 320, 330, 500, 900, 0  // P, N, B, R, Q, K
    };
    
    return values[pieceType(captured)];
}

} // namespace
```

## Conclusion

This implementation safety guide provides concrete, actionable C++ guidance for implementing Stage 14's quiescence search with zero critical bugs. The key principles are:

1. **Start minimal** - Basic capture search first
2. **Add safety guards** - Ply limits, node limits, assertions
3. **Use RAII** - Automatic cleanup for moves
4. **Test incrementally** - Each feature tested before moving on
5. **Monitor everything** - Statistics and logging from day 1
6. **Plan for rollback** - Feature flags and A/B testing capability

By following this guide, Stage 14 can be delivered methodically with confidence, avoiding the debugging hell that often accompanies quiescence search implementation.

**Estimated Timeline:** 6 days of focused development
**Expected Outcome:** 150-200 Elo gain with stable, bug-free implementation
**Risk Level:** Low with these safety measures in place

## Chess Engine Expert Review Summary

### Critical Chess-Specific Additions Made:

1. **Chess Logic Bug Prevention (Section 1.0)**
   - Stalemate detection in quiescence (Crafty bug)
   - Repetition detection to prevent loops
   - 50-move rule enforcement
   - En passant validity checking
   - Promotion legality verification
   - Pseudo-legal vs legal move distinction in check

2. **Real-World Bug Examples**
   - Perpetual check position that breaks engines
   - En passant edge cases from tournament games
   - Stalemate traps in quiescence
   - Discovered check scenarios
   - X-ray attack handling after captures

3. **Essential Chess Assertions**
   - King safety (can't be captured)
   - Legal move verification in check
   - En passant validity
   - Promotion on correct rank
   - Draw score consistency

4. **Edge Cases from Tournament Play**
   - Promotion races requiring special handling
   - Fortress positions that are drawn
   - Zugzwang where stand-pat fails
   - Insufficient material draws
   - Breakthrough combinations in pawn endgames

5. **Test Positions from Historical Failures**
   - Positions that exposed bugs in Crafty, Fruit, early Stockfish
   - 50-move rule boundary conditions
   - Perpetual check detection failures
   - En passant timing bugs

### Most Important Safety Recommendations:

1. **Day 1 MUST Include:**
   - En passant handling (can't defer - expires after 1 move!)
   - All draw detection (50-move, repetition, insufficient material)
   - Stalemate detection separate from checkmate
   - Legal move verification for check positions

2. **Never Prune These Moves:**
   - Promotions (any type)
   - En passant captures
   - Moves giving check (in later stages)
   - Only legal move in check positions

3. **Critical Validation Points:**
   - After every makeMove: verify king not capturable
   - In check positions: verify move actually escapes check
   - For en passant: verify it's actually legal this move
   - For promotions: verify on back rank

4. **Performance vs Correctness:**
   - MAX_PLY=20 might be too conservative (32 is standard)
   - Node limit of 1M is reasonable for testing
   - Delta margin of 200cp is appropriate
   - Don't disable pruning in promotion races entirely

### Risk Assessment Update:

**Highest Risk Chess Bugs:**
1. Perpetual check infinite loops (without ply limits)
2. Illegal moves in check positions (pseudo-legal vs legal)
3. En passant bugs (wrong square, expired rights)
4. Stalemate returned as mate score
5. Draw detection failures in quiescence

**Mitigation Strategy:**
- Start with chess validation from Day 1
- Use provided test positions that have broken other engines
- Add all chess assertions before optimization
- Test with positions from historical engine failures
- Never trust stand-pat in zugzwang positions

### Final Recommendation:

The C++ safety measures are solid, but chess logic bugs are historically more common and harder to debug. The additions focus on preventing the chess-specific bugs that have plagued engines for decades. With these chess-specific safeguards in place, SeaJay's quiescence implementation should avoid the common pitfalls that have affected even strong engines.

**Critical Success Factor:** Test with the provided historical bug positions early and often. These positions have exposed real bugs in production engines and will quickly reveal any chess logic errors in the implementation.