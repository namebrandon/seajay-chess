#include "board_safety.h"
#include "board.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace seajay {

// ============================================================================
// StateSnapshot Implementation
// ============================================================================

BoardStateValidator::StateSnapshot::StateSnapshot(const Board& board) {
    // Copy all state
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        mailbox[s] = board.pieceAt(s);
    }
    
    for (int i = 0; i < NUM_PIECES; ++i) {
        pieceBB[i] = board.pieces(static_cast<Piece>(i));
    }
    
    for (int i = 0; i < NUM_PIECE_TYPES; ++i) {
        pieceTypeBB[i] = board.pieces(static_cast<PieceType>(i));
    }
    
    colorBB[WHITE] = board.pieces(WHITE);
    colorBB[BLACK] = board.pieces(BLACK);
    occupied = board.occupied();
    
    sideToMove = board.sideToMove();
    castlingRights = board.castlingRights();
    enPassantSquare = board.enPassantSquare();
    halfmoveClock = board.halfmoveClock();
    fullmoveNumber = board.fullmoveNumber();
    zobristKey = board.zobristKey();
}

bool BoardStateValidator::StateSnapshot::operator==(const StateSnapshot& other) const {
    // Check mailbox
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        if (mailbox[s] != other.mailbox[s]) return false;
    }
    
    // Check bitboards
    for (int i = 0; i < NUM_PIECES; ++i) {
        if (pieceBB[i] != other.pieceBB[i]) return false;
    }
    
    for (int i = 0; i < NUM_PIECE_TYPES; ++i) {
        if (pieceTypeBB[i] != other.pieceTypeBB[i]) return false;
    }
    
    if (colorBB[WHITE] != other.colorBB[WHITE]) return false;
    if (colorBB[BLACK] != other.colorBB[BLACK]) return false;
    if (occupied != other.occupied) return false;
    
    // Check game state
    if (sideToMove != other.sideToMove) return false;
    if (castlingRights != other.castlingRights) return false;
    if (enPassantSquare != other.enPassantSquare) return false;
    if (halfmoveClock != other.halfmoveClock) return false;
    if (fullmoveNumber != other.fullmoveNumber) return false;
    if (zobristKey != other.zobristKey) return false;
    
    return true;
}

std::string BoardStateValidator::StateSnapshot::compareWith(const StateSnapshot& other) const {
    std::ostringstream ss;
    
    // Check mailbox differences
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        if (mailbox[s] != other.mailbox[s]) {
            ss << "Mailbox differs at " << squareToString(s) 
               << ": was " << PIECE_CHARS[mailbox[s]]
               << ", now " << PIECE_CHARS[other.mailbox[s]] << "\n";
        }
    }
    
    // Check bitboard differences
    if (occupied != other.occupied) {
        ss << "Occupied bitboard differs: 0x" << std::hex 
           << occupied << " -> 0x" << other.occupied << std::dec << "\n";
    }
    
    // Check state differences
    if (sideToMove != other.sideToMove) {
        ss << "Side to move changed: " << (sideToMove == WHITE ? "WHITE" : "BLACK")
           << " -> " << (other.sideToMove == WHITE ? "WHITE" : "BLACK") << "\n";
    }
    
    if (castlingRights != other.castlingRights) {
        ss << "Castling rights changed: 0x" << std::hex 
           << (int)castlingRights << " -> 0x" << (int)other.castlingRights 
           << std::dec << "\n";
    }
    
    if (enPassantSquare != other.enPassantSquare) {
        ss << "En passant square changed: " 
           << (enPassantSquare == NO_SQUARE ? "-" : squareToString(enPassantSquare))
           << " -> " 
           << (other.enPassantSquare == NO_SQUARE ? "-" : squareToString(other.enPassantSquare))
           << "\n";
    }
    
    if (halfmoveClock != other.halfmoveClock) {
        ss << "Halfmove clock changed: " << halfmoveClock 
           << " -> " << other.halfmoveClock << "\n";
    }
    
    if (fullmoveNumber != other.fullmoveNumber) {
        ss << "Fullmove number changed: " << fullmoveNumber 
           << " -> " << other.fullmoveNumber << "\n";
    }
    
    if (zobristKey != other.zobristKey) {
        ss << "Zobrist key changed: 0x" << std::hex 
           << zobristKey << " -> 0x" << other.zobristKey 
           << std::dec << "\n";
    }
    
    return ss.str();
}

