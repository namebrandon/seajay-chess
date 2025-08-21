# Stage 21: Null Move Pruning Implementation Plan

**Feature Branch:** `feature/20250820-null-move-pruning`  
**Target ELO Gain:** +55-75 ELO total  
**Author:** SeaJay Development Team  
**Created:** August 21, 2025  
**Status:** PLANNING

## Overview

Null move pruning is a powerful forward pruning technique where we give the opponent an extra move (pass our turn) and search with reduced depth. If the opponent still can't beat our position, we can prune the current position as it's likely very good for us.

## Critical Process Requirements

### ⚠️ MANDATORY: OpenBench Testing Gate

**AFTER EACH PHASE:**
1. Complete implementation
2. Run `./bin/seajay bench` to get node count
3. Commit with message format: `feat: [Description] (Phase XX) - bench [node-count]`
4. Push to feature branch
5. **🛑 FULL STOP - DO NOT PROCEED** 
6. Human runs OpenBench test
7. Wait for test completion (may take hours)
8. Document results in this file
9. Only proceed to next phase after human confirmation

**NO EXCEPTIONS** - Even Phase A1 with 0 ELO expected must be tested.

## Phase A1: Infrastructure and Search Stack

### Objectives
- Add search stack infrastructure to track null moves
- Implement makeNullMove/unmakeNullMove in Board class
- Add null move statistics tracking
- Add UCI option (disabled by default initially)

### Implementation Tasks

1. **Add SearchStack to SearchInfo** (`src/search/search_info.h`):
```cpp
struct SearchStackEntry {
    Move move = Move::none();
    bool wasNullMove = false;
    int staticEval = 0;
    int moveCount = 0;
};

class SearchInfo {
    // Add:
    SearchStackEntry stack[MAX_PLY];
    
    bool wasNullMove(int ply) const {
        return ply >= 0 && stack[ply].wasNullMove;
    }
};
```

2. **Implement Null Move in Board** (`src/core/board.h/cpp`):
```cpp
void makeNullMove() {
    // Save state
    pushState();
    
    // Switch side
    m_sideToMove = opposite(m_sideToMove);
    m_zobristKey ^= Zobrist::side();
    
    // Clear en passant
    if (m_gameState.enPassantSquare != NO_SQUARE) {
        m_zobristKey ^= Zobrist::enPassant(fileOf(m_gameState.enPassantSquare));
        m_gameState.enPassantSquare = NO_SQUARE;
    }
    
    // Increment counters
    m_gameState.halfMoveClock++;
    m_ply++;
}

void unmakeNullMove() {
    m_ply--;
    popState();
    m_sideToMove = opposite(m_sideToMove);
}
```

3. **Add Statistics Tracking** (`src/search/search_data.h`):
```cpp
struct NullMoveStats {
    uint64_t attempts = 0;
    uint64_t cutoffs = 0;
    uint64_t zugzwangAvoids = 0;
    uint64_t verificationFails = 0;
    
    double cutoffRate() const {
        return attempts > 0 ? (100.0 * cutoffs / attempts) : 0.0;
    }
};
```

4. **Add UCI Option** (`src/uci/uci.cpp`):
```cpp
options["UseNullMove"] = Option(false);  // Start disabled for A1
```

### Expected Outcome
- **ELO Change:** 0 (infrastructure only)
- **Validation:** Code compiles, tests pass, no behavior change

### Test Command
```bash
./build.sh
./bin/seajay bench
# Record node count for commit message
```

### Commit Format
```
feat: Add null move infrastructure and search stack (Phase A1) - bench [NODE_COUNT]

- Added SearchStackEntry to track null moves in search
- Implemented makeNullMove/unmakeNullMove in Board class  
- Added null move statistics tracking
- Added UseNullMove UCI option (disabled by default)
- No functional change expected (0 ELO)
```

### Phase A1 Status
- [x] Implementation complete
- [x] Bench node count: 19191913
- [x] Committed with bench (commit f91a0f5)
- [ ] OpenBench test submitted
- [ ] Test results: _______
- [ ] Human approval to proceed

---

## Phase A2: Basic Null Move Implementation

### Prerequisites
- Phase A1 complete and tested
- No regression confirmed via OpenBench

### Objectives
- Implement basic null move with fixed R=2 reduction
- Simple zugzwang detection (non-pawn material threshold)
- Enable UCI option by default
- Proper mate score handling

### Implementation Tasks

1. **Add Null Move Search** (`src/search/negamax.cpp`):

Location: After TT probe (~line 313), before move generation

