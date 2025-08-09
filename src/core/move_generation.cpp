#include "move_generation.h"
#include "bitboard.h"
#include <algorithm>

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
    
    for (Square sq = 0; sq < 64; ++sq) {
        Bitboard attacks = 0;
        int file = fileOf(sq);
        int rank = rankOf(sq);
        
        for (int offset : knightOffsets) {
            int target = sq + offset;
            if (target >= 0 && target < 64) {
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
    
    for (Square sq = 0; sq < 64; ++sq) {
        Bitboard attacks = 0;
        int file = fileOf(sq);
        int rank = rankOf(sq);
        
        for (int offset : kingOffsets) {
            int target = sq + offset;
            if (target >= 0 && target < 64) {
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
    for (Square sq = 0; sq < 64; ++sq) {
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
    MoveList pseudoLegalMoves;
    generatePseudoLegalMoves(board, pseudoLegalMoves);
    
    // Filter out moves that leave the king in check
    for (Move move : pseudoLegalMoves) {
        if (!leavesKingInCheck(board, move)) {
            moves.push_back(move);
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
        if (epSquare != NO_SQUARE && (attacks & squareBB(epSquare))) {
            // Verify there's actually an enemy pawn to capture
            Square captureSquare = (us == WHITE) ? epSquare - 8 : epSquare + 8;
            if (board.pieceAt(captureSquare) == makePiece(them, PAWN)) {
                moves.addMove(from, epSquare, EN_PASSANT);
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
        Bitboard attacks = bishopAttacks(from, occupied);
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
        Bitboard attacks = bishopAttacks(from, occupied);
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
        Bitboard attacks = rookAttacks(from, occupied);
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
        Bitboard attacks = rookAttacks(from, occupied);
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
        Bitboard attacks = queenAttacks(from, occupied);
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
        Bitboard attacks = queenAttacks(from, occupied);
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
    // Use ray-based generation for now (will be replaced with magic bitboards in Phase 3)
    return ::seajay::bishopAttacks(square, occupied);
}

Bitboard MoveGenerator::rookAttacks(Square square, Bitboard occupied) {
    // Use ray-based generation for now (will be replaced with magic bitboards in Phase 3)
    return ::seajay::rookAttacks(square, occupied);
}

Bitboard MoveGenerator::queenAttacks(Square square, Bitboard occupied) {
    return ::seajay::bishopAttacks(square, occupied) | ::seajay::rookAttacks(square, occupied);
}

Bitboard MoveGenerator::getKingAttacks(Square square) {
    if (!s_tablesInitialized) {
        initializeAttackTables();
    }
    
    return s_kingAttacks[square];
}

// Check detection
bool MoveGenerator::isSquareAttacked(const Board& board, Square square, Color attackingColor) {
    // Check for pawn attacks
    Bitboard pawns = board.pieces(attackingColor, PAWN);
    Bitboard pawnAttacks = getPawnAttacks(square, ~attackingColor); // Reverse perspective
    if (pawns & pawnAttacks) return true;
    
    // Check for knight attacks
    Bitboard knights = board.pieces(attackingColor, KNIGHT);
    if (knights & getKnightAttacks(square)) return true;
    
    // Check for bishop/queen attacks
    Bitboard bishopsQueens = board.pieces(attackingColor, BISHOP) | board.pieces(attackingColor, QUEEN);
    if (bishopsQueens & bishopAttacks(square, board.occupied())) return true;
    
    // Check for rook/queen attacks
    Bitboard rooksQueens = board.pieces(attackingColor, ROOK) | board.pieces(attackingColor, QUEEN);
    if (rooksQueens & rookAttacks(square, board.occupied())) return true;
    
    // Check for king attacks
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
        attacked |= bishopAttacks(square, occupied);
    }
    
    // Rook attacks
    Bitboard rooks = board.pieces(color, ROOK);
    while (rooks) {
        Square square = popLsb(rooks);
        attacked |= rookAttacks(square, occupied);
    }
    
    // Queen attacks
    Bitboard queens = board.pieces(color, QUEEN);
    while (queens) {
        Square square = popLsb(queens);
        attacked |= queenAttacks(square, occupied);
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
    // Proper legal move validation using make/unmake pattern
    
    Square from = moveFrom(move);
    Square to = moveTo(move);
    Piece movingPiece = board.pieceAt(from);
    
    if (movingPiece == NO_PIECE) return true; // Invalid move
    
    Color us = board.sideToMove();
    Color opponent = ~us;
    
    // Handle special cases efficiently without make/unmake when possible
    
    // Case 1: King moves - check if destination is attacked
    if (typeOf(movingPiece) == KING) {
        // For castling, we need additional checks
        if (isCastling(move)) {
            // King cannot castle through check or from check
            if (isSquareAttacked(board, from, opponent)) {
                return true; // Cannot castle from check
            }
            
            // Check intermediate square for castling
            Square intermediate;
            if (to == G1 || to == G8) {
                // Kingside castling
                intermediate = (us == WHITE) ? F1 : F8;
            } else {
                // Queenside castling  
                intermediate = (us == WHITE) ? D1 : D8;
            }
            
            if (isSquareAttacked(board, intermediate, opponent) || 
                isSquareAttacked(board, to, opponent)) {
                return true; // Cannot castle through or to check
            }
            
            return false; // Castling is legal
        } else {
            // Normal king move - just check if destination is attacked
            return isSquareAttacked(board, to, opponent);
        }
    }
    
    // Case 2: En passant moves - need special handling for discovered checks
    if (isEnPassant(move)) {
        // En passant can expose the king to check in complex ways
        // We need make/unmake for this
        Board tempBoard = board; // Create a proper copy
        Board::UndoInfo undo;
        
        tempBoard.makeMove(move, undo);
        bool inCheck = isSquareAttacked(tempBoard, tempBoard.kingSquare(us), opponent);
        tempBoard.unmakeMove(move, undo);
        
        return inCheck;
    }
    
    // Case 3: Check if the moving piece is pinned
    Square kingSquare = board.kingSquare(us);
    if (kingSquare == NO_SQUARE) return true; // No king found - illegal position
    
    if (isPinned(board, from, us)) {
        // Piece is pinned - check if move is along the pin ray
        Bitboard pinRay = getPinRay(board, from, kingSquare);
        if (!(pinRay & squareBB(to))) {
            return true; // Move not along pin ray - leaves king in check
        }
    }
    
    // Case 4: Check for discovered attacks
    // If the moving piece is between the king and an enemy sliding piece,
    // moving it might expose the king to check
    if (couldDiscoverCheck(board, from, kingSquare, opponent)) {
        // Use make/unmake to verify
        Board tempBoard = board; // Create a proper copy
        Board::UndoInfo undo;
        
        tempBoard.makeMove(move, undo);
        bool inCheck = isSquareAttacked(tempBoard, tempBoard.kingSquare(us), opponent);
        tempBoard.unmakeMove(move, undo);
        
        return inCheck;
    }
    
    // No special cases detected - move is likely legal
    return false;
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
        Bitboard rayToKing = ::seajay::rookAttacks(attackerSquare, occupied) & squareBB(kingSquare);
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
        Bitboard rayToKing = ::seajay::bishopAttacks(attackerSquare, occupied) & squareBB(kingSquare);
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
        Bitboard rookAttacksFromKing = ::seajay::rookAttacks(kingSquare, occupied ^ squareBB(from));
        
        if (rookAttacksFromKing & rookAttackers) {
            return true; // Moving this piece would expose king to rook/queen
        }
    }
    
    // Check for discovered bishop/queen attacks
    if (std::abs(rankOf(from) - rankOf(kingSquare)) == 
        std::abs(fileOf(from) - fileOf(kingSquare))) {
        // Moving piece is on same diagonal as king
        Bitboard bishopAttackers = (board.pieces(opponent, BISHOP) | board.pieces(opponent, QUEEN));
        Bitboard bishopAttacksFromKing = ::seajay::bishopAttacks(kingSquare, occupied ^ squareBB(from));
        
        if (bishopAttacksFromKing & bishopAttackers) {
            return true; // Moving this piece would expose king to bishop/queen
        }
    }
    
    return false;
}

} // namespace seajay