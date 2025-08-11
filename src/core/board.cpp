#include "board.h"
#include "board_safety.h"
#include "bitboard.h"
#include "move_generation.h"
#include "../evaluation/evaluate.h"
#include "../evaluation/pst.h"  // For PST::value
#include "../search/search_info.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <cassert>
#include <iostream>

namespace seajay {

std::array<std::array<Hash, NUM_PIECES>, NUM_SQUARES> Board::s_zobristPieces;
std::array<Hash, NUM_SQUARES> Board::s_zobristEnPassant;
std::array<Hash, 16> Board::s_zobristCastling;
Hash Board::s_zobristSideToMove;
bool Board::s_zobristInitialized = false;

// Character-to-piece lookup table
std::array<Piece, 256> Board::PIECE_CHAR_LUT;
bool Board::s_lutInitialized = false;

// Initialize the piece character lookup table
void initPieceCharLookup() {
    if (Board::s_lutInitialized) return;  // Prevent double initialization
    
    Board::PIECE_CHAR_LUT.fill(NO_PIECE);
    Board::PIECE_CHAR_LUT['P'] = WHITE_PAWN;   Board::PIECE_CHAR_LUT['N'] = WHITE_KNIGHT; Board::PIECE_CHAR_LUT['B'] = WHITE_BISHOP;
    Board::PIECE_CHAR_LUT['R'] = WHITE_ROOK;   Board::PIECE_CHAR_LUT['Q'] = WHITE_QUEEN;  Board::PIECE_CHAR_LUT['K'] = WHITE_KING;
    Board::PIECE_CHAR_LUT['p'] = BLACK_PAWN;   Board::PIECE_CHAR_LUT['n'] = BLACK_KNIGHT; Board::PIECE_CHAR_LUT['b'] = BLACK_BISHOP;
    Board::PIECE_CHAR_LUT['r'] = BLACK_ROOK;   Board::PIECE_CHAR_LUT['q'] = BLACK_QUEEN;  Board::PIECE_CHAR_LUT['k'] = BLACK_KING;
    Board::s_lutInitialized = true;
}

Board::Board() {
    if (!s_zobristInitialized) {
        initZobrist();
    }
    if (!s_lutInitialized) {
        initPieceCharLookup();
    }
    
    // Stage 9b: Initialize position history
    m_gameHistory.reserve(MAX_GAME_HISTORY);
    m_lastIrreversiblePly = 0;
    
    clear();
}

void Board::clear() {
    m_mailbox.fill(NO_PIECE);
    m_pieceBB.fill(0);
    m_pieceTypeBB.fill(0);
    m_colorBB.fill(0);
    m_occupied = 0;
    
    m_sideToMove = WHITE;
    m_castlingRights = NO_CASTLING;
    m_enPassantSquare = NO_SQUARE;
    m_halfmoveClock = 0;
    m_fullmoveNumber = 1;
    
    // Clear material tracking
    m_material.clear();
    m_evalCacheValid = false;
    
    // Clear PST score (Stage 9)
    m_pstScore = eval::MgEgScore();
    
    // Stage 9b: Clear game history
    m_gameHistory.clear();
    m_lastIrreversiblePly = 0;
    
    // Bug #002 fix: Properly initialize zobrist key even for empty board
    // Must be done after setting all state variables
    rebuildZobristKey();
}

void Board::setStartingPosition() {
    fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Board::setPiece(Square s, Piece p) {
    // Validate square bounds
    if (!isValidSquare(s)) return;
    
    // Validate piece value (NO_PIECE is 12, valid pieces are 0-11)
    if (p != NO_PIECE && p >= NUM_PIECES) return;
    
    Piece oldPiece = m_mailbox[s];
    
    // Update mailbox first
    m_mailbox[s] = p;
    
    // Handle removal of old piece
    if (oldPiece != NO_PIECE && oldPiece < NUM_PIECES) {
        // Safely extract color and type with validation
        Color oldColor = colorOf(oldPiece);
        PieceType oldType = typeOf(oldPiece);
        
        // Additional safety check
        if (oldColor < NUM_COLORS && oldType < NUM_PIECE_TYPES) {
            updateBitboards(s, oldPiece, false);
            // Update material tracking
            m_material.remove(oldPiece);
            // Update PST score (Stage 9) - remove old piece value
            // PST values are from white's perspective, negate for black
            if (oldColor == WHITE) {
                m_pstScore -= eval::PST::value(oldType, s, WHITE);
            } else {
                m_pstScore += eval::PST::value(oldType, s, BLACK);
            }
        }
    }
    
    // Handle addition of new piece
    if (p != NO_PIECE && p < NUM_PIECES) {
        // Safely extract color and type with validation
        Color newColor = colorOf(p);
        PieceType newType = typeOf(p);
        
        // Additional safety check
        if (newColor < NUM_COLORS && newType < NUM_PIECE_TYPES) {
            updateBitboards(s, p, true);
            // Update material tracking
            m_material.add(p);
            // Update PST score (Stage 9) - add new piece value
            // PST values are from white's perspective, negate for black
            if (newColor == WHITE) {
                m_pstScore += eval::PST::value(newType, s, WHITE);
            } else {
                m_pstScore -= eval::PST::value(newType, s, BLACK);
            }
        }
    }
    
    // Finally update zobrist (single XOR for difference)
    if (oldPiece != NO_PIECE && oldPiece < NUM_PIECES) {
        m_zobristKey ^= s_zobristPieces[s][oldPiece];
    }
    if (p != NO_PIECE && p < NUM_PIECES) {
        m_zobristKey ^= s_zobristPieces[s][p];
    }
    
    // Invalidate evaluation cache
    m_evalCacheValid = false;
}

void Board::removePiece(Square s) {
    // Validate square bounds
    if (!isValidSquare(s)) return;
    
    setPiece(s, NO_PIECE);
}

void Board::movePiece(Square from, Square to) {
    if (!isValidSquare(from) || !isValidSquare(to)) return;
    
    Piece p = m_mailbox[from];
    if (p == NO_PIECE) return;
    
    Piece captured = m_mailbox[to];
    
    // Update mailbox
    m_mailbox[to] = p;
    m_mailbox[from] = NO_PIECE;
    
    // Update bitboards
    updateBitboards(from, p, false);
    updateBitboards(to, p, true);
    if (captured != NO_PIECE) {
        updateBitboards(to, captured, false);
    }
    
    // Update Zobrist key (avoiding double XOR)
    m_zobristKey ^= s_zobristPieces[from][p];  // Remove from source
    m_zobristKey ^= s_zobristPieces[to][p];    // Add to destination
    if (captured != NO_PIECE) {
        m_zobristKey ^= s_zobristPieces[to][captured];  // Remove captured
    }
}

void Board::updateBitboards(Square s, Piece p, bool add) {
    // Validate inputs
    if (!isValidSquare(s) || p >= NUM_PIECES) return;
    
    // Additional safety for color and type extraction
    Color c = colorOf(p);
    PieceType pt = typeOf(p);
    
    // Validate derived values
    if (c >= NUM_COLORS || pt >= NUM_PIECE_TYPES) return;
    
    Bitboard bb = squareBB(s);
    
    if (add) {
        m_pieceBB[p] |= bb;
        m_pieceTypeBB[pt] |= bb;
        m_colorBB[c] |= bb;
        m_occupied |= bb;
    } else {
        m_pieceBB[p] &= ~bb;
        m_pieceTypeBB[pt] &= ~bb;
        m_colorBB[c] &= ~bb;
        m_occupied &= ~bb;
    }
}

void Board::initZobrist() {
    if (s_zobristInitialized) return;  // Prevent double initialization
    
    // Use simple deterministic values instead of random for debugging
    Hash counter = 1;
    
    for (Square s = 0; s < NUM_SQUARES; ++s) {
        for (int p = 0; p < NUM_PIECES; ++p) {
            s_zobristPieces[s][static_cast<size_t>(p)] = counter++;
        }
        s_zobristEnPassant[s] = counter++;
    }
    
    for (int i = 0; i < 16; ++i) {
        s_zobristCastling[static_cast<size_t>(i)] = counter++;
    }
    
    s_zobristSideToMove = counter++;
    s_zobristInitialized = true;
}

void Board::updateZobristKey(Square s, Piece p) {
    if (p != NO_PIECE && isValidSquare(s) && p < NUM_PIECES) {
        // Ensure Zobrist tables are initialized before using them
        if (!s_zobristInitialized) {
            initZobrist();
        }
        m_zobristKey ^= s_zobristPieces[s][p];
    }
}

std::string Board::toFEN() const {
    std::ostringstream fen;
    
    // Board position
    for (int r = 7; r >= 0; --r) {
        int emptyCount = 0;
        for (File f = 0; f < 8; ++f) {
            Square s = makeSquare(f, static_cast<Rank>(r));
            Piece p = m_mailbox[s];
            
            if (p == NO_PIECE) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen << emptyCount;
                    emptyCount = 0;
                }
                fen << PIECE_CHARS[p];
            }
        }
        if (emptyCount > 0) {
            fen << emptyCount;
        }
        if (r > 0) {
            fen << '/';
        }
    }
    
