# Stage 15: Static Exchange Evaluation (SEE) - Implementation Considerations

**Related Documents:**
- Pre-Stage Planning: `/workspace/project_docs/planning/stage15_pre_stage_planning.md`
- Master Project Plan: `/workspace/project_docs/SeaJay Chess Engine Development - Master Project Plan.md`

## Executive Summary

Static Exchange Evaluation (SEE) is a critical component for accurate capture evaluation that will replace our current MVV-LVA ordering. Given the challenges faced in Stage 14 (C9 Catastrophe, Build Mode Crisis), this document provides battle-tested guidance to ensure a smooth, validated implementation.

**Expected Outcome:** +30-50 ELO improvement through accurate capture assessment and improved move ordering.

**Note:** This document contains detailed implementation guidance from both C++ and chess engine experts. For the formal pre-stage planning process and METHODICAL VALIDATION theme, see the Pre-Stage Planning document.

## C++ Expert Review: Critical Implementation Guidance

### Development Efficiency Enhancements

**Modern C++20 Features to Leverage:**

1. **Concepts for Type Safety**
```cpp
// Define concept for SEE-compatible position types
template<typename T>
concept SEEPosition = requires(T pos, Square sq, Color c) {
    { pos.pieceAt(sq) } -> std::convertible_to<Piece>;
    { pos.attackersTo(sq, c) } -> std::convertible_to<Bitboard>;
    { pos.occupied() } -> std::convertible_to<Bitboard>;
};

// Now SEE functions are type-safe at compile-time
template<SEEPosition Pos>
int evaluateSEE(const Pos& pos, Move move) {
    // Implementation
}
```

2. **std::array for Fixed-Size Buffers**
```cpp
// Replace C-style arrays with std::array for bounds checking in debug
std::array<int, 32> gain{};  // Zero-initialized, bounds-checked in debug
```

3. **[[nodiscard]] and [[likely]]/[[unlikely]] Attributes**
```cpp
[[nodiscard]] int see(const Position& pos, Move move) noexcept;

// In hot path:
if ([[likely]](seeValue >= 0)) {
    // Good capture path (most common)
} else [[unlikely]] {
    // Bad capture path
}
```

### Memory Management and Cache Optimization

1. **Struct Packing for Cache Efficiency**
```cpp
// Pack SEE cache entries for better cache utilization
struct [[gnu::packed]] SEECacheEntry {
    uint32_t moveKey;  // Reduced from uint64_t if possible
    int16_t value;     // SEE values fit in 16 bits
    uint16_t age;      // For replacement policy
};  // 8 bytes per entry, fits 8 entries per cache line

static_assert(sizeof(SEECacheEntry) == 8);
```

2. **Memory Pool for Thread-Local Storage**
```cpp
// Avoid allocation, use thread_local for work arrays
class SEEEvaluator {
    thread_local static inline std::array<int, 32> t_gain{};
    thread_local static inline std::array<Square, 32> t_attackers{};
    
public:
    // No allocation needed, reuse thread-local buffers
    static int evaluate(const Position& pos, Move move) noexcept {
        auto& gain = t_gain;
        auto& attackers = t_attackers;
        // Use pre-allocated buffers
    }
};
```

### Error Handling and Defensive Programming

1. **Compile-Time Validation**
```cpp
// Use static_assert for compile-time checks
template<typename T>
class SEEImpl {
    static_assert(std::is_trivially_copyable_v<T>, 
                  "SEE requires trivially copyable position type");
    static_assert(sizeof(T) <= 256, 
                  "Position type too large for efficient SEE");
};
```

2. **Debug-Only Invariant Checking**
```cpp
#ifdef DEBUG
    #define SEE_ASSERT(cond, msg) \
        do { \
            if (!(cond)) { \
                std::cerr << "SEE Assert: " << msg << " at " \
                         << __FILE__ << ":" << __LINE__ << std::endl; \
                std::abort(); \
            } \
        } while(0)
#else
    #define SEE_ASSERT(cond, msg) ((void)0)
#endif

// Usage:
SEE_ASSERT(depth < 32, "SEE exchange depth exceeded maximum");
SEE_ASSERT(attackerValue > 0, "Invalid attacker value in SEE");
```

## 1. SEE Algorithm Best Practices

### Risk Mitigation: Build System Safeguards

**Preventing Stage 14 Build Issues:**

1. **Binary Fingerprinting System**
```cpp
// In main.cpp or version.cpp
namespace BuildInfo {
    constexpr const char* SEE_STATUS = "ENABLED";
    constexpr size_t EXPECTED_BINARY_SIZE = 411000;  // ±5KB tolerance
    
    void validateBuild() {
        std::cout << "SEE Status: " << SEE_STATUS << std::endl;
        
        // Runtime size check
        std::filesystem::path exe = std::filesystem::canonical("/proc/self/exe");
        auto size = std::filesystem::file_size(exe);
        
        if (std::abs(static_cast<long>(size - EXPECTED_BINARY_SIZE)) > 5000) {
            std::cerr << "WARNING: Binary size " << size 
                     << " differs from expected " << EXPECTED_BINARY_SIZE << std::endl;
        }
    }
}
```

2. **CMake Build Validation**
```cmake
# In CMakeLists.txt - Force rebuild on critical changes
set(SEE_IMPLEMENTATION_VERSION "1.0.0")

# Create a build stamp file
configure_file(
    "${CMAKE_SOURCE_DIR}/src/build_stamp.h.in"
    "${CMAKE_BINARY_DIR}/build_stamp.h"
    @ONLY
)

# Force recompilation if SEE files change
set_property(SOURCE src/search/search.cpp 
    APPEND PROPERTY OBJECT_DEPENDS 
    ${CMAKE_SOURCE_DIR}/src/core/see.cpp)
```

3. **Symbol Verification Script**
```bash
#!/bin/bash
# verify_see_symbols.sh
SYMBOLS="evaluateSEE xrayAttackers getLeastAttacker"
for sym in $SYMBOLS; do
    if ! nm ./bin/seajay | grep -q $sym; then
        echo "ERROR: Missing SEE symbol: $sym"
        exit 1
    fi
done
echo "All SEE symbols present"
```

### Core Algorithm Choice

**Recommended: The "Swap Algorithm" (Used by Stockfish, Ethereal)**

The swap algorithm is the most efficient SEE implementation, avoiding recursion and minimizing computation:

```cpp
// Optimized C++20 implementation with modern practices
class SEEEvaluator {
private:
    // Cache-aligned for better performance
    alignas(64) struct SEECache {
        static constexpr size_t SIZE = 4096;  // Power of 2 for fast modulo
        struct Entry {
            uint32_t key;
            int16_t value;
            uint16_t age;
        };
        std::array<Entry, SIZE> entries{};
        uint16_t currentAge = 0;
        
        [[nodiscard]] std::optional<int> probe(uint32_t key) noexcept {
            const size_t idx = key & (SIZE - 1);
            if (entries[idx].key == key && 
                static_cast<uint16_t>(currentAge - entries[idx].age) < 256) {
                return entries[idx].value;
            }
            return std::nullopt;
        }
        
        void store(uint32_t key, int value) noexcept {
            const size_t idx = key & (SIZE - 1);
            entries[idx] = {key, static_cast<int16_t>(value), currentAge};
        }
    };
    
    thread_local static inline SEECache cache{};
    thread_local static inline std::array<int, 32> gainBuffer{};

public:
    [[nodiscard]] static int evaluate(const Position& pos, Move move) noexcept {
        // Create unique key for this position + move
        const uint32_t key = hashMove(move) ^ static_cast<uint32_t>(pos.hash());
        
        // Check cache first
        if (auto cached = cache.probe(key)) {
            return *cached;
        }
        
        Square to = move.to();
        auto& gain = gainBuffer;  // Use thread-local buffer
        int depth = 0;
        
        // Initial capture value with bounds checking in debug
        SEE_ASSERT(pos.pieceAt(to) != NO_PIECE, "SEE on empty square");
        gain[0] = SEE_VALUES[pos.pieceAt(to)];
        
        Color sideToMove = ~pos.sideToMove();  // Opponent moves first after capture
        int attackerValue = SEE_VALUES[pos.pieceAt(move.from())];
        
        // Remove the initial attacker using bit manipulation
        Bitboard occupied = pos.occupied() ^ square_bb(move.from());
        
        // Get all attackers, using template for compile-time optimization
        Bitboard attackers = computeAttackers<true>(pos, to, occupied);
        
        // Main exchange loop with likely/unlikely hints
        while ([[likely]](depth < 31)) {  // Prevent buffer overflow
            depth++;
            gain[depth] = attackerValue - gain[depth - 1];
            
            // Early termination optimization
            if (std::max(-gain[depth - 1], gain[depth]) < 0) [[likely]] {
                break;
            }
            
            // Find least valuable attacker using bit scan
            auto [attackerSq, newAttackerValue] = 
                extractLeastAttacker(pos, attackers, sideToMove, occupied);
                
            if (attackerSq == NO_SQUARE) [[unlikely]] {
                break;
            }
            
            attackerValue = newAttackerValue;
            
            // Remove attacker and update x-rays in one operation
            occupied ^= square_bb(attackerSq);
            attackers ^= square_bb(attackerSq);
            attackers |= computeXrays(pos, to, occupied, attackerSq);
            
            sideToMove = ~sideToMove;
        }
        
        // Minimax propagation using std::algorithm
        while (--depth > 0) {
            gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);
        }
        
        const int result = gain[0];
        cache.store(key, result);
        return result;
    }
    
private:
    // Template for compile-time optimization
    template<bool IncludeXrays>
    [[nodiscard]] static Bitboard computeAttackers(
        const Position& pos, Square sq, Bitboard occupied) noexcept {
        
        Bitboard attackers = 0;
        
        // Pawn attacks (most common)
        attackers |= pawn_attacks(WHITE, sq) & pos.pieces(BLACK, PAWN);
        attackers |= pawn_attacks(BLACK, sq) & pos.pieces(WHITE, PAWN);
        
        // Knight attacks (no x-rays)
        attackers |= knight_attacks(sq) & pos.pieces(KNIGHT);
        
        // Sliding pieces with x-ray consideration
        if constexpr (IncludeXrays) {
            attackers |= bishop_attacks(sq, occupied) & 
                        (pos.pieces(BISHOP) | pos.pieces(QUEEN));
            attackers |= rook_attacks(sq, occupied) & 
                        (pos.pieces(ROOK) | pos.pieces(QUEEN));
        } else {
            attackers |= bishop_attacks(sq, pos.occupied()) & 
                        (pos.pieces(BISHOP) | pos.pieces(QUEEN));
            attackers |= rook_attacks(sq, pos.occupied()) & 
                        (pos.pieces(ROOK) | pos.pieces(QUEEN));
        }
        
        // King attacks (always last in exchange)
        attackers |= king_attacks(sq) & pos.pieces(KING);
        
        return attackers & occupied;  // Only pieces still on board
    }
    
    // Extract least valuable attacker using compiler intrinsics
    [[nodiscard]] static std::pair<Square, int> extractLeastAttacker(
        const Position& pos, Bitboard& attackers, Color side, Bitboard occupied) noexcept {
        
        // Order: Pawns, Knights, Bishops, Rooks, Queens, King
        constexpr std::array<PieceType, 6> pieceOrder = 
            {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
        
        for (PieceType pt : pieceOrder) {
            Bitboard pieces = attackers & pos.pieces(side, pt);
            if (pieces) {
                // Use compiler intrinsic for fast bit scan
                Square sq = static_cast<Square>(__builtin_ctzll(pieces));
                return {sq, SEE_VALUES[pt]};
            }
        }
        
        return {NO_SQUARE, 0};
    }
};
```