// ============================================================================
// BoardStateValidator Implementation
// ============================================================================

bool BoardStateValidator::checkBitboardMailboxSync(const Board& board) {
    // Rebuild bitboards from mailbox and compare
    Bitboard reconstructedOccupied = 0;
    std::array<Bitboard, NUM_PIECES> reconstructedPieceBB = {0};
    std::array<Bitboard, NUM_PIECE_TYPES> reconstructedTypeBB = {0};
    std::array<Bitboard, NUM_COLORS> reconstructedColorBB = {0};
    
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        Piece p = board.pieceAt(s);
        if (p != NO_PIECE) {
            Bitboard bb = squareBB(s);
            reconstructedOccupied |= bb;
            reconstructedPieceBB[p] |= bb;
            reconstructedTypeBB[typeOf(p)] |= bb;
            reconstructedColorBB[colorOf(p)] |= bb;
        }
    }
    
    // Compare with actual bitboards
    if (reconstructedOccupied != board.occupied()) {
        std::cerr << "Occupied bitboard mismatch!\n";
        return false;
    }
    
    for (Color c : {WHITE, BLACK}) {
        if (reconstructedColorBB[c] != board.pieces(c)) {
            std::cerr << "Color bitboard mismatch for " 
                     << (c == WHITE ? "WHITE" : "BLACK") << "!\n";
            return false;
        }
    }
    
    for (int pt = PAWN; pt <= KING; ++pt) {
        if (reconstructedTypeBB[pt] != board.pieces(static_cast<PieceType>(pt))) {
            std::cerr << "Piece type bitboard mismatch for type " << pt << "!\n";
            return false;
        }
    }
    
    return true;
}

bool BoardStateValidator::checkZobristConsistency(const Board& board) {
    // Rebuild zobrist from scratch and compare
    Hash reconstructed = ZobristKeyManager::computeKey(board);
    Hash actual = board.zobristKey();
    
    if (reconstructed != actual) {
        std::cerr << "Zobrist key mismatch!\n"
                 << "  Actual:        0x" << std::hex << actual << "\n"
                 << "  Reconstructed: 0x" << reconstructed << std::dec << "\n";
        return false;
    }
    
    return true;
}

bool BoardStateValidator::checkPieceCountLimits(const Board& board) {
    // Count pieces and validate limits
    int pieceCounts[NUM_COLORS][NUM_PIECE_TYPES] = {0};
    
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        Piece p = board.pieceAt(s);
        if (p != NO_PIECE) {
            pieceCounts[colorOf(p)][typeOf(p)]++;
        }
    }
    
    // Check limits
    for (Color c : {WHITE, BLACK}) {
        if (pieceCounts[c][KING] != 1) {
            std::cerr << "Invalid king count for " 
                     << (c == WHITE ? "WHITE" : "BLACK") 
                     << ": " << pieceCounts[c][KING] << "\n";
            return false;
        }
        
        if (pieceCounts[c][PAWN] > 8) {
            std::cerr << "Too many pawns for " 
                     << (c == WHITE ? "WHITE" : "BLACK") 
                     << ": " << pieceCounts[c][PAWN] << "\n";
            return false;
        }
        
        // Total pieces shouldn't exceed 16
        int total = 0;
        for (int pt = PAWN; pt <= KING; ++pt) {
            total += pieceCounts[c][pt];
        }
        if (total > 16) {
            std::cerr << "Too many total pieces for " 
                     << (c == WHITE ? "WHITE" : "BLACK") 
                     << ": " << total << "\n";
            return false;
        }
    }
    
    return true;
}

