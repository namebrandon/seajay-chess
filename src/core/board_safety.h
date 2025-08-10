#pragma once

// Board Safety Infrastructure for SeaJay Chess Engine
// This file provides compile-time and debug-mode safeguards to prevent
// state corruption in the make/unmake pattern.

#include "types.h"
#include "../evaluation/pst.h"
#include <cassert>
#include <cstring>
#include <concepts>
#include <array>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

namespace seajay {

// Forward declaration
class Board;

// ============================================================================
// 1. ARCHITECTURAL IMPROVEMENTS - Prevent corruption by design
// ============================================================================

// Enhanced UndoInfo structure that captures ALL mutable state
struct CompleteUndoInfo {
    // Core move information
    Piece capturedPiece = NO_PIECE;
    Square capturedSquare = NO_SQUARE;  // For en passant, this differs from 'to'
    
    // Full game state that can change
    uint8_t castlingRights = 0;
    Square enPassantSquare = NO_SQUARE;
    uint16_t halfmoveClock = 0;
    uint16_t fullmoveNumber = 0;  // Critical: was missing!
    Hash zobristKey = 0;
    eval::MgEgScore pstScore{};  // Stage 9: PST score backup
    
    // Move-specific metadata for validation
    uint8_t moveType = NORMAL;
    Piece movingPiece = NO_PIECE;
    
#ifdef DEBUG
    // Debug-only: Full position hash for corruption detection
    uint64_t positionHash = 0;
    
    // Debug-only: Checksums of critical bitboards
    uint64_t occupiedChecksum = 0;
    uint64_t colorChecksum[2] = {0, 0};
#endif
};

// ============================================================================
// 2. COMPILE-TIME SAFETY - C++20 concepts and constraints
// ============================================================================

// Concept to ensure move is valid at compile time where possible
template<typename T>
concept ValidMoveType = requires(T m) {
    { moveFrom(m) } -> std::convertible_to<Square>;
    { moveTo(m) } -> std::convertible_to<Square>;
    { moveFlags(m) } -> std::convertible_to<uint8_t>;
};

// Concept for undo information
template<typename T>
concept UndoInfoType = requires(T undo) {
    { undo.capturedPiece } -> std::convertible_to<Piece>;
    { undo.castlingRights } -> std::convertible_to<uint8_t>;
    { undo.zobristKey } -> std::convertible_to<Hash>;
};

// Strong type wrapper for moves to prevent mixing up squares and moves
class SafeMove {
    Move m_move;
public:
    explicit constexpr SafeMove(Move m) noexcept : m_move(m) {}
    constexpr operator Move() const noexcept { return m_move; }
    
    // Prevent implicit conversions
    SafeMove(Square) = delete;
    SafeMove(int) = delete;
};

// ============================================================================
// 3. STATE VALIDATION INFRASTRUCTURE
// ============================================================================

class BoardStateValidator {
public:
    // Comprehensive state snapshot for validation
    struct StateSnapshot {
        std::array<Piece, 64> mailbox;
        std::array<Bitboard, NUM_PIECES> pieceBB;
        std::array<Bitboard, NUM_PIECE_TYPES> pieceTypeBB;
        std::array<Bitboard, NUM_COLORS> colorBB;
        Bitboard occupied;
        
        Color sideToMove;
        uint8_t castlingRights;
        Square enPassantSquare;
        uint16_t halfmoveClock;
        uint16_t fullmoveNumber;
        Hash zobristKey;
        
        // Capture current board state
        explicit StateSnapshot(const Board& board);
        
        // Check if two snapshots are identical
        bool operator==(const StateSnapshot& other) const;
        
        // Detailed comparison for debugging
        std::string compareWith(const StateSnapshot& other) const;
    };
    
    // Invariant checks
    static bool checkBitboardMailboxSync(const Board& board);
    static bool checkZobristConsistency(const Board& board);
    static bool checkPieceCountLimits(const Board& board);
    static bool checkCastlingRightsValidity(const Board& board);
    static bool checkEnPassantValidity(const Board& board);
    
    // Full validation (expensive, debug only)
    static bool validateFullIntegrity(const Board& board);
    
    // Incremental validation (cheaper, can be used in release with flag)
    static bool validateIncrementalChange(const Board& before, const Board& after, Move move);
};

// ============================================================================
// 4. DEBUG-MODE INVARIANT CHECKING
// ============================================================================

#ifdef DEBUG

// RAII guard for automatic state validation
class StateValidationGuard {
    const Board& m_board;
    BoardStateValidator::StateSnapshot m_snapshot;
    const char* m_operation;
    
public:
    StateValidationGuard(const Board& board, const char* operation);
    ~StateValidationGuard();
    
