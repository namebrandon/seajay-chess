# SeaJay Chess Engine - Stage 2: Position Management Implementation Plan

**Document Version:** 1.1
**Date:** August 2025
**Author:** Brandon Harris
**Stage:** Phase 1, Stage 2 - Position Management
**Last Updated:** Incorporated chess-engine-expert feedback

## Executive Summary

This document outlines the detailed implementation plan for Stage 2 of the SeaJay Chess Engine development. Stage 2 focuses on robust position management, including FEN parsing, position validation, and board display functions. The plan emphasizes correctness, comprehensive error handling, and creating a solid foundation for future stages.

## Stage 2 Objectives

### Primary Goals
- Implement robust FEN (Forsyth-Edwards Notation) parser with comprehensive error handling
- Create position validation system to ensure legal chess positions
- Develop board display functions for debugging and visualization
- Establish round-trip testing (board → FEN → board) with 100% accuracy
- Build foundation for future move generation without introducing technical debt

### Design Principles
1. **Correctness Over Performance**: Stage 2 prioritizes accuracy and robustness
2. **Fail-Safe Parsing**: Invalid input should never corrupt engine state
3. **Clear Error Reporting**: Precise identification of what failed and why
4. **Comprehensive Validation**: Multiple layers of position verification
5. **Future-Proof Design**: Architecture that won't require refactoring in later stages

## Implementation Phases

### Phase 1: Foundation (Error Handling & Parser Structure)

#### 1.1 Error Handling Infrastructure
```cpp
// Custom Result type for C++20 (since std::expected is C++23)
template<typename T, typename E>
class Result {
    std::variant<T, E> m_value;
public:
    constexpr bool hasValue() const noexcept;
    constexpr T& value() &;
    constexpr E& error() &;
};
```

#### 1.2 Error Types Definition
```cpp
enum class FenError {
    InvalidFormat,        // Wrong number of fields
    InvalidBoard,         // Board position parsing failed
    InvalidSideToMove,    // Not 'w' or 'b'
    InvalidCastling,      // Invalid castling rights format
    InvalidEnPassant,     // Invalid en passant square
    InvalidClocks,        // Invalid halfmove/fullmove values
    PositionValidationFailed  // Position fails chess rules
};

struct FenErrorInfo {
    FenError error;
    std::string message;
    size_t position;  // Character position in FEN string
};
```

#### 1.3 FenParser Class Structure
```cpp
class FenParser {
private:
    // Zero-copy tokenization using string_view
    std::vector<std::string_view> tokenize(std::string_view fen);

    // Component parsers
    Result<bool, FenErrorInfo> parseBoardPosition(Board& board, std::string_view);
    Result<bool, FenErrorInfo> parseSideToMove(Board& board, std::string_view);
    Result<bool, FenErrorInfo> parseCastlingRights(Board& board, std::string_view);
    Result<bool, FenErrorInfo> parseEnPassant(Board& board, std::string_view);
    Result<bool, FenErrorInfo> parseHalfmoveClock(Board& board, std::string_view);
    Result<bool, FenErrorInfo> parseFullmoveNumber(Board& board, std::string_view);

public:
    static Result<bool, FenErrorInfo> parse(Board& board, std::string_view fen);
};
```

### Phase 2: Core FEN Parsing

#### 2.1 Parse Strategy
- Parse into temporary Board object
- Validate completely before copying to target
- Use move semantics for efficiency
- Never modify target board if parsing fails

**Critical Safety Check:**
```cpp
// WRONG - Classic overflow bug that many engines have
for (char c : boardStr) {
    if (isdigit(c)) {
        sq += (c - '0');  // Can overflow past H8!
    }
}

// CORRECT - Always validate bounds
for (char c : boardStr) {
    if (isdigit(c)) {
        int skip = c - '0';
        if (sq + skip > 64) return FenErrorInfo{FenError::InvalidBoard,
            "Square index overflow", position};
        sq += skip;
    }
}
```