```cpp
// Null move pruning
constexpr int NULL_MOVE_REDUCTION = 2;
constexpr eval::Score ZUGZWANG_THRESHOLD = eval::Score(ROOK_VALUE + BISHOP_VALUE);

bool canDoNull = !isInCheck 
    && depth >= 3
    && !searchInfo.wasNullMove(ply - 1)
    && board.nonPawnMaterial(board.sideToMove()) > ZUGZWANG_THRESHOLD
    && abs(beta) < eval::MATE_BOUND - MAX_PLY;

if (canDoNull && options["UseNullMove"]) {
    info.nullMoveStats.attempts++;
    
    // Make null move
    board.makeNullMove();
    searchInfo.stack[ply].wasNullMove = true;
    searchInfo.stack[ply].move = Move::none();
    
    // Search with reduction (null window)
    eval::Score nullScore = -negamax(
        board, 
        depth - 1 - NULL_MOVE_REDUCTION,
        ply + 1,
        -beta,
        -beta + eval::Score(1),
        searchInfo,
        info,
        limits,
        tt
    );
    
    // Unmake null move
    board.unmakeNullMove();
    searchInfo.stack[ply].wasNullMove = false;
    
    // Check for cutoff
    if (nullScore >= beta) {
        info.nullMoveStats.cutoffs++;
        
        // Don't return mate scores from null search
        if (abs(nullScore) < eval::MATE_BOUND - MAX_PLY) {
            return nullScore;
        }
        return beta;
    }
} else if (!canDoNull && board.nonPawnMaterial(board.sideToMove()) <= ZUGZWANG_THRESHOLD) {
    info.nullMoveStats.zugzwangAvoids++;
}
```

2. **Enable UCI Option** (`src/uci/uci.cpp`):
```cpp
options["UseNullMove"] = Option(true);  // Now enabled by default
```

3. **Add nonPawnMaterial Method** (`src/core/board.h/cpp`):
```cpp
eval::Score nonPawnMaterial(Color c) const {
    eval::Score value = 0;
    value += popcount(pieces(c, KNIGHT)) * KNIGHT_VALUE;
    value += popcount(pieces(c, BISHOP)) * BISHOP_VALUE;
    value += popcount(pieces(c, ROOK)) * ROOK_VALUE;
    value += popcount(pieces(c, QUEEN)) * QUEEN_VALUE;
    return value;
}
```

### Expected Outcome
- **ELO Change:** +35-45 ELO
- **Node Reduction:** ~15-20% fewer nodes searched
- **Validation:** Pass test positions, no zugzwang failures

### Test Positions
```cpp
// Position 1: Basic tactical position (null move should help)
"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"

// Position 2: Zugzwang position (null move should be avoided)
"8/8/p1p5/1p5p/1P5p/8/PPP2K1p/4R1rk w - - 0 1"

// Position 3: Endgame position
"8/8/8/8/4k3/8/4P3/4K3 w - - 0 1"
```

### Commit Format
```
feat: Implement basic null move pruning with R=2 (Phase A2) - bench [NODE_COUNT]

- Fixed reduction R=2 for all depths
- Zugzwang detection via non-pawn material threshold
- Proper mate score handling
- Expected: +35-45 ELO gain
```

### Phase A2 Status
- [ ] Implementation complete
- [ ] Bench node count: _______
- [ ] Committed with bench
- [ ] OpenBench test submitted
- [ ] Test results: _______
- [ ] Human approval to proceed

---

## Phase A3: Adaptive Reduction and Verification

### Prerequisites
- Phase A2 complete and tested
- Positive ELO gain confirmed

### Objectives
- Implement adaptive reduction based on depth
- Add verification search for very deep searches
- Improve zugzwang detection

### Implementation Tasks

1. **Adaptive Reduction Formula** (`src/search/negamax.cpp`):
```cpp
// Replace fixed reduction with adaptive
int nullMoveReduction = 2;
if (depth >= 8) nullMoveReduction++;
if (depth >= 15) nullMoveReduction++;

// Optional: More aggressive when eval >> beta
if (staticEval - beta >= 200) {
    nullMoveReduction++;
}

// Ensure minimum depth
nullMoveReduction = std::min(nullMoveReduction, depth - 1);
```

2. **Verification Search** (for depth >= 12):
```cpp
if (nullScore >= beta) {
    info.nullMoveStats.cutoffs++;
    
    // Verification for deep searches
    if (depth >= 12 && abs(nullScore) < eval::MATE_BOUND - MAX_PLY) {
        // Search with reduced depth to verify
        eval::Score verifyScore = negamax(
            board,
            depth - nullMoveReduction - 1,
            ply,
            beta - eval::Score(1),
            beta,
            searchInfo,
            info,
            limits,
            tt
        );
        
        if (verifyScore >= beta) {
            return nullScore;
        }
        info.nullMoveStats.verificationFails++;
    } else if (abs(nullScore) < eval::MATE_BOUND - MAX_PLY) {
        return nullScore;
    } else {
        return beta;
    }
}
```

3. **Improved Zugzwang Detection**:
```cpp
bool inZugzwang = board.nonPawnMaterial(board.sideToMove()) <= ROOK_VALUE
    || (board.pieceCount(board.sideToMove(), KNIGHT) + 
        board.pieceCount(board.sideToMove(), BISHOP) == 0);

bool canDoNull = !isInCheck 
    && depth >= 3
    && !searchInfo.wasNullMove(ply - 1)
    && !inZugzwang
    && abs(beta) < eval::MATE_BOUND - MAX_PLY;
```