    // Side to move
    fen << ' ' << (m_sideToMove == WHITE ? 'w' : 'b');
    
    // Castling rights
    fen << ' ';
    if (m_castlingRights == NO_CASTLING) {
        fen << '-';
    } else {
        if (m_castlingRights & WHITE_KINGSIDE) fen << 'K';
        if (m_castlingRights & WHITE_QUEENSIDE) fen << 'Q';
        if (m_castlingRights & BLACK_KINGSIDE) fen << 'k';
        if (m_castlingRights & BLACK_QUEENSIDE) fen << 'q';
    }
    
    // En passant square
    fen << ' ';
    if (m_enPassantSquare == NO_SQUARE) {
        fen << '-';
    } else {
        fen << squareToString(m_enPassantSquare);
    }
    
    // Move clocks
    fen << ' ' << m_halfmoveClock << ' ' << m_fullmoveNumber;
    
    return fen.str();
}

// New safe FEN board parsing with comprehensive error reporting
FenResult Board::parseBoardPosition(std::string_view boardStr) {
    Square sq = SQ_A8;  // Start from A8
    int rank = 7;  // Use int to avoid underflow warning
    int file = 0;
    size_t position = 0;
    
    for (char c : boardStr) {
        if (c == '/') {
            // Validate rank transition
            if (file != 8) {
                return makeFenError(FenError::IncompleteRank, 
                    "Incomplete rank before '/'", position);
            }
            
            rank--;
            if (rank < 0) {
                return makeFenError(FenError::TooManyRanks, 
                    "More than 8 ranks in board position", position);
            }
            
            file = 0;
            sq = static_cast<Square>(rank * 8);  // Move to next rank
        } else if (c >= '1' && c <= '8') {
            // Empty squares - critical buffer overflow protection
            int skip = c - '0';
            if (file + skip > 8) {
                return makeFenError(FenError::BoardOverflow,
                    "Rank overflow: too many squares in rank", position);
            }
            if (sq + skip > NUM_SQUARES) {
                return makeFenError(FenError::BoardOverflow,
                    "Square index overflow", position);
            }
            
            file += skip;
            sq = static_cast<Square>(sq + skip);
        } else if (PIECE_CHAR_LUT[static_cast<uint8_t>(c)] != NO_PIECE) {
            // Valid piece character
            Piece p = PIECE_CHAR_LUT[static_cast<uint8_t>(c)];
            
            if (sq >= NUM_SQUARES) {
                return makeFenError(FenError::BoardOverflow,
                    "Square index out of bounds", position);
            }
            
            // Validate no pawns on back ranks
            int currentRank = rankOf(sq);
            if ((p == WHITE_PAWN && currentRank == 7) || (p == BLACK_PAWN && currentRank == 0)) {
                return makeFenError(FenError::PawnOnBackRank,
                    "Pawn on back rank (1st or 8th)", position);
            }
            
            // Set piece directly in mailbox (will rebuild bitboards later)
            m_mailbox[sq] = p;
            file++;
            sq++;
            
            if (file > 8) {
                return makeFenError(FenError::BoardOverflow,
                    "Rank overflow: too many pieces in rank", position);
            }
        } else {
            return makeFenError(FenError::InvalidPieceChar,
                std::string("Invalid character in board position: '") + c + "'", position);
        }
        position++;
    }
    
    // Validate we completed all 8 ranks and 8 files
    if (rank != 0) {
        return makeFenError(FenError::IncompleteRank,
            "Incomplete board: missing ranks", position);
    }
    if (file != 8) {
        return makeFenError(FenError::IncompleteRank,
            "Incomplete final rank", position);
    }
    
    return true;
}

// Legacy version for backward compatibility
bool Board::parseBoardPositionLegacy(const std::string& boardStr) {
    auto result = parseBoardPosition(boardStr);
    return result.hasValue();
}

FenResult Board::parseSideToMove(std::string_view stmStr) {
    if (stmStr.empty()) {
        return makeFenError(FenError::InvalidSideToMove, "Empty side to move field");
    }
    if (stmStr.length() != 1) {
        return makeFenError(FenError::InvalidSideToMove, 
            "Side to move must be single character 'w' or 'b'");
    }
    
    char c = stmStr[0];
    if (c == 'w') {
        m_sideToMove = WHITE;
    } else if (c == 'b') {
        m_sideToMove = BLACK;
    } else {
        return makeFenError(FenError::InvalidSideToMove,
            std::string("Invalid side to move: '") + c + "' (must be 'w' or 'b')");
    }
    
    return true;
}

FenResult Board::parseCastlingRights(std::string_view castlingStr) {
    m_castlingRights = NO_CASTLING;
    
    if (castlingStr == "-") {
        return true;
    }
    
    size_t position = 0;
    for (char c : castlingStr) {
        switch (c) {
            case 'K':
                if (m_castlingRights & WHITE_KINGSIDE) {
                    return makeFenError(FenError::InvalidCastling,
                        "Duplicate 'K' in castling rights", position);
                }
                m_castlingRights |= WHITE_KINGSIDE;
                break;
            case 'Q':
                if (m_castlingRights & WHITE_QUEENSIDE) {
                    return makeFenError(FenError::InvalidCastling,
                        "Duplicate 'Q' in castling rights", position);
                }
                m_castlingRights |= WHITE_QUEENSIDE;
                break;
            case 'k':
                if (m_castlingRights & BLACK_KINGSIDE) {
                    return makeFenError(FenError::InvalidCastling,
                        "Duplicate 'k' in castling rights", position);
                }
                m_castlingRights |= BLACK_KINGSIDE;
                break;
            case 'q':
                if (m_castlingRights & BLACK_QUEENSIDE) {
                    return makeFenError(FenError::InvalidCastling,
                        "Duplicate 'q' in castling rights", position);
                }
                m_castlingRights |= BLACK_QUEENSIDE;
                break;
            default:
                return makeFenError(FenError::InvalidCastling,
                    std::string("Invalid character in castling rights: '") + c + "'", position);
        }
        position++;
    }
    
    return true;
}

// Legacy version
bool Board::parseCastlingRightsLegacy(const std::string& castlingStr) {
    auto result = parseCastlingRights(castlingStr);
    return result.hasValue();
}

FenResult Board::parseEnPassant(std::string_view epStr) {
    if (epStr == "-") {
        m_enPassantSquare = NO_SQUARE;
        return true;
    }
    
    if (epStr.length() != 2) {
        return makeFenError(FenError::InvalidEnPassant,
            "En passant square must be in format 'A1'-'H8' or '-'");
    }
    
    // Parse square manually for better error reporting
    char fileChar = epStr[0];
    char rankChar = epStr[1];
    
    if (fileChar < 'a' || fileChar > 'h') {
        return makeFenError(FenError::InvalidEnPassant,
            std::string("Invalid file in en passant square: '") + fileChar + "'");
    }
    
    if (rankChar < '1' || rankChar > '8') {
        return makeFenError(FenError::InvalidEnPassant,
            std::string("Invalid rank in en passant square: '") + rankChar + "'");
    }
    
    Square epSquare = makeSquare(static_cast<File>(fileChar - 'a'), static_cast<Rank>(rankChar - '1'));
    
    // En passant square must be on rank 3 (for white) or rank 6 (for black)
    Rank rank = rankOf(epSquare);
    if (m_sideToMove == WHITE && rank != 5) {
        return makeFenError(FenError::InvalidEnPassant,
            "En passant square must be on rank 6 when white to move");
    }
    if (m_sideToMove == BLACK && rank != 2) {
        return makeFenError(FenError::InvalidEnPassant,
            "En passant square must be on rank 3 when black to move");
    }
    
    m_enPassantSquare = epSquare;
    return true;
}

// Legacy version
bool Board::parseEnPassantLegacy(const std::string& epStr) {
    auto result = parseEnPassant(epStr);
    return result.hasValue();
}

FenResult Board::parseHalfmoveClock(std::string_view clockStr) {
    if (clockStr.empty()) {
        return makeFenError(FenError::InvalidClocks, "Empty halfmove clock field");
    }
    
    // Parse using stoi for compatibility
    int value;
    try {
        std::string clockString(clockStr);
        value = std::stoi(clockString);
    } catch (...) {
        return makeFenError(FenError::InvalidClocks,
            "Invalid halfmove clock: not a valid integer");
    }
    
    if (value < 0) {
        return makeFenError(FenError::InvalidClocks,
            "Halfmove clock cannot be negative");
    }
    
    // Clamp to reasonable range (some FENs have invalid clocks)
    if (value > 999) {
        value = 999;  // Clamp rather than reject
    }
    
    m_halfmoveClock = static_cast<uint16_t>(value);
    return true;
}

