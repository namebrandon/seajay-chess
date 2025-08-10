# Draw Detection Implementation Answers

## Specific Answers to Your Questions

### 1. Zobrist Hash Considerations

#### Q: Should test positions verify castling rights are in hash?
**A: YES, absolutely critical.**

```cpp
// Test position pair:
Position A: "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"
Position B: "r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1"

// After Ke2, both lose castling rights and become identical
// But their initial Zobrist hashes MUST be different

// Zobrist initialization must include:
uint64_t zobrist_castle[16];  // All 16 possible castling states
```

#### Q: How to test en passant square handling?
**A: Only hash en passant if capture is actually possible.**

```cpp
// FIDE rule: En passant only matters if there's an enemy pawn to capture
bool should_hash_ep(Square ep_square) {
    if (ep_square == NO_SQUARE) return false;
    
    // Check if any enemy pawn can actually capture
    Bitboard enemy_pawns = get_pawns(opponent);
    Bitboard ep_attackers = pawn_attacks(ep_square, opponent) & enemy_pawns;
    
    return ep_attackers != 0;
}

// Test case:
// "8/8/8/3pP3/8/8/8/8 w - d6 0 1"  // d6 IS relevant
// "8/8/8/4P3/8/8/8/8 w - d6 0 1"  // d6 NOT relevant (no Black pawn)
```

#### Q: Any known Zobrist collision positions?
**A: With 64-bit hashes, collisions are extremely rare.**

```cpp
// Collision probability: 1 in 2^32 after 2^32 positions
// For chess: Negligible in practice

// Test for collisions:
std::unordered_set<uint64_t> seen_hashes;
for (auto& test_position : massive_position_database) {
    uint64_t hash = compute_hash(test_position);
    if (seen_hashes.count(hash)) {
        // Found collision - verify positions are actually different
    }
    seen_hashes.insert(hash);
}
```

### 2. Move Counter Edge Cases

#### Q: Does fifty-move rule check >= 100 or > 99?
**A: Check >= 100 (greater than or equal to 100).**

```cpp
bool isFiftyMoveDraw() const {
    return halfmove_clock >= 100;
}

// Test positions:
// halfmove_clock = 99: NOT a draw
// halfmove_clock = 100: IS a draw
// halfmove_clock = 101: IS a draw
```

#### Q: How to handle positions starting at high move counts?
**A: Accept them as-is from FEN, continue counting normally.**

```cpp
// FEN: "8/8/8/8/3K4/8/3k4/8 w - - 85 142"
// This is legal - game at move 142, 85 halfmoves since pawn/capture

void setPosition(const std::string& fen) {
    // Parse halfmove_clock from FEN field 5
    // Parse fullmove_number from FEN field 6
    // No validation needed - trust the FEN
    
    // Continue counting from these values
    // Draw at halfmove_clock >= 100
}
```

### 3. Tournament Rules

#### Q: Any special cases from FIDE rules?
**A: Yes, several important ones:**

```cpp
// 1. Claim before move (Article 9.2):
// Player can claim draw BEFORE making the repetition move
// UCI doesn't support this, but good to know

// 2. 75-move automatic rule (Article 9.6.2):
// At 150 halfmoves, draw is automatic (no claim needed)
// We should implement this:
bool isAutomaticDraw() const {
    return halfmove_clock >= 150;  // 75 full moves
}

// 3. Dead position (Article 5.2.2):
// Neither side can checkmate by any legal sequence
// More complex than insufficient material
// Example: Blocked pawns + same-color bishops
```

#### Q: Tournament-specific draw conditions?
**A: Some tournaments have special rules:**

```cpp
// 1. No early draw offers (Sofia rules):
// Not relevant for engine

// 2. Armageddon (must have winner):
// Draw = Black wins
// Implement via contempt:
int draw_score = (armageddon_mode && color == BLACK) ? 100 : 0;

// 3. Rapid/Blitz dead position:
// Stricter enforcement in faster time controls
// Flag falls in dead position = draw (not loss)
```