#### 2.2 Board Position Parsing
```cpp
// Efficient character-to-piece lookup table
alignas(64) static constexpr std::array<Piece, 256> PIECE_CHAR_LUT =
    generatePieceCharLookup();

// Parse with comprehensive bounds checking
Result<bool, FenErrorInfo> parseBoardPosition(Board& board, std::string_view boardStr) {
    Square sq = SQ_A8;  // Start from a8

    for (char c : boardStr) {
        if (c == '/') {
            // Validate rank transition
            if (fileOf(sq) != FILE_H) {
                return FenErrorInfo{FenError::InvalidBoard,
                    "Incomplete rank before '/'", position};
            }
            sq -= 16;  // Move to next rank
        } else if (c >= '1' && c <= '8') {
            // Empty squares
            sq += (c - '0');
        } else if (PIECE_CHAR_LUT[c] != NO_PIECE) {
            // Valid piece
            board.setPiece(sq, PIECE_CHAR_LUT[c]);
            sq++;
        } else {
            return FenErrorInfo{FenError::InvalidBoard,
                "Invalid character in board position", position};
        }
    }

    return true;
}
```

#### 2.3 Castling Rights Parsing
- Validate format (KQkq or -)
- Store as bitfield for efficiency
- Defer validation until full position is parsed

#### 2.4 En Passant Parsing
- Validate square notation (e.g., "e3", "e6")
- Ensure correct rank (3 for white, 6 for black)
- Store for later validation against position

#### 2.5 Move Clocks Parsing
- Parse halfmove clock (0-999 range check)
- Parse fullmove number (must be >= 1)
- Use std::from_chars for efficient conversion
- **Consider clamping invalid values** rather than rejecting (many FENs in the wild have invalid clocks)

### Phase 3: Comprehensive Validation

#### 3.1 Basic Position Validation
```cpp
bool validatePosition() const {
    // Both kings must be present (exactly one per side)
    if (!validateKings()) return false;

    // No pawns on 1st or 8th rank
    if (!validatePawns()) return false;

    // Valid piece counts (max 16 per side, max 8 pawns per side)
    if (!validatePieceCounts()) return false;

    // CRITICAL: Side not to move cannot be in check (often missed!)
    // This is a chess rule violation that many engines fail to validate
    if (!validateNotInCheck()) return false;

    return true;
}
```

#### 3.2 Castling Rights Validation
```cpp
bool validateCastlingRights() const {
    // If white can castle kingside
    if (canCastle(WHITE_KINGSIDE)) {
        // King must be on e1, rook on h1
        if (pieceAt(SQ_E1) != WHITE_KING ||
            pieceAt(SQ_H1) != WHITE_ROOK) {
            return false;
        }
    }
    // Similar checks for other castling rights...

    return true;
}
```

#### 3.3 En Passant Validation
```cpp
bool validateEnPassant() const {
    if (m_enPassantSquare == NO_SQUARE) return true;

    // Check rank is correct for side to move
    Rank epRank = rankOf(m_enPassantSquare);
    if (m_sideToMove == WHITE && epRank != RANK_6) return false;
    if (m_sideToMove == BLACK && epRank != RANK_3) return false;

    // Verify pawn could have made double push
    Square pawnSq = m_sideToMove == WHITE ?
        m_enPassantSquare - 8 : m_enPassantSquare + 8;

    Piece pawn = m_sideToMove == WHITE ? BLACK_PAWN : WHITE_PAWN;
    if (pieceAt(pawnSq) != pawn) return false;

    // Verify capturing pawn exists
    bool hasCapturingPawn = false;
    if (fileOf(m_enPassantSquare) > FILE_A) {
        Square leftSq = m_sideToMove == WHITE ?
            m_enPassantSquare - 9 : m_enPassantSquare + 7;
        if (pieceAt(leftSq) == (m_sideToMove == WHITE ?
            WHITE_PAWN : BLACK_PAWN)) {
            hasCapturingPawn = true;
        }
    }
    // Similar check for right side...

    // NOTE: Complex en passant pin validation deferred to Stage 4
    // The famous "illegal en passant" where the captured pawn is pinning
    // the king will be handled during move generation

    return hasCapturingPawn;
}
```

#### 3.3.1 En Passant Pin Edge Cases (Awareness for Stage 4)
```cpp
// CRITICAL BUG SOURCE - Many engines fail this!
// Position after Black plays c7-c5:
// "8/2p5/3p4/KP5r/8/8/8/8 w - c6 0 1"
// White's b5xc6 en passant would expose king to rook on h5
// This validation is expensive and deferred to move generation

// Another pin case - capturing pawn is pinned:
// "8/8/4k3/8/2pP4/8/B7/4K3 b - d3 0 1"
// Black's c4xd3 en passant would expose black king to bishop
```

