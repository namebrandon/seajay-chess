# Bug #013 Fix: Illegal Moves in PV

## Problem Summary
The engine outputs illegal moves in the PV because corrupted or invalid moves from the transposition table are not validated before use.

## Root Cause
1. TT stores moves as raw `uint16_t` without validation
2. Hash collisions can cause moves from different positions to be retrieved
3. No legality check when extracting TT moves
4. `info.bestMove` can contain illegal moves that get printed in PV

## Recommended Fix

### 1. Add Move Validation When Retrieving from TT

In `/workspace/src/search/negamax.cpp`, around line 285, modify the TT move extraction:

```cpp
// Sub-phase 4E: Extract TT move for ordering
// Even if depth is insufficient, we can still use the move
ttMove = static_cast<Move>(ttEntry->move);
if (ttMove != NO_MOVE) {
    // CRITICAL: Validate that the TT move is legal in current position
    MoveList legalMoves;
    MoveGenerator::generateAllMoves(board, legalMoves);
    
    bool isLegal = false;
    for (Move m : legalMoves) {
        if (m == ttMove) {
            isLegal = true;
            break;
        }
    }
    
    if (isLegal) {
        info.ttMoveHits++;
    } else {
        // TT move is illegal - likely from hash collision
        ttMove = NO_MOVE;
        info.ttCollisions++; // Track this as a collision
    }
}
```

### 2. Validate bestMove Before PV Output

In `/workspace/src/search/negamax.cpp`, modify lines 948-949 and 1038-1039:

```cpp
// Output principal variation (just the best move for now)
if (info.bestMove != NO_MOVE) {
    // Validate move is legal before outputting
    MoveList legalMoves;
    MoveGenerator::generateAllMoves(board, legalMoves);
    
    bool isLegal = false;
    for (Move m : legalMoves) {
        if (m == info.bestMove) {
            isLegal = true;
            break;
        }
    }
    
    if (isLegal) {
        std::cout << " pv " << SafeMoveExecutor::moveToString(info.bestMove);
    } else {
        // Log error but don't output illegal move
        std::cerr << "WARNING: Illegal bestMove detected: " 
                  << SafeMoveExecutor::moveToString(info.bestMove) 
                  << " in position: " << board.toFEN() << std::endl;
    }
}
```

### 3. Improve TT Key Verification (Optional but Recommended)

Add stronger key verification to reduce collisions:

```cpp
// In transposition_table.h, change key storage:
struct alignas(16) TTEntry {
    uint64_t key64;      // Store full 64-bit key instead of 32-bit
    uint16_t move;       // Best move from this position
    // ... rest of struct
};

// In transposition_table.cpp probe():
TTEntry* TranspositionTable::probe(Hash key) {
    if (!m_enabled) return nullptr;
    
    m_stats.probes++;
    
    size_t idx = index(key);
    TTEntry* entry = &m_entries[idx];
    
    // Use full 64-bit key comparison
    if (entry->key64 == key) {
        m_stats.hits++;
        return entry;
    }
    
    return nullptr;
}
```

### 4. Add PV Extraction from TT (Proper Solution)

Instead of just outputting `info.bestMove`, implement proper PV extraction:

```cpp
void extractPV(Board& board, TranspositionTable& tt, std::vector<Move>& pv, int maxDepth) {
    pv.clear();
    std::set<Hash> visitedPositions; // Prevent cycles
    
    Board tempBoard = board; // Make a copy
    
    for (int i = 0; i < maxDepth; ++i) {
        Hash key = tempBoard.zobristKey();
        
        // Check for repetition
        if (visitedPositions.count(key)) break;
        visitedPositions.insert(key);
        
        // Probe TT
        TTEntry* entry = tt.probe(key);
        if (!entry || entry->move == NO_MOVE) break;
        
        Move ttMove = static_cast<Move>(entry->move);
        
        // Validate move is legal
        MoveList legalMoves;
        MoveGenerator::generateAllMoves(tempBoard, legalMoves);
        
        bool found = false;
        for (Move m : legalMoves) {
            if (m == ttMove) {
                found = true;
                break;
            }
        }
        
        if (!found) break; // Invalid move in TT
        
        pv.push_back(ttMove);
        
        // Make the move
        UndoInfo undo;
        tempBoard.makeMove(ttMove, undo);
    }
}
```

## Testing the Fix

1. **Test with problem position**: Run the position that triggered the bug and verify no illegal moves appear in PV
2. **Perft validation**: Ensure move generation still passes all perft tests
3. **Performance check**: Measure impact of move validation on NPS
4. **Stress test**: Run long searches to verify no illegal moves appear

## Prevention

1. Always validate TT moves before use
2. Consider using full 64-bit keys to reduce collision probability
3. Implement proper PV extraction from TT
4. Add debug assertions to catch illegal moves early
5. Log TT collisions for monitoring

## Alternative Quick Fix (Minimal Change)

If you need a quick fix with minimal code change, just prevent outputting invalid moves:

```cpp
// In negamax.cpp, lines 948-949:
if (info.bestMove != NO_MOVE) {
    // Quick sanity check - at least validate squares are in range
    Square from = moveFrom(info.bestMove);
    Square to = moveTo(info.bestMove);
    if (from < 64 && to < 64 && from != to) {
        std::cout << " pv " << SafeMoveExecutor::moveToString(info.bestMove);
    }
}
```

This won't fix the root cause but will prevent obviously corrupted moves from being displayed.