FenResult Board::parseFullmoveNumber(std::string_view moveStr) {
    if (moveStr.empty()) {
        return makeFenError(FenError::InvalidClocks, "Empty fullmove number field");
    }
    
    // Parse using stoi for compatibility
    int value;
    try {
        std::string moveString(moveStr);
        value = std::stoi(moveString);
    } catch (...) {
        return makeFenError(FenError::InvalidClocks,
            "Invalid fullmove number: not a valid integer");
    }
    
    if (value < 1) {
        return makeFenError(FenError::InvalidClocks,
            "Fullmove number must be >= 1");
    }
    
    m_fullmoveNumber = static_cast<uint16_t>(value);
    return true;
}

// Critical validation: side not to move cannot be in check
bool Board::validateNotInCheck() const {
    // TODO(Stage4): This is a placeholder - needs attack generation
    // For now, return true to avoid blocking Stage 2 completion
    // This will be properly implemented when we have attack generation in Stage 4
    // The concept: calculate if the side NOT to move has their king in check
    // If so, the position is illegal (violates chess rules)
    return true;  // Placeholder implementation
}

bool Board::validateBitboardSync() const {
    // Verify every piece in mailbox is in corresponding bitboard
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
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
        for (int pt = PAWN; pt <= KING; ++pt) {
            colorPieces |= pieces(c, static_cast<PieceType>(pt));
        }
        if (colorPieces != m_colorBB[c]) return false;
    }
    
    return true;
}

bool Board::validateZobrist() const {
    Hash calculatedKey = 0;
    
    // Recalculate from scratch
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
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

bool Board::validatePosition() const {
    return validatePieceCounts() && 
           validateKings() && 
           validateEnPassant() && 
           validateCastlingRights();
    // Note: validateNotInCheck() is called separately in parseFEN()
    // to provide specific error reporting
}

bool Board::validatePieceCounts() const {
    int pieceCounts[NUM_PIECES] = {0};
    
    // Count all pieces
    for (Square s = 0; s < NUM_SQUARES; ++s) {
        Piece p = m_mailbox[s];
        if (p != NO_PIECE) {
            pieceCounts[p]++;
        }
    }
    
    // Check individual piece limits
    if (pieceCounts[WHITE_PAWN] > 8 || pieceCounts[BLACK_PAWN] > 8) return false;
    if (pieceCounts[WHITE_KNIGHT] > 10 || pieceCounts[BLACK_KNIGHT] > 10) return false; // 2 + 8 promotions
    if (pieceCounts[WHITE_BISHOP] > 10 || pieceCounts[BLACK_BISHOP] > 10) return false;
    if (pieceCounts[WHITE_ROOK] > 10 || pieceCounts[BLACK_ROOK] > 10) return false;
    if (pieceCounts[WHITE_QUEEN] > 9 || pieceCounts[BLACK_QUEEN] > 9) return false; // 1 + 8 promotions
    
    // Check for excessive total pieces (accounting for promotions)
    int whitePieces = 0, blackPieces = 0;
    for (int i = WHITE_PAWN; i <= WHITE_KING; ++i) whitePieces += pieceCounts[i];
    for (int i = BLACK_PAWN; i <= BLACK_KING; ++i) blackPieces += pieceCounts[i];
    
    if (whitePieces > 16 || blackPieces > 16) return false;
    
    return true;
}

bool Board::validateKings() const {
    Bitboard whiteKingBB = pieces(WHITE_KING);
    Bitboard blackKingBB = pieces(BLACK_KING);
    
    int whiteKings = popCount(whiteKingBB);
    int blackKings = popCount(blackKingBB);
    
    if (whiteKings != 1 || blackKings != 1) {
        return false;
    }
    
    // Check that kings are not adjacent
    // Make sure bitboards are non-zero before calling lsb()
    if (whiteKingBB == 0 || blackKingBB == 0) {
        return false;
    }
    
    Square whiteKingSquare = lsb(whiteKingBB);
    Square blackKingSquare = lsb(blackKingBB);
    
    int fileDiff = abs(fileOf(whiteKingSquare) - fileOf(blackKingSquare));
    int rankDiff = abs(rankOf(whiteKingSquare) - rankOf(blackKingSquare));
    
    return !(fileDiff <= 1 && rankDiff <= 1);
}

bool Board::validateEnPassant() const {
    if (m_enPassantSquare == NO_SQUARE) {
        return true;
    }
    
    // Check if there's actually a pawn that could have made the double move
    int epRank = rankOf(m_enPassantSquare);
    int epFile = fileOf(m_enPassantSquare);
    
    if (m_sideToMove == WHITE) {
        // White to move, so black pawn made the double move
        // En passant square should be on rank 6 (5 in 0-based), enemy pawn on rank 5 (4 in 0-based)
        if (epRank != 5) return false;
        Square enemyPawnSquare = makeSquare(static_cast<File>(epFile), 4);
        if (m_mailbox[enemyPawnSquare] != BLACK_PAWN) return false;
        
        // Check that the square behind the pawn (where it came from) is empty
        Square startSquare = makeSquare(static_cast<File>(epFile), 6);
        return m_mailbox[startSquare] == NO_PIECE;
    } else {
        // Black to move, so white pawn made the double move
        // En passant square should be on rank 3 (2 in 0-based), enemy pawn on rank 4 (3 in 0-based)
        if (epRank != 2) return false;
        Square enemyPawnSquare = makeSquare(static_cast<File>(epFile), 3);
        if (m_mailbox[enemyPawnSquare] != WHITE_PAWN) return false;
        
        // Check that the square behind the pawn (where it came from) is empty
        Square startSquare = makeSquare(static_cast<File>(epFile), 1);
        return m_mailbox[startSquare] == NO_PIECE;
    }
}

bool Board::validateCastlingRights() const {
    // Check if castling rights are consistent with piece positions
    
    // White kingside castling
    if (m_castlingRights & WHITE_KINGSIDE) {
        if (m_mailbox[E1] != WHITE_KING || m_mailbox[H1] != WHITE_ROOK) {
            return false;
        }
    }
    
    // White queenside castling
    if (m_castlingRights & WHITE_QUEENSIDE) {
        if (m_mailbox[E1] != WHITE_KING || m_mailbox[A1] != WHITE_ROOK) {
            return false;
        }
    }
    
    // Black kingside castling
    if (m_castlingRights & BLACK_KINGSIDE) {
        if (m_mailbox[E8] != BLACK_KING || m_mailbox[H8] != BLACK_ROOK) {
            return false;
        }
    }
    
    // Black queenside castling
    if (m_castlingRights & BLACK_QUEENSIDE) {
        if (m_mailbox[E8] != BLACK_KING || m_mailbox[A8] != BLACK_ROOK) {
            return false;
        }
    }
    
    return true;
}

// Atomic FEN parser with chess engine best practices
FenResult Board::parseFEN(const std::string& fen) {
    if (fen.empty()) {
        return makeFenError(FenError::InvalidFormat, "Empty FEN string");
    }
    
    // Atomic parsing approach - parse directly into primitive data structures
    // This eliminates recursive Board construction during parsing
    FenParseData parseData;
    
    // Zero-copy tokenization using string_view
    std::vector<std::string_view> tokens;
    size_t start = 0;
    size_t end = 0;
    
    while (end < fen.length()) {
        if (fen[end] == ' ') {
            if (start < end) {
                tokens.emplace_back(fen.data() + start, end - start);
            }
            start = end + 1;
        }
        end++;
    }
    
    // Add final token
    if (start < end) {
        tokens.emplace_back(fen.data() + start, end - start);
    }
    
    // Validate we have exactly 6 fields
    if (tokens.size() != 6) {
        return makeFenError(FenError::InvalidFormat,
            std::string("FEN must have exactly 6 fields, got ") + std::to_string(tokens.size()));
    }
    
    // Parse each component directly into primitive data structures
    auto boardResult = parseBoardPositionDirect(tokens[0], parseData.mailbox);
    if (!boardResult) {
        return boardResult.error();
    }
    
    auto stmResult = parseSideToMoveDirect(tokens[1], parseData.sideToMove);
    if (!stmResult) {
        return stmResult.error();
    }
    
    auto castlingResult = parseCastlingRightsDirect(tokens[2], parseData.castlingRights);
    if (!castlingResult) {
        return castlingResult.error();
    }
    
    auto epResult = parseEnPassantDirect(tokens[3], parseData.enPassantSquare);
    if (!epResult) {
        return epResult.error();
    }
    
    auto halfmoveResult = parseHalfmoveClockDirect(tokens[4], parseData.halfmoveClock);
    if (!halfmoveResult) {
        return halfmoveResult.error();
    }
    
    auto fullmoveResult = parseFullmoveNumberDirect(tokens[5], parseData.fullmoveNumber);
    if (!fullmoveResult) {
        return fullmoveResult.error();
    }
    
    // Validate parsed data before applying to board
    if (!isValidFenData(parseData)) {
        return makeFenError(FenError::PositionValidationFailed, "Invalid FEN data validation failed");
    }
    
    // Apply parsed data atomically to this board
    applyFenData(parseData);
    
    return true;
}

// Legacy interface for backward compatibility
bool Board::fromFEN(const std::string& fen) {
    // Debug: Check if parseFEN hangs
    auto result = parseFEN(fen);
    return result.hasValue();
}

void Board::rebuildZobristKey() {
    m_zobristKey = 0;
    
    // Recalculate from scratch
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        Piece p = m_mailbox[sq];
        if (p != NO_PIECE && p < NUM_PIECES) {  // Added bounds check
            m_zobristKey ^= s_zobristPieces[sq][p];
        }
    }
    
    if (m_sideToMove == BLACK) {
        m_zobristKey ^= s_zobristSideToMove;
    }
    
    if (m_enPassantSquare != NO_SQUARE) {
        m_zobristKey ^= s_zobristEnPassant[m_enPassantSquare];
    }
    
    m_zobristKey ^= s_zobristCastling[m_castlingRights];
}