#### 3.4 Advanced Validation
```cpp
bool validateAdvanced() const {
    // Check promoted pieces don't exceed theoretical max
    // (e.g., can't have 10 queens without enough missing pawns)
    if (!validatePromotedPieceCounts()) return false;

    // Verify pawn structure is achievable
    // (e.g., tripled pawns require specific capture patterns)
    if (!validatePawnStructure()) return false;

    // Check material balance is theoretically possible
    if (!validateMaterialBalance()) return false;

    return true;
}
```

#### 3.5 Bitboard/Mailbox Synchronization
```cpp
bool validateBitboardSync() const {
    // Verify every piece in mailbox is in corresponding bitboard
    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
        Piece p = m_mailbox[sq];
        if (p != NO_PIECE) {
            if (!(m_pieceBB[p] & squareBB(sq))) return false;
            if (!(m_pieceTypeBB[typeOf(p)] & squareBB(sq))) return false;
            if (!(m_colorBB[colorOf(p)] & squareBB(sq))) return false;
        }
    }

    // Verify occupied bitboard matches
    if (m_occupied != (m_colorBB[WHITE] | m_colorBB[BLACK])) return false;

    // Verify piece type bitboards sum correctly
    for (Color c : {WHITE, BLACK}) {
        Bitboard colorPieces = 0;
        for (PieceType pt = PAWN; pt <= KING; ++pt) {
            colorPieces |= pieces(c, pt);
        }
        if (colorPieces != m_colorBB[c]) return false;
    }

    return true;
}
```

#### 3.6 Zobrist Key Validation
```cpp
bool validateZobrist() const {
    Hash calculatedKey = 0;

    // Recalculate from scratch
    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
        Piece p = m_mailbox[sq];
        if (p != NO_PIECE) {
            calculatedKey ^= s_zobristPieces[sq][p];
        }
    }

    if (m_sideToMove == BLACK) {
        calculatedKey ^= s_zobristSideToMove;
    }

    if (m_enPassantSquare != NO_SQUARE) {
        calculatedKey ^= s_zobristEnPassant[m_enPassantSquare];
    }

    calculatedKey ^= s_zobristCastling[m_castlingRights];

    return calculatedKey == m_zobristKey;
}
```

### Phase 4: Output & Display

#### 4.1 FEN Generation
```cpp
std::string Board::toFEN() const {
    std::ostringstream fen;

    // Board position
    for (Rank r = RANK_8; r >= RANK_1; --r) {
        int emptyCount = 0;
        for (File f = FILE_A; f <= FILE_H; ++f) {
            Square sq = makeSquare(f, r);
            Piece p = pieceAt(sq);

            if (p == NO_PIECE) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen << emptyCount;
                    emptyCount = 0;
                }
                fen << PIECE_TO_CHAR[p];
            }
        }
        if (emptyCount > 0) fen << emptyCount;
        if (r > RANK_1) fen << '/';
    }

    // Other fields...
    fen << ' ' << (m_sideToMove == WHITE ? 'w' : 'b');
    fen << ' ' << castlingRightsToString();
    fen << ' ' << enPassantToString();
    fen << ' ' << m_halfmoveClock;
    fen << ' ' << m_fullmoveNumber;

    return fen.str();
}
```

#### 4.2 ASCII Board Display
```cpp
std::string Board::toString() const {
    std::ostringstream os;

    os << "\n  +---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_8; r >= RANK_1; --r) {
        os << (char)('1' + r) << " |";
        for (File f = FILE_A; f <= FILE_H; ++f) {
            Square sq = makeSquare(f, r);
            Piece p = pieceAt(sq);

            if (p == NO_PIECE) {
                os << "   |";
            } else {
                os << ' ' << PIECE_TO_UNICODE[p] << " |";
            }
        }
        os << "\n  +---+---+---+---+---+---+---+---+\n";
    }

    os << "    a   b   c   d   e   f   g   h\n";
    os << "\nFEN: " << toFEN() << "\n";
    os << "Zobrist: 0x" << std::hex << m_zobristKey << std::dec << "\n";

    return os.str();
}
```

#### 4.3 Debug Display
```cpp
#ifdef DEBUG
std::string Board::debugDisplay() const {
    std::ostringstream os;

    os << "=== Board State Debug ===\n";
    os << toString();
    os << "\nBitboards:\n";

    for (Color c : {WHITE, BLACK}) {
        os << (c == WHITE ? "White: " : "Black: ");
        os << bitboardToString(m_colorBB[c]) << "\n";
    }

    os << "\nValidation Status:\n";
    os << "  Position valid: " << validatePosition() << "\n";
    os << "  Bitboard sync: " << validateBitboardSync() << "\n";
    os << "  Zobrist valid: " << validateZobrist() << "\n";

    return os.str();
}
#endif
```

