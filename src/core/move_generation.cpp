#include "move_generation.h"
#include "bitboard.h"
#include "attack_wrapper.h"  // Use runtime-switchable attacks
#include <algorithm>
#include <cstdlib>

namespace seajay {

// Static member initialization
bool MoveGenerator::s_tablesInitialized = false;
std::array<Bitboard, 64> MoveGenerator::s_knightAttacks = {};
std::array<Bitboard, 64> MoveGenerator::s_kingAttacks = {};
std::array<Bitboard, 64> MoveGenerator::s_whitePawnAttacks = {};
std::array<Bitboard, 64> MoveGenerator::s_blackPawnAttacks = {};

void MoveGenerator::initializeAttackTables() {
    if (s_tablesInitialized) return;  // Already initialized
    
    // Initialize knight attack table
    constexpr int knightOffsets[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        Bitboard attacks = 0;
        int file = fileOf(sq);
        int rank = rankOf(sq);
        
        for (int offset : knightOffsets) {
            int targetInt = sq + offset;
            if (targetInt >= 0 && targetInt < 64) {
                Square target = static_cast<Square>(targetInt);
                int targetFile = fileOf(target);
                int targetRank = rankOf(target);
                
                // Check if the knight move is valid (not wrapping around board)
                int fileDiff = std::abs(targetFile - file);
                int rankDiff = std::abs(targetRank - rank);
                
                if ((fileDiff == 2 && rankDiff == 1) || (fileDiff == 1 && rankDiff == 2)) {
                    attacks |= squareBB(target);
                }
            }
        }
        s_knightAttacks[sq] = attacks;
    }
    
    // Initialize king attack table
    constexpr int kingOffsets[8] = {-9, -8, -7, -1, 1, 7, 8, 9};
    
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        Bitboard attacks = 0;
        int file = fileOf(sq);
        int rank = rankOf(sq);
        
        for (int offset : kingOffsets) {
            int targetInt = sq + offset;
            if (targetInt >= 0 && targetInt < 64) {
                Square target = static_cast<Square>(targetInt);
                int targetFile = fileOf(target);
                int targetRank = rankOf(target);
                
                // Check if the king move is valid (adjacent square only)
                int fileDiff = std::abs(targetFile - file);
                int rankDiff = std::abs(targetRank - rank);
                
                if (fileDiff <= 1 && rankDiff <= 1) {
                    attacks |= squareBB(target);
                }
            }
        }
        s_kingAttacks[sq] = attacks;
    }
    
    // Initialize pawn attack tables
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        int file = fileOf(sq);
        int rank = rankOf(sq);
        
        // White pawn attacks (moving "up" the board)
        Bitboard whiteAttacks = 0;
        if (rank < 7) { // Not on 8th rank
            if (file > 0) { // Can capture left
                whiteAttacks |= squareBB(sq + 7); // North-West
            }
            if (file < 7) { // Can capture right
                whiteAttacks |= squareBB(sq + 9); // North-East
            }
        }
        s_whitePawnAttacks[sq] = whiteAttacks;
        
            // Black pawn attacks (moving "down" the board)
        Bitboard blackAttacks = 0;
        if (rank > 0) { // Not on 1st rank
            if (file > 0) { // Can capture left
                blackAttacks |= squareBB(sq - 9); // South-West
            }
            if (file < 7) { // Can capture right
                blackAttacks |= squareBB(sq - 7); // South-East
            }
        }
        s_blackPawnAttacks[sq] = blackAttacks;
    }
    
    s_tablesInitialized = true;
}

void MoveGenerator::generatePseudoLegalMoves(const Board& board, MoveList& moves) {
    if (!s_tablesInitialized) {
        initializeAttackTables();
    }
    
    // Generate moves for all piece types
    generatePawnMoves(board, moves);
    generateKnightMoves(board, moves);
    generateBishopMoves(board, moves);
    generateRookMoves(board, moves);
    generateQueenMoves(board, moves);
    generateKingMoves(board, moves);
    generateCastlingMoves(board, moves);
}

void MoveGenerator::generateLegalMoves(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    
    // CRITICAL FIX: Check if we're in check FIRST
    if (inCheck(board, us)) {
        // When in check, generate only check evasion moves
        generateCheckEvasions(board, moves);
    } else {
        // Not in check - generate all pseudo-legal moves and filter
        MoveList pseudoLegalMoves;
        generatePseudoLegalMoves(board, pseudoLegalMoves);
        
        // Filter out moves that leave the king in check
        for (Move move : pseudoLegalMoves) {
            if (!leavesKingInCheck(board, move)) {
                moves.push_back(move);
            }
        }
    }
}

void MoveGenerator::generateCaptures(const Board& board, MoveList& moves) {
    generatePawnCaptures(board, moves);
    generateKnightCaptures(board, moves);
    generateBishopCaptures(board, moves);
    generateRookCaptures(board, moves);
    generateQueenCaptures(board, moves);
    generateKingCaptures(board, moves);
}

void MoveGenerator::generateQuietMoves(const Board& board, MoveList& moves) {
    generatePawnQuietMoves(board, moves);
    generateKnightQuietMoves(board, moves);
    generateBishopQuietMoves(board, moves);
    generateRookQuietMoves(board, moves);
    generateQueenQuietMoves(board, moves);
    generateKingQuietMoves(board, moves);
    generateCastlingMoves(board, moves);
}