std::string Board::toString() const {
    std::ostringstream ss;
    
    ss << "\n  +---+---+---+---+---+---+---+---+\n";
    for (int r = 7; r >= 0; --r) {
        ss << (char)('1' + r) << " |";
        for (File f = 0; f < 8; ++f) {
            Square s = makeSquare(f, static_cast<Rank>(r));
            Piece p = m_mailbox[s];
            ss << ' ' << PIECE_CHARS[p] << " |";
        }
        ss << "\n  +---+---+---+---+---+---+---+---+\n";
    }
    ss << "    a   b   c   d   e   f   g   h\n\n";
    
    ss << "FEN: " << toFEN() << "\n";
    ss << "Zobrist: 0x" << std::hex << m_zobristKey << std::dec << "\n";
    
    return ss.str();
}

std::string Board::debugDisplay() const {
    std::ostringstream ss;
    
    ss << "=== Board State Debug ===\n";
    ss << toString();
    
    ss << "\nBitboards:\n";
    ss << "White pieces: " << std::hex << m_colorBB[WHITE] << "\n";
    ss << "Black pieces: " << std::hex << m_colorBB[BLACK] << "\n";
    ss << "Occupied:     " << m_occupied << std::dec << "\n";
    
    ss << "\nValidation Status:\n";
    ss << "  Position valid:     " << (validatePosition() ? "PASS" : "FAIL") << "\n";
    ss << "  Not in check:      " << (validateNotInCheck() ? "PASS" : "FAIL") << "\n";
    ss << "  Bitboard sync:     " << (validateBitboardSync() ? "PASS" : "FAIL") << "\n";
    ss << "  Zobrist valid:     " << (validateZobrist() ? "PASS" : "FAIL") << "\n";
    
    return ss.str();
}