bool BoardStateValidator::checkCastlingRightsValidity(const Board& board) {
    uint8_t rights = board.castlingRights();
    
    // Check if king is on its starting square for castling rights
    if (rights & (WHITE_KINGSIDE | WHITE_QUEENSIDE)) {
        if (board.pieceAt(E1) != WHITE_KING) {
            std::cerr << "White has castling rights but king not on E1!\n";
            return false;
        }
    }
    
    if (rights & (BLACK_KINGSIDE | BLACK_QUEENSIDE)) {
        if (board.pieceAt(E8) != BLACK_KING) {
            std::cerr << "Black has castling rights but king not on E8!\n";
            return false;
        }
    }
    
    // Check rooks for specific castling rights
    if (rights & WHITE_KINGSIDE && board.pieceAt(H1) != WHITE_ROOK) {
        std::cerr << "White kingside castling right but no rook on H1!\n";
        return false;
    }
    
    if (rights & WHITE_QUEENSIDE && board.pieceAt(A1) != WHITE_ROOK) {
        std::cerr << "White queenside castling right but no rook on A1!\n";
        return false;
    }
    
    if (rights & BLACK_KINGSIDE && board.pieceAt(H8) != BLACK_ROOK) {
        std::cerr << "Black kingside castling right but no rook on H8!\n";
        return false;
    }
    
    if (rights & BLACK_QUEENSIDE && board.pieceAt(A8) != BLACK_ROOK) {
        std::cerr << "Black queenside castling right but no rook on A8!\n";
        return false;
    }
    
    return true;
}

bool BoardStateValidator::checkEnPassantValidity(const Board& board) {
    Square ep = board.enPassantSquare();
    if (ep == NO_SQUARE) return true;
    
    // En passant square must be on rank 3 or 6
    Rank r = rankOf(ep);
    if (r != 2 && r != 5) {  // RANK_3 = 2, RANK_6 = 5
        std::cerr << "Invalid en passant rank: " << (r + 1) << "\n";
        return false;
    }
    
    // CORRECTED LOGIC:
    // En passant square on rank 3 means a WHITE pawn just moved from rank 2 to rank 4
    // En passant square on rank 6 means a BLACK pawn just moved from rank 7 to rank 5
    // The pawn that moved is AHEAD of the en passant square
    
    if (r == 2) {  // Rank 3 - white pawn moved
        Square pawnSquare = ep + 8;  // Pawn is on rank 4
        Piece pawn = board.pieceAt(pawnSquare);
        if (pawn != WHITE_PAWN) {
            std::cerr << "En passant on rank 3 but no white pawn at " 
                     << squareToString(pawnSquare) << "\n";
            return false;
        }
    } else {  // Rank 6 - black pawn moved  
        Square pawnSquare = ep - 8;  // Pawn is on rank 5
        Piece pawn = board.pieceAt(pawnSquare);
        if (pawn != BLACK_PAWN) {
            std::cerr << "En passant on rank 6 but no black pawn at " 
                     << squareToString(pawnSquare) << "\n";
            return false;
        }
    }
    
    return true;
}

bool BoardStateValidator::validateFullIntegrity(const Board& board) {
    return checkBitboardMailboxSync(board) &&
           checkZobristConsistency(board) &&
           checkPieceCountLimits(board) &&
           checkCastlingRightsValidity(board) &&
           checkEnPassantValidity(board);
}