// Pawn move generation
void MoveGenerator::generatePawnMoves(const Board& board, MoveList& moves) {
    generatePawnCaptures(board, moves);
    generatePawnQuietMoves(board, moves);
}

void MoveGenerator::generatePawnCaptures(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Color them = ~us;
    Bitboard ourPawns = board.pieces(us, PAWN);
    Bitboard theirPieces = board.pieces(them);
    Square epSquare = board.enPassantSquare();
    
    while (ourPawns) {
        Square from = popLsb(ourPawns);
        Bitboard attacks = getPawnAttacks(from, us);
        
        // Regular captures
        Bitboard captures = attacks & theirPieces;
        while (captures) {
            Square to = popLsb(captures);
            
            // Check for promotion
            if ((us == WHITE && rankOf(from) == 6) || (us == BLACK && rankOf(from) == 1)) {
                // Add promotion captures
                moves.addMove(from, to, PROMO_KNIGHT_CAPTURE);
                moves.addMove(from, to, PROMO_BISHOP_CAPTURE);
                moves.addMove(from, to, PROMO_ROOK_CAPTURE);
                moves.addMove(from, to, PROMO_QUEEN_CAPTURE);
            } else {
                moves.addMove(from, to, CAPTURE);
            }
        }
        
        // En passant capture
        if (epSquare != NO_SQUARE) {
            // Check if this pawn can capture en passant
            int pawnRank = rankOf(from);
            int pawnFile = fileOf(from);
            int epFile = fileOf(epSquare);
            int epRank = rankOf(epSquare);
            
            // White pawns must be on rank 5 (0-indexed as 4), en passant to rank 6 (0-indexed as 5)
            // Black pawns must be on rank 4 (0-indexed as 3), en passant to rank 3 (0-indexed as 2)
            bool correctRank = (us == WHITE && pawnRank == 4 && epRank == 5) || 
                               (us == BLACK && pawnRank == 3 && epRank == 2);
            
            // Pawn must be adjacent to the en passant file
            bool adjacentFile = std::abs(pawnFile - epFile) == 1;
            
            if (correctRank && adjacentFile) {
                // Verify there's actually an enemy pawn to capture
                Square captureSquare = (us == WHITE) ? epSquare - 8 : epSquare + 8;
                if (board.pieceAt(captureSquare) == makePiece(them, PAWN)) {
                    moves.addMove(from, epSquare, EN_PASSANT);
                }
            }
        }
    }
}

