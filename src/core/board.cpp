#include "board.h"
#include "bitboard.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>

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
    if (oldPiece != NO_PIECE) {
        updateBitboards(s, oldPiece, false);
        updateZobristKey(s, oldPiece);
    }
    
    m_mailbox[s] = p;
    if (p != NO_PIECE) {
        updateBitboards(s, p, true);
        updateZobristKey(s, p);
    }
}

void Board::removePiece(Square s) {
    setPiece(s, NO_PIECE);
}

void Board::movePiece(Square from, Square to) {
    if (!isValidSquare(from) || !isValidSquare(to)) return;
    
    Piece p = m_mailbox[from];
    if (p == NO_PIECE) return;
    
    // Remove captured piece if any
    Piece captured = m_mailbox[to];
    if (captured != NO_PIECE) {
        updateBitboards(to, captured, false);
        m_zobristKey ^= s_zobristPieces[to][captured];
    }
    
    // Move the piece
    m_mailbox[to] = p;
    m_mailbox[from] = NO_PIECE;
    
    updateBitboards(from, p, false);
    updateBitboards(to, p, true);
    
    // Update Zobrist key correctly
    m_zobristKey ^= s_zobristPieces[from][p];  // Remove from source
    m_zobristKey ^= s_zobristPieces[to][p];    // Add to destination
}

void Board::updateBitboards(Square s, Piece p, bool add) {
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
    std::mt19937_64 rng(0x1234567890ABCDEF);
    std::uniform_int_distribution<Hash> dist;
    
    for (Square s = 0; s < NUM_SQUARES; ++s) {
        for (int p = 0; p < NUM_PIECES; ++p) {
            s_zobristPieces[s][static_cast<size_t>(p)] = dist(rng);
        }
        s_zobristEnPassant[s] = dist(rng);
    }
    
    for (int i = 0; i < 16; ++i) {
        s_zobristCastling[static_cast<size_t>(i)] = dist(rng);
    }
    
    s_zobristSideToMove = dist(rng);
    s_zobristInitialized = true;
}

void Board::updateZobristKey(Square s, Piece p) {
    if (p != NO_PIECE && isValidSquare(s) && p < NUM_PIECES) {
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
    Square sq = SQ_A8;  // Start from a8
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
            "En passant square must be in format 'a1'-'h8' or '-'");
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
    int whiteKings = popCount(pieces(WHITE_KING));
    int blackKings = popCount(pieces(BLACK_KING));
    
    if (whiteKings != 1 || blackKings != 1) {
        return false;
    }
    
    // Check that kings are not adjacent
    Square whiteKingSquare = lsb(pieces(WHITE_KING));
    Square blackKingSquare = lsb(pieces(BLACK_KING));
    
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
    Board tempBoard;
    tempBoard.clear();
    
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
#endif

} // namespace seajay