# Draw Detection Edge Cases and Performance Benchmarks

## Critical Edge Cases for Stage 9b

### 1. Zobrist Hash Considerations

#### Castling Rights in Hash
```cpp
// Test: Position identical except for castling rights
// These must have DIFFERENT Zobrist hashes:
Position A: "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"
Position B: "r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1"

// Validation:
// After Ke2-Ke1-Ke2, position A should NOT repeat (rights lost)
// Position B could repeat after same moves
```

#### En Passant Square Handling
```cpp
// Test: En passant "phantom" square
// Position after e2-e4:
"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"

// After Black plays a6:
"rnbqkbnr/1ppppppp/p7/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2"

// These positions are DIFFERENT even though pieces are same
// The en passant square e3 must be in the Zobrist hash
```

#### Known Zobrist Collision Test
```cpp
// Use 64-bit Zobrist to minimize collisions
// Test with millions of positions for collision rate
// Expected: < 1 collision per billion positions with good PRNG

// Stress test:
for (int i = 0; i < 1000000; ++i) {
    // Generate random legal position
    // Store hash
    // Check for collisions
}
```

### 2. Move Counter Edge Cases

#### Fifty-Move Rule Boundary
```cpp
// Critical: Rule triggers at halfmove_clock >= 100, not > 100
// Test positions:
"8/8/8/8/3K4/8/3k4/8 w - - 99 50"  // NOT a draw
"8/8/8/8/3K4/8/3k4/8 w - - 100 50" // IS a draw

// After any non-pawn, non-capture move from 99:
// Should immediately be draw
```

#### High Starting Move Counts
```cpp
// Position from late endgame:
"8/8/8/8/3K4/8/3k4/8 w - - 85 142"

// Verify:
// - 15 more non-pawn/capture moves = draw
// - Move counter continues correctly
// - Full move number (142) doesn't affect fifty-move rule
```

### 3. Tournament Rule Compliance

#### FIDE Article 9.2 (Threefold Repetition)
```cpp
// "The same position has occurred three times with same player to move"
// Key: Same castling rights, same en passant possibilities

// Test: Claim during opponent's time
// In tournament play, can claim BEFORE making move that causes repetition
// Our engine should detect this for "claim draw" UCI extension
```

#### FIDE Article 9.3 (Fifty-Move Rule)
```cpp
// "Fifty moves by each player without pawn move or capture"
// Note: It's 50 FULL moves = 100 halfmoves

// Special: 75-move rule (automatic)
// At 150 halfmoves, arbiter can declare draw even without claim
```

#### Dead Position (Article 5.2.2)
```cpp
// "Position where neither player can checkmate by any legal sequence"
// More than just insufficient material:

// Example: Blocked pawn structure with bishops
"8/p7/P7/8/2b5/8/3B4/8 w - - 0 1"
// Pawns blocked, bishops on same color = dead position
```

### 4. Performance Benchmarks

#### Draw Detection Overhead Targets
```cpp
// Acceptable performance impact:
// - Repetition check: < 0.5% of search time
// - Fifty-move check: < 0.1% of search time  
// - Insufficient material: < 0.1% of search time
// - Total draw detection: < 1% of search time

// Benchmark test:
// Position: Middlegame with 30+ pieces
// Search: Depth 10
// Measure: Time with/without draw detection
// Target: < 1% slowdown
```

#### Memory Usage
```cpp
// Position history storage:
// - Game history: Fixed array of 1024 positions (enough for longest game)
// - Search stack: Array of MAX_PLY (128) positions
// - Each position: 8 bytes (Zobrist hash)
// - Total: ~9KB for draw detection

// Optimization: Use circular buffer for game history
```

### 5. Common Implementation Bugs

#### Bug 1: Repetition Count Off-by-One
```cpp
// WRONG:
if (repetition_count >= 3) return true;  // Counts current position twice

// CORRECT:
if (repetition_count >= 2) return true;  // Current + 2 previous = threefold
```

#### Bug 2: Not Resetting Search Repetition Table
```cpp
// WRONG:
void search() {
    // Search without clearing history
}

// CORRECT:
void search() {
    search_history.clear();  // Reset for each search
    // But keep game_history!
}
```