### Phase 5: Testing Suite

#### 5.1 Test Categories

##### Standard Positions
- Starting position
- Common endgame positions
- Tactical puzzles from test suites
- Famous game positions

##### Critical Test Positions (Chess Engine Expert Recommendations)
```cpp
// En Passant Pin Test - Horizontal pin
const char* EP_PIN_HORIZONTAL = "8/2p5/3p4/KP5r/8/8/8/8 w - - 0 1";
// After c7-c5, en passant c6 would expose white king to rook

// En Passant Pin Test - Vertical pin
const char* EP_PIN_VERTICAL = "8/8/4k3/8/2pP4/8/B7/4K3 b - d3 0 1";
// d2-d4 just played, c4xd3 would expose black king to bishop

// Kiwipete - Complex position with many edge cases
const char* KIWIPETE = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

// Position 4 - Castling and promotion edge cases
const char* POSITION_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";

// Steven Edwards position - Everything at once
const char* EDWARDS_POS = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";

// Invalid: King moved but castling rights still set
const char* INVALID_CASTLE_1 = "r3kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Invalid: Rook not in corner but castling rights set
const char* INVALID_CASTLE_2 = "rn2k2r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
```

##### Invalid Positions (Should Reject)
- Missing kings
- Pawns on back ranks
- Too many pieces (>16 per side, >8 pawns per side)
- Invalid castling rights (pieces moved)
- Impossible en passant squares
- **Side not to move in check** (critical validation)

##### Edge Cases
- Empty board
- Maximum legal pieces
- All pawns promoted
- Positions after 50-move rule
- Positions with move number > 500
- Triple check (impossible)
- Opposite bishops on same color with all pawns

##### Malformed FEN Strings
- Wrong number of fields
- Invalid characters
- Incomplete ranks
- Out of range numbers
- Buffer overflow attempts
- Case sensitivity errors ('k' vs 'K')

#### 5.2 Round-Trip Testing
```cpp
TEST(FenParser, RoundTripConsistency) {
    const std::vector<std::string> testPositions = loadTestPositions();

    for (const auto& fen : testPositions) {
        Board board1;
        ASSERT_TRUE(board1.fromFEN(fen));

        std::string generatedFen = board1.toFEN();

        Board board2;
        ASSERT_TRUE(board2.fromFEN(generatedFen));

        // Verify boards are identical using position hash (not Zobrist)
        ASSERT_EQ(board1.positionHash(), board2.positionHash());

        // Verify all validation passes
        ASSERT_TRUE(board1.validatePosition());
        ASSERT_TRUE(board1.validateBitboardSync());
        ASSERT_TRUE(board1.validateZobrist());

        // Verify FEN strings match (canonical form)
        ASSERT_EQ(canonicalizeFEN(fen), generatedFen);
    }
}

// Position hash for testing (separate from Zobrist)
uint64_t Board::positionHash() const {
    uint64_t hash = 0;
    // Hash mailbox state
    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
        hash = hash * 31 + m_mailbox[sq];
    }
    // Hash game state
    hash = hash * 31 + m_sideToMove;
    hash = hash * 31 + m_castlingRights;
    hash = hash * 31 + m_enPassantSquare;
    hash = hash * 31 + m_halfmoveClock;
    hash = hash * 31 + m_fullmoveNumber;
    return hash;
}
```

#### 5.3 Fuzz Testing
```cpp
TEST(FenParser, FuzzTesting) {
    std::mt19937 rng(12345);

    for (int i = 0; i < 10000; ++i) {
        std::string randomFen = generateRandomString(rng, 1, 200);

        Board board;
        auto result = board.fromFEN(randomFen);

        // Should not crash
        // If successful, should pass validation
        if (result) {
            ASSERT_TRUE(board.validatePosition());
            ASSERT_TRUE(board.validateBitboardSync());
        }
    }
}
```

#### 5.4 Performance Benchmarks
```cpp
BENCHMARK(FenParsing) {
    Board board;
    const std::string fen =
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    for (int i = 0; i < 1000000; ++i) {
        board.fromFEN(fen);
    }
}
```