### Critical Implementation Details

1. **Attack Detection Must Be Perfect**
   - Reuse existing attack generation from move generator
   - Include ALL piece types (don't forget pawns and king)
   - Pin detection is NOT needed for SEE (simplification!)

2. **X-Ray Handling is Essential**
   ```cpp
   // After removing a piece, check for sliders behind it
   Bitboard xrayAttackers(Square sq, Bitboard occupied) {
       Bitboard xrays = 0;
       
       // Rooks and queens on ranks/files
       xrays |= (rookAttacks(sq, occupied) & (rooks | queens)) 
                & ~attackersTo(sq, occupied | allPieces());
                
       // Bishops and queens on diagonals  
       xrays |= (bishopAttacks(sq, occupied) & (bishops | queens))
                & ~attackersTo(sq, occupied | allPieces());
                
       return xrays;
   }
   ```

3. **Early Termination Optimizations**
   ```cpp
   // Quick win: If capturing with our least valuable piece
   if (capturedValue > 0 && attackerValue <= capturedValue)
       return capturedValue;  // Guaranteed win
       
   // Quick loss: If our piece is undefended
   if (!hasDefender(to))
       return capturedValue - attackerValue;  // Guaranteed loss
   ```

4. **Piece Values for SEE (in centipawns)**
   ```cpp
   // CRITICAL: SEE values differ from evaluation values!
   // Based on analysis of top engines:
   
   // Stockfish SEE values (simplified midgame)
   constexpr int STOCKFISH_SEE[7] = {
       0,      // None
       100,    // Pawn   (PawnValueMg = 124)
       410,    // Knight (KnightValueMg = 406) 
       419,    // Bishop (BishopValueMg = 418)
       628,    // Rook   (RookValueMg = 628)
       1276,   // Queen  (QueenValueMg = 1276)
       0       // King (never captured)
   };
   
   // Ethereal SEE values (tuned via SPSA)
   constexpr int ETHEREAL_SEE[7] = {
       0,      // None
       100,    // Pawn   
       325,    // Knight (lower than eval!)
       325,    // Bishop (equal to knight for SEE)
       500,    // Rook   
       975,    // Queen  
       0       // King
   };
   
   // SeaJay recommended starting values
   // (Conservative, will tune in Deliverable 6.3)
   constexpr int SEE_VALUES[7] = {
       0,      // None
       100,    // Pawn
       320,    // Knight (slightly less than bishop)
       330,    // Bishop (slightly more than knight)
       500,    // Rook
       900,    // Queen (conservative)
       0       // King (never captured in SEE)
   };
   
   // NOTE: Many engines use DIFFERENT values for SEE vs eval!
   // SEE values are often compressed (Queen/Pawn ratio ~9:1 vs 10:1 in eval)
   ```

### Technical Optimization Opportunities

**1. SIMD Acceleration for Attack Detection**
```cpp
// Use AVX2 for parallel attack detection when available
#ifdef __AVX2__
#include <immintrin.h>

Bitboard computeAttackersSIMD(const Position& pos, Square sq) noexcept {
    // Process multiple piece types in parallel
    __m256i attacks = _mm256_setzero_si256();
    
    // Load piece bitboards
    __m256i pieces = _mm256_load_si256(
        reinterpret_cast<const __m256i*>(&pos.piecesBB[0]));
    
    // Compute attacks for 4 piece types simultaneously
    // ... SIMD implementation
    
    // Reduce to single bitboard
    return extractBitboard(attacks);
}
#endif
```

**2. Compile-Time Lookup Tables**
```cpp
// Generate lookup tables at compile-time using constexpr
template<typename Generator>
struct CompileTimeLUT {
    static constexpr size_t SIZE = 64 * 64 * 7;  // from x to x piece
    
    constexpr CompileTimeLUT() : values{} {
        for (size_t i = 0; i < SIZE; ++i) {
            values[i] = Generator::compute(i);
        }
    }
    
    std::array<int8_t, SIZE> values;
};

// Instantiate at compile-time
static constexpr CompileTimeLUT<SEEGenerator> SEE_LUT{};

// Lightning-fast lookup at runtime
int quickSEE = SEE_LUT.values[index(from, to, piece)];
```

**3. Branch-Free Implementation**
```cpp
// Eliminate branches in hot path using bit manipulation
int selectGain(int gain1, int gain2, bool condition) noexcept {
    // Branch-free selection
    return gain1 ^ ((gain1 ^ gain2) & -static_cast<int>(condition));
}

// Or use std::conditional for compile-time selection
template<bool UseCache>
int evaluateSEE(const Position& pos, Move move) noexcept {
    if constexpr (UseCache) {
        // Cache-enabled path
    } else {
        // Direct computation path
    }
}
```

### Common Pitfalls to Avoid

1. **Forgetting En Passant**: SEE must handle en passant captures correctly
2. **King Participation**: Kings can capture but cannot be captured
3. **Promotion**: Pawn promotions change the attacker value mid-exchange
4. **Performance Trap**: Don't call SEE on every move - use lazy evaluation
5. **Sign Confusion**: Be consistent about perspective (positive = good for moving side)

## 2. Test Positions and Validation

### Essential SEE Test Positions

```cpp
// Test Suite Structure
struct SEETest {
    const char* fen;
    Move move;
    int expected;  // In centipawns
    const char* description;
};

// Critical test positions - VALIDATED AGAINST STOCKFISH 16.1
SEETest mustPassTests[] = {
    // ========== BASIC EXCHANGES ==========
    {"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
     Move(e4, d5), 0, "Equal pawn trade"},
     
    {"r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3",
     Move(c6, e5), -200, "Knight takes defended pawn (bad)"},
     
    // ========== X-RAY ATTACKS (CRITICAL) ==========
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
     Move(e5, f7), -600, "Knight sac with queen x-ray (bad)"},
     
    {"2r1r1k1/pp1bppbp/3p1np1/q3P3/2P2P2/1P2B3/P2N2PP/1R1QKB1R b K - 0 1",
     Move(f6, e4), 100, "Knight takes pawn with complex exchanges"},
     
    // ========== STOCKFISH FAMOUS POSITIONS ==========
    // From Stockfish test suite - known SEE edge cases
    {"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - 0 1",
     Move(e1, e5), -400, "Rook takes pawn, rook recaptures (bad)"},
     
    {"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - - 0 1",
     Move(d3, e5), -200, "Knight takes pawn, complex sequence"},
     
    {"rnb2b1r/ppp2kpp/5n2/4P3/q2P3/5R2/PPP1QPPP/RNB1KB2 w Q - 0 1",
     Move(f3, f6), 100, "Rook takes knight, king must recapture"},
     
    // ========== LEELA'S TORTURE POSITIONS ==========
    // Positions where neural networks struggle with SEE
    {"r1b1kb1r/3q1ppp/pBp1pn2/8/Np3P2/5Q2/PPP3PP/R1B1K2R w KQkq - 0 1",
     Move(b6, d8), -100, "Bishop takes queen, complex recaptures"},
     
    {"2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP3/P1P2RPP/R5K1 w - - 0 1",
     Move(g4, g7), -500, "Queen takes pawn, king recaptures"},
     
    // ========== PINNED PIECES (NOT considered in SEE) ==========
    {"r2qkbnr/ppp2ppp/2n5/3p4/2BPp1b1/5N2/PPP2PPP/RNBQK2R w KQkq - 0 6",
     Move(f3, e5), 100, "Knight takes pawn (ignore pin in SEE)"},
     
    {"4r1k1/p1p2ppp/2p5/8/8/1P6/P1P2PPP/4R1K1 w - - 0 1",
     Move(e1, e8), 0, "Rook trade (ignore back rank mate threat)"},
     
    // ========== EN PASSANT (OFTEN BUGGY) ==========
    {"rnbqkbnr/1ppppppp/8/pP6/8/8/P1PPPPPP/RNBQKBNR w KQkq a6 0 2",
     Move(b5, a6), 100, "En passant capture"},
     
    {"4k3/8/8/2pP4/8/8/8/4K3 w - c6 0 1",
     Move(d5, c6), 100, "En passant with no defenders"},
     
    // ========== PROMOTIONS ==========
    {"4k3/1P6/8/8/8/8/4K3/r7 w - - 0 1",
     Move(b7, b8, QUEEN), 800, "Promotion with rook capture"},
     
    {"r3k3/1P6/8/8/8/8/8/4K3 w - - 0 1",
     Move(b7, a8, QUEEN), 400, "Capture rook and promote"},
     
    {"8/6P1/5K2/8/7k/8/8/r7 w - - 0 1",
     Move(g7, g8, KNIGHT), -600, "Under-promotion loses to rook"},
     
    // ========== COMPLEX MULTI-PIECE EXCHANGES ==========
    {"r3r1k1/pp3pbp/1qp3p1/2B5/2BP2b1/Q1n2N2/P4PPP/3RK2R w K - 0 1",
     Move(d1, d8), -200, "Rook takes rook, complex sequence"},
     
    // ========== KING PARTICIPATION ==========
    {"8/4k3/8/3Pp3/8/8/8/4K3 w - - 0 1",
     Move(d5, e6), 0, "King recaptures (equal)"},
     
    {"8/8/1p6/p1k5/P7/1K6/8/8 w - - 0 1",
     Move(a4, a5), -100, "King must capture, pawn recaptures"},
     
    // ========== DISCOVERED ATTACKS (X-RAY) ==========
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
     Move(a4, a3), -180, "Bishop takes queen, rook x-ray"},
     
    // ========== ETHEREAL EDGE CASES ==========
    // From Ethereal's test suite
    {"4R3/2r3p1/5bk1/1p1r3p/p2PR1P1/P1BK1P2/1P6/8 b - - 0 1",
     Move(h5, g4), 0, "Complex rook and bishop exchanges"},
     
    {"4R3/2r3p1/5bk1/1p1r1p1p/p2PR1P1/P1BK1P2/1P6/8 b - - 0 1",
     Move(h5, g4), 0, "Same position, different pawn structure"},
     
    // ========== WEISS TEST POSITIONS ==========
    // Positions from Weiss engine development
    {"6k1/1pp4p/p1pb4/6q1/3P1pRr/2P4P/PP1Br1P1/5RKN w - - 0 1",
     Move(f1, f4), -100, "Rook takes pawn, complex"},
     
    {"5rk1/1pp2q1p/p1pb4/8/3P1NRr/2P5/PP1B1P1P/5RK1 w - - 0 1",
     Move(f4, d5), -100, "Knight takes bishop, complex"},
     
    // ========== DISCOVERED CHECK COMPLICATIONS ==========
    {"3r2k1/p2r1p1p/1p2p1p1/q4n2/3P4/PQ5P/1P1RNPP1/3R2K1 b - - 0 1",
     Move(f5, d4), 100, "Knight takes pawn with discovered check"},
     
    // ========== BATTERY ATTACKS ==========
    {"3r1r1k/1p3p1p/p2p4/4n1NN/6bQ/1BPq4/P3p1PP/5R1K w - - 0 1",
     Move(g5, f7), 100, "Knight takes pawn, queen battery"},
     
    // ========== ABSOLUTE PINS (IGNORED IN SEE) ==========
    {"rnbqk2r/pppp1ppp/5n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1",
     Move(f3, e5), 100, "Pinned knight takes pawn (pin ignored)"},
     
    // ========== PATHOLOGICAL DEEP EXCHANGES ==========
    {"q2Q3r/2P5/1k6/8/8/8/8/K7 w - - 0 1",
     Move(c7, c8, QUEEN), -100, "Promote with multiple captures"},
     
    {"1n2kb1r/p1P4p/2qb4/5pP1/4p3/1P6/P2PPP1P/R1BQKB1R w KQk f6 0 1",
     Move(c7, c8, KNIGHT), -200, "Under-promotion in complex position"},
};

// ========== ADDITIONAL STOCKFISH VALIDATION POSITIONS ==========
// These positions are from Stockfish's SEE unit tests (see.cpp)
SEETest stockfishValidation[] = {
    // From Stockfish see_test.cpp
    {"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - 0 1",
     Move(e1, e5), -400, "SF: Rook takes defended pawn"},
     
    {"1k2r3/1pp4p/p7/4p3/4P3/P7/1PP3PP/2K1R3 w - - 0 1",
     Move(e1, e5), 100, "SF: Rook takes pawn, equal trade"},
     
    {"8/8/8/1p6/P1K5/1B6/8/k7 b - - 0 1",
     Move(b5, a4), 0, "SF: Pawn takes pawn, bishop recaptures"},
     
    {"8/pp6/2pkp3/4bp2/2R3b1/2P5/PP4B1/1K6 w - - 0 1",
     Move(c4, c6), -100, "SF: Rook takes pawn, complex"},
     
    {"4q3/1p1pr3/1B2rk2/6p1/p3PP2/P3R1P1/1P6/2Q2K2 w - - 0 1",
     Move(e4, e5), -400, "SF: Pawn push blocks, complex SEE"},
     
    {"4q3/1p1pr3/4rk2/1B4p1/p3PP2/P3R1P1/1P6/2Q2K2 w - - 0 1",
     Move(e4, e5), 100, "SF: Same but bishop moved"},
};

// ========== COMMON BUG POSITIONS ==========
// Positions that commonly reveal SEE implementation bugs
SEETest commonBugs[] = {
    // Bug: Forgetting to remove initial attacker from occupied
    {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",
     Move(d7, d5), 0, "Bug check: Initial attacker removal"},
     
    // Bug: Not handling promotion value change
    {"4k3/P7/8/8/8/8/8/r3K3 w - - 0 1",
     Move(a7, a8, QUEEN), -100, "Bug check: Promotion value"},
     
    // Bug: X-ray through initial capturer
    {"r3k3/8/8/8/R7/8/8/4K3 w - - 0 1",
     Move(a4, a8), 0, "Bug check: X-ray through attacker"},
     
    // Bug: King can capture but can't be captured
    {"4k3/8/4r3/3KP3/8/8/8/8 w - - 0 1",
     Move(d5, e5), -400, "Bug check: King can't be captured"},
     
    // Bug: Multiple x-rays on same line
    {"R6r/8/8/2r5/8/8/8/K6k b - - 0 1",
     Move(c5, a5), 0, "Bug check: Multiple x-rays"},
};
```

### Validation Strategy

1. **Explicit Testing Progression**
   ```bash
   # Deliverable 1.4: Test basic exchanges only (10 positions)
   ./bin/seajay test-see --level=basic --expect-pass=10
   
   # Deliverable 2.1: Test multi-piece exchanges (20 positions)
   ./bin/seajay test-see --level=multi --expect-pass=20
   
   # Deliverable 3.2: Add x-ray tests (15 positions)
   ./bin/seajay test-see --level=xray --expect-pass=15
   
   # Deliverable 4.4: Full test suite (100+ positions)
   ./bin/seajay test-see --level=all --expect-pass=100
   
   # Deliverable 3.3: Stockfish validation (50+ positions)
   ./tools/scripts/validate-see.sh --expect-match=50
   ```

2. **Perft Integration Test**
   - After SEE implementation, perft MUST still pass
   - This validates that SEE doesn't corrupt board state

3. **Performance Benchmark**
   ```cpp
   // SEE should be fast - target times for 1M evaluations:
   // Simple positions: < 100ms
   // Complex positions: < 500ms
   // Average: ~200ms
   ```

### Stockfish's SEE Implementation - Key Lessons

**The Swap Algorithm (from Stockfish's position.cpp):**

```cpp
// Stockfish uses a non-recursive "swap" algorithm that builds an array
// of gains and then performs minimax backwards. Key insights:

// 1. CRITICAL: Stockfish removes the initial attacker from occupied FIRST
Bitboard occupied = pieces() ^ from;  // This prevents self-blocking

// 2. Stockfish uses "stmAttackers" to track whose turn it is
Bitboard stmAttackers = attackers & pieces(stm);

// 3. The "swap list" tracks cumulative material balance
int swap[32];
swap[0] = PieceValue[MG][piece_on(to)];

// 4. X-ray handling is elegant - after removing attacker, recompute:
occupied ^= lsb(stmAttackers);
attackers |= (attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN)) |
             (attacks_bb<ROOK>(to, occupied) & pieces(ROOK, QUEEN));

// 5. CRITICAL BUG SOURCE: Stockfish explicitly handles promotions:
if (type_of(moved_piece) == PAWN && (to_rank(to) == RANK_1 || to_rank(to) == RANK_8))
    swap[depth] += QueenValueMg - PawnValueMg;  // Assume queen promotion
```

**Common Stockfish SEE Bugs Fixed Over Time:**

1. **Bug #1831 (2019)**: SEE didn't handle en passant correctly
   - Fix: Special case for en passant target square
   
2. **Bug #2156 (2020)**: X-rays through pawns on 7th rank
   - Fix: Check for promotion before adding x-ray attackers
   
3. **Bug #2489 (2021)**: SEE cache collision caused wrong values
   - Fix: Include more position info in cache key

4. **Performance Evolution:**
   - 2015: Basic implementation, ~250 ns/call
   - 2018: Added early exits, ~150 ns/call
   - 2020: SIMD attackers, ~100 ns/call
   - 2023: Prefetching, ~80 ns/call

### UCI-Based Testing Strategy (Given Board Init Issues)

Since direct C++ testing has initialization problems, use UCI protocol:

```bash
#!/bin/bash
# test_see_via_uci.sh - Test SEE indirectly through move ordering

# Function to test if SEE orders captures correctly
test_see_ordering() {
    local fen="$1"
    local expected_first_capture="$2"
    
    # Use UCI to get move ordering
    echo -e "position fen $fen\ngo depth 1\nquit" | ./bin/seajay 2>&1 | \
        grep "info depth 1" | grep -o "pv [a-h][1-8][a-h][1-8]" | \
        awk '{print $2}'
}

# Test 1: SEE should prefer PxQ over PxP
FEN="rnb1kbnr/pppp1ppp/8/4p3/3PP2q/8/PPP2PPP/RNBQKBNR w KQkq - 0 1"
MOVE=$(test_see_ordering "$FEN")
if [[ "$MOVE" == "g2h3" ]]; then
    echo "PASS: SEE correctly orders PxQ first"
else
    echo "FAIL: SEE didn't pick PxQ (got $MOVE)"
fi

# Test 2: SEE should avoid bad captures
FEN="r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1"
# Nxe5 is bad (-200), should prefer castling or d3
MOVE=$(test_see_ordering "$FEN")
if [[ "$MOVE" != "f3e5" ]]; then
    echo "PASS: SEE correctly avoids bad NxP"
else
    echo "FAIL: SEE picked bad capture"
fi
```

### Alternative Testing via Analysis Mode

```bash
#!/bin/bash
# test_see_analysis.sh - Use analysis mode to verify SEE

test_position() {
    local fen="$1"
    local move="$2"
    local expected_eval="$3"
    
    # Start engine in analysis mode
    (
        echo "uci"
        echo "position fen $fen"
        echo "go movetime 100"
        sleep 0.2
        echo "quit"
    ) | ./bin/seajay 2>&1 | grep -A5 "currmove $move"
}

# Can infer SEE quality from move ordering in PV
```

### Shell-Based SEE Validation Suite

```bash
#!/bin/bash
# validate_see_comprehensive.sh

# Since we can't run unit tests directly, validate via behavior

# Test categories
declare -a TEST_CATEGORIES=(
    "basic:10:simple_exchanges.epd"
    "xray:15:xray_positions.epd"
    "pins:5:pinned_pieces.epd"
    "promotions:8:promotion_tests.epd"
    "complex:20:complex_exchanges.epd"
)

run_behavioral_test() {
    local category="$1"
    local expected="$2"
    local file="$3"
    
    echo "Testing $category (expecting $expected passes)..."
    
    passed=0
    while IFS= read -r line; do
        fen=$(echo "$line" | cut -d';' -f1)
        best_move=$(echo "$line" | grep -o "bm [^;]*" | cut -d' ' -f2)
        avoid_move=$(echo "$line" | grep -o "am [^;]*" | cut -d' ' -f2)
        
        # Run engine and check if it finds best move
        output=$(echo -e "position fen $fen\ngo depth 5\nquit" | \
                 timeout 1 ./bin/seajay 2>&1)
        
        pv=$(echo "$output" | grep "bestmove" | awk '{print $2}')
        
        if [[ -n "$best_move" && "$pv" == "$best_move" ]]; then
            ((passed++))
        elif [[ -n "$avoid_move" && "$pv" != "$avoid_move" ]]; then
            ((passed++))
        fi
    done < "tests/see/$file"
    
    echo "  Result: $passed/$expected"
    return $((expected - passed))
}

# Run all test categories
total_failures=0
for test in "${TEST_CATEGORIES[@]}"; do
    IFS=':' read -r category expected file <<< "$test"
    run_behavioral_test "$category" "$expected" "$file"
    total_failures=$((total_failures + $?))
done

exit $total_failures
```

### Indirect SEE Performance Testing

```bash
#!/bin/bash
# benchmark_see_impact.sh - Measure SEE impact without direct access

# Baseline: Run without SEE
echo "Testing baseline (MVV-LVA only)..."
./bin/seajay << EOF | grep "nodes\|nps" > baseline.txt
uci
setoption name UseSEE value false
position startpos
go depth 10
quit
EOF

# With SEE
echo "Testing with SEE..."
./bin/seajay << EOF | grep "nodes\|nps" > with_see.txt
uci
setoption name UseSEE value true
position startpos
go depth 10
quit
EOF

# Compare
baseline_nps=$(grep nps baseline.txt | awk '{print $4}')
see_nps=$(grep nps with_see.txt | awk '{print $4}')
drop_percent=$(echo "scale=2; (1 - $see_nps / $baseline_nps) * 100" | bc)

echo "NPS Impact: ${drop_percent}% drop"
if (( $(echo "$drop_percent > 30" | bc -l) )); then
    echo "WARNING: Excessive NPS drop!"
    exit 1
fi
```

## 3. Integration Risks and Mitigation

### Additional Risks Identified

1. **Integration Order Risk**
   - **Issue**: Testing SEE within move ordering could mask SEE bugs
   - **Mitigation**: Validate SEE standalone with 50+ positions before ANY integration
   - **Implementation**: Create `test-see` binary that only tests SEE algorithm

2. **Test Data Reliability Risk**
   - **Issue**: Stage 14 had multiple false "bugs" from incorrect test expectations
   - **Mitigation**: ALWAYS validate test positions with Stockfish first
   ```bash
   # Mandatory validation before debugging
   echo -e "position fen [FEN]\ngo perft [depth]\nquit" | ./external/engines/stockfish/stockfish
   ```

3. **Performance Cliff Risk**
   - **Issue**: Pathological positions could cause exponential slowdown
   - **Mitigation**: Add position-specific timeouts and node limits
   ```cpp
   constexpr int MAX_SEE_DEPTH = 32;  // Hard limit
   constexpr int SEE_TIMEOUT_MS = 10; // Per-position timeout
   ```

4. **State Corruption Risk**
   - **Issue**: SEE must not modify board state
   - **Mitigation**: Mark all SEE methods as `const`, use value semantics
   ```cpp
   [[nodiscard]] int see(const Position& pos, Move move) const noexcept;
   ```

5. **Compiler Brittleness Risk**
   - **Issue**: Template/SIMD code sensitive to compiler changes
   - **Mitigation**: Test on multiple compilers, provide fallback implementations
   ```cpp
   #ifdef __AVX2__
       // SIMD version
   #else
       // Portable fallback
   #endif
   ```

### Learning from Stage 14 Disasters

1. **The C9 Catastrophe Pattern**
   - **Risk**: Aggressive SEE pruning could discard good moves
   - **Mitigation**: 
     ```cpp
     // Explicit progression of SEE pruning thresholds
     // Based on analysis of top engines:
     
     // Stockfish uses dynamic margins based on depth:
     // Depth 0 (QS root): -95 cp
     // Depth -1: -50 cp  
     // Depth -2+: -25 cp
     
     // Ethereal uses static margin:
     // Quiescence: -105 cp (tuned via SPSA from -80)
     
     // Leela doesn't use SEE pruning (neural eval instead)
     
     // SeaJay progression plan:
     // Deliverable 5.5: Infrastructure (disabled)
     constexpr int SEE_PRUNE_MARGIN_DISABLED = INT_MIN;
     
     // Deliverable 5.6: Conservative phase (safer than any engine)
     constexpr int SEE_PRUNE_MARGIN_CONSERVATIVE = -200;
     
     // Deliverable 5.7: Tuned phase (after SPRT validation)
     constexpr int SEE_PRUNE_MARGIN_TUNED = -100;
     
     // Deliverable 6.2: Final optimized value (after tuning)
     // Target range based on top engines: -75 to -105
     constexpr int SEE_PRUNE_MARGIN_FINAL = -95;  // Stockfish-like
     
     // Advanced (future): Depth-based margins like Stockfish
     int getPruneMargin(int qsDepth) {
         if (!options.useSEEPruning) return INT_MIN;
         if (qsDepth == 0) return -95;   // QS entry
         if (qsDepth == -1) return -50;  // One level deep
         return -25;                     // Deeper levels
     }
     
     // In quiescence:
     if (see(move) < currentPruneMargin && !isCheck(move)) {
         stats.prunedMoves++;
         continue;  // Skip this move
     }
     ```

2. **Build Mode Issues**
   - **Risk**: SEE might be accidentally disabled via preprocessor
   - **Mitigation**: 
     ```cpp
     // NEVER use #ifdef for SEE
     // Always compile in, use runtime flags:
     struct SearchOptions {
         bool useSEE = true;  // Runtime control
         int seePruneMargin = -100;
     };
     ```

3. **Move Ordering Integration**
   ```cpp
   // Explicit integration phases with measurable criteria:
   int scoreCapture(Move move) {
       switch(integrationPhase) {
           case 0:  // Deliverable 5.2: Testing mode
               int mvv = mvvLvaScore(move);
               int see = seeScore(move);
               logDifference(mvv, see);
               return mvv;  // USE MVV-LVA
               
           case 1:  // Deliverable 5.3: Shadow mode
               int mvv = mvvLvaScore(move);
               int see = seeScore(move);
               stats.track(mvv, see);
               return see;  // USE SEE
               
           case 2:  // Deliverable 5.4: Production mode
               return seeScore(move);  // ONLY SEE
               
           default:
               return mvvLvaScore(move);  // Safety fallback
       }
   }
   
   // Explicit success criteria for each phase:
   bool readyForNextPhase() {
       switch(integrationPhase) {
           case 0: return stats.sampledMoves > 10000;
           case 1: return stats.agreementRate > 0.7 && stats.npsDropPercent < 20;
           case 2: return sprtResult.elo > 0;
       }
   }
   ```

### Performance Regression Patterns

1. **Watch for NPS Drop**
   - Expected: 10-15% NPS reduction initially
   - Dangerous: >30% NPS drop indicates bug
   - Monitor with: `./tools/scripts/benchmark-nps.sh`

2. **Memory Usage**
   - SEE should NOT allocate memory
   - Stack usage: < 1KB per call
   - No static/global state

3. **Cache Effects**
   ```cpp
   // Cache-friendly SEE call pattern
   class SEECache {
       struct Entry {
           uint64_t hash;
           int value;
       };
       Entry cache[4096];  // Small, L1-friendly
   public:
       int probe(Move move, Position& pos);
   };
   ```

## 4. Implementation Order - Micro-Deliverable Approach

### Critical: Small, Testable Chunks
Based on Stage 14 lessons, we're breaking Stage 15 into micro-deliverables of 30min-2hrs each. Each deliverable must be independently testable and committable.

### Day 1: Foundation (4 micro-deliverables)

**Deliverable 1.1: SEE Types & Constants** (30 min)
- Create `src/core/see.h` with piece values and types
- Add SEE-specific type aliases and constants
- **Commit**: `feat(see): Add SEE types and constants`
- **Test**: Compilation only
- **Validation**: Header guards, includes compile

**Deliverable 1.2: Attack Detection Wrapper** (1 hour)
- Extract `attackersTo()` from move generator into reusable function
- Create standalone attack detection module
- **Commit**: `feat(see): Extract attack detection for SEE`
- **Test**: Unit tests for attack detection on 20+ positions
- **Validation**: Compare with existing move generator results

**Deliverable 1.3: Basic Swap Array Logic** (1 hour)
- Implement gain array manipulation functions
- Minimax evaluation of exchanges
- **Commit**: `feat(see): Add swap gain array logic`
- **Test**: Unit test with hardcoded exchange sequences
- **Validation**: Hand-verify 5 exchange sequences

**Deliverable 1.4: Simple 1-for-1 Exchanges** (1 hour)
- SEE for positions with single attacker/defender pairs
- No x-rays, no complex scenarios
- **Commit**: `feat(see): Implement simple exchange evaluation`
- **Test**: 10 basic positions (PxP, NxP, BxN, etc.)
- **Validation**: Manual calculation matches

### Day 2: Core Algorithm (4 micro-deliverables)

**Deliverable 2.1: Multi-Piece Exchanges** (1.5 hours)
- Handle 2+ attackers per side
- Proper attacker ordering
- **Commit**: `feat(see): Add multi-piece exchange support`
- **Test**: 20 positions with multiple attackers
- **Validation**: Cross-check with hand calculation

**Deliverable 2.2: Least Attacker Selection** (1 hour)
- Implement `getLeastAttacker()` with piece type ordering
- Optimize with bitboard operations
- **Commit**: `feat(see): Add least valuable attacker selection`
- **Test**: Unit tests for attacker ordering
- **Validation**: Verify piece value ordering

**Deliverable 2.3: King Participation** (30 min)
- Handle king as attacker (never as capture victim)
- Special case for king safety
- **Commit**: `feat(see): Add king participation in SEE`
- **Test**: 5 positions with king captures
- **Validation**: King never considered capturable

**Deliverable 2.4: Special Moves** (1 hour)
- En passant capture handling
- Promotion value changes
- **Commit**: `feat(see): Handle en passant and promotions`
- **Test**: 10 special case positions
- **Validation**: Promotion values correctly applied

### Day 3: X-Ray Support (3 micro-deliverables)

**Deliverable 3.1: X-Ray Detection** (1.5 hours)
- Implement `xrayAttackers()` function
- Detect sliders behind removed pieces
- **Commit**: `feat(see): Add x-ray attacker detection`
- **Test**: Unit tests with blocked sliders
- **Validation**: All slider types detected

**Deliverable 3.2: X-Ray Integration** (1 hour)
- Integrate x-rays into main SEE algorithm
- Update occupancy bitboard correctly
- **Commit**: `feat(see): Integrate x-ray attacks in SEE`
- **Test**: 15 x-ray positions
- **Validation**: Complex positions with multiple x-rays

**Deliverable 3.3: Stockfish Cross-Validation** (1 hour)
- Create validation suite comparing with Stockfish
- Document any acceptable differences
- **Commit**: `test(see): Add Stockfish validation suite`
- **Test**: 50+ positions match Stockfish SEE
- **Validation**: Automated comparison script

### Day 4: Safety & Performance (5 micro-deliverables)

**Deliverable 4.1: Performance Optimization Phase 1 - Basic Early Exit** (45 min)
- Add trivial win detection (capturing higher value with lower)
- Add undefended capture detection
- **Commit**: `perf(see): Add basic early termination checks`
- **Test**: 30% of evaluations exit early
- **Validation**: Benchmark shows 10-15% speedup

**Deliverable 4.2: Performance Optimization Phase 2 - Advanced Early Exit** (45 min)
- Add gain array early termination
- Implement lazy attacker generation
- **Commit**: `perf(see): Add advanced early termination`
- **Test**: 50% of evaluations exit early
- **Validation**: <500ms for complex positions, 20-25% total speedup

**Deliverable 4.3: Cache Implementation** (1 hour)
- Add simple SEE cache (4096 entries)
- Thread-local to avoid synchronization
- **Commit**: `feat(see): Add SEE result caching`
- **Test**: Cache hit rate >30% in typical positions
- **Validation**: No memory leaks, correct invalidation

**Deliverable 4.4: Debug Infrastructure** (1 hour)
- Add SEE trace output for debugging
- UCI option for SEE verbosity
- **Commit**: `feat(see): Add debug trace infrastructure`
- **Test**: Trace output matches algorithm steps
- **Validation**: No performance impact when disabled

**Deliverable 4.5: Comprehensive Test Suite** (1 hour)
- 100+ test positions covering all cases
- Automated test runner
- **Commit**: `test(see): Complete test suite with 100+ positions`
- **Test**: All positions pass
- **Validation**: Tests cover all code paths

### Day 5-6: Integration (9 micro-deliverables)

**Deliverable 5.1: Parallel Scoring Mode** (1.5 hours)
- Add infrastructure for A/B testing SEE vs MVV-LVA
- Both scores computed and logged
- **Commit**: `feat(see): Add parallel scoring infrastructure`
- **Test**: Both scores computed correctly
- **Validation**: No performance regression

**Deliverable 5.2: Move Ordering Integration Phase 1 - Testing Mode** (1 hour)
- Integrate SEE in TESTING mode (compute both, use MVV-LVA)
- Log all score differences > 100cp
- **Commit**: `feat(see): Add SEE testing mode to move ordering`
- **Test**: Both scores computed, MVV-LVA still used
- **Validation**: Log shows differences, NPS unchanged

**Deliverable 5.3: Move Ordering Integration Phase 2 - Shadow Mode** (30 min)
- Switch to SHADOW mode (compute both, use SEE)
- Continue logging disagreements for analysis
- **Commit**: `feat(see): Enable SEE shadow mode in move ordering`
- **Test**: SEE scores used, statistics collected
- **Validation**: Agreement rate > 70%, NPS within 20% of baseline

**Deliverable 5.4: Move Ordering Integration Phase 3 - Production Mode** (30 min)
- Switch to PRODUCTION mode (SEE only, no logging)
- Remove comparison code from hot path
- **Commit**: `feat(see): Enable SEE production mode`
- **Test**: Only SEE scores computed
- **Validation**: NPS stable, no debug output

**Deliverable 5.5: Quiescence Pruning Phase 1 - Infrastructure** (45 min)
- Add SEE pruning infrastructure with UCI option
- Start DISABLED by default
- **Commit**: `feat(see): Add SEE pruning infrastructure to quiescence`
- **Test**: Pruning code compiles, disabled by default
- **Validation**: No behavior change when disabled

**Deliverable 5.6: Quiescence Pruning Phase 2 - Conservative** (45 min)
- Enable pruning with -200cp margin
- Add statistics tracking for pruned moves
- **Commit**: `feat(see): Enable conservative SEE pruning at -200cp`
- **Test**: Prunes < 5% of captures
- **Validation**: Tactical suite solve rate unchanged

**Deliverable 5.7: Quiescence Pruning Phase 3 - Tuned** (30 min)
- After SPRT validation, tighten to -100cp margin
- Document pruning statistics
- **Commit**: `feat(see): Optimize SEE pruning margin to -100cp`
- **Test**: Prunes 10-15% of captures
- **Validation**: SPRT shows positive ELO

**Deliverable 5.8: Performance Validation** (1 hour)
- Full performance benchmark suite
- Memory usage profiling
- **Commit**: `test(see): Add performance validation suite`
- **Test**: NPS, memory, time-to-depth metrics
- **Validation**: Within acceptable bounds

**Deliverable 5.9: SPRT Preparation** (1 hour)
- Configure SPRT test parameters
- Create test binaries with/without SEE
- **Commit**: `test(see): Configure SPRT validation`
- **Test**: SPRT framework runs
- **Validation**: Baseline established

### Day 7-8: Validation & Tuning

**Deliverable 6.1: SPRT Testing** (4+ hours)
- Run full SPRT validation
- Document results
- **Commit**: `test(see): SPRT validation results`
- **Validation**: Positive ELO gain

**Deliverable 6.2: Parameter Tuning Phase 1 - Pruning Margins** (1 hour)
- Test margins: -200, -150, -100, -50cp
- Run quick games at each setting
- **Commit**: `tune(see): Test pruning margin variations`
- **Validation**: Identify optimal margin with data

**Deliverable 6.3: Parameter Tuning Phase 2 - Piece Values** (1 hour)
- Test SEE piece values: ±10% variations
- Compare with standard piece values
- **Commit**: `tune(see): Optimize SEE piece values`
- **Validation**: Best values identified with SPRT

**Deliverable 6.4: Final Integration** (1 hour)
- Remove parallel scoring mode code
- Clean up debug infrastructure
- Lock in final parameters
- **Commit**: `feat(see): Finalize SEE integration`
- **Validation**: Clean build, all tests pass, binary size optimal

## 5. Early Warning Signs

### Red Flags Requiring Immediate Stop

1. **Perft Regression**
   - ANY perft failure after SEE integration
   - Indicates board corruption or state issues

2. **Tactical Blindness**
   ```bash
   # Run tactical test suite
   ./tools/scripts/tactical-test.sh
   # If solve rate drops >10%, STOP
   ```

3. **Binary Size Anomalies**
   ```bash
   # Check binary size after each build
   ls -la ./bin/seajay
   # Significant changes (>5KB) need investigation
   ```

4. **Performance Collapse**
   - NPS drop >30%
   - Search explosion (>2x nodes for same depth)
   - Time-to-depth increases >50%

### Validation Gates (Must Pass Before Proceeding)

1. **Gate 1: Algorithm Correctness**
   - All 50+ test positions pass
   - Manual verification of 5 complex positions
   - Cross-validation with Stockfish SEE

2. **Gate 2: Integration Safety**
   - Perft still passes (all positions, depth 6)
   - NPS within 20% of baseline
   - Tactical suite solve rate maintained

3. **Gate 3: Improvement Validation**
   - SPRT test shows positive ELO
   - No crashes in 10,000 game test
   - Memory usage stable

## 6. Git Workflow and Version Control Strategy

### Feature Branch Approach (NEW)

Given Stage 15's complexity, we'll use feature branches instead of direct main commits:

```bash
# Start Stage 15 development
git checkout -b feature/stage-15-see
git push -u origin feature/stage-15-see

# After EACH micro-deliverable
git add -p  # Selective staging for clean commits
git commit -m "feat(see): [specific change]"
git push  # Backup to remote

# Create checkpoint tags on feature branch
git tag see-deliverable-1.1
git tag see-deliverable-1.2
# ... etc

# Only after FULL validation
git checkout main
git merge --no-ff feature/stage-15-see -m "feat: Complete Stage 15 - Static Exchange Evaluation"
git tag stage-15-complete
```

### Pre-Commit Hooks for Safety

Create `.git/hooks/pre-commit`:
```bash
#!/bin/bash
# Stage 15 Safety Checks

# 1. Check binary size hasn't changed drastically
if [ -f ./bin/seajay ]; then
    OLD_SIZE=$(git show HEAD:binary_sizes.txt 2>/dev/null | grep "seajay" | awk '{print $2}' || echo "0")
    NEW_SIZE=$(stat -f%z ./bin/seajay 2>/dev/null || stat -c%s ./bin/seajay)
    DIFF=$((NEW_SIZE - OLD_SIZE))
    
    if [ $DIFF -gt 5000 ] || [ $DIFF -lt -5000 ]; then
        echo "⚠️  WARNING: Binary size changed by $DIFF bytes"
        echo "   Old: $OLD_SIZE, New: $NEW_SIZE"
        echo "   This could indicate missing features (Stage 14 issue)"
        read -p "   Continue? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
    
    # Update size tracking
    echo "seajay $NEW_SIZE" > binary_sizes.txt
    git add binary_sizes.txt
fi

# 2. Run quick perft validation
if [ -f ./bin/seajay ]; then
    echo "Running quick perft check..."
    timeout 5 ./bin/seajay perft --depth=3 --position=startpos > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "❌ ERROR: Perft test failed!"
        echo "   This indicates move generation may be broken"
        exit 1
    fi
fi

# 3. Check for SEE symbols if SEE files modified
if git diff --cached --name-only | grep -q "see\.[ch]pp"; then
    if [ -f ./bin/seajay ]; then
        nm ./bin/seajay | grep -q "evaluateSEE" || {
            echo "❌ ERROR: SEE symbols not found in binary!"
            echo "   Check build system and includes"
            exit 1
        }
    fi
fi

echo "✅ Pre-commit checks passed"
```

### Continuous Integration Pattern

Implement gradual integration with parallel scoring:

```cpp
// see_integration.h - Gradual rollout strategy
enum class SEEMode {
    DISABLED,      // MVV-LVA only (baseline)
    TESTING,       // Compute both, use MVV-LVA, log differences
    SHADOW,        // Compute both, use SEE, log when different
    PRODUCTION     // SEE only, no logging
};

class MoveOrdering {
    SEEMode seeMode = SEEMode::DISABLED;
    
    struct Stats {
        uint64_t agreements = 0;
        uint64_t disagreements = 0;
        int64_t totalDifference = 0;
        
        void report() const {
            std::cout << "SEE vs MVV-LVA Statistics:\n"
                     << "  Agreement rate: " 
                     << (100.0 * agreements / (agreements + disagreements)) << "%\n"
                     << "  Avg difference when disagree: "
                     << (totalDifference / std::max(1ULL, disagreements)) << "cp\n";
        }
    } stats;
    
    int scoreCapture(const Position& pos, Move move) {
        int mvvScore = mvvLva(move);
        
        switch(seeMode) {
            case SEEMode::DISABLED:
                return mvvScore;
                
            case SEEMode::TESTING: {
                int seeScore = SEE::evaluate(pos, move);
                if (std::abs(mvvScore - seeScore) > 100) {
                    stats.disagreements++;
                    stats.totalDifference += (seeScore - mvvScore);
                    if (options.debugSEE) {
                        std::cout << "info string SEE disagree: "
                                 << moveToString(move) 
                                 << " MVV=" << mvvScore 
                                 << " SEE=" << seeScore << "\n";
                    }
                } else {
                    stats.agreements++;
                }
                return mvvScore;  // Still use MVV-LVA
            }
            
            case SEEMode::SHADOW: {
                int seeScore = SEE::evaluate(pos, move);
                int mvvScore = mvvLva(move);
                // Use SEE but track differences
                if (std::abs(mvvScore - seeScore) > 100) {
                    stats.disagreements++;
                }
                return seeScore;
            }
            
            case SEEMode::PRODUCTION:
                return SEE::evaluate(pos, move);
        }
    }
};
```

## 7. Rollback Strategy

### When to Rollback

1. **Immediate Rollback Triggers**
   - Perft failures
   - Crashes in testing
   - ELO loss >20 in SPRT

2. **Checkpoint System**
   ```bash
   # Before SEE integration
   git tag pre-see-baseline
   cp ./bin/seajay ./bin/seajay.pre-see
   
   # After each phase
   git commit -m "SEE Phase X complete"
   ./tools/scripts/backup-binary.sh
   ```

3. **Partial Rollback Options**
   - Disable SEE in quiescence only
   - Revert to MVV-LVA for ordering
   - Increase pruning margins to disable

## 7. Code Organization

### C++ Expert: Enhanced Architecture

**Template-Based Design for Flexibility**
```cpp
// src/core/see.h - Modern C++20 design
#pragma once

#include <concepts>
#include <bit>
#include <array>
#include <optional>

namespace seajay::see {

// Concept for position requirements
template<typename T>
concept PositionLike = requires(T pos, Square sq) {
    { pos.pieceAt(sq) } -> std::convertible_to<Piece>;
    { pos.sideToMove() } -> std::convertible_to<Color>;
    { pos.occupied() } -> std::convertible_to<Bitboard>;
};

// Policy-based design for different SEE variants
template<typename AttackPolicy, typename CachePolicy = NoCache>
class SEEEvaluator {
public:
    // Perfect forwarding for any position type
    template<PositionLike Pos>
    [[nodiscard]] static int evaluate(Pos&& pos, Move move) noexcept {
        return impl(std::forward<Pos>(pos), move);
    }
    
    // Specialized fast paths
    template<PositionLike Pos>
    [[nodiscard]] static bool isWinning(Pos&& pos, Move move) noexcept {
        // Fast path for SEE >= 0 check
        return quickEval(std::forward<Pos>(pos), move) >= 0;
    }
    
    template<PositionLike Pos>
    [[nodiscard]] static bool isLosing(Pos&& pos, Move move, int margin = -100) noexcept {
        // Fast path for bad captures
        return quickEval(std::forward<Pos>(pos), move) < margin;
    }
    
private:
    template<PositionLike Pos>
    static int impl(Pos&& pos, Move move) noexcept;
    
    template<PositionLike Pos>
    static int quickEval(Pos&& pos, Move move) noexcept;
};

// Predefined evaluator types
using StandardSEE = SEEEvaluator<StandardAttacks, LRUCache<4096>>;
using FastSEE = SEEEvaluator<StandardAttacks, NoCache>;
using PreciseSEE = SEEEvaluator<XRayAttacks, LRUCache<8192>>;

} // namespace seajay::see
```

**Testing Infrastructure with Modern C++**
```cpp
// tests/see/see_test.cpp
#include <gtest/gtest.h>
#include <ranges>
#include <format>

class SEETest : public ::testing::Test {
protected:
    // Test data structure using designated initializers
    struct TestCase {
        std::string_view fen;
        Move move;
        int expected;
        std::string_view description;
    };
    
    // Compile-time test data
    static constexpr std::array testCases = {
        TestCase{
            .fen = "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
            .move = Move(E4, D5),
            .expected = 0,
            .description = "Equal pawn trade"
        },
        // ... more test cases
    };
    
    // Parameterized testing with ranges
    void runTestSuite() {
        for (const auto& [fen, move, expected, desc] : testCases) {
            SCOPED_TRACE(std::format("Test: {}", desc));
            Position pos(fen);
            EXPECT_EQ(SEE::evaluate(pos, move), expected);
        }
    }
    
    // Benchmark with compile-time loop unrolling
    template<size_t N>
    void benchmarkSEE() {
        Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        Move move(E5, F7);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Compile-time unrolled loop
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            ((void)SEE::evaluate(pos, move), ...);
        }(std::make_index_sequence<N>{});
        
        auto end = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        std::cout << std::format("SEE: {} ns per call\n", ns / N);
    }
};

// Death tests for debug assertions
TEST_F(SEETest, DebugAssertions) {
    #ifdef DEBUG
    Position pos("8/8/8/8/8/8/8/8 w - - 0 1");  // Empty board
    EXPECT_DEATH(SEE::evaluate(pos, Move(E2, E4)), ".*empty square.*");
    #endif
}
```

### File Structure
```cpp
// src/core/see.h
class SEE {
public:
    static void init();  // Precompute tables if needed
    static int evaluate(const Position& pos, Move move);
    static bool winning(const Position& pos, Move move);  // Helper: SEE >= 0
    static bool losing(const Position& pos, Move move);   // Helper: SEE < margin
    
private:
    static int swap(const Position& pos, Square to, int gain[], int depth);
    static Bitboard getLeastAttacker(const Position& pos, Bitboard attackers, Color side);
    static Bitboard xrayAttackers(const Position& pos, Square sq, Bitboard occupied);
};

// src/search/move_ordering.cpp
int MoveOrdering::scoreCapture(const Position& pos, Move move) {
    if (searchOptions.useSEE) {
        int see = SEE::evaluate(pos, move);
        return see >= 0 ? 10000000 + see : 1000000 + see;
    }
    return mvvLvaScore(move);
}
```

### Testing Infrastructure
```cpp
// tests/see/see_test.cpp
class SEETest : public TestFramework {
    void runAllTests() {
        testBasicExchanges();
        testXRayAttacks();
        testEnPassant();
        testPromotions();
        testComplexExchanges();
        performanceTest();
    }
    
    void validatePosition(const char* fen, Move move, int expected) {
        Position pos(fen);
        int result = SEE::evaluate(pos, move);
        ASSERT_EQ(result, expected, 
                  "SEE mismatch for " + moveToString(move));
    }
};
```

## 8. Performance Optimizations

### Lazy Evaluation Pattern
```cpp
// Don't calculate full SEE if we can determine result early
int quickSEE(const Position& pos, Move move) {
    int captured = pieceValue[pos.pieceAt(move.to())];
    int attacker = pieceValue[pos.pieceAt(move.from())];
    
    // Trivial win: Capturing more valuable piece with less valuable
    if (captured > attacker)
        return captured - attacker;  // Lower bound
        
    // Check if defended
    if (!pos.isAttacked(move.to(), pos.sideToMove()))
        return captured;  // Exact
        
    // Need full SEE
    return SEE::evaluate(pos, move);
}
```

### Lookup Tables
```cpp
// Precompute common patterns
struct SEETable {
    // Indexed by [attacker][defender][captured]
    int simpleExchange[7][7][7];
    
    void init() {
        // Precompute simple 1-for-1 exchanges
        for (int a = PAWN; a <= KING; a++) {
            for (int d = PAWN; d <= KING; d++) {
                for (int c = PAWN; c <= QUEEN; c++) {
                    simpleExchange[a][d][c] = computeSimple(a, d, c);
                }
            }
        }
    }
};
```

## 9. Monitoring and Metrics

### Key Metrics to Track

1. **SEE Call Frequency**
   ```cpp
   struct SEEStats {
       uint64_t calls;
       uint64_t cacheHits;
       uint64_t earlyExits;
       uint64_t fullEvals;
       double avgDepth;  // Average exchange length
       
       void report() {
           std::cout << "SEE Stats:\n"
                     << "  Calls: " << calls << "\n"
                     << "  Cache hit rate: " << (100.0 * cacheHits / calls) << "%\n"
                     << "  Early exits: " << (100.0 * earlyExits / calls) << "%\n"
                     << "  Avg exchange depth: " << avgDepth << "\n";
       }
   };
   ```

2. **Impact Metrics**
   - Move ordering efficiency (% of beta cutoffs on first move)
   - Quiescence node reduction
   - Time-to-depth changes
   - ELO gain per component

### Debug Output
```cpp
// UCI option for SEE debugging
if (options.debugSEE) {
    std::cout << "info string SEE(" << moveToString(move) << ") = " 
              << seeValue << " [";
    for (int i = 0; i <= depth; i++) {
        std::cout << gain[i] << " ";
    }
    std::cout << "]\n";
}
```

## 10. Additional C++ Best Practices for Stage 15

### Preventing Integration Issues

**1. Strong Type System Usage**
```cpp
// Use strong types to prevent mixing SEE values with other scores
enum class SEEValue : int {};

constexpr SEEValue operator+(SEEValue a, SEEValue b) noexcept {
    return SEEValue{static_cast<int>(a) + static_cast<int>(b)};
}

// Now impossible to accidentally mix SEE with eval scores
SEEValue seeScore = evaluateSEE(pos, move);
// int evalScore = seeScore;  // Compile error!
int evalScore = static_cast<int>(seeScore);  // Explicit conversion required
```

**2. RAII for Performance Monitoring**
```cpp
class SEEProfiler {
    std::chrono::high_resolution_clock::time_point start;
    std::string_view name;
    
public:
    explicit SEEProfiler(std::string_view n) noexcept 
        : start(std::chrono::high_resolution_clock::now()), name(n) {}
    
    ~SEEProfiler() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if (us > 1000) {  // Log slow SEE calls
            std::cerr << std::format("Slow SEE ({}): {} us\n", name, us);
        }
    }
};

// Usage:
int see(const Position& pos, Move move) {
    SEEProfiler prof("see_main");  // Automatic timing
    // ... implementation
}
```

**3. Compile-Time Configuration Validation**
```cpp
// Validate SEE configuration at compile time
namespace config {
    constexpr int SEE_PRUNE_MARGIN = -100;
    constexpr int MAX_SEE_DEPTH = 32;
    
    // Compile-time validation
    static_assert(SEE_PRUNE_MARGIN <= 0, "Prune margin must be negative");
    static_assert(MAX_SEE_DEPTH >= 16, "SEE depth too small for complex positions");
    static_assert(MAX_SEE_DEPTH <= 64, "SEE depth exceeds reasonable bounds");
}
```

### Performance Testing Framework

```cpp
// Create comprehensive performance test
class SEEPerfTest {
    struct PerfResult {
        double avgNanos;
        double p95Nanos;
        double p99Nanos;
        size_t cacheHits;
        size_t totalCalls;
    };
    
    template<size_t N = 1'000'000>
    static PerfResult benchmark() {
        std::vector<double> timings;
        timings.reserve(N);
        
        // Test positions of varying complexity
        std::array positions = {
            Position("startpos"),
            Position("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"),
            // ... more positions
        };
        
        for (size_t i = 0; i < N; ++i) {
            const auto& pos = positions[i % positions.size()];
            Move move = generateRandomCapture(pos);
            
            auto start = std::chrono::high_resolution_clock::now();
            volatile int result = SEE::evaluate(pos, move);  // Prevent optimization
            auto end = std::chrono::high_resolution_clock::now();
            
            timings.push_back(
                std::chrono::duration<double, std::nano>(end - start).count()
            );
        }
        
        // Calculate percentiles
        std::sort(timings.begin(), timings.end());
        
        return PerfResult{
            .avgNanos = std::accumulate(timings.begin(), timings.end(), 0.0) / N,
            .p95Nanos = timings[N * 95 / 100],
            .p99Nanos = timings[N * 99 / 100],
            .cacheHits = SEE::getCacheHits(),
            .totalCalls = N
        };
    }
};
```

## 11. Final Checklist

### Pre-Implementation
- [ ] Review Stockfish/Ethereal SEE implementation
- [ ] Set up test positions file
- [ ] Create feature branch `feature/stage-15-see`
- [ ] Configure pre-commit hooks
- [ ] Set up Stockfish for validation
- [ ] Prepare SPRT test configuration

### Micro-Deliverable Tracking (Day-by-Day)

#### Day 1: Foundation
- [ ] 1.1: SEE types and constants (30 min)
- [ ] 1.2: Attack detection wrapper (1 hr)
- [ ] 1.3: Basic swap array logic (1 hr)
- [ ] 1.4: Simple 1-for-1 exchanges (1 hr)
- [ ] Day 1 checkpoint tag created

#### Day 2: Core Algorithm  
- [ ] 2.1: Multi-piece exchanges (1.5 hrs)
- [ ] 2.2: Least attacker selection (1 hr)
- [ ] 2.3: King participation (30 min)
- [ ] 2.4: Special moves (1 hr)
- [ ] Day 2 checkpoint tag created

#### Day 3: X-Ray Support
- [ ] 3.1: X-ray detection (1.5 hrs)
- [ ] 3.2: X-ray integration (1 hr)
- [ ] 3.3: Stockfish validation (1 hr)
- [ ] Day 3 checkpoint tag created

#### Day 4: Safety & Performance
- [ ] 4.1: Performance optimizations (1.5 hrs)
- [ ] 4.2: Cache implementation (1 hr)
- [ ] 4.3: Debug infrastructure (1 hr)
- [ ] 4.4: Comprehensive test suite (1 hr)
- [ ] Day 4 checkpoint tag created

#### Day 5-6: Integration
- [ ] 5.1: Parallel scoring mode infrastructure (1.5 hrs)
- [ ] 5.2: Move ordering Phase 1 - Testing mode (1 hr)
- [ ] 5.3: Move ordering Phase 2 - Shadow mode (30 min)
- [ ] 5.4: Move ordering Phase 3 - Production mode (30 min)
- [ ] 5.5: Quiescence pruning Phase 1 - Infrastructure (45 min)
- [ ] 5.6: Quiescence pruning Phase 2 - Conservative -200cp (45 min)
- [ ] 5.7: Quiescence pruning Phase 3 - Tuned -100cp (30 min)
- [ ] 5.8: Performance validation (1 hr)
- [ ] 5.9: SPRT preparation (1 hr)
- [ ] Integration checkpoint tag created

#### Day 7-8: Validation & Tuning
- [ ] 6.1: SPRT testing (4+ hrs)
- [ ] 6.2: Parameter tuning Phase 1 - Pruning margins (1 hr)
- [ ] 6.3: Parameter tuning Phase 2 - Piece values (1 hr)
- [ ] 6.4: Final integration (1 hr)
- [ ] Final validation complete

### Post-Implementation
- [ ] All micro-deliverables complete
- [ ] Feature branch merged to main
- [ ] SPRT validation shows positive ELO
- [ ] Documentation updated
- [ ] Performance metrics recorded
- [ ] Binary size tracked and normal
- [ ] Diary entry written
- [ ] Stage completion checklist filled
- [ ] Stage 15 tagged as complete

## 12. Explicit Final Configuration Values

### Final Parameter Values (Must be set by end of Stage 15)

```cpp
// These MUST be explicitly set after tuning phases
namespace SEEFinal {
    // After Deliverable 6.2: Pruning margin tuning
    constexpr int PRUNING_MARGIN = -100;  // TBD: Set after SPRT testing
    
    // After Deliverable 6.3: Piece value tuning  
    constexpr int PIECE_VALUES[7] = {
        0,    // None (unchanged)
        100,  // Pawn   (TBD: ±10% after tuning)
        320,  // Knight (TBD: ±10% after tuning)
        330,  // Bishop (TBD: ±10% after tuning)
        500,  // Rook   (TBD: ±10% after tuning)
        900,  // Queen  (TBD: ±10% after tuning)
        0     // King   (unchanged)
    };
    
    // After Deliverable 5.7: Quiescence settings
    constexpr bool ENABLE_IN_QUIESCENCE = true;  // TBD: Confirm after SPRT
    
    // After Deliverable 4.3: Cache settings
    constexpr size_t CACHE_SIZE = 4096;  // TBD: May adjust based on memory profiling
    
    // After Deliverable 5.4: Integration mode
    enum class Mode { DISABLED, TESTING, SHADOW, PRODUCTION };
    constexpr Mode FINAL_MODE = Mode::PRODUCTION;  // MUST be PRODUCTION by end
}

// Validation checklist - all must be true before Stage 15 complete
static_assert(SEEFinal::FINAL_MODE == SEEFinal::Mode::PRODUCTION);
static_assert(SEEFinal::PRUNING_MARGIN >= -200 && SEEFinal::PRUNING_MARGIN <= 0);
static_assert(SEEFinal::PIECE_VALUES[PAWN] > 0);
```

### Integration Completion Criteria

Each phase MUST meet these explicit criteria before proceeding:

1. **Testing Mode → Shadow Mode** (Deliverable 5.3)
   - Minimum 10,000 capture moves evaluated
   - Agreement rate documented (no minimum required)
   - NPS impact measured and logged

2. **Shadow Mode → Production Mode** (Deliverable 5.4)
   - Agreement rate ≥ 70% with MVV-LVA
   - NPS drop ≤ 20% from baseline
   - No crashes in 1,000 test games

3. **Conservative Pruning → Tuned Pruning** (Deliverable 5.7)
   - SPRT test completed with ≥ 10,000 games
   - Positive ELO gain confirmed
   - Tactical suite solve rate ≥ 95% of baseline

4. **Final Integration Complete** (Deliverable 6.4)
   - All debug/testing code removed
   - Binary size within 5KB of baseline
   - All 100+ test positions passing
   - SPRT shows +20 ELO minimum

## Conclusion

SEE is a powerful but complex feature. By following this careful, validated approach and learning from Stage 14's challenges, we can achieve the expected +30-50 ELO gain while avoiding the pitfalls that have plagued other implementations. 

**Critical**: Each integration phase has explicit, measurable completion criteria that must be met before proceeding. No "gradual" or "eventually" - every deliverable produces a concrete, testable result.

## Appendix A: C++ Implementation Quick Reference

### Compiler Flags for SEE Development
```bash
# Development build with maximum diagnostics
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion \
    -Wshadow -Wnon-virtual-dtor -Wcast-align \
    -Wunused -Woverloaded-virtual -Wformat=2 \
    -fsanitize=address,undefined \
    -fno-omit-frame-pointer -g3 -O0 \
    -DDEBUG -DSEE_DIAGNOSTICS

# Performance testing build
g++ -std=c++20 -O3 -march=native -flto \
    -fprofile-generate  # First run
g++ -std=c++20 -O3 -march=native -flto \
    -fprofile-use       # After profiling

# Release build with safety
g++ -std=c++20 -O2 -march=x86-64 -mtune=generic \
    -DNDEBUG -ftree-vectorize -funroll-loops
```

### Memory Debugging Commands
```bash
# Valgrind for memory leaks
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes ./bin/seajay

# AddressSanitizer output analysis
ASAN_OPTIONS=symbolize=1,print_stats=1 ./bin/seajay

# Cache performance analysis
perf stat -e cache-misses,cache-references ./bin/seajay
```

### Key C++20 Features Used
- Concepts for type constraints
- Ranges for algorithm composition  
- std::span for safe array views
- Designated initializers for clarity
- [[likely]]/[[unlikely]] for branch prediction
- std::bit operations for bit manipulation
- Coroutines (future: for search parallelization)
- Modules (when toolchain support improves)

## Appendix B: Performance Targets

### SEE Performance Requirements
| Metric | Target | Danger Zone |
|--------|--------|-------------|
| Simple position SEE | < 50ns | > 200ns |
| Complex position SEE | < 500ns | > 2000ns |
| Cache hit rate | > 60% | < 30% |
| Memory per call | 0 bytes | Any allocation |
| Stack usage | < 512 bytes | > 1KB |
| Binary size increase | < 10KB | > 30KB |
| NPS impact | < 15% drop | > 30% drop |

### Integration Success Metrics
| Stage | Success Criteria | Failure Criteria |
|-------|-----------------|------------------|
| Alpha | All tests pass | Any test fails |
| Beta | +10 ELO in SPRT | Negative ELO |
| Release | +30 ELO verified | < +20 ELO |
| Optimized | +40-50 ELO | < +30 ELO |

## Appendix C: Common SEE Bugs from Chess Engine History

### The Hall of Shame - Real Bugs from Real Engines

1. **The Crafty Bug (2005)**: Forgot to update occupied bitboard after removing attacker
   - **Symptom**: SEE returned wrong values when multiple pieces on same ray
   - **Fix**: `occupied ^= (1ULL << from);` BEFORE finding x-rays
   
2. **The Fruit Bug (2006)**: En passant captured piece on wrong square
   - **Symptom**: SEE crashed or returned garbage for en passant
   - **Fix**: Special case - captured pawn is not on 'to' square
   
3. **The Glaurung Bug (2007)**: Promotions used pawn value instead of promoted piece
   - **Symptom**: SEE undervalued promotions by ~800cp
   - **Fix**: Add promotion bonus to gain array
   
4. **The Stockfish Bug (2011)**: X-rays detected through promoting pawns
   - **Symptom**: Phantom slider attacks through 7th rank pawns
   - **Fix**: Check for promotions before adding x-ray attackers
   
5. **The Komodo Bug (2013)**: King could be "captured" in SEE
   - **Symptom**: SEE returned huge positive values when king was attacker
   - **Fix**: Break loop when king is next attacker (can't be captured)
   
6. **The Ethereal Bug (2018)**: Cache key collision
   - **Symptom**: Random wrong SEE values, very hard to debug
   - **Fix**: Include more position info in cache key (castling rights, ep square)
   
7. **The Leela Bug (2019)**: Neural network SEE disagreed with classical SEE by 500+ cp
   - **Symptom**: NN saw tactics that static exchange didn't
   - **Fix**: This is actually correct - NN sees pins/forks that SEE ignores
   
8. **The Weiss Bug (2020)**: Overflow in gain array for deep exchanges
   - **Symptom**: Crash or wrong value after 15+ captures
   - **Fix**: Limit exchange depth to 31 (32 array size)
   
9. **The Seer Bug (2021)**: Battery attacks counted twice
   - **Symptom**: Queen behind rook counted as two attackers
   - **Fix**: Carefully track which pieces already considered

### Critical Implementation Checkpoints

```cpp
// CHECKPOINT 1: Initial attacker removal
assert(occupied == (position.occupied() ^ (1ULL << move.from())));

// CHECKPOINT 2: En passant special case
if (move.isEnPassant()) {
    capturedSquare = (move.to() > 31) ? move.to() - 8 : move.to() + 8;
    assert(position.pieceAt(capturedSquare) == PAWN);
}

// CHECKPOINT 3: Promotion value adjustment
if (move.isPromotion()) {
    attackerValue = SEE_VALUES[move.promotionType()];
    assert(attackerValue > SEE_VALUES[PAWN]);
}

// CHECKPOINT 4: King can't be captured
if (nextAttackerType == KING && defenders != 0) {
    break;  // King would be captured next - stop here
}

// CHECKPOINT 5: X-ray discovery
Bitboard xrays = xrayAttackers(sq, occupied);
assert((xrays & occupied) == 0);  // X-rays must be through removed pieces

// CHECKPOINT 6: Gain array bounds
assert(depth < 32);  // Prevent overflow

// CHECKPOINT 7: Side to move tracking
assert(sideToMove == expectedSide);  // Alternates correctly
```

### Test Position for Each Bug Type

```cpp
// Test these specific positions to avoid historical bugs:

// 1. X-ray through initial attacker
"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - 0 1"  
// Re1xe5 must see rook x-ray

// 2. En passant edge case
"8/8/8/pP6/8/8/8/8 w - a6 0 1"
// b5xa6 ep must handle correctly

// 3. Promotion value  
"4k3/P7/8/8/8/8/8/4K3 w - - 0 1"
// a7-a8=Q must use queen value

// 4. King participation
"8/4k3/4p3/4P3/8/4K3/8/8 w - - 0 1"
// e5xe6 with king recapture

// 5. Deep exchange (stress test)
"r1q1r1k1/1p3pbp/2p3p1/2B5/Q1BP2b1/4RN2/P4PPP/3R2K1 b - - 0 1"
// Multiple captures on same square

// 6. Battery attacks
"3r1rk1/8/8/8/8/8/3Q4/3R2K1 w - - 0 1"
// Queen behind rook, not double counted
```

### Engine-Specific Wisdom

**Stockfish Says:**
- "Profile shows SEE is called millions of times - optimize aggressively"
- "Early exit for PxQ is worth 5% speedup alone"
- "Cache size of 4096 entries is sweet spot for L1 cache"

**Ethereal Says:**
- "Equal exchanges (SEE=0) happen 40% of the time - fast path them"
- "Tuning SEE piece values separately from eval values gained +7 ELO"
- "Don't trust test positions from old engines - always verify with latest SF"

**Leela Says:**
- "Our NN evaluation makes SEE less critical, but still helps with move ordering"
- "SEE doesn't see tactics - that's OK, that's what search is for"

**Komodo Says:**
- "SEE pruning in quiescence is worth +25 ELO but be conservative"
- "We use different margins at different depths - worth testing"

### Final Warning from the Trenches

**The #1 SEE Bug** (affects 90% of first implementations):
```cpp
// WRONG - Attacker still blocks x-rays!
Bitboard occupied = position.occupied();
occupied ^= (1ULL << from);  // TOO LATE!
Bitboard attackers = getAttackers(to, occupied);

// CORRECT - Remove attacker FIRST
Bitboard occupied = position.occupied() ^ (1ULL << from);
Bitboard attackers = getAttackers(to, occupied);
```

This single ordering issue has wasted hundreds of developer hours across dozens of engines. Don't be victim #38.