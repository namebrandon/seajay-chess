#pragma once

#include "types.h"
#include "move_list.h"
#include "board.h"
#include "bitboard.h"

namespace seajay {

/**
 * Move generation interface and utilities
 * 
 * This class provides pseudo-legal and legal move generation for all piece types.
 * In Stage 3, we focus on correctness over performance. Magic bitboards and other
 * optimizations will be added in Phase 3.
 */
class MoveGenerator {
public:
    // Static interface - no instance needed
    MoveGenerator() = delete;
    
    // Main move generation functions
    
    /**
     * Generate all pseudo-legal moves for the current position
     * Pseudo-legal means the move follows piece movement rules but may leave king in check
     * 
     * @param board The current board position
     * @param moves Output move list to append moves to
     */
    static void generatePseudoLegalMoves(const Board& board, MoveList& moves);
    
    /**
     * Generate all legal moves for the current position  
     * Legal moves are pseudo-legal moves that don't leave the king in check
     * 
     * @param board The current board position
     * @param moves Output move list to append moves to
     */
    static void generateLegalMoves(const Board& board, MoveList& moves);
    
    /**
     * Generate only capture moves (pseudo-legal)
     * Used for quiescence search and move ordering
     * 
     * @param board The current board position
     * @param moves Output move list to append moves to
     */
    static void generateCaptures(const Board& board, MoveList& moves);
    
    /**
     * Generate only quiet moves (non-captures, pseudo-legal)
     * Used for move ordering and search pruning
     * 
     * @param board The current board position
     * @param moves Output move list to append moves to  
     */
    static void generateQuietMoves(const Board& board, MoveList& moves);
    
    // Check and pin detection
    
    /**
     * Test if the given square is under attack by the given color
     * 
     * @param board The current board position
     * @param square The square to test
     * @param attackingColor The color of the attacking pieces
     * @return true if the square is under attack
     */
    static bool isSquareAttacked(const Board& board, Square square, Color attackingColor);
    
    /**
     * Test if the current side to move is in check
     * 
     * @param board The current board position
     * @return true if the side to move is in check
     */
    static bool inCheck(const Board& board);
    
    /**
     * Test if the given color's king is in check
     * 
     * @param board The current board position
     * @param kingColor The color of the king to test
     * @return true if the king is in check
     */
    static bool inCheck(const Board& board, Color kingColor);
    
    /**
     * Generate all squares attacked by the given color
     * Used for king safety evaluation and move legality
     * 
     * @param board The current board position
     * @param color The attacking color
     * @return Bitboard of all attacked squares
     */
    static Bitboard getAttackedSquares(const Board& board, Color color);
    
    // Move validation
    
    /**
     * Test if a move is pseudo-legal in the current position
     * 
     * @param board The current board position
     * @param move The move to test
     * @return true if the move is pseudo-legal
     */
    static bool isPseudoLegal(const Board& board, Move move);
    
    /**
     * Test if a move is legal in the current position
     * A legal move is pseudo-legal and doesn't leave the king in check
     * 
     * @param board The current board position
     * @param move The move to test
     * @return true if the move is legal
     */
    static bool isLegal(const Board& board, Move move);
    
    // Utility functions
    
    /**
     * Count the number of legal moves in the current position
     * Used for perft testing and position analysis
     * 
     * @param board The current board position
     * @return Number of legal moves
     */
    static size_t countLegalMoves(const Board& board);
    
private:
    // Internal move generation functions for each piece type
    
    // Pawn move generation
    static void generatePawnMoves(const Board& board, MoveList& moves);
    static void generatePawnCaptures(const Board& board, MoveList& moves);
    static void generatePawnQuietMoves(const Board& board, MoveList& moves);
    
    // Knight move generation
    static void generateKnightMoves(const Board& board, MoveList& moves);
    static void generateKnightCaptures(const Board& board, MoveList& moves);
    static void generateKnightQuietMoves(const Board& board, MoveList& moves);
    
    // Bishop move generation
    static void generateBishopMoves(const Board& board, MoveList& moves);
    static void generateBishopCaptures(const Board& board, MoveList& moves);
    static void generateBishopQuietMoves(const Board& board, MoveList& moves);
    