uint64_t Board::positionHash() const {
    uint64_t hash = 0;
    
    // Hash mailbox state
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
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

#ifdef DEBUG
// Debug validation functions - active in debug builds for Stage 4 debugging
void Board::validateSync() const {
    assert(validateBitboardSync());
}

void Board::validateZobristDebug() const {
    assert(validateZobrist());
}

void Board::validateStateIntegrity() const {
    assert(BoardStateValidator::validateFullIntegrity(*this));
}
#endif

// ============================================================================
// Incremental Zobrist Updates - Prevent double XOR issues
// ============================================================================

void Board::updateZobristForMove(Move move, Piece movingPiece, Piece capturedPiece) {
    Square from = moveFrom(move);
    Square to = moveTo(move);
    uint8_t flags = moveFlags(move);
    
    // Remove moving piece from origin
    m_zobristKey ^= s_zobristPieces[from][movingPiece];
    
    // Handle captures
    if (capturedPiece != NO_PIECE && flags != EN_PASSANT) {
        m_zobristKey ^= s_zobristPieces[to][capturedPiece];
    }
    
    // Add piece to destination (or promoted piece)
    if (flags & PROMOTION) {
        PieceType promotedType = promotionType(move);
        Piece promotedPiece = makePiece(colorOf(movingPiece), promotedType);
        m_zobristKey ^= s_zobristPieces[to][promotedPiece];
    } else {
        m_zobristKey ^= s_zobristPieces[to][movingPiece];
    }
}

void Board::updateZobristForCastling(Color us, bool kingside) {
    if (us == WHITE) {
        if (kingside) {
            // King: E1 -> G1
            m_zobristKey ^= s_zobristPieces[E1][WHITE_KING];
            m_zobristKey ^= s_zobristPieces[G1][WHITE_KING];
            // Rook: H1 -> F1
            m_zobristKey ^= s_zobristPieces[H1][WHITE_ROOK];
            m_zobristKey ^= s_zobristPieces[F1][WHITE_ROOK];
        } else {
            // King: E1 -> C1
            m_zobristKey ^= s_zobristPieces[E1][WHITE_KING];
            m_zobristKey ^= s_zobristPieces[C1][WHITE_KING];
            // Rook: A1 -> D1
            m_zobristKey ^= s_zobristPieces[A1][WHITE_ROOK];
            m_zobristKey ^= s_zobristPieces[D1][WHITE_ROOK];
        }
    } else {
        if (kingside) {
            // King: E8 -> G8
            m_zobristKey ^= s_zobristPieces[E8][BLACK_KING];
            m_zobristKey ^= s_zobristPieces[G8][BLACK_KING];
            // Rook: H8 -> F8
            m_zobristKey ^= s_zobristPieces[H8][BLACK_ROOK];
            m_zobristKey ^= s_zobristPieces[F8][BLACK_ROOK];
        } else {
            // King: E8 -> C8
            m_zobristKey ^= s_zobristPieces[E8][BLACK_KING];
            m_zobristKey ^= s_zobristPieces[C8][BLACK_KING];
            // Rook: A8 -> D8
            m_zobristKey ^= s_zobristPieces[A8][BLACK_ROOK];
            m_zobristKey ^= s_zobristPieces[D8][BLACK_ROOK];
        }
    }
}

void Board::updateZobristForEnPassant(Square oldEP, Square newEP) {
    if (oldEP != NO_SQUARE) {
        m_zobristKey ^= s_zobristEnPassant[oldEP];
    }
    if (newEP != NO_SQUARE) {
        m_zobristKey ^= s_zobristEnPassant[newEP];
    }
}

void Board::updateZobristForCastlingRights(uint8_t oldRights, uint8_t newRights) {
    if (oldRights != newRights) {
        m_zobristKey ^= s_zobristCastling[oldRights];
        m_zobristKey ^= s_zobristCastling[newRights];
    }
}

void Board::updateZobristSideToMove() {
    m_zobristKey ^= s_zobristSideToMove;
}

bool Board::isAttacked(Square s, Color byColor) const {
    // Use the move generator's comprehensive attack detection
    // This checks for attacks from all piece types: pawns, knights, bishops, rooks, queens, and kings
    return seajay::isSquareAttacked(*this, s, byColor);
}

Square Board::kingSquare(Color c) const {
    // Find the king square for the given color
    Piece king = makePiece(c, KING);
    Bitboard kingBB = m_pieceBB[king];
    
    if (kingBB == 0) {
        return NO_SQUARE;
    }
    
    // Use bit scan to find the king square
    // Since there should only be one king, we can use lsb from bitboard.h
    return lsb(kingBB);
}

// Public interface with safety checks
void Board::makeMove(Move move, UndoInfo& undo) {
#ifdef DEBUG
    VALIDATE_STATE_GUARD(*this, "makeMove");
    validateStateIntegrity();
#endif
    
    makeMoveInternal(move, undo);
    
#ifdef DEBUG
    validateStateIntegrity();
#endif
}

void Board::unmakeMove(Move move, const UndoInfo& undo) {
#ifdef DEBUG
    VALIDATE_STATE_GUARD(*this, "unmakeMove");
    validateStateIntegrity();
#endif
    
    unmakeMoveInternal(move, undo);
    
#ifdef DEBUG
    validateStateIntegrity();
#endif
}

// Enhanced interface with complete undo info
void Board::makeMove(Move move, CompleteUndoInfo& undo) {
#ifdef DEBUG
    VALIDATE_STATE_GUARD(*this, "makeMove-complete");
    validateStateIntegrity();
    undo.positionHash = positionHash();
    undo.occupiedChecksum = m_occupied;
    undo.colorChecksum[WHITE] = m_colorBB[WHITE];
    undo.colorChecksum[BLACK] = m_colorBB[BLACK];
#endif
    
    makeMoveInternal(move, undo);
    
#ifdef DEBUG
    validateStateIntegrity();
#endif
}

void Board::unmakeMove(Move move, const CompleteUndoInfo& undo) {
#ifdef DEBUG
    VALIDATE_STATE_GUARD(*this, "unmakeMove-complete");
    validateStateIntegrity();
#endif
    
    unmakeMoveInternal(move, undo);
    
#ifdef DEBUG
    validateStateIntegrity();
    // Verify complete restoration
    assert(undo.positionHash == positionHash());
    assert(undo.occupiedChecksum == m_occupied);
    assert(undo.colorChecksum[WHITE] == m_colorBB[WHITE]);
    assert(undo.colorChecksum[BLACK] == m_colorBB[BLACK]);
#endif
}

// Internal implementation for legacy UndoInfo
void Board::makeMoveInternal(Move move, UndoInfo& undo) {
    // Stage 9b: Track position before making move
    // Skip history tracking during search for performance
    if (!m_inSearch) {
        pushGameHistory();
    }
    
    // Save current state for undo
    undo.castlingRights = m_castlingRights;
    undo.enPassantSquare = m_enPassantSquare;
    undo.halfmoveClock = m_halfmoveClock;
    undo.fullmoveNumber = m_fullmoveNumber;  // FIXED: Save fullmove number!
    undo.zobristKey = m_zobristKey;
    undo.pstScore = m_pstScore;  // Stage 9: Save PST score
    
    Square from = moveFrom(move);
    Square to = moveTo(move);
    Piece movingPiece = pieceAt(from);
    Piece capturedPiece = pieceAt(to);
    
    // Stage 9b: Check if move is irreversible (pawn move or capture)
    if (!m_inSearch && (capturedPiece != NO_PIECE || typeOf(movingPiece) == PAWN)) {
        // Mark this as last irreversible move
        m_lastIrreversiblePly = m_gameHistory.size();  // Correct index (will be position after push)
    }
    
    undo.capturedPiece = capturedPiece;
    
    // Save old zobrist components for proper incremental update
    uint8_t oldCastlingRights = m_castlingRights;
    Square oldEnPassant = m_enPassantSquare;
    
    // Handle special moves
    uint8_t moveType = moveFlags(move);
    
    if (moveType == CASTLING) {
        // Handle castling WITHOUT using setPiece (which modifies zobrist)
        Color us = colorOf(movingPiece);
        bool kingside = (to == G1 || to == G8);
        
        Square rookFrom, rookTo;
        if (kingside) {
            rookFrom = (us == WHITE) ? H1 : H8;
            rookTo = (us == WHITE) ? F1 : F8;
        } else {
            rookFrom = (us == WHITE) ? A1 : A8;
            rookTo = (us == WHITE) ? D1 : D8;
        }
        
        Piece rook = pieceAt(rookFrom);
        
        // Update mailbox directly
        m_mailbox[from] = NO_PIECE;
        m_mailbox[to] = movingPiece;
        m_mailbox[rookFrom] = NO_PIECE;
        m_mailbox[rookTo] = rook;
        
        // Update bitboards
        updateBitboards(from, movingPiece, false);
        updateBitboards(to, movingPiece, true);
        updateBitboards(rookFrom, rook, false);
        updateBitboards(rookTo, rook, true);
        
        // Single zobrist update for castling
        updateZobristForCastling(us, kingside);
        
        // Update PST for castling (Stage 9)
        // King and rook moved
        if (us == WHITE) {
            m_pstScore -= eval::PST::value(KING, from, WHITE);
            m_pstScore += eval::PST::value(KING, to, WHITE);
            m_pstScore -= eval::PST::value(ROOK, rookFrom, WHITE);
            m_pstScore += eval::PST::value(ROOK, rookTo, WHITE);
        } else {
            m_pstScore += eval::PST::value(KING, from, BLACK);
            m_pstScore -= eval::PST::value(KING, to, BLACK);
            m_pstScore += eval::PST::value(ROOK, rookFrom, BLACK);
            m_pstScore -= eval::PST::value(ROOK, rookTo, BLACK);
        }
        
        // Material unchanged for castling!
        m_evalCacheValid = false;
        
    } else if (moveType == EN_PASSANT) {
        // Handle en passant
        Color us = colorOf(movingPiece);
        Square capturedSquare = (us == WHITE) ? to - 8 : to + 8;
        Piece capturedPawn = pieceAt(capturedSquare);
        undo.capturedPiece = capturedPawn;  // Save the actual captured pawn
        
        // Update mailbox directly
        m_mailbox[from] = NO_PIECE;
        m_mailbox[to] = movingPiece;
        m_mailbox[capturedSquare] = NO_PIECE;
        
        // Update bitboards
        updateBitboards(from, movingPiece, false);
        updateBitboards(to, movingPiece, true);
        updateBitboards(capturedSquare, capturedPawn, false);
        
        // Update zobrist for en passant
        m_zobristKey ^= s_zobristPieces[from][movingPiece];
        m_zobristKey ^= s_zobristPieces[to][movingPiece];
        m_zobristKey ^= s_zobristPieces[capturedSquare][capturedPawn];
        
        // Update material for en passant capture
        m_material.remove(capturedPawn);  // Remove captured pawn
        m_insufficientMaterialCached = false;  // Invalidate cache after capture
        
        // Update PST for en passant (Stage 9)
        if (us == WHITE) {
            // Our pawn moved
            m_pstScore -= eval::PST::value(PAWN, from, WHITE);
            m_pstScore += eval::PST::value(PAWN, to, WHITE);
            // Captured enemy pawn
            m_pstScore += eval::PST::value(PAWN, capturedSquare, BLACK);
        } else {
            // Our pawn moved
            m_pstScore += eval::PST::value(PAWN, from, BLACK);
            m_pstScore -= eval::PST::value(PAWN, to, BLACK);
            // Captured enemy pawn
            m_pstScore -= eval::PST::value(PAWN, capturedSquare, WHITE);
        }
        
        m_evalCacheValid = false;
        
    } else if (moveType & PROMOTION) {
        // Handle promotion
        PieceType promotedType = promotionType(move);
        Color us = colorOf(movingPiece);
        Piece promotedPiece = makePiece(us, promotedType);
        
        // Update mailbox directly
        m_mailbox[from] = NO_PIECE;
        m_mailbox[to] = promotedPiece;
        
        // Update bitboards
        updateBitboards(from, movingPiece, false);  // Remove pawn
        if (capturedPiece != NO_PIECE) {
            updateBitboards(to, capturedPiece, false);  // Remove captured
        }
        updateBitboards(to, promotedPiece, true);  // Add promoted piece
        
        // Update zobrist for promotion
        m_zobristKey ^= s_zobristPieces[from][movingPiece];  // Remove pawn
        if (capturedPiece != NO_PIECE) {
            m_zobristKey ^= s_zobristPieces[to][capturedPiece];  // Remove captured
        }
        m_zobristKey ^= s_zobristPieces[to][promotedPiece];  // Add promoted
        
        // Update material for promotion
        m_material.remove(movingPiece);  // Remove pawn
        if (capturedPiece != NO_PIECE) {
            m_material.remove(capturedPiece);  // Remove captured
        }
        m_material.add(promotedPiece);  // Add promoted piece
        m_insufficientMaterialCached = false;  // Invalidate cache after promotion
        
        // Update PST for promotion (Stage 9)
        if (us == WHITE) {
            // Remove pawn PST
            m_pstScore -= eval::PST::value(PAWN, from, WHITE);
            // Add promoted piece PST
            m_pstScore += eval::PST::value(promotedType, to, WHITE);
            // Remove captured piece PST if any
            if (capturedPiece != NO_PIECE) {
                m_pstScore += eval::PST::value(typeOf(capturedPiece), to, BLACK);
            }
        } else {
            // Remove pawn PST
            m_pstScore += eval::PST::value(PAWN, from, BLACK);
            // Add promoted piece PST
            m_pstScore -= eval::PST::value(promotedType, to, BLACK);
            // Remove captured piece PST if any
            if (capturedPiece != NO_PIECE) {
                m_pstScore -= eval::PST::value(typeOf(capturedPiece), to, WHITE);
            }
        }
        
        m_evalCacheValid = false;
        
    } else {
        // Normal move
        // Update mailbox directly
        m_mailbox[from] = NO_PIECE;
        m_mailbox[to] = movingPiece;
        
        // Update bitboards
        updateBitboards(from, movingPiece, false);
        if (capturedPiece != NO_PIECE) {
            updateBitboards(to, capturedPiece, false);
        }
        updateBitboards(to, movingPiece, true);
        
        // Update zobrist
        updateZobristForMove(move, movingPiece, capturedPiece);
        
        // Update material for normal moves (only if capture)
        if (capturedPiece != NO_PIECE) {
            m_material.remove(capturedPiece);
            m_evalCacheValid = false;
            m_insufficientMaterialCached = false;  // Invalidate cache after capture
        }
        
        // Update PST for normal moves (Stage 9)
        Color us = colorOf(movingPiece);
        PieceType movingType = typeOf(movingPiece);
        
        if (us == WHITE) {
            // Moving our piece
            m_pstScore -= eval::PST::value(movingType, from, WHITE);
            m_pstScore += eval::PST::value(movingType, to, WHITE);
            // Capturing enemy piece
            if (capturedPiece != NO_PIECE) {
                m_pstScore += eval::PST::value(typeOf(capturedPiece), to, BLACK);
            }
        } else {
            // Moving our piece
            m_pstScore += eval::PST::value(movingType, from, BLACK);
            m_pstScore -= eval::PST::value(movingType, to, BLACK);
            // Capturing enemy piece
            if (capturedPiece != NO_PIECE) {
                m_pstScore -= eval::PST::value(typeOf(capturedPiece), to, WHITE);
            }
        }
    }
    
    // Update castling rights
    uint8_t newCastlingRights = m_castlingRights;
    if (typeOf(movingPiece) == KING) {
        // King moved - lose all castling rights
        Color us = colorOf(movingPiece);
        if (us == WHITE) {
            newCastlingRights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
        } else {
            newCastlingRights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
        }
    } else if (typeOf(movingPiece) == ROOK) {
        // Rook moved - lose castling rights for that side
        if (from == A1) newCastlingRights &= ~WHITE_QUEENSIDE;
        else if (from == H1) newCastlingRights &= ~WHITE_KINGSIDE;
        else if (from == A8) newCastlingRights &= ~BLACK_QUEENSIDE;
        else if (from == H8) newCastlingRights &= ~BLACK_KINGSIDE;
    }
    
    // Check if a rook was captured (affects castling rights)
    if (to == A1) newCastlingRights &= ~WHITE_QUEENSIDE;
    else if (to == H1) newCastlingRights &= ~WHITE_KINGSIDE;
    else if (to == A8) newCastlingRights &= ~BLACK_QUEENSIDE;
    else if (to == H8) newCastlingRights &= ~BLACK_KINGSIDE;
    
    // Update zobrist for castling rights change
    if (newCastlingRights != oldCastlingRights) {
        updateZobristForCastlingRights(oldCastlingRights, newCastlingRights);
    }
    m_castlingRights = newCastlingRights;
    
    // Update en passant square
    Square newEnPassant = NO_SQUARE;
    if (typeOf(movingPiece) == PAWN && std::abs(to - from) == 16) {
        // Pawn moved two squares - set en passant square
        newEnPassant = (from + to) / 2;
    }
    
    // Update zobrist for en passant change
    if (newEnPassant != oldEnPassant) {
        updateZobristForEnPassant(oldEnPassant, newEnPassant);
    }
    m_enPassantSquare = newEnPassant;
    
    // Update halfmove clock
    if (typeOf(movingPiece) == PAWN || capturedPiece != NO_PIECE) {
        m_halfmoveClock = 0;
    } else {
        m_halfmoveClock++;
    }
    
    // Update fullmove number (after black's move)
    if (m_sideToMove == BLACK) {
        m_fullmoveNumber++;
    }
    
    // Switch side to move and update zobrist
    m_sideToMove = ~m_sideToMove;
    updateZobristSideToMove();
    
#ifdef DEBUG
    // Verify zobrist consistency
    Hash reconstructed = ZobristKeyManager::computeKey(*this);
    if (reconstructed != 0 && reconstructed != m_zobristKey) {
        std::cerr << "Zobrist mismatch after make!\n";
        std::cerr << "  Expected: 0x" << std::hex << reconstructed << "\n";
        std::cerr << "  Actual:   0x" << m_zobristKey << std::dec << "\n";
    }
#endif
}

// Internal implementation for legacy UndoInfo
void Board::unmakeMoveInternal(Move move, const UndoInfo& undo) {
    // Restore state first (before any piece movements)
    m_castlingRights = undo.castlingRights;
    m_enPassantSquare = undo.enPassantSquare;
    m_halfmoveClock = undo.halfmoveClock;
    m_fullmoveNumber = undo.fullmoveNumber;  // FIXED: Restore fullmove number!
    m_zobristKey = undo.zobristKey;
    m_pstScore = undo.pstScore;  // Stage 9: Restore PST score
    
    // Invalidate insufficient material cache (safe approach)
    m_insufficientMaterialCached = false;
    
    // Switch side back
    m_sideToMove = ~m_sideToMove;
    
    Square from = moveFrom(move);
    Square to = moveTo(move);
    Piece movingPiece = pieceAt(to); // The piece is now at destination
    
    uint8_t moveType = moveFlags(move);
    
    if (moveType == CASTLING) {
        // Handle castling - restore pieces WITHOUT using setPiece
        Color us = colorOf(movingPiece);
        bool kingside = (to == G1 || to == G8);
        
        Square rookFrom, rookTo;
        if (kingside) {
            rookFrom = (us == WHITE) ? F1 : F8;
            rookTo = (us == WHITE) ? H1 : H8;
        } else {
            rookFrom = (us == WHITE) ? D1 : D8;
            rookTo = (us == WHITE) ? A1 : A8;
        }
        
        Piece rook = pieceAt(rookFrom);
        
        // Restore mailbox directly (zobrist already restored)
        m_mailbox[to] = NO_PIECE;
        m_mailbox[from] = movingPiece;
        m_mailbox[rookFrom] = NO_PIECE;
        m_mailbox[rookTo] = rook;
        
        // Restore bitboards
        updateBitboards(to, movingPiece, false);
        updateBitboards(from, movingPiece, true);
        updateBitboards(rookFrom, rook, false);
        updateBitboards(rookTo, rook, true);
        
        // Material unchanged for castling
        m_evalCacheValid = false;
    } else if (moveType == EN_PASSANT) {
        // Handle en passant - restore the captured pawn
        Color us = colorOf(movingPiece);
        Square capturedSquare = (us == WHITE) ? to - 8 : to + 8;
        
        // Restore mailbox directly (zobrist already restored)
        m_mailbox[to] = NO_PIECE;
        m_mailbox[from] = movingPiece;
        m_mailbox[capturedSquare] = undo.capturedPiece;
        
        // Restore bitboards
        updateBitboards(to, movingPiece, false);
        updateBitboards(from, movingPiece, true);
        updateBitboards(capturedSquare, undo.capturedPiece, true);
        
        // Restore material for en passant
        m_material.add(undo.capturedPiece);
        m_evalCacheValid = false;
    } else if (moveType & PROMOTION) {
        // Handle promotion - restore original pawn
        Color us = colorOf(movingPiece);
        Piece originalPawn = makePiece(us, PAWN);
        
        // Restore mailbox directly (zobrist already restored)
        m_mailbox[to] = undo.capturedPiece;  // May be NO_PIECE
        m_mailbox[from] = originalPawn;
        
        // Restore bitboards
        updateBitboards(to, movingPiece, false);  // Remove promoted piece
        updateBitboards(from, originalPawn, true);  // Restore pawn
        if (undo.capturedPiece != NO_PIECE) {
            updateBitboards(to, undo.capturedPiece, true);  // Restore captured
        }
        
        // Restore material for promotion
        m_material.remove(movingPiece);  // Remove promoted piece
        m_material.add(originalPawn);     // Restore pawn
        if (undo.capturedPiece != NO_PIECE) {
            m_material.add(undo.capturedPiece);  // Restore captured
        }
        m_evalCacheValid = false;
    } else {
        // Normal move
        // Restore mailbox directly (zobrist already restored)
        m_mailbox[to] = undo.capturedPiece;  // May be NO_PIECE
        m_mailbox[from] = movingPiece;
        
        // Restore bitboards
        updateBitboards(to, movingPiece, false);
        updateBitboards(from, movingPiece, true);
        if (undo.capturedPiece != NO_PIECE) {
            updateBitboards(to, undo.capturedPiece, true);
        }
        
        // Restore material for normal move
        if (undo.capturedPiece != NO_PIECE) {
            m_material.add(undo.capturedPiece);
            m_evalCacheValid = false;
        }
    }
    
#ifdef DEBUG
    // Verify state restoration
    if (m_zobristKey != undo.zobristKey) {
        std::cerr << "Zobrist key not properly restored!\n";
        std::cerr << "  Expected: 0x" << std::hex << undo.zobristKey << "\n";
        std::cerr << "  Actual:   0x" << m_zobristKey << std::dec << "\n";
    }
#endif
    
    // Stage 9b: Remove position from history on unmake
    // Skip history tracking during search for performance
    if (!m_inSearch && !m_gameHistory.empty()) {
        m_gameHistory.pop_back();
    }
}

// Implementation for CompleteUndoInfo
void Board::makeMoveInternal(Move move, CompleteUndoInfo& undo) {
    // Save complete state
    undo.capturedPiece = pieceAt(moveTo(move));
    undo.capturedSquare = moveTo(move);  // Default to 'to' square
    undo.castlingRights = m_castlingRights;
    undo.enPassantSquare = m_enPassantSquare;
    undo.halfmoveClock = m_halfmoveClock;
    undo.fullmoveNumber = m_fullmoveNumber;
    undo.zobristKey = m_zobristKey;
    undo.pstScore = m_pstScore;  // Stage 9: Save PST score
    undo.moveType = moveFlags(move);
    undo.movingPiece = pieceAt(moveFrom(move));
    
    // Special handling for en passant capture square
    if (undo.moveType == EN_PASSANT) {
        Color us = colorOf(undo.movingPiece);
        undo.capturedSquare = (us == WHITE) ? moveTo(move) - 8 : moveTo(move) + 8;
        undo.capturedPiece = pieceAt(undo.capturedSquare);
    }
    
    // Use the standard make implementation
    UndoInfo basicUndo;
    makeMoveInternal(move, basicUndo);
}

void Board::unmakeMoveInternal(Move move, const CompleteUndoInfo& undo) {
    // Convert to basic undo and use standard unmake
    UndoInfo basicUndo;
    basicUndo.capturedPiece = undo.capturedPiece;
    basicUndo.castlingRights = undo.castlingRights;
    basicUndo.enPassantSquare = undo.enPassantSquare;
    basicUndo.halfmoveClock = undo.halfmoveClock;
    basicUndo.fullmoveNumber = undo.fullmoveNumber;
    basicUndo.zobristKey = undo.zobristKey;
    
    unmakeMoveInternal(move, basicUndo);
}

eval::Score Board::evaluate() const noexcept {
    if (!m_evalCacheValid) {
        m_evalCache = eval::evaluate(*this);
        m_evalCacheValid = true;
    }
    return m_evalCache;
}

void Board::recalculatePSTScore() {
    // Recalculate PST score from scratch (Stage 9)
    m_pstScore = eval::MgEgScore();
    
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        Piece p = m_mailbox[sq];
        if (p != NO_PIECE && p < NUM_PIECES) {  // Added bounds check
            Color c = colorOf(p);
            PieceType pt = typeOf(p);
            
            // Additional validation
            if (c >= NUM_COLORS || pt >= NUM_PIECE_TYPES) continue;
            
            // PST values are from white's perspective
            // Add for white pieces, subtract for black pieces
            if (c == WHITE) {
                m_pstScore += eval::PST::value(pt, sq, WHITE);
            } else {
                m_pstScore -= eval::PST::value(pt, sq, BLACK);
            }
        }
    }
}