void MoveGenerator::generatePawnQuietMoves(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Bitboard ourPawns = board.pieces(us, PAWN);
    Bitboard occupied = board.occupied();
    
    int forward = (us == WHITE) ? 8 : -8;
    int startRank = (us == WHITE) ? 1 : 6;
    
    while (ourPawns) {
        Square from = popLsb(ourPawns);
        int toInt = static_cast<int>(from) + forward;
        
        // Single pawn push
        if (toInt >= 0 && toInt < 64) {
            Square to = static_cast<Square>(toInt);
            if (!(occupied & squareBB(to))) {
                // Check for promotion (pawn is moving from 7th rank to 8th rank)
                if ((us == WHITE && rankOf(from) == 6) || (us == BLACK && rankOf(from) == 1)) {
                    moves.addPromotionMoves(from, to);
                } else {
                    moves.addMove(from, to, NORMAL);
                    
                    // Double pawn push
                    if (rankOf(from) == startRank) {
                        int doubleToInt = static_cast<int>(to) + forward;
                        if (doubleToInt >= 0 && doubleToInt < 64) {
                            Square doubleTo = static_cast<Square>(doubleToInt);
                            if (!(occupied & squareBB(doubleTo))) {
                                moves.addMove(from, doubleTo, DOUBLE_PAWN);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Knight move generation
void MoveGenerator::generateKnightMoves(const Board& board, MoveList& moves) {
    generateKnightCaptures(board, moves);
    generateKnightQuietMoves(board, moves);
}

void MoveGenerator::generateKnightCaptures(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Color them = ~us;
    Bitboard ourKnights = board.pieces(us, KNIGHT);
    Bitboard theirPieces = board.pieces(them);
    
    while (ourKnights) {
        Square from = popLsb(ourKnights);
        Bitboard attacks = getKnightAttacks(from);
        Bitboard captures = attacks & theirPieces;
        
        while (captures) {
            Square to = popLsb(captures);
            moves.addMove(from, to, CAPTURE);
        }
    }
}

void MoveGenerator::generateKnightQuietMoves(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Bitboard ourKnights = board.pieces(us, KNIGHT);
    Bitboard occupied = board.occupied();
    
    while (ourKnights) {
        Square from = popLsb(ourKnights);
        Bitboard attacks = getKnightAttacks(from);
        Bitboard quietMoves = attacks & ~occupied;
        
        while (quietMoves) {
            Square to = popLsb(quietMoves);
            moves.addMove(from, to, NORMAL);
        }
    }
}

// Bishop move generation
void MoveGenerator::generateBishopMoves(const Board& board, MoveList& moves) {
    generateBishopCaptures(board, moves);
    generateBishopQuietMoves(board, moves);
}

void MoveGenerator::generateBishopCaptures(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Color them = ~us;
    Bitboard ourBishops = board.pieces(us, BISHOP);
    Bitboard theirPieces = board.pieces(them);
    Bitboard occupied = board.occupied();
    
    while (ourBishops) {
        Square from = popLsb(ourBishops);
        Bitboard attacks = seajay::getBishopAttacks(from, occupied);
        Bitboard captures = attacks & theirPieces;
        
        while (captures) {
            Square to = popLsb(captures);
            moves.addMove(from, to, CAPTURE);
        }
    }
}

void MoveGenerator::generateBishopQuietMoves(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Bitboard ourBishops = board.pieces(us, BISHOP);
    Bitboard occupied = board.occupied();
    
    while (ourBishops) {
        Square from = popLsb(ourBishops);
        Bitboard attacks = seajay::getBishopAttacks(from, occupied);
        Bitboard quietMoves = attacks & ~occupied;
        
        while (quietMoves) {
            Square to = popLsb(quietMoves);
            moves.addMove(from, to, NORMAL);
        }
    }
}

// Rook move generation
void MoveGenerator::generateRookMoves(const Board& board, MoveList& moves) {
    generateRookCaptures(board, moves);
    generateRookQuietMoves(board, moves);
}

void MoveGenerator::generateRookCaptures(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Color them = ~us;
    Bitboard ourRooks = board.pieces(us, ROOK);
    Bitboard theirPieces = board.pieces(them);
    Bitboard occupied = board.occupied();
    
    while (ourRooks) {
        Square from = popLsb(ourRooks);
        Bitboard attacks = seajay::getRookAttacks(from, occupied);
        Bitboard captures = attacks & theirPieces;
        
        while (captures) {
            Square to = popLsb(captures);
            moves.addMove(from, to, CAPTURE);
        }
    }
}

void MoveGenerator::generateRookQuietMoves(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Bitboard ourRooks = board.pieces(us, ROOK);
    Bitboard occupied = board.occupied();
    
    while (ourRooks) {
        Square from = popLsb(ourRooks);
        Bitboard attacks = seajay::getRookAttacks(from, occupied);
        Bitboard quietMoves = attacks & ~occupied;
        
        while (quietMoves) {
            Square to = popLsb(quietMoves);
            moves.addMove(from, to, NORMAL);
        }
    }
}

// Queen move generation
void MoveGenerator::generateQueenMoves(const Board& board, MoveList& moves) {
    generateQueenCaptures(board, moves);
    generateQueenQuietMoves(board, moves);
}

void MoveGenerator::generateQueenCaptures(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Color them = ~us;
    Bitboard ourQueens = board.pieces(us, QUEEN);
    Bitboard theirPieces = board.pieces(them);
    Bitboard occupied = board.occupied();
    
    while (ourQueens) {
        Square from = popLsb(ourQueens);
        Bitboard attacks = seajay::getQueenAttacks(from, occupied);
        Bitboard captures = attacks & theirPieces;
        
        while (captures) {
            Square to = popLsb(captures);
            moves.addMove(from, to, CAPTURE);
        }
    }
}

void MoveGenerator::generateQueenQuietMoves(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Bitboard ourQueens = board.pieces(us, QUEEN);
    Bitboard occupied = board.occupied();
    
    while (ourQueens) {
        Square from = popLsb(ourQueens);
        Bitboard attacks = seajay::getQueenAttacks(from, occupied);
        Bitboard quietMoves = attacks & ~occupied;
        
        while (quietMoves) {
            Square to = popLsb(quietMoves);
            moves.addMove(from, to, NORMAL);
        }
    }
}

// King move generation
void MoveGenerator::generateKingMoves(const Board& board, MoveList& moves) {
    generateKingCaptures(board, moves);
    generateKingQuietMoves(board, moves);
}

void MoveGenerator::generateKingCaptures(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Color them = ~us;
    Bitboard ourKing = board.pieces(us, KING);
    Bitboard theirPieces = board.pieces(them);
    
    if (ourKing) {
        Square from = lsb(ourKing);
        Bitboard attacks = getKingAttacks(from);
        Bitboard captures = attacks & theirPieces;
        
        while (captures) {
            Square to = popLsb(captures);
            moves.addMove(from, to, CAPTURE);
        }
    }
}

void MoveGenerator::generateKingQuietMoves(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Bitboard ourKing = board.pieces(us, KING);
    Bitboard occupied = board.occupied();
    
    if (ourKing) {
        Square from = lsb(ourKing);
        Bitboard attacks = getKingAttacks(from);
        Bitboard quietMoves = attacks & ~occupied;
        
        while (quietMoves) {
            Square to = popLsb(quietMoves);
            moves.addMove(from, to, NORMAL);
        }
    }
}

// Castling move generation
void MoveGenerator::generateCastlingMoves(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Color them = ~us;
    
    // FIX: Cannot castle when in check (this check is already correct)
    if (inCheck(board, us)) {
        return; // Cannot castle when in check
    }
    
    // Kingside castling
    if (us == WHITE && board.canCastle(WHITE_KINGSIDE)) {
        if (!(board.occupied() & (squareBB(F1) | squareBB(G1)))) {
            if (!isSquareAttacked(board, F1, them) && !isSquareAttacked(board, G1, them)) {
                moves.addMove(E1, G1, CASTLING);
            }
        }
    } else if (us == BLACK && board.canCastle(BLACK_KINGSIDE)) {
        if (!(board.occupied() & (squareBB(F8) | squareBB(G8)))) {
            if (!isSquareAttacked(board, F8, them) && !isSquareAttacked(board, G8, them)) {
                moves.addMove(E8, G8, CASTLING);
            }
        }
    }
    
    // Queenside castling
    if (us == WHITE && board.canCastle(WHITE_QUEENSIDE)) {
        // B1 only needs to be empty, not unattacked (pieces only, not attacks)
        if (!(board.occupied() & (squareBB(B1) | squareBB(C1) | squareBB(D1)))) {
            if (!isSquareAttacked(board, C1, them) && !isSquareAttacked(board, D1, them)) {
                moves.addMove(E1, C1, CASTLING);
            }
        }
    } else if (us == BLACK && board.canCastle(BLACK_QUEENSIDE)) {
        // B8 only needs to be empty, not unattacked (pieces only, not attacks)
        if (!(board.occupied() & (squareBB(B8) | squareBB(C8) | squareBB(D8)))) {
            if (!isSquareAttacked(board, C8, them) && !isSquareAttacked(board, D8, them)) {
                moves.addMove(E8, C8, CASTLING);
            }
        }
    }
}

// Attack generation functions
Bitboard MoveGenerator::getPawnAttacks(Square square, Color color) {
    if (!s_tablesInitialized) {
        initializeAttackTables();
    }
    
    return (color == WHITE) ? s_whitePawnAttacks[square] : s_blackPawnAttacks[square];
}

Bitboard MoveGenerator::getKnightAttacks(Square square) {
    if (!s_tablesInitialized) {
        initializeAttackTables();
    }
    
    return s_knightAttacks[square];
}

Bitboard MoveGenerator::bishopAttacks(Square square, Bitboard occupied) {
#ifdef USE_MAGIC_BITBOARDS
    // Use magic bitboards for fast attack generation
    return magicBishopAttacks(square, occupied);
#else
    // Use ray-based generation
    return seajay::getBishopAttacks(square, occupied);
#endif
}

Bitboard MoveGenerator::rookAttacks(Square square, Bitboard occupied) {
#ifdef USE_MAGIC_BITBOARDS
    // Use magic bitboards for fast attack generation
    return magicRookAttacks(square, occupied);
#else
    // Use ray-based generation
    return seajay::getRookAttacks(square, occupied);
#endif
}

Bitboard MoveGenerator::queenAttacks(Square square, Bitboard occupied) {
#ifdef USE_MAGIC_BITBOARDS
    // Use magic bitboards for fast attack generation
    return magicQueenAttacks(square, occupied);
#else
    // Use ray-based generation
    return seajay::getBishopAttacks(square, occupied) | seajay::getRookAttacks(square, occupied);
#endif
}

// Public wrappers for SEE (Stage 15)
Bitboard MoveGenerator::getBishopAttacks(Square square, Bitboard occupied) {
    return seajay::getBishopAttacks(square, occupied);
}

Bitboard MoveGenerator::getRookAttacks(Square square, Bitboard occupied) {
    return seajay::getRookAttacks(square, occupied);
}

Bitboard MoveGenerator::getQueenAttacks(Square square, Bitboard occupied) {
    return seajay::getQueenAttacks(square, occupied);
}

Bitboard MoveGenerator::getKingAttacks(Square square) {
    if (!s_tablesInitialized) {
        initializeAttackTables();
    }
    
    return s_kingAttacks[square];
}

// Check detection - Phase 2.1.a optimized version
bool MoveGenerator::isSquareAttacked(const Board& board, Square square, Color attackingColor) {
    // Phase 2.1.a: Reordered checks for early exit, optimized queen handling
    
    // 1. Check knight attacks first (most common attackers in middlegame, simple lookup)
    Bitboard knights = board.pieces(attackingColor, KNIGHT);
    if (knights & getKnightAttacks(square)) return true;
    
    // 2. Check pawn attacks second (numerous pieces, simple calculation)
    Bitboard pawns = board.pieces(attackingColor, PAWN);
    if (pawns) {
        Bitboard pawnAttacks = getPawnAttacks(square, ~attackingColor); // Reverse perspective
        if (pawns & pawnAttacks) return true;
    }
    
    // 3. Get occupied bitboard once (avoid multiple calls)
    Bitboard occupied = board.occupied();
    
    // 4. Check queen attacks (can attack like both bishop and rook)
    Bitboard queens = board.pieces(attackingColor, QUEEN);
    if (queens) {
        // Compute combined diagonal and straight attacks for queens
        // Use direct magic bitboard calls to avoid runtime config check in hot path
        Bitboard queenAttacks = seajay::magicBishopAttacks(square, occupied) | 
                               seajay::magicRookAttacks(square, occupied);
        if (queens & queenAttacks) return true;
    }
    
    // 5. Check bishop attacks (only if no queen found it on diagonal)
    Bitboard bishops = board.pieces(attackingColor, BISHOP);
    if (bishops) {
        // Direct magic bitboard call for hot path optimization
        Bitboard bishopAttacks = seajay::magicBishopAttacks(square, occupied);
        if (bishops & bishopAttacks) return true;
    }
    
    // 6. Check rook attacks (only if no queen found it on rank/file)
    Bitboard rooks = board.pieces(attackingColor, ROOK);
    if (rooks) {
        // Direct magic bitboard call for hot path optimization
        Bitboard rookAttacks = seajay::magicRookAttacks(square, occupied);
        if (rooks & rookAttacks) return true;
    }
    
    // 7. Check king attacks last (least likely attacker in most positions)
    Bitboard king = board.pieces(attackingColor, KING);
    if (king & getKingAttacks(square)) return true;
    
    return false;
}

bool MoveGenerator::inCheck(const Board& board) {
    return inCheck(board, board.sideToMove());
}

bool MoveGenerator::inCheck(const Board& board, Color kingColor) {
    Bitboard king = board.pieces(kingColor, KING);
    if (!king) return false; // No king (shouldn't happen in valid position)
    
    Square kingSquare = lsb(king);
    return isSquareAttacked(board, kingSquare, ~kingColor);
}

Bitboard MoveGenerator::getAttackedSquares(const Board& board, Color color) {
    Bitboard attacked = 0;
    Bitboard occupied = board.occupied();
    
    // Pawn attacks
    Bitboard pawns = board.pieces(color, PAWN);
    while (pawns) {
        Square square = popLsb(pawns);
        attacked |= getPawnAttacks(square, color);
    }
    
    // Knight attacks
    Bitboard knights = board.pieces(color, KNIGHT);
    while (knights) {
        Square square = popLsb(knights);
        attacked |= getKnightAttacks(square);
    }
    
    // Bishop attacks
    Bitboard bishops = board.pieces(color, BISHOP);
    while (bishops) {
        Square square = popLsb(bishops);
        attacked |= seajay::getBishopAttacks(square, occupied);
    }
    
    // Rook attacks
    Bitboard rooks = board.pieces(color, ROOK);
    while (rooks) {
        Square square = popLsb(rooks);
        attacked |= seajay::getRookAttacks(square, occupied);
    }
    
    // Queen attacks
    Bitboard queens = board.pieces(color, QUEEN);
    while (queens) {
        Square square = popLsb(queens);
        attacked |= seajay::getQueenAttacks(square, occupied);
    }
    
    // King attacks
    Bitboard king = board.pieces(color, KING);
    if (king) {
        Square square = lsb(king);
        attacked |= getKingAttacks(square);
    }
    
    return attacked;
}

// Move validation
bool MoveGenerator::isPseudoLegal(const Board& board, Move move) {
    // TODO: Implement thorough pseudo-legal move validation
    // For Stage 3, we'll implement a basic version
    Square from = moveFrom(move);
    Square to = moveTo(move);
    
    if (!isValidSquare(from) || !isValidSquare(to)) return false;
    if (from == to) return false;
    
    Piece piece = board.pieceAt(from);
    if (piece == NO_PIECE) return false;
    if (colorOf(piece) != board.sideToMove()) return false;
    
    // Basic validation - more thorough implementation in Stage 4
    return true;
}

bool MoveGenerator::isLegal(const Board& board, Move move) {
    if (!isPseudoLegal(board, move)) return false;
    return !leavesKingInCheck(board, move);
}

size_t MoveGenerator::countLegalMoves(const Board& board) {
    MoveList moves;
    generateLegalMoves(board, moves);
    return moves.size();
}

// Helper functions
bool MoveGenerator::leavesKingInCheck(const Board& board, Move move) {
    // FIX: Always use make/unmake for correct validation
    // The optimizations were incomplete and causing bugs
    
    Color us = board.sideToMove();
    Color opponent = ~us;
    
    // Create a copy and make the move
    Board tempBoard = board;
    Board::UndoInfo undo;
    
    tempBoard.makeMove(move, undo);
    
    // Check if our king is in check after the move
    Square kingSquare = tempBoard.kingSquare(us);
    if (kingSquare == NO_SQUARE) {
        // No king found - this shouldn't happen in a valid position
        tempBoard.unmakeMove(move, undo);
        return true;
    }
    
    bool inCheck = isSquareAttacked(tempBoard, kingSquare, opponent);
    
    tempBoard.unmakeMove(move, undo);
    
    return inCheck;
}

// Pin detection implementation
Bitboard MoveGenerator::getPinnedPieces(const Board& board, Color kingColor) {
    Square kingSquare = board.kingSquare(kingColor);
    if (kingSquare == NO_SQUARE) return 0;
    
    Color opponent = ~kingColor;
    Bitboard pinned = 0;
    Bitboard occupied = board.occupied();
    
    // Check for pins along rook/queen rays (rank and file)
    Bitboard rookAttackers = (board.pieces(opponent, ROOK) | board.pieces(opponent, QUEEN));
    while (rookAttackers) {
        Square attackerSquare = popLsb(rookAttackers);
        
        // Check if this attacker can pin along a rank/file to the king
        Bitboard rayToKing = seajay::getRookAttacks(attackerSquare, occupied) & squareBB(kingSquare);
        if (rayToKing) {
            // There's a ray from attacker to king - check what's in between
            Bitboard between = ::seajay::between(attackerSquare, kingSquare) & occupied;
            
            if (popCount(between) == 1) {
                // Exactly one piece between attacker and king - it's pinned
                Square pinnedSquare = lsb(between);
                Piece pinnedPiece = board.pieceAt(pinnedSquare);
                
                // Only our pieces can be pinned
                if (colorOf(pinnedPiece) == kingColor) {
                    pinned |= squareBB(pinnedSquare);
                }
            }
        }
    }
    
    // Check for pins along bishop/queen rays (diagonals)
    Bitboard bishopAttackers = (board.pieces(opponent, BISHOP) | board.pieces(opponent, QUEEN));
    while (bishopAttackers) {
        Square attackerSquare = popLsb(bishopAttackers);
        
        // Check if this attacker can pin along a diagonal to the king
        Bitboard rayToKing = seajay::getBishopAttacks(attackerSquare, occupied) & squareBB(kingSquare);
        if (rayToKing) {
            // There's a ray from attacker to king - check what's in between
            Bitboard between = ::seajay::between(attackerSquare, kingSquare) & occupied;
            
            if (popCount(between) == 1) {
                // Exactly one piece between attacker and king - it's pinned
                Square pinnedSquare = lsb(between);
                Piece pinnedPiece = board.pieceAt(pinnedSquare);
                
                // Only our pieces can be pinned
                if (colorOf(pinnedPiece) == kingColor) {
                    pinned |= squareBB(pinnedSquare);
                }
            }
        }
    }
    
    return pinned;
}

bool MoveGenerator::isPinned(const Board& board, Square square, Color kingColor) {
    Bitboard pinnedPieces = getPinnedPieces(board, kingColor);
    return pinnedPieces & squareBB(square);
}

// Helper function to get the pin ray for a pinned piece
Bitboard MoveGenerator::getPinRay(const Board& board, Square pinnedSquare, Square kingSquare) {
    // Get the ray from the pinned piece through the king
    Bitboard occupied = board.occupied();
    Color kingColor = colorOf(board.pieceAt(kingSquare));
    Color opponent = ~kingColor;
    
    // Check rook/queen pins
    if (rankOf(pinnedSquare) == rankOf(kingSquare) || fileOf(pinnedSquare) == fileOf(kingSquare)) {
        // Rank or file alignment - rook-style pin
        Bitboard rookAttackers = (board.pieces(opponent, ROOK) | board.pieces(opponent, QUEEN));
        
        while (rookAttackers) {
            Square attacker = popLsb(rookAttackers);
            Bitboard between = ::seajay::between(attacker, kingSquare);
            
            if ((between & occupied) == squareBB(pinnedSquare)) {
                // This attacker pins our piece
                return ::seajay::between(attacker, kingSquare) | squareBB(attacker) | squareBB(kingSquare);
            }
        }
    }
    
    // Check bishop/queen pins
    if (std::abs(rankOf(pinnedSquare) - rankOf(kingSquare)) == 
        std::abs(fileOf(pinnedSquare) - fileOf(kingSquare))) {
        // Diagonal alignment - bishop-style pin
        Bitboard bishopAttackers = (board.pieces(opponent, BISHOP) | board.pieces(opponent, QUEEN));
        
        while (bishopAttackers) {
            Square attacker = popLsb(bishopAttackers);
            Bitboard between = ::seajay::between(attacker, kingSquare);
            
            if ((between & occupied) == squareBB(pinnedSquare)) {
                // This attacker pins our piece
                return ::seajay::between(attacker, kingSquare) | squareBB(attacker) | squareBB(kingSquare);
            }
        }
    }
    
    return 0; // Not pinned or no ray found
}

// Helper function to check if moving a piece could discover a check
bool MoveGenerator::couldDiscoverCheck(const Board& board, Square from, Square kingSquare, Color opponent) {
    // Check if there's a potential discoverer between the king and enemy sliding pieces
    Bitboard occupied = board.occupied();
    
    // Check for discovered rook/queen attacks
    if (rankOf(from) == rankOf(kingSquare) || fileOf(from) == fileOf(kingSquare)) {
        // Moving piece is on same rank/file as king
        Bitboard rookAttackers = (board.pieces(opponent, ROOK) | board.pieces(opponent, QUEEN));
        Bitboard rookAttacksFromKing = seajay::getRookAttacks(kingSquare, occupied ^ squareBB(from));
        
        if (rookAttacksFromKing & rookAttackers) {
            return true; // Moving this piece would expose king to rook/queen
        }
    }
    
    // Check for discovered bishop/queen attacks
    if (std::abs(rankOf(from) - rankOf(kingSquare)) == 
        std::abs(fileOf(from) - fileOf(kingSquare))) {
        // Moving piece is on same diagonal as king
        Bitboard bishopAttackers = (board.pieces(opponent, BISHOP) | board.pieces(opponent, QUEEN));
        Bitboard bishopAttacksFromKing = seajay::getBishopAttacks(kingSquare, occupied ^ squareBB(from));
        
        if (bishopAttacksFromKing & bishopAttackers) {
            return true; // Moving this piece would expose king to bishop/queen
        }
    }
    
    return false;
}

// New helper functions for check evasion

Bitboard MoveGenerator::getCheckers(const Board& board, Square kingSquare, Color attackingColor) {
    Bitboard checkers = 0;
    
    // Check for pawn checks
    Bitboard pawnAttacks = getPawnAttacks(kingSquare, ~attackingColor);
    checkers |= pawnAttacks & board.pieces(attackingColor, PAWN);
    
    // Check for knight checks
    Bitboard knightAttacks = getKnightAttacks(kingSquare);
    checkers |= knightAttacks & board.pieces(attackingColor, KNIGHT);
    
    // Check for bishop/queen checks
    Bitboard bishopAttacks = seajay::getBishopAttacks(kingSquare, board.occupied());
    checkers |= bishopAttacks & (board.pieces(attackingColor, BISHOP) | board.pieces(attackingColor, QUEEN));
    
    // Check for rook/queen checks
    Bitboard rookAttacks = seajay::getRookAttacks(kingSquare, board.occupied());
    checkers |= rookAttacks & (board.pieces(attackingColor, ROOK) | board.pieces(attackingColor, QUEEN));
    
    // King cannot give check (but include for completeness)
    Bitboard kingAttacks = getKingAttacks(kingSquare);
    checkers |= kingAttacks & board.pieces(attackingColor, KING);
    
    return checkers;
}

void MoveGenerator::generateCheckEvasions(const Board& board, MoveList& moves) {
    Color us = board.sideToMove();
    Color them = ~us;
    
    Square kingSquare = board.kingSquare(us);
    if (kingSquare == NO_SQUARE) return; // No king - invalid position
    
    // Find all checking pieces
    Bitboard checkers = getCheckers(board, kingSquare, them);
    int numCheckers = popCount(checkers);
    
    if (numCheckers == 0) {
        // Not in check - shouldn't happen if this function is called correctly
        // Generate all legal moves as fallback
        MoveList pseudoLegalMoves;
        generatePseudoLegalMoves(board, pseudoLegalMoves);
        for (Move move : pseudoLegalMoves) {
            if (!leavesKingInCheck(board, move)) {
                moves.push_back(move);
            }
        }
        return;
    }
    
    // Always generate king moves (king must move away from check)
    generateKingEvasions(board, moves, kingSquare);
    
    // If double check, only king moves are possible
    if (numCheckers > 1) {
        return;
    }
    
    // Single check - can also block or capture the checking piece
    Square checkerSquare = lsb(checkers);
    
    // Generate captures of the checking piece
    generateCapturesOf(board, moves, checkerSquare);
    
    // For sliding piece checks, generate blocking moves
    Piece checkerPiece = board.pieceAt(checkerSquare);
    PieceType checkerType = typeOf(checkerPiece);
    
    if (checkerType == BISHOP || checkerType == ROOK || checkerType == QUEEN) {
        // Can block sliding piece attacks
        Bitboard blockSquares = ::seajay::between(checkerSquare, kingSquare);
        if (blockSquares) {
            generateBlockingMoves(board, moves, blockSquares);
        }
    }
    
    // CRITICAL FIX: Handle en passant in check evasion
    // En passant can evade check by:
    // 1. Capturing the checking pawn (if it just moved two squares)
    // 2. Blocking a sliding piece check
    Square epSquare = board.enPassantSquare();
    if (epSquare != NO_SQUARE) {
        // Find pawns that can capture en passant
        Bitboard ourPawns = board.pieces(us, PAWN);
        
        // Determine the rank and file for en passant
        int epRank = rankOf(epSquare);
        int epFile = fileOf(epSquare);
        
        // Check correct rank for en passant
        int requiredPawnRank = (us == WHITE) ? 4 : 3;  // 0-indexed
        
        while (ourPawns) {
            Square from = popLsb(ourPawns);
            int pawnRank = rankOf(from);
            int pawnFile = fileOf(from);
            
            // Check if this pawn can capture en passant
            if (pawnRank == requiredPawnRank && std::abs(pawnFile - epFile) == 1) {
                // Verify the en passant square is correct
                if ((us == WHITE && epRank == 5) || (us == BLACK && epRank == 2)) {
                    // The en passant capture could potentially evade check
                    // Add it to the move list - it will be validated later
                    moves.addMove(from, epSquare, EN_PASSANT);
                }
            }
        }
    }
    
    // Filter out any moves that still leave king in check
    // (necessary for complex cases like pinned pieces)
    MoveList validMoves;
    for (Move move : moves) {
        if (!leavesKingInCheck(board, move)) {
            validMoves.push_back(move);
        }
    }
    moves = validMoves;
}

void MoveGenerator::generateKingEvasions(const Board& board, MoveList& moves, Square kingSquare) {
    Color us = board.sideToMove();
    Color them = ~us;
    
    Bitboard kingMoves = getKingAttacks(kingSquare);
    Bitboard ourPieces = board.pieces(us);
    
    // Remove squares occupied by our pieces
    kingMoves &= ~ourPieces;
    
    while (kingMoves) {
        Square to = popLsb(kingMoves);
        
        // Check if this square is safe using the specialized function
        // that removes the king from occupancy for slider attacks
        if (isKingMoveSafe(board, kingSquare, to, them)) {
            if (board.pieceAt(to) != NO_PIECE) {
                moves.addMove(kingSquare, to, CAPTURE);
            } else {
                moves.addMove(kingSquare, to, NORMAL);
            }
        }
    }
}

bool MoveGenerator::isKingMoveSafe(const Board& board, Square from, Square to, Color enemyColor) {
    // Remove king from occupancy to properly detect slider attacks
    Bitboard occupancyNoKing = board.occupied() ^ squareBB(from);
    
    // Check non-slider attacks normally (these don't depend on king blocking)
    // Pawn attacks - check if enemy pawns can attack the destination square
    Bitboard enemyPawns = board.pieces(enemyColor, PAWN);
    if (enemyPawns & getPawnAttacks(to, ~enemyColor)) {
        return false;
    }
    
    // Knight attacks - check if enemy knights can attack the destination square
    Bitboard enemyKnights = board.pieces(enemyColor, KNIGHT);
    if (enemyKnights & getKnightAttacks(to)) {
        return false;
    }
    
    // King attacks - check if enemy king can attack the destination square
    Bitboard enemyKing = board.pieces(enemyColor, KING);
    if (enemyKing & getKingAttacks(to)) {
        return false;
    }
    
    // Check slider attacks with modified occupancy
    // For sliders, we need to check if attacks FROM the destination square
    // can reach enemy pieces, using the modified occupancy
    
    // Bishop and diagonal queen attacks
    Bitboard bishopAttacks = getBishopAttacks(to, occupancyNoKing);
    Bitboard bishopsQueens = board.pieces(enemyColor, BISHOP) | board.pieces(enemyColor, QUEEN);
    if (bishopAttacks & bishopsQueens) {
        return false;
    }
    
    // Rook and orthogonal queen attacks
    Bitboard rookAttacks = getRookAttacks(to, occupancyNoKing);
    Bitboard rooksQueens = board.pieces(enemyColor, ROOK) | board.pieces(enemyColor, QUEEN);
    if (rookAttacks & rooksQueens) {
        return false;
    }
    
    return true;
}

void MoveGenerator::generateCapturesOf(const Board& board, MoveList& moves, Square target) {
    Color us = board.sideToMove();
    
    Bitboard targetBB = squareBB(target);
    
    // Pawn captures
    Bitboard ourPawns = board.pieces(us, PAWN);
    while (ourPawns) {
        Square from = popLsb(ourPawns);
        Bitboard attacks = getPawnAttacks(from, us);
        if (attacks & targetBB) {
            // Check for promotion
            if ((us == WHITE && rankOf(from) == 6) || (us == BLACK && rankOf(from) == 1)) {
                moves.addMove(from, target, PROMO_QUEEN_CAPTURE);
                moves.addMove(from, target, PROMO_KNIGHT_CAPTURE);
                moves.addMove(from, target, PROMO_ROOK_CAPTURE);
                moves.addMove(from, target, PROMO_BISHOP_CAPTURE);
            } else {
                moves.addMove(from, target, CAPTURE);
            }
        }
    }
    
    // Knight captures
    Bitboard ourKnights = board.pieces(us, KNIGHT);
    while (ourKnights) {
        Square from = popLsb(ourKnights);
        if (getKnightAttacks(from) & targetBB) {
            moves.addMove(from, target, CAPTURE);
        }
    }
    
    // Bishop captures
    Bitboard ourBishops = board.pieces(us, BISHOP);
    while (ourBishops) {
        Square from = popLsb(ourBishops);
        if (seajay::getBishopAttacks(from, board.occupied()) & targetBB) {
            moves.addMove(from, target, CAPTURE);
        }
    }
    
    // Rook captures
    Bitboard ourRooks = board.pieces(us, ROOK);
    while (ourRooks) {
        Square from = popLsb(ourRooks);
        if (seajay::getRookAttacks(from, board.occupied()) & targetBB) {
            moves.addMove(from, target, CAPTURE);
        }
    }
    
    // Queen captures
    Bitboard ourQueens = board.pieces(us, QUEEN);
    while (ourQueens) {
        Square from = popLsb(ourQueens);
        if (seajay::getQueenAttacks(from, board.occupied()) & targetBB) {
            moves.addMove(from, target, CAPTURE);
        }
    }
}

void MoveGenerator::generateBlockingMoves(const Board& board, MoveList& moves, Bitboard blockSquares) {
    Color us = board.sideToMove();
    Bitboard occupied = board.occupied();
    
    // Generate moves to block squares
    while (blockSquares) {
        Square blockSq = popLsb(blockSquares);
        
        // Pawn blocks
        int pawnDirection = (us == WHITE) ? 8 : -8;  // WHITE moves +8 (up), BLACK moves -8 (down)
        
        // Single pawn push to block
        Square pawnFrom = static_cast<Square>(blockSq - pawnDirection);
        if (isValidSquare(pawnFrom) && board.pieceAt(pawnFrom) == makePiece(us, PAWN)) {
            // Check for promotion
            if ((us == WHITE && rankOf(pawnFrom) == 6) || (us == BLACK && rankOf(pawnFrom) == 1)) {
                moves.addPromotionMoves(pawnFrom, blockSq);
            } else {
                moves.addMove(pawnFrom, blockSq, NORMAL);
            }
        }
        
        // Double pawn push to block
        int startRank = (us == WHITE) ? 1 : 6;
        Square doublePawnFrom = static_cast<Square>(blockSq - 2 * pawnDirection);
        if (isValidSquare(doublePawnFrom) && rankOf(doublePawnFrom) == startRank &&
            board.pieceAt(doublePawnFrom) == makePiece(us, PAWN) &&
            board.pieceAt(static_cast<Square>(blockSq - pawnDirection)) == NO_PIECE) {
            moves.addMove(doublePawnFrom, blockSq, DOUBLE_PAWN);
        }
        
        // Knight blocks
        Bitboard knightAttackers = getKnightAttacks(blockSq) & board.pieces(us, KNIGHT);
        while (knightAttackers) {
            Square from = popLsb(knightAttackers);
            moves.addMove(from, blockSq, NORMAL);
        }
        
        // Bishop blocks
        Bitboard bishopAttackers = seajay::getBishopAttacks(blockSq, occupied) & board.pieces(us, BISHOP);
        while (bishopAttackers) {
            Square from = popLsb(bishopAttackers);
            moves.addMove(from, blockSq, NORMAL);
        }
        
        // Rook blocks
        Bitboard rookAttackers = seajay::getRookAttacks(blockSq, occupied) & board.pieces(us, ROOK);
        while (rookAttackers) {
            Square from = popLsb(rookAttackers);
            moves.addMove(from, blockSq, NORMAL);
        }
        
        // Queen blocks
        Bitboard queenAttackers = seajay::getQueenAttacks(blockSq, occupied) & board.pieces(us, QUEEN);
        while (queenAttackers) {
            Square from = popLsb(queenAttackers);
            moves.addMove(from, blockSq, NORMAL);
        }
    }
}

} // namespace seajay