#### Bug 3: Wrong En Passant Hash
```cpp
// WRONG:
if (en_passant_square != NO_SQUARE) {
    hash ^= zobrist_ep[en_passant_square];  // Always hash
}

// CORRECT:
if (en_passant_square != NO_SQUARE && can_capture_ep()) {
    hash ^= zobrist_ep[file_of(en_passant_square)];  // Only if capturable
}
```

#### Bug 4: Fifty-Move Reset
```cpp
// WRONG:
if (is_capture(move)) halfmove_clock = 0;  // Doesn't reset on pawn moves

// CORRECT:
if (is_capture(move) || piece_type(moved_piece) == PAWN) {
    halfmove_clock = 0;
}
```

#### Bug 5: Search Draw Score
```cpp
// WRONG:
if (is_draw()) return 0;  // Doesn't consider contempt

// CORRECT:
if (is_draw()) {
    // Could add contempt factor for tournament play
    return draw_score[ply & 1];  // Can vary by ply for search stability
}
```

### 6. Perft-Style Draw Validation

#### Draw Perft Extension
```cpp
// Modified perft that counts draws at each depth:
struct PerftDrawResult {
    uint64_t nodes;
    uint64_t draws_by_repetition;
    uint64_t draws_by_fifty_move;
    uint64_t draws_by_insufficient;
    uint64_t draws_by_stalemate;
};

// Test position with known draw counts:
// "8/8/8/8/3K4/8/3k4/8 w - - 98 49"
// Depth 3: X nodes, Y fifty-move draws
```

### 7. Integration Test Scenarios

#### Scenario 1: Time Pressure Draw Claims
```cpp
// UCI extension for draw claims:
// "position fen ... moves ..."
// "go wtime 1000 btime 1000"  // 1 second each
// Engine should claim draw immediately if available
```

#### Scenario 2: Tablebase Integration
```cpp
// When using tablebases:
// - Tablebase draw takes precedence
// - But still check fifty-move for DTZ accuracy
// - Insufficient material redundant with TB
```

#### Scenario 3: Multi-PV with Draws
```cpp
// "go multipv 3"
// If position is drawn:
// - All PVs should show 0 score
// - No crash or undefined behavior
```

### 8. Performance Profiling Points

```cpp
// Key functions to profile:
1. isThreefoldRepetition()  // Should use hash table lookup
2. updateRepetitionTable()   // Should be O(1) insertion
3. isInsufficientMaterial()  // Should be simple bit checks
4. isFiftyMoveDraw()         // Should be single comparison

// Target performance:
// 10M positions/second with full draw detection
// < 50 CPU cycles per draw check
```

### 9. Statistical Validation

```bash
# SPRT test specifically for draw detection:
./fast-chess -engine cmd=seajay_new -engine cmd=seajay_old \
    -openings file=drawish_positions.epd \
    -each tc=10+0.1 -rounds 10000 \
    -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05

# Expected: No Elo loss from draw detection
# Possibly small Elo gain from avoiding bad draws
```

### 10. Debug Output Format

```cpp
// Recommended debug output for draw detection:
info string Draw Detection Debug:
info string   Position Hash: 0x123456789ABCDEF0
info string   Repetition Count: 2 (including current)
info string   Previous Occurrences: ply 15, ply 23
info string   Fifty-Move Clock: 45
info string   Material: WK+WR vs BK+BR (sufficient)
info string   Draw Status: NOT_DRAW
```

## Final Validation Checklist

- [ ] All 30 test positions pass Stockfish validation
- [ ] Perft with draw detection matches Stockfish perft
- [ ] Performance overhead < 1% in middlegame positions
- [ ] Memory usage < 10KB for draw detection
- [ ] No crashes in 1M random positions
- [ ] SPRT shows no Elo loss
- [ ] UCI draw claiming works correctly
- [ ] Search returns score 0 for all draw types
- [ ] Draw detection works at all search depths
- [ ] Repetition table correctly handles game/search boundary