### Expected Outcome
- **ELO Change:** +15-20 ELO (cumulative +50-65)
- **Better endgame play:** Fewer zugzwang failures
- **Deeper searches:** More aggressive pruning at high depths

### Commit Format
```
feat: Add adaptive null move reduction and verification (Phase A3) - bench [NODE_COUNT]

- Adaptive R based on depth (2-4)
- Verification search for depth >= 12
- Improved zugzwang detection
- Expected: +15-20 additional ELO
```

### Phase A3 Status
- [ ] Implementation complete
- [ ] Bench node count: _______
- [ ] Committed with bench
- [ ] OpenBench test submitted
- [ ] Test results: _______
- [ ] Human approval to proceed

---

## Phase A4: Tuning and Static Null Move

### Prerequisites
- Phase A3 complete and tested
- Core null move working well

### Objectives
- Fine-tune parameters via testing
- Add static null move pruning (reverse futility)
- Consider double null move allowance

### Implementation Tasks

1. **Static Null Move Pruning** (before expensive null move):
```cpp
// Static null move pruning (reverse futility)
if (depth <= 3 && !isInCheck && abs(beta) < eval::MATE_BOUND - MAX_PLY) {
    eval::Score staticEval = evaluate(board);
    searchInfo.stack[ply].staticEval = staticEval;
    
    // Margin based on depth
    eval::Score margin = eval::Score(120 * depth);
    
    if (staticEval - margin >= beta) {
        info.nullMoveStats.staticCutoffs++;
        return staticEval;
    }
}
```

2. **Double Null Move** (optional):
```cpp
// Allow null if previous wasn't null, or if 2 moves ago was null
bool allowNull = !isInCheck && 
    (!searchInfo.wasNullMove(ply - 1) || 
     (ply >= 2 && searchInfo.wasNullMove(ply - 2)));
```

3. **Tunable Parameters**:
```cpp
// Make these UCI options for tuning
options["NullMoveBaseReduction"] = Option(2, 2, 4);
options["NullMoveDepthDivisor"] = Option(8, 4, 12);
options["NullMoveStaticMargin"] = Option(120, 80, 200);
options["NullMoveVerifyDepth"] = Option(12, 8, 16);
```

### Expected Outcome
- **ELO Change:** +5-10 ELO (cumulative +55-75)
- **Fine-tuned parameters:** Optimal for SeaJay's eval
- **Static pruning:** Faster shallow searches

### Commit Format
```
feat: Add static null move and tuning parameters (Phase A4) - bench [NODE_COUNT]

- Static null move pruning for depth <= 3
- Tunable parameters via UCI
- Optional double null move
- Expected: +5-10 additional ELO
```

### Phase A4 Status
- [ ] Implementation complete
- [ ] Bench node count: _______
- [ ] Committed with bench
- [ ] OpenBench test submitted
- [ ] Test results: _______
- [ ] Feature complete

---

## Testing Protocol

### After Each Phase

1. **Build and Bench**:
```bash
./build.sh
./bin/seajay bench
# Note the node count for commit
```

2. **Run Test Positions**:
```bash
./bin/seajay << EOF
position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1
go depth 10
position fen 8/8/p1p5/1p5p/1P5p/8/PPP2K1p/4R1rk w - - 0 1
go depth 10
quit
EOF
```

3. **Verify Statistics**:
Check that null move statistics are being tracked and reported in UCI info output.

### OpenBench Configuration

```
Dev Branch: feature/20250820-null-move-pruning
Base Branch: main
Time Control: 8+0.08
Book: 8moves_v3.pgn
Concurrency: 4
SPRT: elo0=0 elo1=5 alpha=0.05 beta=0.05
```

---

## Risk Mitigation

### Critical Safety Checks

1. **Never null move**:
   - When in check
   - After another null move
   - In obvious zugzwang positions
   - At very low depths (< 3)

2. **Always verify**:
   - Mate scores are handled correctly
   - TT is not polluted with null positions
   - Board state is properly restored

3. **Monitor for**:
   - Zugzwang failures in endgames
   - Excessive null move attempts
   - Search instability

---

## Expert Review Notes

Based on analysis of Stockfish, Ethereal, and other engines:

1. **Start simple** - Fixed R=2 is proven effective
2. **Search stack is critical** - Must track null moves properly
3. **TT pollution is a common bug** - Never store null searches
4. **Mate score safety** - Critical for correctness
5. **Zugzwang threshold** - 8 pawns of non-pawn material is standard

---

## Current Status

**Phase:** A1 COMPLETE - AWAITING OPENBENCH TEST  
**Branch:** feature/20250820-null-move-pruning  
**Last Updated:** August 21, 2025  
**Commit:** f91a0f5 (bench 19191913)

### Progress Tracking

- [x] Phase A1: Infrastructure (COMPLETE - awaiting test)
- [ ] Phase A2: Basic Implementation  
- [ ] Phase A3: Adaptive Reduction
- [ ] Phase A4: Tuning & Static

### Notes

- Each phase MUST be tested via OpenBench before proceeding
- Expected total gain: +55-75 ELO
- Implementation follows SeaJay's proven phased development model