#### Q: Time control considerations?
**A: Draw claims affect time management:**

```cpp
// 1. Instant draw claim saves time:
if (is_threefold_repetition()) {
    // Claim immediately, don't search
    return DRAW_SCORE;
}

// 2. Fifty-move approaching:
if (halfmove_clock > 90) {
    // Might affect time allocation
    // Less time needed if draw is forced
}

// 3. Insufficient material:
if (is_insufficient_material()) {
    // No point searching deeply
    return DRAW_SCORE;
}
```

### 4. Performance Benchmarks

#### Q: What's acceptable overhead for draw detection?
**A: Target < 1% total overhead in typical positions.**

```cpp
// Benchmark results from strong engines:
// Stockfish: ~0.3% overhead
// Komodo: ~0.5% overhead
// Leela: ~0.2% overhead (GPU helps)

// SeaJay targets:
// - Repetition detection: < 0.5% overhead
// - Fifty-move check: < 0.1% overhead
// - Insufficient material: < 0.1% overhead
// - Total: < 0.7% overhead

// Measurement method:
auto start = std::chrono::high_resolution_clock::now();
search_with_draw_detection(position, depth);
auto time_with = get_elapsed_ms(start);

start = std::chrono::high_resolution_clock::now();
search_without_draw_detection(position, depth);
auto time_without = get_elapsed_ms(start);

double overhead = 100.0 * (time_with - time_without) / time_without;
```

#### Q: How many positions should we test?
**A: Comprehensive test suite should have 50-100 positions.**

```cpp
// Minimum test set:
// - 10 threefold repetition cases
// - 10 fifty-move rule cases
// - 10 insufficient material cases
// - 5 stalemate cases
// - 5 complex multi-draw cases
// - 10 search integration cases
// Total: 50 positions minimum

// Extended test set:
// - 20 repetition variants (castling, ep, etc.)
// - 20 fifty-move edge cases
// - 20 insufficient material combinations
// - 10 tournament rule scenarios
// - 20 search behavior tests
// - 10 performance stress tests
// Total: 100 positions recommended
```

#### Q: Perft-style validation for draws?
**A: Yes, extend perft to count draw types.**

```cpp
struct DrawPerft {
    uint64_t nodes = 0;
    uint64_t repetitions = 0;
    uint64_t fifty_moves = 0;
    uint64_t insufficient = 0;
    uint64_t stalemates = 0;
    
    void perft(Board& board, int depth) {
        if (depth == 0) {
            nodes++;
            if (board.isThreefoldRepetition()) repetitions++;
            if (board.isFiftyMoveDraw()) fifty_moves++;
            if (board.isInsufficientMaterial()) insufficient++;
            if (board.isStalemate()) stalemates++;
            return;
        }
        
        MoveList moves;
        board.generateMoves(moves);
        for (Move m : moves) {
            board.makeMove(m);
            perft(board, depth - 1);
            board.unmakeMove(m);
        }
    }
};

// Test position with known draw counts:
// "8/8/8/8/3K4/8/3k4/8 w - - 98 49"
// Depth 3 should have specific draw counts to validate
```

## Summary of Key Implementation Points

1. **Zobrist Hash MUST Include:**
   - All piece positions
   - Side to move
   - Castling rights (all 16 states)
   - En passant square (only if capturable)

2. **Fifty-Move Rule:**
   - Triggers at halfmove_clock >= 100
   - Reset on ANY pawn move or capture
   - Consider 75-move automatic rule (>= 150)

3. **Repetition Detection:**
   - Need both game history and search stack
   - Current position + 2 previous = threefold
   - Must match castling and en passant exactly

4. **Performance Targets:**
   - < 1% total overhead
   - Use hash table for O(1) repetition lookup
   - Simple bit operations for insufficient material

5. **Testing Requirements:**
   - 50-100 test positions minimum
   - All validated against Stockfish
   - Include edge cases and tournament scenarios
   - Performance benchmarks on typical positions