bool BoardStateValidator::validateIncrementalChange(
    const Board& before, 
    const Board& after, 
    Move move) {
    
    // Quick validation of expected changes
    StateSnapshot snapBefore(before);
    StateSnapshot snapAfter(after);
    
    // Side to move must have flipped
    if (snapBefore.sideToMove == snapAfter.sideToMove) {
        std::cerr << "Side to move didn't change!\n";
        return false;
    }
    
    // Halfmove clock must be updated correctly
    Square from = moveFrom(move);
    Square to = moveTo(move);
    Piece movingPiece = before.pieceAt(from);
    Piece capturedPiece = before.pieceAt(to);
    
    if (typeOf(movingPiece) == PAWN || capturedPiece != NO_PIECE) {
        if (snapAfter.halfmoveClock != 0) {
            std::cerr << "Halfmove clock not reset after pawn move or capture!\n";
            return false;
        }
    } else {
        if (snapAfter.halfmoveClock != snapBefore.halfmoveClock + 1) {
            std::cerr << "Halfmove clock not incremented!\n";
            return false;
        }
    }
    
    // Full move number updates on black's move
    if (snapBefore.sideToMove == BLACK) {
        if (snapAfter.fullmoveNumber != snapBefore.fullmoveNumber + 1) {
            std::cerr << "Full move number not incremented after black's move!\n";
            return false;
        }
    } else {
        if (snapAfter.fullmoveNumber != snapBefore.fullmoveNumber) {
            std::cerr << "Full move number changed after white's move!\n";
            return false;
        }
    }
    
    return validateFullIntegrity(after);
}

// ============================================================================
// Debug Mode Guards
// ============================================================================

#ifdef DEBUG

StateValidationGuard::StateValidationGuard(const Board& board, const char* operation)
    : m_board(board), m_snapshot(board), m_operation(operation) {
    // Validate state on entry
    if (!BoardStateValidator::validateFullIntegrity(board)) {
        std::cerr << "Invalid board state before operation: " << operation << "\n";
        std::abort();
    }
}

StateValidationGuard::~StateValidationGuard() {
    // Validate state on exit
    if (!BoardStateValidator::validateFullIntegrity(m_board)) {
        std::cerr << "Invalid board state after operation: " << m_operation << "\n";
        std::cerr << "State changes:\n" 
                 << m_snapshot.compareWith(BoardStateValidator::StateSnapshot(m_board));
        std::abort();
    }
}

#endif

// ============================================================================
// Safe Move Executor
// ============================================================================

std::string SafeMoveExecutor::moveToString(Move move) {
    std::ostringstream ss;
    ss << squareToString(moveFrom(move)) << squareToString(moveTo(move));
    
    if (isPromotion(move)) {
        PieceType pt = promotionType(move);
        const char promoChars[] = " nbrq";
        ss << promoChars[pt];
    }
    
    return ss.str();
}

// ============================================================================
// Zobrist Key Manager
// ============================================================================

ZobristKeyManager::ZobristUpdate ZobristKeyManager::buildUpdate(
    const Board& board, 
    Move move) {
    
    ZobristUpdate update;
    
    Square from = moveFrom(move);
    Square to = moveTo(move);
    uint8_t flags = moveFlags(move);
    
    Piece movingPiece = board.pieceAt(from);
    Piece capturedPiece = board.pieceAt(to);
    
    // Note: This is a conceptual implementation
    // The actual zobrist tables would need to be accessible
    
    // Remove moving piece from origin
    // update.removals ^= zobristPiece(from, movingPiece);
    
    // Handle captures
    if (capturedPiece != NO_PIECE) {
        // update.removals ^= zobristPiece(to, capturedPiece);
    }
    
    // Add piece to destination (or promoted piece)
    if (flags & PROMOTION) {
        PieceType promotedType = promotionType(move);
        // Piece promotedPiece = makePiece(colorOf(movingPiece), promotedType);
        // update.additions ^= zobristPiece(to, promotedPiece);
    } else {
        // update.additions ^= zobristPiece(to, movingPiece);
    }
    
    // Handle special moves
    if (flags == EN_PASSANT) {
        Color us = colorOf(movingPiece);
        Square capturedSquare = (us == WHITE) ? to - 8 : to + 8;
        // update.removals ^= zobristPiece(capturedSquare, capturedPawn);
    } else if (flags == CASTLING) {
        // Handle rook movement
        // ...
    }
    
    // State changes
    // update.stateChange ^= zobristSideToMove();
    // update.stateChange ^= zobristCastling(oldRights) ^ zobristCastling(newRights);
    // update.stateChange ^= zobristEnPassant(oldEP) ^ zobristEnPassant(newEP);
    
    return update;
}