    // Rook move generation
    static void generateRookMoves(const Board& board, MoveList& moves);
    static void generateRookCaptures(const Board& board, MoveList& moves);
    static void generateRookQuietMoves(const Board& board, MoveList& moves);
    
    // Queen move generation
    static void generateQueenMoves(const Board& board, MoveList& moves);
    static void generateQueenCaptures(const Board& board, MoveList& moves);
    static void generateQueenQuietMoves(const Board& board, MoveList& moves);
    
    // King move generation
    static void generateKingMoves(const Board& board, MoveList& moves);
    static void generateKingCaptures(const Board& board, MoveList& moves);
    static void generateKingQuietMoves(const Board& board, MoveList& moves);
    
    // Castling move generation
    static void generateCastlingMoves(const Board& board, MoveList& moves);
    
    // Attack generation for each piece type (used for check detection)
    static Bitboard getPawnAttacks(Square square, Color color);
    static Bitboard getKnightAttacks(Square square);
    static Bitboard getBishopAttacks(Square square, Bitboard occupied);
    static Bitboard getRookAttacks(Square square, Bitboard occupied);
    static Bitboard getQueenAttacks(Square square, Bitboard occupied);
    static Bitboard getKingAttacks(Square square);
    
    // Wrapper functions for bitboard attack functions
    static Bitboard bishopAttacks(Square square, Bitboard occupied);
    static Bitboard rookAttacks(Square square, Bitboard occupied);
    static Bitboard queenAttacks(Square square, Bitboard occupied);
    
    // Helper functions
    static bool isValidMove(const Board& board, Move move);
    
    // Check evasion helpers
    static void generateCheckEvasions(const Board& board, MoveList& moves);
    static void generateKingEvasions(const Board& board, MoveList& moves, Square kingSquare);
    static void generateCapturesOf(const Board& board, MoveList& moves, Square target);
    static void generateBlockingMoves(const Board& board, MoveList& moves, Bitboard blockSquares);
    static Bitboard getCheckers(const Board& board, Square kingSquare, Color attackingColor);
    
public:
    // Pin detection and legal move helpers (made public for testing)
    static bool leavesKingInCheck(const Board& board, Move move);
    static Bitboard getPinnedPieces(const Board& board, Color kingColor);
    static bool isPinned(const Board& board, Square square, Color kingColor);
    static Bitboard getPinRay(const Board& board, Square pinnedSquare, Square kingSquare);
    static bool couldDiscoverCheck(const Board& board, Square from, Square kingSquare, Color opponent);
    
private:
    
    // Pre-computed attack lookup tables
    static void initializeAttackTables();
    static bool s_tablesInitialized;
    
    // Knight attack lookup table
    static std::array<Bitboard, 64> s_knightAttacks;
    
    // King attack lookup table  
    static std::array<Bitboard, 64> s_kingAttacks;
    
    // Pawn attack lookup tables (separate for each color)
    static std::array<Bitboard, 64> s_whitePawnAttacks;
    static std::array<Bitboard, 64> s_blackPawnAttacks;
    
    // Direction vectors for sliding pieces
    static constexpr std::array<int, 4> ROOK_DIRECTIONS = {NORTH, SOUTH, EAST, WEST};
    static constexpr std::array<int, 4> BISHOP_DIRECTIONS = {NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST};
};

// Convenience functions

/**
 * Generate all legal moves and return them in a new MoveList
 */
inline MoveList generateLegalMoves(const Board& board) {
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    return moves;
}

/**
 * Generate all pseudo-legal moves and return them in a new MoveList
 */
inline MoveList generatePseudoLegalMoves(const Board& board) {
    MoveList moves;
    MoveGenerator::generatePseudoLegalMoves(board, moves);
    return moves;
}

/**
 * Test if the current side to move is in check
 */
inline bool inCheck(const Board& board) {
    return MoveGenerator::inCheck(board);
}

/**
 * Test if a square is attacked by the given color
 */
inline bool isSquareAttacked(const Board& board, Square square, Color attackingColor) {
    return MoveGenerator::isSquareAttacked(board, square, attackingColor);
}

} // namespace seajay