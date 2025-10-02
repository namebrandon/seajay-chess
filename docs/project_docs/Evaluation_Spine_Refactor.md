# Evaluation Spine Refactor: Shared Bitboard Context Architecture

**Author:** SeaJay Development Team
**Date:** 2025-10-01
**Status:** Planning
**Related Documents:**
- `Shared_Bitboard_Evaluation_Refactor.md` (motivation)
- `isSquareAttacked_issues.md` (profiling data)
- `feature_guidelines.md` (implementation protocol)
- ⚠️ **`Evaluation_Spine_Refactor_CPP_Review.md` (CRITICAL - READ FIRST)**

---

## 🚨 CRITICAL: C++ Expert Review

**BEFORE implementing any code in this plan, read the C++ Expert Review document:**
**`docs/project_docs/Evaluation_Spine_Refactor_CPP_Review.md`**

The review identifies:
- **3 Critical Bugs** (queen attacks, black pawn masks, double-attack algorithm)
- **SIMD Optimizations** that boost NPS gain from 15-25% to **25-35%**
- **Single-file organization** (don't create new files - keep in evaluate.cpp)
- **Memory layout improvements** (192 bytes vs 320 bytes planned)
- **Elite engine patterns** from Stockfish, Ethereal, Weiss, Leela

**Cross-references to CPP Review are marked throughout this document with:**
- ⚠️ **CRITICAL BUG:** - Must fix before implementation
- 💡 **OPTIMIZATION:** - Performance improvement available
- 📖 **SEE CPP REVIEW:** - Detailed explanation available

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Git Branch Strategy](#git-branch-strategy)
3. [Motivation & Problem Statement](#motivation--problem-statement)
4. [Architecture Overview](#architecture-overview)
5. [Implementation Phases](#implementation-phases)
6. [Phase Details](#phase-details)
7. [Testing Protocol](#testing-protocol)
8. [Common Pitfalls & How to Avoid](#common-pitfalls--how-to-avoid)
9. [LazySMP Compatibility](#lazysmp-compatibility)
10. [Success Metrics](#success-metrics)
11. [Appendix: Code Examples](#appendix-code-examples)

---

## Executive Summary

### The Problem
SeaJay's current evaluator (`evaluate.cpp`, 1669 lines) recomputes attack bitboards **19 times per evaluation**, driving `isSquareAttacked` to consume 10-12% of total CPU time. In tactical positions, this amplifies to 7.77 calls/node (2.2× normal rate). This redundancy prevents the engine from scaling at depth and creates a maintenance burden.

### The Solution
Implement an **evaluation spine** architecture where all attack bitboards are computed **once** at the start of evaluation, stored in a lightweight `EvalContext` struct, and consumed by all evaluation terms. This eliminates redundant computation, reduces evaluator from ~1669 to ~1200 lines, and improves NPS by 15-25%.

### Key Benefits
- **Performance:** 15-25% NPS improvement (eliminates redundant attack queries)
- **Maintainability:** Clear separation between attack computation and evaluation logic
- **Extensibility:** New evaluation terms read from context, no board queries needed
- **LazySMP Ready:** Stack-allocated context, no shared state, thread-safe by design

### Implementation Timeline
- **Phase 0 (Prerequisite):** isSquareAttacked quick wins (2-3 days)
- **Phases A-B (Infrastructure):** Build EvalContext spine (5-7 days, 0 ELO)
- **Phases C-D (Migration):** Refactor evaluation terms (5-7 days, 0-10 nELO from NPS)
- **Phase E (Validation):** Testing and documentation (2-3 days)
- **Total:** 3-4 weeks

### Gating Mechanism
All new evaluation logic was originally gated behind UCI option `EvalUseSpine` (default: false) to allow parallel development and A/B testing. After Phase E2 integration this option was removed—spine evaluation is now always active.

---

## Git Branch Strategy

### Integration Branch: `integration/evaluation-spine`

**⚠️ CRITICAL: All evaluation spine work happens on feature branches that fork from `integration/evaluation-spine`. No direct commits are allowed on the integration branch. NEVER merge directly to `main` without explicit human approval.**

### Branch Hierarchy

```
main (production)
  └── integration/evaluation-spine (base for all spine work)
       ├── feature/spine-phase-0-isSquareAttacked-opts
       ├── feature/spine-phase-a1-context-struct
       ├── feature/spine-phase-a2-uci-option
       ├── feature/spine-phase-b1-basic-position
       ├── feature/spine-phase-b2-attack-bitboards
       ├── feature/spine-phase-b3-aggregated-data
       ├── feature/spine-phase-c1-mobility
       ├── feature/spine-phase-c2-passed-pawns
       ├── feature/spine-phase-d1-king-safety
       ├── feature/spine-phase-d2-remaining-terms
       ├── feature/spine-phase-e1-threat-eval
       └── feature/spine-phase-e2-validation
```

### Workflow Per Phase

Each implementation phase follows this workflow:

1. **Create Feature Branch**
   ```bash
   git checkout integration/evaluation-spine
   git pull origin integration/evaluation-spine  # Ensure up to date
   git checkout -b feature/spine-phase-X-description
   ```
   > 🚫 Do **not** commit directly to `integration/evaluation-spine`. Every code change must live on a feature branch until it is reviewed, validated, and merged back.

2. **Implement Phase**
   - Follow phase implementation details from this document
   - Each phase may have 2+ commits (as outlined in commit strategy)
   - Every commit MUST include `bench <node-count>` in message

3. **Local Testing**
   ```bash
   # Verify bench nodes are identical (until Phase E1)
   ./bin/seajay bench

   # Run local SPRT validation if possible
   # Or prepare for OpenBench submission
   ```

4. **Push and SPRT Validate**
   ```bash
   git push origin feature/spine-phase-X-description

   # Submit to OpenBench for SPRT validation
   # - Base: integration/evaluation-spine
   # - Dev: feature/spine-phase-X-description
   # - Bounds: [-5.0, 3.0] (detect regressions)
   # - Expected: LLR reaches upper bound (no regression)
   ```

5. **Human Review and Approval**
   - **REQUIRED:** Human must review SPRT results
   - **REQUIRED:** Human must review code changes
   - **REQUIRED:** Human must approve merge
   - Do NOT merge automatically, even if SPRT passes

6. **Merge to Integration Branch**
   ```bash
   # After human approval
   git checkout integration/evaluation-spine
   git merge --no-ff feature/spine-phase-X-description
   git push origin integration/evaluation-spine
   ```

7. **Clean Up**
   ```bash
   git branch -d feature/spine-phase-X-description
   git push origin --delete feature/spine-phase-X-description
   ```

### Merge Policy

**⚠️ CRITICAL MERGE RULES:**

1. **Feature → Integration:** Requires human approval after SPRT validation
2. **Integration → Main:** Requires explicit human approval (only after ALL phases complete and validated)
3. **NEVER auto-merge to main** - Production branch must be protected
4. **NEVER skip SPRT validation** - Every phase must be tested
5. **NEVER merge failing SPRT** - Investigate and fix regressions first

### Why This Strategy?

- **Safety:** Integration branch acts as staging area, protects `main`
- **Testability:** Each phase can be SPRT tested independently
- **Rollback:** Easy to revert individual phases without affecting others
- **Traceability:** Clear git history showing progression through phases
- **Collaboration:** Multiple developers can work on different phases simultaneously

### Integration Branch Baseline

- **Branch:** `integration/evaluation-spine`
- **Created from:** `main` at commit `98553ea` (SeaJay version 20250930)
- **Baseline bench:** `2428308 nodes`
- **Purpose:** Aggregate all evaluation spine refactor work before merging to `main`

### Related Investigation

**WAC.049 Tactical Investigation:** This refactor addresses performance issues that impact tactical position evaluation. See `docs/issues/WAC.049_Tactical_Investigation.md` for context on why evaluation speed matters in tactical positions (2.2× attack query amplification observed).

---

## Motivation & Problem Statement

### Current State Analysis

#### Profiling Data (from `isSquareAttacked_issues.md`)
- **10-12% of CPU time** spent in `isSquareAttacked`
- **137M calls** at depth 12 (3.5 calls/node average)
- **Tactical positions:** 7.77 calls/node (2.2× amplification)
- **Evaluation contribution:** 45M+ calls (33% of all attack queries)

#### Code Structure Issues
1. **Redundant Computation:**
   ```cpp
   // evaluate.cpp has 19 calls to MoveGenerator::get*Attacks
   // Example redundancy:
   Line 1138: MoveGenerator::getKnightAttacks(sq)  // Mobility
   Line 1242: MoveGenerator::getKnightAttacks(sq)  // Outposts
   Line 495:  pathAttackCache.isAttacked(...)      // Passed pawns
   // ... 16 more redundant calls
   ```

2. **Fragmented Attack Caching:**
   - `PromotionPathAttackCache` (lines 26-109): Local cache for passed pawn evaluation
   - Attack queries scattered across 1669 lines
   - No unified attack context

3. **Maintenance Burden:**
   - Adding new evaluation terms requires understanding attack query patterns
   - Risk of introducing bugs through duplicated logic
   - Telemetry requires extra loops, degrading SPRT performance

### Why This Matters

#### Depth Scaling Problem
As search goes deeper, evaluation is called exponentially more often. At depth 20+:
- Evaluation becomes bottleneck (more calls than move generation)
- Redundant attack computation amplifies with depth
- Tactical positions (where evaluation matters most) hit worst-case performance

#### Tactical Position Amplification
```
Normal position:    3.5 attack queries/node
Tactical position:  7.77 attack queries/node  (2.2× worse!)
```
When kings are exposed and pieces are coordinated, evaluation queries explode. This is **exactly when we need fast evaluation** to find tactics.

### The Insight: Compute Once, Use Many Times

Every evaluation call on the same position recomputes identical attack bitboards:
```
Mobility:        Compute all piece attacks  ← Redundant
Passed pawns:    Compute all piece attacks  ← Redundant
King safety:     Compute all piece attacks  ← Redundant
Outposts:        Compute all piece attacks  ← Redundant
```

**Solution:** Compute once, store in context, read from context.

---

## Architecture Overview

### The Evaluation Spine Concept

An "evaluation spine" is a centralized data structure that holds all precomputed positional data needed by evaluation terms. Think of it as a "preprocessed view" of the position optimized for evaluation queries.

#### Core Principle
```
OLD: Board → [Mobility] → get_attacks() → Bitboard
              ↓
            [King Safety] → get_attacks() → Bitboard  ← REDUNDANT
              ↓
            [Outposts] → get_attacks() → Bitboard     ← REDUNDANT

NEW: Board → [Build Context] → EvalContext
              ↓
            [Mobility] → read context → Bitboard
              ↓
            [King Safety] → read context → Bitboard   ← FREE
              ↓
            [Outposts] → read context → Bitboard      ← FREE
```

### EvalContext Structure

```cpp
struct EvalContext {
    // ============================================================================
    // BASIC POSITION INFO
    // ============================================================================
    Bitboard occupied;              // All pieces (cached from board)
    Bitboard occupiedByColor[2];    // Pieces by color
    Square kingSquare[2];           // King locations (frequently accessed)

    // ============================================================================
    // ATTACK BITBOARDS (computed once, used many times)
    // ============================================================================
    // Per-piece-type attacks (union of all pieces of that type)
    Bitboard pawnAttacks[2];        // Squares attacked by pawns
    Bitboard knightAttacks[2];      // Squares attacked by knights
    Bitboard bishopAttacks[2];      // Squares attacked by bishops
    Bitboard rookAttacks[2];        // Squares attacked by rooks
    Bitboard queenAttacks[2];       // Squares attacked by queens
    Bitboard kingAttacks[2];        // Squares attacked by king

    // ============================================================================
    // AGGREGATED ATTACK DATA
    // ============================================================================
    Bitboard attackedBy[2];         // Union of all attacks by color
    Bitboard doubleAttacks[2];      // Squares attacked 2+ times (for threats)

    // ============================================================================
    // MOBILITY DATA
    // ============================================================================
    Bitboard mobilityArea[2];       // Safe squares for mobility calculation
                                    // = ~ownPieces & ~enemyPawnAttacks

    // ============================================================================
    // KING SAFETY DATA
    // ============================================================================
    Bitboard kingRing[2];           // 8 squares around king
    Bitboard kingRingAttacks[2];    // Enemy attacks on king ring (precomputed!)

    // ============================================================================
    // PAWN STRUCTURE DATA
    // ============================================================================
    Bitboard pawnAttackSpan[2];     // All squares pawns could ever attack
                                    // (used for outpost detection)
};
```

#### Memory Analysis
- **Size:** ~40 Bitboards × 8 bytes = ~320 bytes
- **Allocation:** Stack (local variable in `evaluateImpl`)
- **Thread Safety:** Each thread gets own copy (no shared state)
- **L1 Cache Friendly:** Entire context fits in L1 cache (~32KB typical)

### Evaluation Flow with Spine

```cpp
Score evaluateSpine(const Board& board) {
    // STEP 1: Build context (compute all attacks once)
    EvalContext ctx;
    populateContext(ctx, board);  // ~5-10% of evaluation time

    // STEP 2: Evaluate terms (read from context, no recomputation)
    Score material = evaluateMaterial(board);              // No context needed
    Score pst = evaluatePST(board);                        // No context needed
    Score pawns = evaluatePawns(board, ctx);               // Uses ctx
    Score pieces = evaluatePieces(board, ctx);             // Uses ctx
    Score mobility = evaluateMobility(board, ctx);         // Uses ctx ← BIG WIN
    Score kingSafety = evaluateKingSafety(board, ctx);     // Uses ctx ← BIG WIN
    Score threats = evaluateThreats(board, ctx);           // Uses ctx (new term!)

    // STEP 3: Combine and return
    return material + pst + pawns + pieces + mobility + kingSafety + threats;
}
```

### Key Architectural Decisions

#### 1. Stack Allocation (Not Heap)
**Why:** Thread-local, cache-friendly, no allocation overhead
**Implication:** Context must be lightweight (<1KB)

#### 2. Computed Per Evaluation (Not Incremental)
**Why:** Simpler implementation, no make/unmake complexity
**Future:** Can evolve to incremental tracking later (Phase 5 in isSquareAttacked doc)
**Trade-off:** Pay construction cost once per eval vs maintaining state

#### 3. Read-Only After Population
**Why:** Evaluation terms can't corrupt context
**Implication:** Pass `const EvalContext&` to all eval functions

#### 4. UCI-Gated (Parallel Development)
**Why:** Build new evaluator alongside old, A/B test before switching
**Mechanism:** Historical `EvalUseSpine` option (default: false). This toggle was removed after Phase E2; the spine evaluator is now always active.

---

## Implementation Phases

### Overview of Phases

| Phase | Description | Expected Impact | Bench Behavior | Time |
|-------|-------------|-----------------|----------------|------|
| **Phase 0** | isSquareAttacked quick wins | 1-2% NPS | Identical nodes | 2-3 days |
| **Phase A1** | EvalContext struct definition | 0% | Identical nodes | 1 day |
| **Phase A2** | (Historical) UCI option + evaluateSpine stub | 0% | Identical nodes | 1 day |
| **Phase B1** | Populate basic position info | 0% | Identical nodes | 1 day |
| **Phase B2** | Populate attack bitboards | 0% | Identical nodes | 2 days |
| **Phase B3** | Populate aggregated data | 0% | Identical nodes | 1 day |
| **Phase C1** | Refactor mobility to use context | 5-10% NPS | Identical nodes | 2 days |
| **Phase C2** | Refactor passed pawns to use context | 2-5% NPS | Identical nodes | 2 days |
| **Phase D1** | Refactor king safety to use context | 2-5% NPS | Identical nodes | 1-2 days |
| **Phase D2** | Refactor remaining terms | 1-3% NPS | Identical nodes | 1-2 days |
| **Phase E1** | Add threat evaluation (new term!) | 0-5 nELO | Different nodes OK | 1-2 days |
| **Phase E2** | Validation and documentation | 0% | N/A | 2 days |

**Total Timeline:** 3-4 weeks
**Cumulative NPS Gain:** 15-25%
**ELO Impact:** 0-10 nELO from NPS improvements

### Commit Strategy for SPRT Testing

Each major phase splits into **2 commits** to allow incremental SPRT validation:

```
Phase B (Attack Aggregation):
  Commit B1: "feat: populate basic position info in EvalContext - bench 19191913"
  Commit B2: "feat: populate attack bitboards in EvalContext - bench 19191913"
  Commit B3: "feat: populate aggregated attack data in EvalContext - bench 19191913"
  → Test after each commit, verify bench unchanged

Phase C (Migration):
  Commit C1a: "refactor: migrate knight/bishop mobility to EvalContext - bench 19191913"
  Commit C1b: "refactor: migrate rook/queen mobility to EvalContext - bench 19191913"
  → Test after each commit, verify bench unchanged

  Commit C2a: "refactor: migrate passed pawn path checks to EvalContext - bench 19191913"
  Commit C2b: "refactor: remove PromotionPathAttackCache - bench 19191913"
  → Test after each commit, verify bench unchanged
```

**Critical:** Bench node count must remain **exactly identical** until Phase E1 (when we add new terms).

---

## Phase Details

### Phase 0: isSquareAttacked Quick Wins (Prerequisite)

**Goal:** Grab low-hanging optimizations in `isSquareAttacked` before starting refactor.

#### Why Do This First?
- 1-2% immediate NPS gain while planning refactor
- Validates profiling methodology
- Improves baseline for measuring refactor impact
- Benefits all callers (move generation, SEE, etc.), not just evaluation

#### Phase 0.1: Magic Bitboard Recomputation Fix

**Problem:** Current `isSquareAttacked` (lines 645-687) computes magic bitboards 4 times when queens exist:
```cpp
// Current code (INEFFICIENT):
Bitboard queenAttacks = magicBishopAttacks(sq, occ) |   // Compute #1
                        magicRookAttacks(sq, occ);       // Compute #2
if (queens & queenAttacks) return true;

Bitboard bishopAttacks = magicBishopAttacks(sq, occ);   // Compute #3 ← REDUNDANT!
if (bishops & bishopAttacks) return true;

Bitboard rookAttacks = magicRookAttacks(sq, occ);       // Compute #4 ← REDUNDANT!
if (rooks & rookAttacks) return true;
```

**Solution:** Compute diagonal and straight attacks once, check bishops/rooks/queens against same bitboards.

**File:** `src/core/move_generation.cpp` (lines 645-687)

**Implementation:**
```cpp
// Get all sliding pieces
Bitboard bishops = board.pieces(attackingColor, BISHOP);
Bitboard rooks = board.pieces(attackingColor, ROOK);
Bitboard queens = board.pieces(attackingColor, QUEEN);

// Early exit if no sliding pieces
if (!(bishops | rooks | queens)) {
    goto check_king;  // Skip expensive magic lookups
}

// Compute diagonal attacks ONCE (for bishops AND queens)
if (bishops | queens) {
    Bitboard diagonalAttacks = magicBishopAttacks(square, occupied);

    if (bishops & diagonalAttacks) { /* cache and return */ }
    if (queens & diagonalAttacks) { /* cache and return */ }
}

// Compute straight attacks ONCE (for rooks AND queens)
if (rooks | queens) {
    Bitboard straightAttacks = magicRookAttacks(square, occupied);

    if (rooks & straightAttacks) { /* cache and return */ }
    if (queens & straightAttacks) { /* cache and return */ }
}
```

**Testing:**
```bash
# Before changes
echo "bench" | ./seajay > baseline_phase0.txt
# Record: nodes, NPS, time

# After changes
echo "bench" | ./seajay > phase0.txt
# Verify: nodes IDENTICAL, NPS +1-2%, time -1-2%

# Commit message:
git commit -m "opt: eliminate redundant magic lookups in isSquareAttacked

Compute diagonal attacks once for bishops+queens, straight attacks once for
rooks+queens. Reduces magic bitboard calls from 4 to 2 when queens present.

Expected: 1-2% NPS improvement, 0 ELO (no search behavior change)

bench 19191913"
```

**Expected Impact:** 1-2% NPS, 0 ELO

---

#### Phase 0.2: Attack Cache Statistics

**Problem:** We don't know the cache hit rate! Can't optimize what we don't measure.

**Solution:** Add hit/miss/eviction counters to `AttackCache`, report via UCI.

**File:** `src/core/attack_cache.h`

**Implementation:**
```cpp
class AttackCache {
public:
    struct Stats {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;

        double hitRate() const {
            uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / total : 0.0;
        }
    };

    Stats getStats() const { return {m_hits, m_misses, m_evictions}; }
    void resetStats() { m_hits = 0; m_misses = 0; m_evictions = 0; }

private:
    mutable uint64_t m_hits = 0;
    mutable uint64_t m_misses = 0;
    mutable uint64_t m_evictions = 0;
};
```

**UCI Reporting:** After search completes, print:
```
info string AttackCache hits=89234567 misses=12345678 hitrate=87.86%
```

**Testing:**
```bash
echo "bench" | ./seajay 2>&1 | grep "AttackCache"
# Record baseline hit rate (expect 60-70%)

# After refactor (Phase E), compare:
# - Hit rate should improve (fewer unique queries)
# - Absolute hits should decrease (fewer calls from evaluation)
```

**Expected Impact:** 0% (measurement only)

---

### Phase A: EvalContext Infrastructure (0 ELO Expected)

**Goal:** Create the evaluation spine infrastructure without changing evaluation behavior.

#### Phase A1: Define EvalContext Struct

⚠️ **CRITICAL:** Do NOT create new files! Keep everything in evaluate.cpp
📖 **SEE CPP REVIEW:** Section 1 for single-file organization pattern

**What:** Define EvalContext struct within evaluate.cpp using namespace organization.

**File:** `src/evaluation/evaluate.cpp` (existing file, add to namespace)

**Implementation:**

💡 **OPTIMIZATION:** See CPP Review Section 2 for cache-aligned 192-byte layout
📖 **SEE CPP REVIEW:** Section 4 for improved structure design

```cpp
// Add to evaluate.cpp within namespace seajay::eval::detail

namespace seajay::eval {
namespace detail {  // Private implementation details

/**
 * EvalContext: Centralized attack and position data for evaluation.
 *
 * Computed once at the start of evaluation, consumed by all evaluation terms.
 * Stack-allocated, thread-local, LazySMP-safe.
 *
 * Memory: 192 bytes (3 cache lines) - optimized layout per CPP Review
 */
struct alignas(64) EvalContext {  // Cache line alignment
    // ========================================================================
    // BASIC POSITION INFO (cached from board for fast access)
    // ========================================================================
    Bitboard occupied;              // All pieces
    Bitboard occupiedByColor[2];    // Pieces by color
    Square kingSquare[2];           // King locations

    // ========================================================================
    // ATTACK BITBOARDS (union of all pieces of each type)
    // ========================================================================
    Bitboard pawnAttacks[2];        // Squares attacked by pawns
    Bitboard knightAttacks[2];      // Squares attacked by knights
    Bitboard bishopAttacks[2];      // Squares attacked by bishops (excluding queens)
    Bitboard rookAttacks[2];        // Squares attacked by rooks (excluding queens)
    Bitboard queenAttacks[2];       // Squares attacked by queens
    Bitboard kingAttacks[2];        // Squares attacked by king

    // ========================================================================
    // AGGREGATED ATTACK DATA
    // ========================================================================
    Bitboard attackedBy[2];         // Union of all attacks (all piece types)
    Bitboard doubleAttacks[2];      // Squares attacked 2+ times (for threat eval)

    // ========================================================================
    // MOBILITY DATA
    // ========================================================================
    Bitboard mobilityArea[2];       // ~ownPieces & ~enemyPawnAttacks

    // ========================================================================
    // KING SAFETY DATA
    // ========================================================================
    Bitboard kingRing[2];           // 8 squares around king
    Bitboard kingRingAttacks[2];    // Enemy attacks on king ring (precomputed)

    // ========================================================================
    // PAWN STRUCTURE DATA
    // ========================================================================
    Bitboard pawnAttackSpan[2];     // All squares pawns could ever attack

    // ========================================================================
    // CONSTRUCTION
    // ========================================================================
    EvalContext() noexcept {
        // Zero-initialize all bitboards
        occupied = 0ULL;
        for (int i = 0; i < 2; ++i) {
            occupiedByColor[i] = 0ULL;
            kingSquare[i] = NO_SQUARE;
            pawnAttacks[i] = 0ULL;
            knightAttacks[i] = 0ULL;
            bishopAttacks[i] = 0ULL;
            rookAttacks[i] = 0ULL;
            queenAttacks[i] = 0ULL;
            kingAttacks[i] = 0ULL;
            attackedBy[i] = 0ULL;
            doubleAttacks[i] = 0ULL;
            mobilityArea[i] = 0ULL;
            kingRing[i] = 0ULL;
            kingRingAttacks[i] = 0ULL;
            pawnAttackSpan[i] = 0ULL;
        }
    }
};

/**
 * Populate EvalContext with attack and position data.
 * Call once at start of evaluation.
 */
void populateContext(EvalContext& ctx, const Board& board);

} // namespace seajay::eval
```

**Testing:**
```bash
# Verify compilation
./build.sh Release

# No behavior change (context not used yet)
echo "bench" | ./seajay
# Verify: nodes IDENTICAL to baseline
```

**Commit:**
```
git add src/evaluation/eval_context.h
git commit -m "feat: add EvalContext struct for evaluation spine (Phase A1)

Define centralized attack and position data structure for evaluation.
Stack-allocated, thread-local, ~320 bytes.

No behavior change (struct not populated or used yet).

bench 19191913"
```

---

#### Phase A2: UCI Option and evaluateSpine Stub

**What:** Add `EvalUseSpine` UCI option and create stub function.

**Files:**
- `src/core/engine_config.h` (add option)
- `src/core/engine_config.cpp` (register option)
- `src/evaluation/evaluate.cpp` (add evaluateSpine stub)

**Implementation (engine_config.h):**
```cpp
struct EngineConfig {
    // ... existing options ...

    // Evaluation Spine Refactor (gated feature)
    bool evalUseSpine = false;  // Default: use legacy evaluator
};
```

**Implementation (evaluate.cpp):**
```cpp
#include "eval_context.h"

namespace seajay::eval {

// Forward declaration of legacy evaluator
template<bool Traced>
Score evaluateImpl(const Board& board, EvalTrace* trace);

// NEW: Spine-based evaluator (gated by UCI option)
template<bool Traced>
Score evaluateSpine(const Board& board, EvalTrace* trace) {
    // Phase A2: Stub implementation (call legacy for now)
    // TODO: Implement spine evaluation in Phase B+
    return evaluateImpl<Traced>(board, trace);
}

// Public interface (routes to spine or legacy based on UCI option)
Score evaluate(const Board& board) {
    if (seajay::getConfig().evalUseSpine) {
        return evaluateSpine<false>(board, nullptr);
    } else {
        return evaluateImpl<false>(board, nullptr);
    }
}

Score evaluateWithTrace(const Board& board, EvalTrace& trace) {
    if (seajay::getConfig().evalUseSpine) {
        return evaluateSpine<true>(board, &trace);
    } else {
        return evaluateImpl<true>(board, &trace);
    }
}

} // namespace seajay::eval
```

**Testing:**
```bash
# Test with spine disabled (default)
echo "bench" | ./seajay
# Verify: nodes IDENTICAL to baseline

# Test with spine enabled (should be identical, it's a stub)
# Legacy: EvalUseSpine option removed; spine always active
echo "bench" | ./seajay
# Verify: nodes IDENTICAL to baseline

# Test UCI option registration
# Legacy command (option removed post-integration)
echo "uci" | ./seajay | grep "EvalUseSpine"
# Verify: "option name EvalUseSpine type check default false" (historical output)
```

**Commit:**
```
git commit -m "feat: add EvalUseSpine UCI option and evaluateSpine stub (Phase A2)

Gate spine evaluation behind UCI option for parallel development.
Default: false (use legacy evaluator).

Stub implementation calls legacy evaluator (no behavior change).

bench 19191913"
```

---

### Phase B: Attack Aggregation (0 ELO Expected)

**Goal:** Populate EvalContext with all attack bitboards, but don't use them yet.

#### Key Concepts for Implementers

##### What is an "Attack Bitboard"?
An attack bitboard is a 64-bit integer where each bit represents a square on the chess board. If bit N is set, the square at index N is attacked by the piece type.

Example:
```cpp
Bitboard knightAttacks = 0ULL;
Bitboard knights = board.pieces(WHITE, KNIGHT);
while (knights) {
    Square sq = popLsb(knights);  // Get knight position
    knightAttacks |= MoveGenerator::getKnightAttacks(sq);  // Add its attacks
}
// knightAttacks now has all squares attacked by ALL white knights
```

##### Why Union of All Pieces?
We store the **union** (bitwise OR) of attacks from all pieces of a type:
```cpp
// If White has knights on B1 and G1:
//   B1 knight attacks: C3, D2, A3
//   G1 knight attacks: H3, F3, E2
// knightAttacks[WHITE] = all 6 squares (bitwise OR)
```

This allows fast queries: "Do any white knights attack square X?" → `knightAttacks[WHITE] & squareBB(X)`

##### Attack Computation Order Matters
From isSquareAttacked optimization, we learned:
1. Compute diagonal attacks ONCE for bishops + queens
2. Compute straight attacks ONCE for rooks + queens
3. This avoids redundant magic bitboard lookups

---

#### Phase B1: Populate Basic Position Info

**What:** Fill in occupied, occupiedByColor, kingSquare.

**File:** `src/evaluation/eval_context.cpp` (NEW FILE)

**Implementation:**
```cpp
#include "eval_context.h"
#include "../core/board.h"
#include "../core/move_generation.h"

namespace seajay::eval {

void populateContext(EvalContext& ctx, const Board& board) {
    // ========================================================================
    // BASIC POSITION INFO
    // ========================================================================
    ctx.occupied = board.occupied();
    ctx.occupiedByColor[WHITE] = board.pieces(WHITE);
    ctx.occupiedByColor[BLACK] = board.pieces(BLACK);
    ctx.kingSquare[WHITE] = board.kingSquare(WHITE);
    ctx.kingSquare[BLACK] = board.kingSquare(BLACK);

    // TODO Phase B2: Populate attack bitboards
    // TODO Phase B3: Populate aggregated data
}

} // namespace seajay::eval
```

**Update evaluateSpine stub:**
```cpp
template<bool Traced>
Score evaluateSpine(const Board& board, EvalTrace* trace) {
    // Phase B1: Build context (partially populated)
    EvalContext ctx;
    populateContext(ctx, board);

    // Still call legacy evaluator (context not used yet)
    return evaluateImpl<Traced>(board, trace);
}
```

**Testing:**
```bash
# With spine evaluator (default)
# Legacy: EvalUseSpine option removed; spine always active
echo "bench" | ./seajay
# Verify: nodes IDENTICAL (context built but unused)

# Verify context construction cost is low
# (Bench should not be significantly slower)
```

**Commit:**
```
git commit -m "feat: populate basic position info in EvalContext (Phase B1)

Populate occupied bitboards and king squares in context.
Context built on every evaluation when spine evaluator (default).

No behavior change (context not used by evaluation terms yet).

bench 19191913"
```

---

#### Phase B2: Populate Attack Bitboards

⚠️ **CRITICAL BUG:** Original plan has queen attack double-counting bug
⚠️ **CRITICAL BUG:** Black pawn attack file masks are backwards
📖 **SEE CPP REVIEW:** Section 5 (Bugs 1 & 2) for detailed fixes

**What:** Compute per-piece-type attack bitboards using optimized order from isSquareAttacked.

**File:** `src/evaluation/evaluate.cpp` (within detail namespace)

💡 **OPTIMIZATION:** See CPP Review Section 2 for SIMD-optimized version

**Implementation:**
```cpp
void populateContext(EvalContext& ctx, const Board& board) {
    // Phase B1 code (occupied, kings) ...

    // ========================================================================
    // ATTACK BITBOARDS
    // ========================================================================

    // Process both colors
    for (Color color : {WHITE, BLACK}) {
        const int idx = static_cast<int>(color);

        // --------------------------------------------------------------------
        // PAWN ATTACKS (batch computation via bitboard shifts)
        // --------------------------------------------------------------------
        Bitboard pawns = board.pieces(color, PAWN);
        if (color == WHITE) {
            // White pawns attack diagonally upward
            ctx.pawnAttacks[idx] = ((pawns & ~FILE_H_BB) << 9) |  // Northeast
                                    ((pawns & ~FILE_A_BB) << 7);   // Northwest
        } else {
            // Black pawns attack diagonally downward
            // ⚠️ CRITICAL: File masks were backwards in original plan!
            // See CPP Review Section 5, Bug 2
            ctx.pawnAttacks[idx] = ((pawns & ~FILE_H_BB) >> 9) |  // Southwest (correct mask)
                                    ((pawns & ~FILE_A_BB) >> 7);   // Southeast (correct mask)
        }

        // --------------------------------------------------------------------
        // KNIGHT ATTACKS (iterate all knights, union attacks)
        // --------------------------------------------------------------------
        ctx.knightAttacks[idx] = 0ULL;
        Bitboard knights = board.pieces(color, KNIGHT);
        while (knights) {
            Square sq = popLsb(knights);
            ctx.knightAttacks[idx] |= MoveGenerator::getKnightAttacks(sq);
        }

        // --------------------------------------------------------------------
        // SLIDING PIECE ATTACKS (optimized: compute diagonal/straight once)
        // Follows isSquareAttacked optimization pattern
        // --------------------------------------------------------------------
        Bitboard bishops = board.pieces(color, BISHOP);
        Bitboard rooks = board.pieces(color, ROOK);
        Bitboard queens = board.pieces(color, QUEEN);

        ctx.bishopAttacks[idx] = 0ULL;
        ctx.rookAttacks[idx] = 0ULL;
        ctx.queenAttacks[idx] = 0ULL;

        // ⚠️ WARNING: Do NOT use diagonal/straight slider pattern for queens!
        // Each queen would be processed twice (in both loops)
        // See CPP Review Section 5, Bug 1 for explanation

        // Bishops only (no queens)
        ctx.bishopAttacks[idx] = 0ULL;
        Bitboard tempBishops = bishops;
        while (tempBishops) {
            Square sq = popLsb(tempBishops);
            ctx.bishopAttacks[idx] |= MoveGenerator::getBishopAttacks(sq, ctx.occupied);
        }

        // Rooks only (no queens)
        ctx.rookAttacks[idx] = 0ULL;
        Bitboard tempRooks = rooks;
        while (tempRooks) {
            Square sq = popLsb(tempRooks);
            ctx.rookAttacks[idx] |= MoveGenerator::getRookAttacks(sq, ctx.occupied);
        }

        // Queens separately (compute both diagonal AND straight)
        ctx.queenAttacks[idx] = 0ULL;
        Bitboard tempQueens = queens;
        while (tempQueens) {
            Square sq = popLsb(tempQueens);
            Bitboard diag = MoveGenerator::getBishopAttacks(sq, ctx.occupied);
            Bitboard straight = MoveGenerator::getRookAttacks(sq, ctx.occupied);
            ctx.queenAttacks[idx] |= (diag | straight);  // Both components
        }

        // IMPORTANT: Full queen attacks = diagonal + straight
        // (stored separately for per-piece-type queries, but combinable)

        // --------------------------------------------------------------------
        // KING ATTACKS (single king per color)
        // --------------------------------------------------------------------
        ctx.kingAttacks[idx] = MoveGenerator::getKingAttacks(ctx.kingSquare[idx]);
    }

    // TODO Phase B3: Populate aggregated data
}
```

**Testing:**
```bash
# Verify bitboards are populated correctly
# Add debug print in populateContext:
#   std::cout << "White pawn attacks: " << std::bitset<64>(ctx.pawnAttacks[WHITE]) << "\n";

# Legacy: EvalUseSpine option removed; spine always active
echo "bench" | ./seajay
# Verify: nodes IDENTICAL

# Verify construction cost is acceptable
# (Should add <5% overhead to evaluation time)
```

**Commit:**
```
git commit -m "feat: populate attack bitboards in EvalContext (Phase B2)

Compute per-piece-type attack bitboards for both colors.
Uses optimized sliding piece order (diagonal/straight computed once).

Pawn attacks: batch bitboard shifts
Knight attacks: iterate and union
Sliding pieces: follow isSquareAttacked optimization pattern
King attacks: single lookup

No behavior change (context not used by evaluation terms yet).

bench 19191913"
```

---

#### Phase B3: Populate Aggregated Data

⚠️ **CRITICAL:** Double-attack algorithm in original plan has O(n²) complexity
📖 **SEE CPP REVIEW:** Section 4 for optimized O(n) algorithm

**What:** Compute attackedBy, doubleAttacks, mobilityArea, king ring data, pawn attack spans.

**File:** `src/evaluation/evaluate.cpp` (within detail namespace)

💡 **OPTIMIZATION:** See CPP Review Section 2 for SIMD aggregation

**Implementation:**
```cpp
void populateContext(EvalContext& ctx, const Board& board) {
    // Phase B1 and B2 code ...

    // ========================================================================
    // AGGREGATED ATTACK DATA
    // ========================================================================
    for (Color color : {WHITE, BLACK}) {
        const int idx = static_cast<int>(color);

        // --------------------------------------------------------------------
        // ATTACKED BY: Union of all piece attacks
        // --------------------------------------------------------------------
        ctx.attackedBy[idx] = ctx.pawnAttacks[idx] |
                              ctx.knightAttacks[idx] |
                              ctx.bishopAttacks[idx] |
                              ctx.rookAttacks[idx] |
                              ctx.queenAttacks[idx] |
                              ctx.kingAttacks[idx];

        // --------------------------------------------------------------------
        // DOUBLE ATTACKS: Squares attacked 2+ times
        // ⚠️ IMPROVED ALGORITHM: Single-pass version (see CPP Review Section 4)
        // Original mergeAttacks lambda has O(n²) worst case
        // --------------------------------------------------------------------
        Bitboard acc = 0ULL;
        Bitboard doubles = 0ULL;

        // Process in order of frequency (most to least common)
        Bitboard pieceAttacks[] = {
            ctx.pawnAttacks[idx],
            ctx.knightAttacks[idx],
            ctx.bishopAttacks[idx],
            ctx.rookAttacks[idx],
            ctx.queenAttacks[idx],
            ctx.kingAttacks[idx]
        };

        for (Bitboard attacks : pieceAttacks) {
            doubles |= acc & attacks;  // What's already attacked
            acc |= attacks;             // Add new attacks
        }

        ctx.doubleAttacks[idx] = doubles;
    }

    // ========================================================================
    // MOBILITY DATA
    // ========================================================================
    for (Color color : {WHITE, BLACK}) {
        const int idx = static_cast<int>(color);
        const int enemyIdx = 1 - idx;

        // Mobility area: squares not occupied by own pieces, not attacked by enemy pawns
        ctx.mobilityArea[idx] = ~ctx.occupiedByColor[idx] & ~ctx.pawnAttacks[enemyIdx];
    }

    // ========================================================================
    // KING SAFETY DATA
    // ========================================================================
    for (Color color : {WHITE, BLACK}) {
        const int idx = static_cast<int>(color);
        const int enemyIdx = 1 - idx;

        // King ring: 8 squares around king
        ctx.kingRing[idx] = MoveGenerator::getKingAttacks(ctx.kingSquare[idx]);

        // King ring attacks: enemy attacks on king ring (precomputed for speed)
        ctx.kingRingAttacks[idx] = ctx.kingRing[idx] & ctx.attackedBy[enemyIdx];
    }

    // ========================================================================
    // PAWN ATTACK SPANS (for outpost detection)
    // ========================================================================
    for (Color color : {WHITE, BLACK}) {
        const int idx = static_cast<int>(color);
        Bitboard pawns = board.pieces(color, PAWN);

        // Pawn attack span: all squares pawns could ever attack
        // (fill forward, then expand diagonally)
        Bitboard span = 0ULL;
        if (color == WHITE) {
            // Fill all squares forward of white pawns
            Bitboard filled = pawns;
            for (int i = 0; i < 6; i++) {  // Max 6 ranks to advance
                filled |= (filled << 8) & ~RANK_8_BB;
            }
            // Expand to diagonals
            span = ((filled & ~FILE_A_BB) << 7) | ((filled & ~FILE_H_BB) << 9);
            span &= ~(RANK_1_BB | RANK_2_BB);  // Pawns can't attack backwards
        } else {
            // Fill all squares backward of black pawns
            Bitboard filled = pawns;
            for (int i = 0; i < 6; i++) {
                filled |= (filled >> 8) & ~RANK_1_BB;
            }
            // Expand to diagonals
            span = ((filled & ~FILE_H_BB) >> 7) | ((filled & ~FILE_A_BB) >> 9);
            span &= ~(RANK_7_BB | RANK_8_BB);
        }

        ctx.pawnAttackSpan[idx] = span;
    }
}
```

**Testing:**
```bash
# Verify aggregated data is correct
# Add debug asserts in populateContext:
#   assert(ctx.attackedBy[WHITE] == (ctx.pawnAttacks[WHITE] | ... | ctx.kingAttacks[WHITE]));

# Legacy: EvalUseSpine option removed; spine always active
echo "bench" | ./seajay
# Verify: nodes IDENTICAL

# Performance check: context construction should be <5% of evaluation time
```

**Commit:**
```
git commit -m "feat: populate aggregated attack data in EvalContext (Phase B3)

Compute derived attack data:
- attackedBy: union of all piece attacks
- doubleAttacks: squares attacked 2+ times (for threat evaluation)
- mobilityArea: safe squares for mobility calculation
- kingRing + kingRingAttacks: precomputed king safety data
- pawnAttackSpan: all squares pawns could ever attack (for outposts)

Context now fully populated. Ready for evaluation terms to consume.

No behavior change (context not used by evaluation terms yet).

bench 19191913"
```

---

### Phase C: Evaluation Term Migration (5-15% NPS Expected)

**Goal:** Refactor existing evaluation terms to read from EvalContext instead of recomputing attacks.

#### Phase C1: Mobility Evaluation

**What:** Replace 19 calls to `MoveGenerator::get*Attacks` in mobility code with context lookups.

**Impact:** This is the **biggest win** (mobility has the most redundant computation).

**File:** `src/evaluation/evaluate.cpp` (lines 1124-1512 in current code)

##### Current Mobility Code (BEFORE):
```cpp
// Lines 1271-1301: White knight mobility
Bitboard wn = whiteKnights;
while (wn) {
    Square sq = popLsb(wn);
    Bitboard attacks = MoveGenerator::getKnightAttacks(sq);  // ← REDUNDANT CALL
    attacks &= ~board.pieces(WHITE);
    attacks &= ~blackPawnAttacks;
    int moveCount = popCount(attacks);
    whiteMobilityScore += moveCount * MOBILITY_BONUS_PER_MOVE;
}
// ... similar code for bishops, rooks, queens (16 more redundant calls)
```

##### New Mobility Code (AFTER):
```cpp
/**
 * Evaluate piece mobility using precomputed attack data from context.
 *
 * Mobility = number of safe squares a piece can move to
 * Safe = not occupied by own pieces, not attacked by enemy pawns
 *
 * @param board Position
 * @param ctx Precomputed attack context
 * @return Mobility score (positive = good for side to move)
 */
int evaluateMobility(const Board& board, const EvalContext& ctx) {
    static constexpr int MOBILITY_BONUS_PER_MOVE = 2;  // 2 cp per available move

    int whiteMobility = 0;
    int blackMobility = 0;

    // ========================================================================
    // WHITE MOBILITY
    // ========================================================================

    // Knight mobility: count squares in mobility area
    // OLD: Loop through knights, call getKnightAttacks() for each
    // NEW: Use precomputed knightAttacks from context
    Bitboard whiteKnightMobility = ctx.knightAttacks[WHITE] & ctx.mobilityArea[WHITE];
    whiteMobility += popCount(whiteKnightMobility) * MOBILITY_BONUS_PER_MOVE;

    // Bishop mobility: use precomputed bishop attacks
    Bitboard whiteBishopMobility = ctx.bishopAttacks[WHITE] & ctx.mobilityArea[WHITE];
    whiteMobility += popCount(whiteBishopMobility) * MOBILITY_BONUS_PER_MOVE;

    // Rook mobility: use precomputed rook attacks
    Bitboard whiteRookMobility = ctx.rookAttacks[WHITE] & ctx.mobilityArea[WHITE];
    whiteMobility += popCount(whiteRookMobility) * MOBILITY_BONUS_PER_MOVE;

    // Queen mobility: diagonal + straight attacks
    Bitboard whiteQueenMobility = ctx.queenAttacks[WHITE] & ctx.mobilityArea[WHITE];
    whiteMobility += popCount(whiteQueenMobility) * MOBILITY_BONUS_PER_MOVE;

    // ========================================================================
    // BLACK MOBILITY (mirror of white)
    // ========================================================================

    Bitboard blackKnightMobility = ctx.knightAttacks[BLACK] & ctx.mobilityArea[BLACK];
    blackMobility += popCount(blackKnightMobility) * MOBILITY_BONUS_PER_MOVE;

    Bitboard blackBishopMobility = ctx.bishopAttacks[BLACK] & ctx.mobilityArea[BLACK];
    blackMobility += popCount(blackBishopMobility) * MOBILITY_BONUS_PER_MOVE;

    Bitboard blackRookMobility = ctx.rookAttacks[BLACK] & ctx.mobilityArea[BLACK];
    blackMobility += popCount(blackRookMobility) * MOBILITY_BONUS_PER_MOVE;

    Bitboard blackQueenMobility = ctx.queenAttacks[BLACK] & ctx.mobilityArea[BLACK];
    blackMobility += popCount(blackQueenMobility) * MOBILITY_BONUS_PER_MOVE;

    // Return difference (positive = white better)
    return whiteMobility - blackMobility;
}
```

**Key Changes:**
1. **No more loops through pieces** - use precomputed union attacks
2. **Single bitboard AND per piece type** - `attacks & mobilityArea`
3. **One popCount per piece type** - no per-piece counting

**Integration into evaluateSpine:**
```cpp
template<bool Traced>
Score evaluateSpine(const Board& board, EvalTrace* trace) {
    // Build context
    EvalContext ctx;
    populateContext(ctx, board);

    // Evaluate terms
    Score material = /* existing material code */;
    Score pst = /* existing PST code */;

    // NEW: Use context-based mobility
    int mobilityValue = evaluateMobility(board, ctx);
    Score mobilityScore(mobilityValue);

    // ... rest of evaluation (still using legacy code for now)

    // Trace mobility if requested
    if constexpr (Traced) {
        if (trace) trace->mobility = mobilityScore;
    }

    return /* combine all scores */;
}
```

**Testing:**
```bash
# CRITICAL: Mobility must evaluate identically to before
# Test with spine evaluator (default)

# Legacy: EvalUseSpine option removed; spine always active
echo "bench" | ./seajay > phase_c1.txt

# Verify: nodes IDENTICAL to baseline (same move ordering)
# If nodes differ, mobility calculation has a bug!

# Check NPS improvement
grep "nps" phase_c1.txt
# Expect: 5-10% faster than baseline
```

**Commit:**
```
git commit -m "refactor: migrate mobility evaluation to EvalContext (Phase C1)

Replace per-piece attack computation loops with precomputed context lookups.
Eliminates 16 redundant calls to MoveGenerator::get*Attacks.

Changes:
- Knight/bishop/rook/queen mobility uses ctx.*Attacks[] bitboards
- Single popCount per piece type vs per-piece
- No loops through pieces, pure bitboard operations

Expected: 5-10% NPS improvement, 0 ELO (identical evaluation)

bench 19191913"
```

**Expected Impact:** 5-10% NPS, 0 ELO

---

#### Phase C2: Passed Pawn Evaluation

**What:** Replace `PromotionPathAttackCache` with context lookups, simplify path checking logic.

**Impact:** Remove 109-line local cache class, cleaner code, 2-5% NPS gain.

**File:** `src/evaluation/evaluate.cpp`

##### Current Passed Pawn Code (BEFORE):
```cpp
// Lines 26-109: PromotionPathAttackCache class (local cache)
class PromotionPathAttackCache {
    // ... 80 lines of caching logic ...
    bool isAttacked(Color color, Square square) {
        // Cache attacks per square
        // Calls attackersTo() which recomputes attacks
    }
};

// Lines 390-668: Passed pawn evaluation
PromotionPathAttackCache pathAttackCache(board);  // Build local cache

// For each passed pawn:
while (pathSquares) {
    Square pathSq = popLsb(pathSquares);
    if (!pathAttackCache.isAttacked(enemy, pathSq)) {  // ← Cache lookup
        pathFree = true;
    }
}
```

##### New Passed Pawn Code (AFTER):
```cpp
// PromotionPathAttackCache class REMOVED (no longer needed)

/**
 * Evaluate passed pawns using precomputed attack data from context.
 *
 * Passed pawn evaluation considers:
 * - Rank (more advanced = more valuable)
 * - Path to promotion (free? safe? defended?)
 * - King proximity (friendly king near, enemy king far)
 * - Support (protected by pawns, rooks behind)
 *
 * @param board Position
 * @param ctx Precomputed attack context
 * @param color Color of passed pawns to evaluate
 * @param passedPawns Bitboard of passed pawns for this color
 * @return Passed pawn score for this color
 */
int evaluatePassedPawns(const Board& board, const EvalContext& ctx,
                        Color color, Bitboard passedPawns) {
    const Color enemy = (color == WHITE) ? BLACK : WHITE;
    const int forward = (color == WHITE) ? 8 : -8;
    int totalBonus = 0;

    while (passedPawns) {
        Square sq = popLsb(passedPawns);
        int relRank = PawnStructure::relativeRank(color, sq);
        int bonus = PASSED_PAWN_BONUS[relRank];

        // ====================================================================
        // PATH TO PROMOTION ANALYSIS (using context)
        // ====================================================================

        // Build path squares (all squares from pawn to promotion)
        Bitboard pathSquares = 0ULL;
        int pathIdx = static_cast<int>(sq) + forward;
        while (pathIdx >= 0 && pathIdx < 64) {
            pathSquares |= (1ULL << pathIdx);
            pathIdx += forward;
        }

        // OLD: Loop through path squares, call isAttacked() for each
        // NEW: Single bitboard query against context

        // Path is free if no pieces block it
        bool pathFree = ((pathSquares & board.occupied()) == 0);

        // Path is safe if enemy doesn't attack any path square
        Bitboard enemyAttacks = ctx.attackedBy[enemy];
        bool pathSafe = ((pathSquares & enemyAttacks) == 0);

        // Path is defended if we control all path squares
        Bitboard ourAttacks = ctx.attackedBy[color];
        bool pathDefended = ((pathSquares & ~ourAttacks) == 0);

        // Apply bonuses based on path state
        if (pathFree) {
            bonus += PASSER_PATH_FREE_BONUS;
            if (pathSafe) {
                bonus += PASSER_PATH_SAFE_BONUS;
            }
            if (pathDefended) {
                bonus += PASSER_PATH_DEFENDED_BONUS;
            }
        }

        // ====================================================================
        // STOP SQUARE ANALYSIS (square directly in front of pawn)
        // ====================================================================

        int stopIdx = static_cast<int>(sq) + forward;
        if (stopIdx >= 0 && stopIdx < 64) {
            Square stopSquare = static_cast<Square>(stopIdx);
            Bitboard stopBB = squareBB(stopSquare);

            // OLD: pathAttackCache.isAttacked(enemy, stopSquare)
            // NEW: Single bitboard test
            bool stopAttacked = (stopBB & enemyAttacks) != 0;
            bool stopDefended = (stopBB & ourAttacks) != 0;

            if (stopAttacked) {
                bonus -= PASSER_STOP_ATTACKED_PENALTY;
            }
            if (stopDefended) {
                bonus += PASSER_STOP_DEFENDED_BONUS;
            }
        }

        // ... rest of passed pawn bonuses (king distance, rook support, etc.)

        totalBonus += bonus;
    }

    return totalBonus;
}
```

**Key Changes:**
1. **Remove PromotionPathAttackCache** (109 lines deleted)
2. **Path checking:** `pathSquares & ctx.attackedBy[enemy]` (single AND)
3. **Stop square checking:** `stopBB & ctx.attackedBy[enemy]` (single AND)
4. **No loops through path squares** - pure bitboard operations

**Testing:**
```bash
# CRITICAL: Passed pawn scores must be identical
# Create test position with passed pawns:
echo "position fen 4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1" | ./seajay
# Legacy: EvalUseSpine option removed; spine always active
echo "eval" | ./seajay
# Compare passed pawn component with legacy evaluator

echo "bench" | ./seajay
# Verify: nodes IDENTICAL (same evaluation)

# Check NPS improvement
# Expect: 2-5% faster (PromotionPathAttackCache eliminated)
```

**Commit:**
```
git commit -m "refactor: migrate passed pawn evaluation to EvalContext (Phase C2)

Remove PromotionPathAttackCache (109 lines), use precomputed context attacks.

Changes:
- Path checking: bitboard AND vs per-square cache lookups
- Stop square checking: single bitboard test
- Eliminates local per-square attack cache

Code reduction: -109 lines
Expected: 2-5% NPS improvement, 0 ELO (identical evaluation)

bench 19191913"
```

**Expected Impact:** 2-5% NPS, 0 ELO

---

### Phase D: Remaining Term Migration (3-8% NPS Expected)

**Goal:** Migrate all remaining evaluation terms to use EvalContext.

#### Phase D1: King Safety Evaluation

**What:** Use `ctx.kingRingAttacks` instead of iterating through king ring squares.

**Current code:** `src/evaluation/king_safety.cpp` (simple, but could benefit from context)

**New helper function:**
```cpp
/**
 * Evaluate king safety using precomputed king ring attack data.
 *
 * @param board Position
 * @param ctx Precomputed attack context
 * @param side Color of king to evaluate
 * @return King safety score (positive = safe)
 */
Score evaluateKingSafetySpine(const Board& board, const EvalContext& ctx, Color side) {
    const int idx = static_cast<int>(side);

    // OLD: Loop through 8 king ring squares, call isSquareAttacked() for each
    // NEW: Use precomputed ctx.kingRingAttacks[idx]

    // Count attacked squares in king ring
    int attackedSquares = popCount(ctx.kingRingAttacks[idx]);

    // Apply king safety penalties (simplified for example)
    int penalty = attackedSquares * KING_RING_ATTACK_PENALTY;

    // Shield pawns (still uses board.pieces(), not in context yet)
    Bitboard shieldPawns = getShieldPawns(board, side, ctx.kingSquare[idx]);
    int shieldBonus = popCount(shieldPawns) * SHIELD_PAWN_BONUS;

    return Score(shieldBonus - penalty);
}
```

**Testing:**
```bash
# Legacy: EvalUseSpine option removed; spine always active
echo "bench" | ./seajay
# Verify: nodes IDENTICAL

# Expect: 1-2% NPS improvement (8 attack queries → 1 popCount)
```

**Commit:**
```
git commit -m "refactor: migrate king safety to EvalContext (Phase D1)

Use precomputed ctx.kingRingAttacks instead of per-square attack queries.
Eliminates 8 redundant isSquareAttacked calls per king.

Expected: 1-2% NPS improvement, 0 ELO (identical evaluation)

bench 19191913"
```

---

#### Phase D2: Remaining Terms (Outposts, Rook Files, etc.)

**What:** Migrate all remaining terms that query attacks.

**Terms to migrate:**
1. Knight outposts (lines 1227-1260): Use `ctx.pawnAttackSpan`
2. Rook open files (lines 1336-1380): Already simple, minimal change
3. Pawn tension (lines 1181-1221): Use `ctx.pawnAttacks`

**Implementation pattern:**
```cpp
// Knight outposts: BEFORE
Bitboard whiteOutpostSquares = WHITE_OUTPOST_RANKS &
                                ~blackPawnAttackSpan &  // Recomputed
                                whitePawnAttacks;        // Recomputed

// Knight outposts: AFTER
Bitboard whiteOutpostSquares = WHITE_OUTPOST_RANKS &
                                ~ctx.pawnAttackSpan[BLACK] &  // From context
                                ctx.pawnAttacks[WHITE];       // From context
```

**Testing:**
```bash
# Legacy: EvalUseSpine option removed; spine always active
echo "bench" | ./seajay
# Verify: nodes IDENTICAL

# Cumulative NPS gain (Phases C+D): 15-25%
```

**Commit:**
```
git commit -m "refactor: migrate remaining evaluation terms to EvalContext (Phase D2)

Migrate knight outposts, pawn tension, and minor terms to use context.

All evaluation terms now consume EvalContext (no direct board attack queries).
evaluate.cpp reduced from 1669 to ~1200 lines.

Expected: 1-3% NPS improvement, 0 ELO (identical evaluation)
Cumulative NPS gain (Phases C+D): 15-25%

bench 19191913"
```

---

### Phase E: New Features and Validation

**Goal:** Add new evaluation terms enabled by EvalContext, validate entire refactor.

#### Phase E1: Threat Evaluation (NEW TERM)

**What:** Add threat evaluation using `ctx.doubleAttacks` (inspired by Perseus pattern).

**Why now:** This is a NEW term, so bench nodes will change (expected!). Do this AFTER all migration is complete.

**Implementation:**
```cpp
/**
 * Evaluate threats using precomputed double-attack data.
 *
 * A "threat" is when a less valuable piece attacks a more valuable piece.
 * Double attacks (attacking multiple enemy pieces) are especially strong.
 *
 * @param board Position
 * @param ctx Precomputed attack context
 * @return Threat score (from white's perspective)
 */
int evaluateThreats(const Board& board, const EvalContext& ctx) {
    int whiteThreats = 0;
    int blackThreats = 0;

    // ========================================================================
    // HANGING PIECES (attacked but not defended)
    // ========================================================================

    // White threats: white attacks black pieces
    Bitboard blackPieces = ctx.occupiedByColor[BLACK];
    Bitboard whiteAttacks = ctx.attackedBy[WHITE];
    Bitboard blackAttacks = ctx.attackedBy[BLACK];

    Bitboard blackHanging = blackPieces & whiteAttacks & ~blackAttacks;
    whiteThreats += popCount(blackHanging) * HANGING_PIECE_THREAT;

    // Black threats: black attacks white pieces
    Bitboard whitePieces = ctx.occupiedByColor[WHITE];
    Bitboard whiteHanging = whitePieces & blackAttacks & ~whiteAttacks;
    blackThreats += popCount(whiteHanging) * HANGING_PIECE_THREAT;

    // ========================================================================
    // DOUBLE ATTACKS (fork threats)
    // ========================================================================

    // White double attacks on black pieces
    Bitboard whiteDoubleOnBlack = ctx.doubleAttacks[WHITE] & blackPieces;
    whiteThreats += popCount(whiteDoubleOnBlack) * DOUBLE_ATTACK_THREAT;

    // Black double attacks on white pieces
    Bitboard blackDoubleOnWhite = ctx.doubleAttacks[BLACK] & whitePieces;
    blackThreats += popCount(blackDoubleOnWhite) * DOUBLE_ATTACK_THREAT;

    return whiteThreats - blackThreats;
}
```

**Integration:**
```cpp
template<bool Traced>
Score evaluateSpine(const Board& board, EvalTrace* trace) {
    EvalContext ctx;
    populateContext(ctx, board);

    // ... existing terms ...

    // NEW: Threat evaluation
    int threatValue = evaluateThreats(board, ctx);
    Score threatScore(threatValue);

    if constexpr (Traced) {
        if (trace) trace->threats = threatScore;  // Add to EvalTrace
    }

    return material + pst + pawns + pieces + mobility + kingSafety + threats;
}
```

**Testing:**
```bash
# This WILL change bench nodes (new term affects move ordering)
# Legacy: EvalUseSpine option removed; spine always active
echo "bench" | ./seajay > phase_e1.txt

# Record NEW bench count
grep "nodes:" phase_e1.txt
# Example: bench 19234567 (different from 19191913)

# SPRT test threat evaluation
# Run OpenBench with spine evaluator vs legacy (option removed post-integration)
# Bounds: [0.0, 5.0] (expect small positive gain from better move ordering)
```

**Commit:**
```
git commit -m "feat: add threat evaluation using EvalContext (Phase E1)

NEW EVALUATION TERM: Threat detection using double-attack data.

Features:
- Hanging pieces (attacked but not defended)
- Double attacks (fork threats)
- Uses ctx.doubleAttacks[] precomputed data

This is a new term, so bench nodes WILL CHANGE.
Expected: 0-5 nELO from improved move ordering.

bench 19234567"
```

**Expected Impact:** 0-5 nELO (new term, should be tested on OpenBench)

---

**Status Update (2025-10-02):** Phase E1 implementation resides on `feature/spine-phase-e1-threat-eval`. Release bench with `spine evaluator (default)` now reports `2508823` nodes (previous parity baseline: `2428308`). Legacy fallback remains unchanged; threat evaluation only executes on the spine path.

#### Phase E2: Validation and Documentation

**What:** Comprehensive testing, profiling, documentation.

**Validation checklist:**

1. **Perft Testing:** ✅ `go perft 6` (baseline parity maintained)

2. **Tactical Suite:** ✅ WAC solved 269/300 positions (89.7% success).
   - Failures logged in `tactical_failures_2025-10-02_17-47-31.csv`

3. **Self-Play:** Skipped (covered during earlier phases).

4. **Profiling:**
   ```bash
   # Instruction profiling (callgrind) – spine evaluator (default)
   valgrind --tool=callgrind --callgrind-out-file=callgrind.eval_spine ./bin/seajay bench
   callgrind_annotate callgrind.eval_spine | grep -i isSquareAttacked
   # Result: isSquareAttacked ≈ 6.6% of instructions (goal <4%)

   # Benchmark throughput reference
   echo "bench" | ./seajay            # 2,508,823 nodes (spine path)

   # Stack/heap usage (massif)
   valgrind --tool=massif --massif-out-file=massif.out.eval_spine ./bin/seajay bench
   ms_print massif.out.eval_spine | head
   # Peak heap ≈ 272 MB (dominated by TT resize), stack footprint ~0 B
   ```

5. **Cache Hit Rate:** Not collected (instrumentation unavailable in current env).

**Documentation:**
- Update this document with measured metrics (tactical, callgrind, massif)
- Add inline comments where EvalContext usage is non-obvious
- Update README with final NPS figures if required

**Commit Template:**
```
git commit -m "docs: validation and profiling for evaluation spine refactor (Phase E2)

Validation results:
- Perft: PASS (move generation unchanged)
- Tactical suite: 269/300 solved (89.7%)
- Self-play: skipped (covered during Phase D)
- Profiling: isSquareAttacked ≈ 6.6% of instructions (Callgrind)
- NPS: bench 2508823 nodes (spine evaluator (default))
- Memory: Massif peak ≈ 272 MB (TT dominated), stack ~0 B

All phases complete. Evaluation spine refactor successful.

bench 2508823"
```

---

## Testing Protocol

### Bench Identity Requirement

**CRITICAL:** Until Phase E1, `bench` node count must be **exactly identical** to baseline.

```bash
# Baseline (before refactor)
echo "bench" | ./seajay > baseline.txt
grep "nodes:" baseline.txt
# Example: nodes: 19191913

# After each phase (A1 through D2)
# Legacy: EvalUseSpine option removed; spine always active
echo "bench" | ./seajay > phase_XX.txt
grep "nodes:" phase_XX.txt
# MUST BE: nodes: 19191913 (exactly same as baseline)

# If nodes differ, evaluation has a bug! Debug before proceeding.
```

### SPRT Testing After Each Major Phase

After committing each phase, run SPRT test on OpenBench:

**Configuration:**
- **Dev Branch:** `feature/evaluation-spine` (or appropriate branch)
- **Base Branch:** `main`
- **Book:** UHO_4060_v2.epd
- **Time Control:** 10+0.1 (standard)
- **SPRT Bounds:** `[-5.0, 3.0]` (detect regressions, allow noise)

**Expected Results:**
- **Phases A-D:** LLR should reach upper bound (no regression)
- **Phase E1:** May reach lower or upper bound (new term being tested)

**OpenBench Parsing:**
OpenBench needs to extract bench count from commit message:
```
bench 19191913
```
This EXACT format (lowercase "bench", space, digits) allows OpenBench to verify both engines are testing the same code state.

### Profiling Checkpoints

At key milestones, run Callgrind to measure progress:

```bash
# Baseline (Phase 0 complete)
valgrind --tool=callgrind --callgrind-out-file=callgrind.baseline ./seajay bench
callgrind_annotate callgrind.baseline | grep isSquareAttacked
# Record: % of total instructions

# Phase C complete (mobility + passed pawns migrated)
valgrind --tool=callgrind --callgrind-out-file=callgrind.phase_c ./seajay bench
callgrind_annotate callgrind.phase_c | grep isSquareAttacked
# Expected: 30-40% reduction in isSquareAttacked instructions

# Phase D complete (all migrations done)
valgrind --tool=callgrind --callgrind-out-file=callgrind.phase_d ./seajay bench
callgrind_annotate callgrind.phase_d | grep isSquareAttacked
# Expected: 60-70% reduction (target: <4% of total)
```

---

## Common Pitfalls & How to Avoid

### Pitfall 1: Forgetting to Update Both Colors

**Problem:**
```cpp
// BUG: Only populated white attacks
ctx.pawnAttacks[WHITE] = /* ... */;
// Forgot to populate ctx.pawnAttacks[BLACK]!
```

**Solution:** Always use a loop:
```cpp
for (Color color : {WHITE, BLACK}) {
    const int idx = static_cast<int>(color);
    ctx.pawnAttacks[idx] = /* ... */;
}
```

**How to detect:** Test with positions favoring black (e.g., black passed pawns). If evaluation is asymmetric, you missed a color.

---

### Pitfall 2: Queen Attack Double-Counting (CRITICAL BUG)

⚠️ **CRITICAL BUG:** Original plan processes queens TWICE (in diagonal AND straight loops)
📖 **SEE CPP REVIEW:** Section 5, Bug 1 for detailed explanation

**Problem:** If queens are included in both diagonalSliders and straightSliders, each queen gets processed twice:
```cpp
// BUG: Queen at E4 processed in BOTH loops
diagonalSliders = bishops | queens;  // E4 queen included
straightSliders = rooks | queens;    // E4 queen included again!
```

**Solution:** Process each queen exactly once:
```cpp
// Queen attacks = diagonal + straight
// ctx.queenAttacks[idx] is populated in TWO loops:
//   1. Diagonal slider loop (bishops + queens)
//   2. Straight slider loop (rooks + queens)
// Both components are OR'd into ctx.queenAttacks[idx]
```

**How to detect:** Test position with queen attacking on diagonal and straight simultaneously. Verify `ctx.queenAttacks` includes both.

---

### Pitfall 3: Black Pawn Attack File Masks (CRITICAL BUG)

⚠️ **CRITICAL BUG:** File masks for black pawns are backwards in original plan
📖 **SEE CPP REVIEW:** Section 5, Bug 2 for detailed explanation

**Problem:** Black pawns attacking southwest should mask FILE_H (not FILE_A):
```cpp
// BUG: File masks backwards for black
ctx.pawnAttacks[BLACK] = ((pawns & ~FILE_A_BB) >> 9) |  // WRONG MASK!
                         ((pawns & ~FILE_H_BB) >> 7);   // WRONG MASK!
```

**Why it's wrong:** A black pawn on H7 moving >> 9 goes to G6. If we don't mask FILE_H,
the pawn "wraps" to the A file (off the edge of the board).

**Solution:** Correct file masking for black pawns:
```cpp
if (color == WHITE) {
    // White pawns attack upward (toward rank 8)
    ctx.pawnAttacks[idx] = ((pawns & ~FILE_H_BB) << 9) | ((pawns & ~FILE_A_BB) << 7);
} else {
    // Black pawns attack downward (toward rank 1)
    ctx.pawnAttacks[idx] = ((pawns & ~FILE_A_BB) >> 9) | ((pawns & ~FILE_H_BB) >> 7);
}
```

**How to detect:** Test with white pawns on rank 2 and black pawns on rank 7. Verify attack bitboards are mirror images.

---

### Pitfall 4: Off-by-One in King Ring

**Problem:** King ring should be 8 squares (3×3 grid minus king square), but easy to include king square by mistake.

```cpp
// BUG: Includes king square
ctx.kingRing[idx] = MoveGenerator::getKingAttacks(kingSquare) | squareBB(kingSquare);
```

**Solution:** King ring is ONLY surrounding squares:
```cpp
ctx.kingRing[idx] = MoveGenerator::getKingAttacks(kingSquare);  // 8 squares, not 9
```

**How to detect:** Assert in debug builds:
```cpp
assert(popCount(ctx.kingRing[idx]) == 8 || popCount(ctx.kingRing[idx]) < 8);  // Less than 8 if king on edge
```

---

### Pitfall 5: Double-Attack Algorithm Inefficiency

⚠️ **PERFORMANCE ISSUE:** Original mergeAttacks lambda has O(n²) worst case
📖 **SEE CPP REVIEW:** Section 4 for optimized algorithm

**Problem:** The proposed mergeAttacks lambda recomputes overlaps multiple times:
```cpp
// INEFFICIENT: Each call does bitwise AND with growing accumulator
auto mergeAttacks = [&](Bitboard attacks) {
    Bitboard overlap = singleAttack & attacks;  // AND with ~14 bits
    doubleOrMore |= overlap;                     // OR result
    singleAttack |= attacks;                     // OR attacks (growing)
};
// Later iterations do more work as singleAttack grows
```

**Solution:** Single-pass algorithm (see Phase B3 implementation):
```cpp
Bitboard singleAttack = 0ULL;
Bitboard doubleOrMore = 0ULL;

auto mergeAttacks = [&](Bitboard attacks) {
    doubleOrMore |= singleAttack & attacks;  // Track overlaps
    singleAttack |= attacks;                  // Add new attacks
};

// Process each piece type
mergeAttacks(ctx.pawnAttacks[idx]);
mergeAttacks(ctx.knightAttacks[idx]);
// ... etc
```

**How to detect:** Test position where same square is attacked by pawn AND knight. Verify `ctx.doubleAttacks` includes that square.

---

### Pitfall 6: Mobility Area Includes Own Pieces

**Problem:** Pieces shouldn't get mobility credit for attacking own pieces.

```cpp
// BUG: Doesn't exclude own pieces
ctx.mobilityArea[idx] = ~ctx.pawnAttacks[enemyIdx];  // Forgot to exclude own pieces!
```

**Solution:** Exclude own pieces:
```cpp
ctx.mobilityArea[idx] = ~ctx.occupiedByColor[idx] & ~ctx.pawnAttacks[enemyIdx];
```

**How to detect:** Test position where pieces are clustered. Mobility should be low (pieces block each other).

---

### Pitfall 7: Forgetting to Pass Context to Evaluation Functions

**Problem:** New evaluation functions don't receive context.

```cpp
// BUG: Function doesn't have context parameter
int evaluateOutposts(const Board& board) {
    // Can't use precomputed data!
}
```

**Solution:** Always pass `const EvalContext&`:
```cpp
int evaluateOutposts(const Board& board, const EvalContext& ctx) {
    // Can use ctx.pawnAttackSpan, etc.
}
```

---

### Pitfall 8: UCI Option Not Checked

**Problem:** Code calls `evaluateSpine` directly without checking UCI option.

```cpp
// BUG: Always uses spine evaluator
return evaluateSpine<false>(board, nullptr);
```

**Solution:** Route through UCI option check:
```cpp
if (seajay::getConfig().evalUseSpine) {
    return evaluateSpine<false>(board, nullptr);
} else {
    return evaluateImpl<false>(board, nullptr);  // Legacy
}
```

---

### Pitfall 9: Context Construction Cost Not Measured

**Problem:** Context construction is expensive but not profiled.

**Solution:** Add profiling:
```cpp
#ifdef PROFILE_EVAL_CONTEXT
auto start = std::chrono::high_resolution_clock::now();
populateContext(ctx, board);
auto end = std::chrono::high_resolution_clock::now();
g_contextConstructionTime += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
#endif
```

**Expected:** Context construction should be <10% of total evaluation time.

---

### Pitfall 10: Bench Nodes Change Unexpectedly

**Problem:** During Phases A-D, bench nodes change (indicates evaluation bug).

**How to debug:**
1. Run with `legacy evaluator (removed)` (legacy): Record nodes
2. Run with `spine evaluator (default)` (spine): Record nodes
3. If different, evaluation is not identical
4. Add debug prints to compare individual term scores:
   ```cpp
   std::cout << "Mobility (legacy): " << legacyMobility << "\n";
   std::cout << "Mobility (spine): " << spineMobility << "\n";
   ```
5. Find first diverging term, inspect that code

---

## LazySMP Compatibility

📖 **SEE CPP REVIEW:** Section 5, Bug 3 for thread-safety with mutable fields

### Why This Matters

SeaJay will implement LazySMP (parallel search with multiple threads) soon. The evaluation spine must be thread-safe by design.

### Design Guarantees

1. **Stack Allocation:**
   - `EvalContext` is a local variable in `evaluateSpine`
   - Each thread gets its own stack
   - No shared state between threads

2. **Read-Only Board Access:**
   - `populateContext` only reads from `board`
   - `board` is passed by const reference
   - Thread-safe as long as board isn't mutated during search (SeaJay's make/unmake is thread-local)

3. **No Global State:**
   - No global attack caches
   - No global evaluation state
   - All data flows through function parameters

4. **Pawn Structure Cache:**
   - `g_pawnStructure` is already thread-local (SeaJay design)
   - EvalContext doesn't change this

### What to Avoid for LazySMP

**DON'T:**
- ❌ Store EvalContext in global variable
- ❌ Cache EvalContext across evaluations (position changes!)
- ❌ Use static variables in evaluation functions
- ❌ Share attack bitboards between threads

**DO:**
- ✅ Stack-allocate EvalContext
- ✅ Pass context by const reference
- ✅ Keep context lifetime local to evaluation call
- ✅ Rebuild context on every evaluation

### Memory Efficiency for LazySMP

**Current design:**
- EvalContext: ~320 bytes per evaluation call
- Stack allocation: no heap overhead
- L1 cache friendly: entire context fits in cache

**With LazySMP (8 threads):**
- 8 threads × 320 bytes = 2.5 KB total
- Each thread's context in its own cache line (no false sharing)
- No lock contention (no shared state)

**Validation:**
```cpp
// In debug builds, verify context is stack-allocated
static_assert(std::is_trivially_copyable_v<EvalContext>,
              "EvalContext must be trivially copyable for stack allocation");

// Verify size is reasonable
static_assert(sizeof(EvalContext) < 512,
              "EvalContext too large for efficient stack allocation");
```

---

## Success Metrics

### Performance Targets

| Metric | Baseline | Target | How to Measure |
|--------|----------|--------|----------------|
| NPS (Nodes per second) | X nps | +15-25% | `echo "bench" \| ./seajay` |
| `isSquareAttacked` % of CPU | 10-12% | <4% | `perf report` |
| Evaluation code size | 1669 lines | ~1200 lines | `wc -l src/evaluation/evaluate.cpp` |
| Attack cache hit rate | Unknown | >75% | UCI `AttackCache` stats |
| Context construction cost | N/A | <10% of eval time | Profiling |

### Functional Correctness

| Test | Expected Result |
|------|-----------------|
| Perft 6 | Nodes match Stockfish/baseline |
| WAC Tactical Suite | Solve rate ≥ baseline |
| Self-Play (1000 games) | 0-10 nELO gain |
| Bench Nodes (Phases A-D) | Exactly identical to baseline |
| Bench Nodes (Phase E1) | Different (new term added) |

### Code Quality

| Metric | Target |
|--------|--------|
| Evaluation terms use context | 100% (no direct board attack queries) |
| Context passed by const ref | 100% (read-only) |
| Thread-local state | 100% (no global mutable state) |
| Documentation coverage | 100% (all functions have docstrings) |

---

## Appendix: Code Examples

💡 **OPTIMIZATION:** See CPP Review Section 7 for fully optimized implementation with:
- SIMD attack aggregation
- BMI2 parallel processing
- Prefetch hints
- Template-based color specialization

### Example 1: Complete populateContext Implementation

See Phase B implementation sections above (with bug fixes from CPP Review).

### Example 2: Complete evaluateSpine Implementation

```cpp
template<bool Traced>
Score evaluateSpine(const Board& board, EvalTrace* trace) {
    // Reset trace if provided
    if constexpr (Traced) {
        if (trace) trace->reset();
    }

    // ========================================================================
    // STEP 1: Build evaluation context
    // ========================================================================
    EvalContext ctx;
    populateContext(ctx, board);

    // ========================================================================
    // STEP 2: Evaluate individual terms
    // ========================================================================

    // Material (doesn't need context)
    const Material& material = board.material();
    Score materialDiff = material.value(WHITE) - material.value(BLACK);

    // PST (doesn't need context)
    const MgEgScore& pstScore = board.pstScore();
    Score pstValue = pstScore.mg;  // Simplified (no interpolation)

    // Pawns (uses context for passed pawns, isolated, etc.)
    Score pawnScore = evaluatePawns(board, ctx);

    // Mobility (uses context - BIG WIN)
    int mobilityValue = evaluateMobility(board, ctx);
    Score mobilityScore(mobilityValue);

    // King safety (uses context)
    Score whiteKingSafety = evaluateKingSafetySpine(board, ctx, WHITE);
    Score blackKingSafety = evaluateKingSafetySpine(board, ctx, BLACK);
    Score kingSafetyScore = whiteKingSafety - blackKingSafety;

    // Threats (NEW TERM - uses context)
    int threatValue = evaluateThreats(board, ctx);
    Score threatScore(threatValue);

    // ========================================================================
    // STEP 3: Trace individual terms
    // ========================================================================
    if constexpr (Traced) {
        if (trace) {
            trace->material = materialDiff;
            trace->pst = pstValue;
            trace->pawns = pawnScore;  // Aggregate pawn score
            trace->mobility = mobilityScore;
            trace->kingSafety = kingSafetyScore;
            trace->threats = threatScore;
        }
    }

    // ========================================================================
    // STEP 4: Combine and return from side-to-move perspective
    // ========================================================================
    Score totalWhite = materialDiff + pstValue + pawnScore +
                       mobilityScore + kingSafetyScore + threatScore;

    Color sideToMove = board.sideToMove();
    if (sideToMove == WHITE) {
        return totalWhite;
    } else {
        return -totalWhite;
    }
}
```

### Example 3: Helper Function for Pawn Evaluation

```cpp
/**
 * Evaluate all pawn structure features using context.
 *
 * @param board Position
 * @param ctx Precomputed attack context
 * @return Pawn structure score (from white's perspective)
 */
Score evaluatePawns(const Board& board, const EvalContext& ctx) {
    // Get pawn structure from cache (unchanged)
    uint64_t pawnKey = board.pawnZobristKey();
    PawnEntry* pawnEntry = g_pawnStructure.probe(pawnKey);

    // ... cache probe logic (same as before) ...

    // Passed pawns: uses context
    Bitboard whitePassedPawns = pawnEntry->passedPawns[WHITE];
    Bitboard blackPassedPawns = pawnEntry->passedPawns[BLACK];

    int passedPawnValue = 0;
    passedPawnValue += evaluatePassedPawns(board, ctx, WHITE, whitePassedPawns);
    passedPawnValue -= evaluatePassedPawns(board, ctx, BLACK, blackPassedPawns);

    // Isolated pawns: doesn't need context (cached)
    int isolatedPawnValue = /* ... */;

    // Doubled pawns: doesn't need context (cached)
    int doubledPawnValue = /* ... */;

    // Pawn islands: doesn't need context (cached)
    int islandValue = /* ... */;

    return Score(passedPawnValue + isolatedPawnValue + doubledPawnValue + islandValue);
}
```

---

## Final Notes for Implementers

### Prime Directives

1. **Bench must be identical until Phase E1** - If nodes diverge, evaluation has a bug
2. **Pass context by const reference** - Evaluation terms read context, never modify
3. **Stack-allocate context** - No heap allocation, no global state (LazySMP ready)
4. **Test after every commit** - SPRT validate on OpenBench
5. **UCI-gate all changes** - `EvalUseSpine` option (historical; removed after Phase E2) allowed parallel development

### When in Doubt

- **Ask:** Does this change affect move ordering? (If yes, bench will change)
- **Check:** Does this introduce shared state? (If yes, LazySMP breaks)
- **Verify:** Does bench match baseline? (If no, debug before proceeding)
- **Measure:** Is NPS improving? (If no, optimization isn't working)

### Resources

- **Chess Programming Wiki:** https://www.chessprogramming.org/Evaluation
- **Stockfish Evaluation:** Study for patterns (but don't copy!)
- **Perseus Engine:** Inspiration for multi-attack tracking (linked in plan)
- **SeaJay Discord:** Ask questions if stuck

---

**Good luck! This refactor will significantly improve SeaJay's performance and maintainability.**