// Direct FEN parsing helpers (atomic approach)
FenResult Board::parseBoardPositionDirect(std::string_view boardStr, std::array<Piece, 64>& mailbox) {
    // Clear the mailbox first
    mailbox.fill(NO_PIECE);
    
    int rank = 7;  // Start at rank 8 (index 7)
    int file = 0;
    
    for (char c : boardStr) {
        if (c == '/') {
            if (rank == 0) {
                return makeFenError(FenError::InvalidFormat, "Too many ranks in board position");
            }
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            int empty = c - '0';
            file += empty;
            if (file > 8) {
                return makeFenError(FenError::InvalidFormat, "Too many files in rank");
            }
        } else {
            if (file >= 8) {
                return makeFenError(FenError::InvalidFormat, "Too many pieces in rank");
            }
            
            Piece piece = NO_PIECE;
            switch (c) {
                case 'P': piece = WHITE_PAWN; break;
                case 'N': piece = WHITE_KNIGHT; break;
                case 'B': piece = WHITE_BISHOP; break;
                case 'R': piece = WHITE_ROOK; break;
                case 'Q': piece = WHITE_QUEEN; break;
                case 'K': piece = WHITE_KING; break;
                case 'p': piece = BLACK_PAWN; break;
                case 'n': piece = BLACK_KNIGHT; break;
                case 'b': piece = BLACK_BISHOP; break;
                case 'r': piece = BLACK_ROOK; break;
                case 'q': piece = BLACK_QUEEN; break;
                case 'k': piece = BLACK_KING; break;
                default:
                    return makeFenError(FenError::InvalidFormat, 
                        std::string("Invalid piece character: ") + c);
            }
            
            Square sq = makeSquare(File(file), Rank(rank));
            mailbox[sq] = piece;
            file++;
        }
    }
    
    if (rank != 0 || file != 8) {
        return makeFenError(FenError::InvalidFormat, "Incomplete board position");
    }
    
    return true;
}