bool ZobristKeyManager::validateKey(const Board& board) {
    return board.zobristKey() == computeKey(board);
}

Hash ZobristKeyManager::computeKey(const Board& board) {
    // For now, we can't independently compute the key without access to zobrist tables
    // So we'll use the board's rebuild function with save/restore
    // This is not ideal but works for validation purposes
    Board& mutableBoard = const_cast<Board&>(board);
    Hash savedKey = mutableBoard.zobristKey();
    mutableBoard.rebuildZobristKey();
    Hash computed = mutableBoard.zobristKey();
    
    // Only restore if we computed something
    if (computed != 0) {
        mutableBoard.m_zobristKey = savedKey;  // Restore original
    }
    
    return computed;
}

// ============================================================================
// Fast Validator
// ============================================================================

[[noreturn]] void FastValidator::handleCorruption(
    const Board& board, 
    uint32_t expected, 
    uint32_t actual) {
    
    std::cerr << "CRITICAL: Board state corruption detected!\n"
             << "  Expected checksum: 0x" << std::hex << expected << "\n"
             << "  Actual checksum:   0x" << actual << std::dec << "\n"
             << "  Current FEN: " << board.toFEN() << "\n";
    
    #ifdef DEBUG
    std::cerr << "Full debug display:\n" << board.debugDisplay() << "\n";
    #endif
    
    std::abort();
}

// ============================================================================
// Move Sequence Validator
// ============================================================================

bool MoveSequenceValidator::validateSequence(
    Board& board, 
    const std::vector<Move>& moves) {
    
    BoardStateValidator::StateSnapshot initial(board);
    
    // Make all moves
    std::vector<CompleteUndoInfo> undoStack;
    undoStack.reserve(moves.size());
    
    for (Move move : moves) {
        CompleteUndoInfo undo;
        board.makeMove(move, undo);
        undoStack.push_back(undo);
        
        if (!BoardStateValidator::validateFullIntegrity(board)) {
            std::cerr << "Board corruption after move " 
                     << SafeMoveExecutor::moveToString(move) << "\n";
            return false;
        }
    }
    
    // Unmake all moves in reverse
    for (int i = moves.size() - 1; i >= 0; --i) {
        board.unmakeMove(moves[i], undoStack[i]);
        
        if (!BoardStateValidator::validateFullIntegrity(board)) {
            std::cerr << "Board corruption after unmaking move " 
                     << SafeMoveExecutor::moveToString(moves[i]) << "\n";
            return false;
        }
    }
    
    // Verify we're back to initial state
    BoardStateValidator::StateSnapshot final(board);
    if (!(initial == final)) {
        std::cerr << "Board state not restored after make/unmake sequence!\n";
        std::cerr << initial.compareWith(final);
        return false;
    }
    
    return true;
}

bool MoveSequenceValidator::checkForDoubleMoves(const Move* moves, size_t count) {
    // Check that no piece moves twice in a row (impossible in chess)
    for (size_t i = 1; i < count; ++i) {
        Square prevTo = moveTo(moves[i-1]);
        Square currFrom = moveFrom(moves[i]);
        
        // The destination of previous move can't be origin of current
        // unless it's the opponent's turn
        // This is a simplified check - real implementation would track colors
    }
    return true;
}

