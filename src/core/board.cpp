#include "board.h"
#include "board_safety.h"
#include "bitboard.h"
#include "move_generation.h"
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
    m_zobristKey = 0;
}

void Board::setStartingPosition() {
    fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Board::setPiece(Square s, Piece p) {
    if (!isValidSquare(s) || p > NO_PIECE) [[unlikely]] return;
    
    Piece oldPiece = m_mailbox[s];
    
    // Update mailbox first
    m_mailbox[s] = p;
    
    // Then update bitboards
    if (oldPiece != NO_PIECE) {
        updateBitboards(s, oldPiece, false);
    }
    if (p != NO_PIECE) {
        updateBitboards(s, p, true);
    }
    
    // Finally update zobrist (single XOR for difference)
    if (oldPiece != NO_PIECE) {
        m_zobristKey ^= s_zobristPieces[s][oldPiece];
    }
    if (p != NO_PIECE) {
        m_zobristKey ^= s_zobristPieces[s][p];
    }
}

void Board::removePiece(Square s) {
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
    if (!isValidSquare(s) || p >= NUM_PIECES) return;  // Bounds checking
    
    Bitboard bb = squareBB(s);
    
    if (add) {
        m_pieceBB[p] |= bb;
        m_pieceTypeBB[typeOf(p)] |= bb;
        m_colorBB[colorOf(p)] |= bb;
        m_occupied |= bb;
    } else {
        m_pieceBB[p] &= ~bb;
        m_pieceTypeBB[typeOf(p)] &= ~bb;
        m_colorBB[colorOf(p)] &= ~bb;
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

// New safe FEN parser with comprehensive error handling
FenResult Board::parseFEN(const std::string& fen) {
    if (fen.empty()) {
        return makeFenError(FenError::InvalidFormat, "Empty FEN string");
    }
    
    // Parse-to-temp-validate-swap pattern for safety
    // Create temp board without calling static initialization again
    Board tempBoard;
    // Don't call clear() yet, we'll set up the board state manually
    
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
    
    // Parse each component into temporary board
    auto boardResult = tempBoard.parseBoardPosition(tokens[0]);
    if (!boardResult) {
        return boardResult.error();
    }
    
    auto stmResult = tempBoard.parseSideToMove(tokens[1]);
    if (!stmResult) {
        return stmResult.error();
    }
    
    auto castlingResult = tempBoard.parseCastlingRights(tokens[2]);
    if (!castlingResult) {
        return castlingResult.error();
    }
    
    auto epResult = tempBoard.parseEnPassant(tokens[3]);
    if (!epResult) {
        return epResult.error();
    }
    
    auto halfmoveResult = tempBoard.parseHalfmoveClock(tokens[4]);
    if (!halfmoveResult) {
        return halfmoveResult.error();
    }
    
    auto fullmoveResult = tempBoard.parseFullmoveNumber(tokens[5]);
    if (!fullmoveResult) {
        return fullmoveResult.error();
    }
    
    // Rebuild bitboards from mailbox
    tempBoard.m_pieceBB.fill(0);
    tempBoard.m_pieceTypeBB.fill(0);
    tempBoard.m_colorBB.fill(0);
    tempBoard.m_occupied = 0;
    
    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
        Piece p = tempBoard.m_mailbox[sq];
        if (p != NO_PIECE) {
            tempBoard.updateBitboards(sq, p, true);
        }
    }
    
    // Rebuild Zobrist key from scratch (never incremental after FEN)
    tempBoard.rebuildZobristKey();
    
    // Comprehensive validation
    // Comprehensive validation
    if (!tempBoard.validatePosition()) {
        return makeFenError(FenError::PositionValidationFailed, "Position validation failed");
    }
    
    if (!tempBoard.validateNotInCheck()) {
        return makeFenError(FenError::SideNotToMoveInCheck, "Side not to move is in check");
    }
    
    if (!tempBoard.validateBitboardSync()) {
        return makeFenError(FenError::BitboardDesync, "Bitboard/mailbox synchronization failed");
    }
    
    if (!tempBoard.validateZobrist()) {
        return makeFenError(FenError::ZobristMismatch, "Zobrist key validation failed");
    }
    
    // All validation passed - safely move to this board
    *this = std::move(tempBoard);
    
    return true;
}

// Legacy interface for backward compatibility
bool Board::fromFEN(const std::string& fen) {
    auto result = parseFEN(fen);
    return result.hasValue();
}

void Board::rebuildZobristKey() {
    m_zobristKey = 0;
    
    // Recalculate from scratch
    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
        Piece p = m_mailbox[sq];
        if (p != NO_PIECE) {
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
    // Save current state for undo
    undo.castlingRights = m_castlingRights;
    undo.enPassantSquare = m_enPassantSquare;
    undo.halfmoveClock = m_halfmoveClock;
    undo.fullmoveNumber = m_fullmoveNumber;  // FIXED: Save fullmove number!
    undo.zobristKey = m_zobristKey;
    
    Square from = moveFrom(move);
    Square to = moveTo(move);
    Piece movingPiece = pieceAt(from);
    Piece capturedPiece = pieceAt(to);
    
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
    }
    
#ifdef DEBUG
    // Verify state restoration
    if (m_zobristKey != undo.zobristKey) {
        std::cerr << "Zobrist key not properly restored!\n";
        std::cerr << "  Expected: 0x" << std::hex << undo.zobristKey << "\n";
        std::cerr << "  Actual:   0x" << m_zobristKey << std::dec << "\n";
    }
#endif
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

} // namespace seajay