## Critical Implementation Notes

### Zobrist Key Management
- **Never** incrementally update Zobrist during FEN parsing
- Always rebuild from scratch after successful parse
- Include all state (castling, EP, side to move) in hash
- **Chess Engine Expert Confirmed**: This approach is correct and prevents subtle bugs

### Error Handling Philosophy
- Fail fast with clear error messages
- Never partially modify board state
- Use RAII and move semantics for exception safety
- Parse-Validate-Swap pattern is industry best practice

### Common Pitfalls to Avoid (From Expert Review)
- **Buffer Overflow**: Always check bounds when skipping empty squares
- **Rank/File Confusion**: Start from a8 (not a1) when parsing
- **Case Sensitivity**: FEN is case-sensitive ('K' vs 'k' matters)
- **Side to Move Check**: Must validate side not to move isn't in check

### Performance Considerations
- Use lookup tables for character validation
- Leverage string_view for zero-copy parsing
- Pack parse state for cache efficiency
- Add `noexcept` where appropriate

### Debug vs Release Builds
```cpp
#ifdef DEBUG
    #define VALIDATE_SYNC() assert(validateBitboardSync())
    #define VALIDATE_ZOBRIST() assert(validateZobrist())
    // Keep validation functions active for Stage 4 debugging!
#else
    #define VALIDATE_SYNC()
    #define VALIDATE_ZOBRIST()
#endif
```

### Important for Future Stages
- **Keep validation functions** even after Stage 2 completion
- Use `validatePosition()` in debug builds after every make/unmake in Stage 4
- This will catch subtle bugs that would otherwise take hours to debug

## Validation Checklist

Before marking Stage 2 complete:

- [ ] All standard test positions parse correctly
- [ ] All invalid positions are rejected with clear errors
- [ ] Round-trip tests pass for 1000+ positions
- [ ] Fuzz testing shows no crashes or undefined behavior
- [ ] Zobrist keys are always consistent
- [ ] Bitboard/mailbox representations always synchronized
- [ ] Performance benchmark shows < 1μs per FEN parse
- [ ] Code passes all static analysis checks
- [ ] Unit test coverage > 95%
- [ ] Documentation complete with examples

## Risk Mitigation

### Identified Risks
1. **Zobrist Corruption**: Mitigated by full recalculation
2. **State Desync**: Mitigated by validation layers
3. **Buffer Overflows**: Mitigated by bounds checking
4. **Invalid State Acceptance**: Mitigated by comprehensive validation
5. **Performance Regression**: Mitigated by benchmarking

### Regression Prevention
- Comprehensive test suite runs on every commit
- Performance benchmarks tracked over time
- Validation functions preserved for future stages

## Dependencies and Integration

### Prerequisites
- Stage 1 (Board Representation) complete and tested
- Types, bitboard utilities, and basic board structure in place

### Integration Points
- Stage 3 (UCI) will use FEN parser for position setup
- Stage 4 (Move Generation) will rely on validated positions
- Stage 5 (Testing) will use FEN for perft test positions

## Success Criteria

Stage 2 is complete when:
1. FEN parser handles all valid positions correctly
2. Invalid positions are rejected with clear error messages
3. Round-trip (board → FEN → board) is 100% accurate
4. All validation layers are implemented and tested
5. Board display functions work correctly
6. Test coverage exceeds 95%
7. No known bugs or validation failures
8. Performance meets requirements (< 1μs per parse)
9. All expert-recommended test positions pass validation
10. Side-not-to-move check validation implemented
11. Buffer overflow protection verified
12. Position hash function implemented for testing

## Timeline Estimate

Based on the comprehensive nature of this implementation:
- Phase 1 (Foundation): 2-3 hours
- Phase 2 (Core Parsing): 3-4 hours
- Phase 3 (Validation): 4-5 hours
- Phase 4 (Output/Display): 2-3 hours
- Phase 5 (Testing): 3-4 hours

**Total Estimate**: 14-19 hours of focused development

## Conclusion

This implementation plan prioritizes correctness and robustness over speed, establishing a rock-solid foundation for the SeaJay chess engine. The comprehensive validation and error handling will prevent subtle bugs from propagating to later stages, while the extensive testing ensures reliability under all conditions.

The modular design allows for future optimizations without architectural changes, and the clear separation of concerns makes the code maintainable and debuggable. Following this plan will result in a production-quality position management system suitable for a 3200+ Elo engine.