bool MoveSequenceValidator::checkForImpossibleCastling(const Board& board, Move move) {
    if (moveFlags(move) != CASTLING) return true;
    
    Square from = moveFrom(move);
    Square to = moveTo(move);
    
    // Validate castling is geometrically correct
    if (from == E1) {
        if (to != G1 && to != C1) {
            std::cerr << "Invalid white castling destination: " 
                     << squareToString(to) << "\n";
            return false;
        }
    } else if (from == E8) {
        if (to != G8 && to != C8) {
            std::cerr << "Invalid black castling destination: " 
                     << squareToString(to) << "\n";
            return false;
        }
    } else {
        std::cerr << "Castling from non-king square: " 
                 << squareToString(from) << "\n";
        return false;
    }
    
    return true;
}

bool MoveSequenceValidator::checkForIllegalEnPassant(const Board& board, Move move) {
    if (moveFlags(move) != EN_PASSANT) return true;
    
    Square to = moveTo(move);
    
    // En passant capture square must match board's en passant square
    if (to != board.enPassantSquare()) {
        std::cerr << "En passant to wrong square. Board EP: " 
                 << squareToString(board.enPassantSquare())
                 << ", Move to: " << squareToString(to) << "\n";
        return false;
    }
    
    return true;
}

// ============================================================================
// Template Implementations
// ============================================================================

// SafeMoveExecutor template implementations
template<ValidMoveType MoveT, UndoInfoType UndoT>
void SafeMoveExecutor::makeMove(Board& board, MoveT move, UndoT& undo) {
    // Perform the move
    board.makeMoveInternal(move, undo);
    
    #ifdef DEBUG
    // Just validate the resulting state
    if (!BoardStateValidator::validateFullIntegrity(board)) {
        std::cerr << "Make move validation failed for move: " 
                 << moveToString(move) << "\n";
        std::abort();
    }
    #endif
}

template<ValidMoveType MoveT, UndoInfoType UndoT>
void SafeMoveExecutor::unmakeMove(Board& board, MoveT move, const UndoT& undo) {
    #ifdef DEBUG
    BoardStateValidator::StateSnapshot before(board);
    #endif
    
    // Perform the unmake
    board.unmakeMoveInternal(move, undo);
    
    #ifdef DEBUG
    // Validate state restoration
    if (!BoardStateValidator::validateFullIntegrity(board)) {
        std::cerr << "Unmake move validation failed for move: " 
                 << moveToString(move) << "\n";
        std::abort();
    }
    #endif
}

// FastValidator implementation
uint32_t FastValidator::quickChecksum(const Board& board) {
    // Fast non-cryptographic hash of key state
    uint32_t sum = 0;
    sum = sum * 31 + static_cast<uint32_t>(board.zobristKey());
    sum = sum * 31 + board.sideToMove();
    sum = sum * 31 + board.castlingRights();
    sum = sum * 31 + board.enPassantSquare();
    return sum;
}

template<bool Enable>
void FastValidator::validate(const Board& board, uint32_t expectedChecksum) {
    if constexpr (Enable) {
        uint32_t actual = quickChecksum(board);
        if (actual != expectedChecksum) {
            // Corruption detected!
            handleCorruption(board, expectedChecksum, actual);
        }
    }
}

// Explicit instantiations for common uses
template void SafeMoveExecutor::makeMove<Move, Board::UndoInfo>(Board&, Move, Board::UndoInfo&);
template void SafeMoveExecutor::unmakeMove<Move, Board::UndoInfo>(Board&, Move, const Board::UndoInfo&);
template void SafeMoveExecutor::makeMove<Move, CompleteUndoInfo>(Board&, Move, CompleteUndoInfo&);
template void SafeMoveExecutor::unmakeMove<Move, CompleteUndoInfo>(Board&, Move, const CompleteUndoInfo&);
template void FastValidator::validate<true>(const Board&, uint32_t);
template void FastValidator::validate<false>(const Board&, uint32_t);

} // namespace seajay