FenResult Board::parseSideToMoveDirect(std::string_view stmStr, Color& sideToMove) {
    if (stmStr.empty()) {
        return makeFenError(FenError::InvalidFormat, "Empty side to move");
    }
    
    if (stmStr[0] == 'w') {
        sideToMove = WHITE;
    } else if (stmStr[0] == 'b') {
        sideToMove = BLACK;
    } else {
        return makeFenError(FenError::InvalidFormat, "Invalid side to move (must be 'w' or 'b')");
    }
    
    return true;
}

FenResult Board::parseCastlingRightsDirect(std::string_view castlingStr, uint8_t& castlingRights) {
    castlingRights = 0;
    
    if (castlingStr == "-") {
        return true;
    }
    
    for (char c : castlingStr) {
        switch (c) {
            case 'K': castlingRights |= WHITE_KINGSIDE; break;
            case 'Q': castlingRights |= WHITE_QUEENSIDE; break;
            case 'k': castlingRights |= BLACK_KINGSIDE; break;
            case 'q': castlingRights |= BLACK_QUEENSIDE; break;
            default:
                return makeFenError(FenError::InvalidFormat, 
                    std::string("Invalid castling rights character: ") + c);
        }
    }
    
    return true;
}

FenResult Board::parseEnPassantDirect(std::string_view epStr, Square& enPassantSquare) {
    if (epStr == "-") {
        enPassantSquare = NO_SQUARE;
        return true;
    }
    
    if (epStr.length() != 2) {
        return makeFenError(FenError::InvalidFormat, "Invalid en passant square format");
    }
    
    char fileChar = epStr[0];
    char rankChar = epStr[1];
    
    if (fileChar < 'a' || fileChar > 'h' || rankChar < '1' || rankChar > '8') {
        return makeFenError(FenError::InvalidFormat, "Invalid en passant square coordinates");
    }
    
    File file = File(fileChar - 'a');
    Rank rank = Rank(rankChar - '1');
    enPassantSquare = makeSquare(file, rank);
    
    return true;
}

FenResult Board::parseHalfmoveClockDirect(std::string_view clockStr, uint16_t& halfmoveClock) {
    try {
        int value = std::stoi(std::string(clockStr));
        if (value < 0) {
            return makeFenError(FenError::InvalidFormat, "Halfmove clock cannot be negative");
        }
        halfmoveClock = static_cast<uint16_t>(value);
        return true;
    } catch (const std::exception&) {
        return makeFenError(FenError::InvalidFormat, "Invalid halfmove clock format");
    }
}

FenResult Board::parseFullmoveNumberDirect(std::string_view moveStr, uint16_t& fullmoveNumber) {
    try {
        int value = std::stoi(std::string(moveStr));
        if (value <= 0) {
            return makeFenError(FenError::InvalidFormat, "Fullmove number must be positive");
        }
        fullmoveNumber = static_cast<uint16_t>(value);
        return true;
    } catch (const std::exception&) {
        return makeFenError(FenError::InvalidFormat, "Invalid fullmove number format");
    }
}

bool Board::isValidFenData(const FenParseData& parseData) const {
    // Basic validation of parsed FEN data
    // Count kings
    int whiteKings = 0, blackKings = 0;
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        Piece p = parseData.mailbox[sq];
        if (p == WHITE_KING) whiteKings++;
        if (p == BLACK_KING) blackKings++;
    }
    
    // Must have exactly one king of each color
    if (whiteKings != 1 || blackKings != 1) {
        return false;
    }
    
    // Validate en passant square if set
    if (parseData.enPassantSquare != NO_SQUARE) {
        Rank epRank = rankOf(parseData.enPassantSquare);
        if (parseData.sideToMove == WHITE && epRank != 5) {  // Rank 6 (0-indexed as 5)
            return false;
        }
        if (parseData.sideToMove == BLACK && epRank != 2) {  // Rank 3 (0-indexed as 2)
            return false;
        }
    }
    
    return true;
}