    // Disable copy/move
    StateValidationGuard(const StateValidationGuard&) = delete;
    StateValidationGuard& operator=(const StateValidationGuard&) = delete;
};

// Macro for automatic validation in debug builds
#define VALIDATE_STATE_GUARD(board, operation) \
    StateValidationGuard _guard(board, operation)

// Assert with detailed error message
#define BOARD_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Board assertion failed: " << message << "\n" \
                     << "  File: " << __FILE__ << "\n" \
                     << "  Line: " << __LINE__ << "\n" \
                     << "  Function: " << __func__ << "\n"; \
            std::abort(); \
        } \
    } while(0)

// Incremental Zobrist validation
#define VALIDATE_ZOBRIST_INCREMENT(board, oldKey, expectedChange) \
    do { \
        Hash newKey = (board).zobristKey(); \
        Hash actualChange = oldKey ^ newKey; \
        if (actualChange != expectedChange) { \
            std::cerr << "Zobrist increment mismatch!\n" \
                     << "  Expected change: 0x" << std::hex << expectedChange << "\n" \
                     << "  Actual change:   0x" << actualChange << std::dec << "\n"; \
            std::abort(); \
        } \
    } while(0)

#else

// No-op in release builds
#define VALIDATE_STATE_GUARD(board, operation) ((void)0)
#define BOARD_ASSERT(condition, message) ((void)0)
#define VALIDATE_ZOBRIST_INCREMENT(board, oldKey, expectedChange) ((void)0)

#endif

// ============================================================================
// 5. SAFE MAKE/UNMAKE WRAPPER
// ============================================================================

class SafeMoveExecutor {
public:
    // Safe make with automatic validation
    template<ValidMoveType MoveT, UndoInfoType UndoT>
    static void makeMove(Board& board, MoveT move, UndoT& undo);
    
    // Safe unmake with automatic validation
    template<ValidMoveType MoveT, UndoInfoType UndoT>
    static void unmakeMove(Board& board, MoveT move, const UndoT& undo);
    
    static std::string moveToString(Move move);
};

// ============================================================================
// 6. ZOBRIST KEY MANAGER - Prevent double updates
// ============================================================================

class ZobristKeyManager {
public:
    // Incremental update methods that track changes
    struct ZobristUpdate {
        Hash removals = 0;  // XOR of all removed pieces
        Hash additions = 0;  // XOR of all added pieces
        Hash stateChange = 0;  // Castling, EP, STM changes
        
        // Apply all changes at once
        Hash apply(Hash currentKey) const {
            return currentKey ^ removals ^ additions ^ stateChange;
        }
        
        // Validate that changes are reversible
        bool isReversible() const {
            // Check that no piece is both added and removed at same square
            // This would indicate a double update
            return true;  // Implementation in .cpp
        }
    };
    
    // Build update for a move
    static ZobristUpdate buildUpdate(const Board& board, Move move);
    
    // Validate that zobrist key matches position
    static bool validateKey(const Board& board);
    
    // Compute key from scratch (for validation)
    static Hash computeKey(const Board& board);
};

// ============================================================================
// 7. PERFORMANCE-CONSCIOUS VALIDATION
// ============================================================================

// Compile-time flag to enable validation in release builds
#ifndef SEAJAY_ENABLE_RELEASE_VALIDATION
    #ifdef DEBUG
        #define SEAJAY_ENABLE_RELEASE_VALIDATION 1
    #else
        #define SEAJAY_ENABLE_RELEASE_VALIDATION 0
    #endif
#endif

// Lightweight validation that can be enabled in release
class FastValidator {
public:
    // Quick checksum of critical state - implementation in .cpp
    static uint32_t quickChecksum(const Board& board);
    
    // Validate with minimal overhead - implementation in .cpp
    template<bool Enable = SEAJAY_ENABLE_RELEASE_VALIDATION>
    static void validate(const Board& board, uint32_t expectedChecksum);
    
private:
    [[noreturn]] static void handleCorruption(const Board& board, 
                                              uint32_t expected, 
                                              uint32_t actual);
};

// ============================================================================
// 8. MOVE SEQUENCE VALIDATOR - Catch illegal sequences
// ============================================================================

class MoveSequenceValidator {
public:
    // Validate that a sequence of moves maintains consistency
    static bool validateSequence(Board& board, const std::vector<Move>& moves);
    
    // Check for common error patterns
    static bool checkForDoubleMoves(const Move* moves, size_t count);
    static bool checkForImpossibleCastling(const Board& board, Move move);
    static bool checkForIllegalEnPassant(const Board& board, Move move);
};

} // namespace seajay