void Board::applyFenData(const FenParseData& parseData) {
    // Apply parsed data atomically to this board
    m_mailbox = parseData.mailbox;
    m_sideToMove = parseData.sideToMove;
    m_castlingRights = parseData.castlingRights;
    m_enPassantSquare = parseData.enPassantSquare;
    m_halfmoveClock = parseData.halfmoveClock;
    m_fullmoveNumber = parseData.fullmoveNumber;
    
    // Rebuild all derived state
    m_pieceBB.fill(0);
    m_pieceTypeBB.fill(0);
    m_colorBB.fill(0);
    m_occupied = 0;
    m_material.clear();
    m_evalCacheValid = false;
    m_pstScore = eval::MgEgScore();
    
    for (int i = 0; i < 64; ++i) {
        Square sq = static_cast<Square>(i);
        Piece p = m_mailbox[sq];
        if (p != NO_PIECE && p < NUM_PIECES) {  // Added bounds check
            updateBitboards(sq, p, true);
            m_material.add(p);
        }
    }
    
    // Rebuild PST score from scratch
    recalculatePSTScore();
    
    // Rebuild Zobrist key from scratch
    rebuildZobristKey();
}

// Stage 9b Repetition Detection methods
void Board::clearGameHistory() {
    m_gameHistory.clear();
    m_lastIrreversiblePly = 0;
}

bool Board::isRepetitionDraw() const {
    if (m_gameHistory.empty()) return false;
    
    Hash currentKey = zobristKey();
    int repetitions = 0;
    
    // Can only repeat positions since last irreversible move
    size_t searchLimit = std::min(m_gameHistory.size(), 
                                  static_cast<size_t>(m_halfmoveClock));
    
    // Determine starting index based on parity of history size
    // If history has even entries, current player (who hasn't moved yet) matches even indices
    // If history has odd entries, current player matches odd indices
    size_t startIdx = (m_gameHistory.size() % 2 == 0) ? 0 : 1;
    
    // Search forward through history for positions with same side to move
    for (size_t i = startIdx; i < searchLimit; i += 2) {
        if (m_gameHistory[i] == currentKey) {
            repetitions++;
            // Current + 2 previous = threefold
            if (repetitions >= 2) {
                return true;
            }
        }
    }
    
    return false;
}

bool Board::isFiftyMoveRule() const {
    // Check for fifty-move rule (100 half moves)
    return m_halfmoveClock >= 100;
}

bool Board::isDraw() const {
    return isRepetitionDraw() || 
           isFiftyMoveRule() || 
           isInsufficientMaterial();
    // Note: Stalemate is handled separately in search
}

bool Board::isInsufficientMaterial() const {
    // Use cached result if available
    if (!m_insufficientMaterialCached) {
        m_insufficientMaterialValue = computeInsufficientMaterial();
        m_insufficientMaterialCached = true;
    }
    return m_insufficientMaterialValue;
}

bool Board::computeInsufficientMaterial() const {
    // Simple insufficient material detection
    // K vs K, KN vs K, KB vs K, KB vs KB (same color)
    
    uint32_t whiteQueens = std::popcount(pieces(WHITE, QUEEN));
    uint32_t blackQueens = std::popcount(pieces(BLACK, QUEEN));
    uint32_t whiteRooks = std::popcount(pieces(WHITE, ROOK));
    uint32_t blackRooks = std::popcount(pieces(BLACK, ROOK));
    uint32_t whitePawns = std::popcount(pieces(WHITE, PAWN));
    uint32_t blackPawns = std::popcount(pieces(BLACK, PAWN));
    
    // If there are pawns, queens, or rooks, material is sufficient
    if (whiteQueens || blackQueens || whiteRooks || blackRooks || 
        whitePawns || blackPawns) {
        return false;
    }
    
    uint32_t whiteBishops = std::popcount(pieces(WHITE, BISHOP));
    uint32_t blackBishops = std::popcount(pieces(BLACK, BISHOP));
    uint32_t whiteKnights = std::popcount(pieces(WHITE, KNIGHT));
    uint32_t blackKnights = std::popcount(pieces(BLACK, KNIGHT));
    
    uint32_t whiteMinor = whiteBishops + whiteKnights;
    uint32_t blackMinor = blackBishops + blackKnights;
    
    // K vs K
    if (whiteMinor == 0 && blackMinor == 0) return true;
    
    // KN vs K or KB vs K
    if ((whiteMinor == 1 && blackMinor == 0) || 
        (whiteMinor == 0 && blackMinor == 1)) {
        return true;
    }
    
    // KB vs KB (same color squares)
    if (whiteBishops == 1 && blackBishops == 1 && 
        whiteKnights == 0 && blackKnights == 0) {
        // Check if bishops are on same color squares
        Bitboard whiteBishopBB = pieces(WHITE, BISHOP);
        Bitboard blackBishopBB = pieces(BLACK, BISHOP);
        
        // Make sure bitboards are not empty before calling lsb
        if (whiteBishopBB && blackBishopBB) {
            Square whiteBishopSq = lsb(whiteBishopBB);
            Square blackBishopSq = lsb(blackBishopBB);
            if ((whiteBishopSq + rankOf(whiteBishopSq)) % 2 == 
                (blackBishopSq + rankOf(blackBishopSq)) % 2) {
                return true;
            }
        }
    }
    
    return false;
}

void Board::pushGameHistory() {
    // Add current position to game history
    m_gameHistory.push_back(zobristKey());
    
    // Prevent unbounded growth in very long games
    if (m_gameHistory.size() > MAX_GAME_HISTORY) {
        // Remove oldest entries, keeping most recent MAX_GAME_HISTORY/2
        m_gameHistory.erase(m_gameHistory.begin(), 
                           m_gameHistory.begin() + MAX_GAME_HISTORY/2);
        // Adjust irreversible ply marker
        if (m_lastIrreversiblePly >= MAX_GAME_HISTORY/2) {
            m_lastIrreversiblePly -= MAX_GAME_HISTORY/2;
        } else {
            m_lastIrreversiblePly = 0;
        }
    }
}

void Board::clearHistoryBeforeIrreversible() {
    // Keep only positions since last irreversible move
    if (m_lastIrreversiblePly < m_gameHistory.size()) {
        // Erase from beginning to irreversible position
        m_gameHistory.erase(m_gameHistory.begin(), 
                           m_gameHistory.begin() + m_lastIrreversiblePly);
        m_lastIrreversiblePly = 0;
    }
}

size_t Board::gameHistorySize() const {
    return m_gameHistory.size();
}

Hash Board::gameHistoryAt(size_t index) const {
    if (index >= m_gameHistory.size()) {
        return Hash(0);  // Explicit Hash constructor
    }
    return m_gameHistory[index];
}

// Stage 9b Phase 2: Search-specific draw detection
bool Board::isRepetitionDrawInSearch(const SearchInfo& searchInfo, int searchPly) const {
    Hash currentKey = zobristKey();
    
    // OPTIMIZATION: Early exit - Can't have repetition with very few moves
    if (m_halfmoveClock < 4) return false;
    
    // OPTIMIZATION: Limit search depth based on 50-move rule
    int maxLookback = std::min(static_cast<int>(m_halfmoveClock), 100);
    
    // PHASE 1: Check for repetition within search path (1 repetition = draw)
    if (searchInfo.isRepetitionInSearch(currentKey, searchPly)) {
        return true;
    }
    
    // PHASE 2: Check game history before search started
    // OPTIMIZATION: Start from most recent and stop at first repetition for draw check
    size_t gameHistorySize = m_gameHistory.size();
    
    // Search within halfmove clock boundary and maxLookback
    size_t searchLimit = std::min(gameHistorySize, 
                                  static_cast<size_t>(maxLookback));
    
    // Search backwards through game history (every 2nd position for same side to move)
    // OPTIMIZATION: Stop at first repetition found (one repetition in history = draw)
    for (size_t i = 0; i < searchLimit; i += 2) {
        size_t idx = gameHistorySize - 1 - i;
        if (idx < m_gameHistory.size() && m_gameHistory[idx] == currentKey) {
            return true;  // First repetition found = draw
        }
    }
    
    return false;
}

bool Board::isDrawInSearch(const SearchInfo& searchInfo, int searchPly) const {
    return isRepetitionDrawInSearch(searchInfo, searchPly) ||
           isFiftyMoveRule() ||
           isInsufficientMaterial();
    // Note: Stalemate is handled separately in negamax
}

} // namespace seajay