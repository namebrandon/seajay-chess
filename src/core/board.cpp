#include "board.h"
#include "bitboard.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <cmath>
#include <string>

namespace seajay {

std::array<std::array<Hash, NUM_PIECES>, NUM_SQUARES> Board::s_zobristPieces;
std::array<Hash, NUM_SQUARES> Board::s_zobristEnPassant;
std::array<Hash, 16> Board::s_zobristCastling;
Hash Board::s_zobristSideToMove;
bool Board::s_zobristInitialized = false;

Board::Board() {
    if (!s_zobristInitialized) {
        initZobrist();
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
            s_zobristPieces[s][p] = dist(rng);
        }
        s_zobristEnPassant[s] = dist(rng);
    }
    
    for (int i = 0; i < 16; ++i) {
        s_zobristCastling[i] = dist(rng);
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
    
    for (int r = 7; r >= 0; --r) {
        int emptyCount = 0;
        for (File f = 0; f < 8; ++f) {
            Square s = makeSquare(f, r);
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
    
    fen << ' ' << (m_sideToMove == WHITE ? 'w' : 'b') << ' ';
    
    if (m_castlingRights == NO_CASTLING) {
        fen << '-';
    } else {
        if (m_castlingRights & WHITE_KINGSIDE) fen << 'K';
        if (m_castlingRights & WHITE_QUEENSIDE) fen << 'Q';
        if (m_castlingRights & BLACK_KINGSIDE) fen << 'k';
        if (m_castlingRights & BLACK_QUEENSIDE) fen << 'q';
    }
    
    fen << ' ';
    
    if (m_enPassantSquare == NO_SQUARE) {
        fen << '-';
    } else {
        fen << squareToString(m_enPassantSquare);
    }
    
    fen << ' ' << m_halfmoveClock << ' ' << m_fullmoveNumber;
    
    return fen.str();
}

bool Board::parseBoardPosition(const std::string& boardStr) {
    Square s = 56; // Start at a8 (rank 8, file a)
    int rank = 7;
    int file = 0;
    
    for (char c : boardStr) {
        if (c == '/') {
            // Validate we completed the rank
            if (file != 8) {
                return false;
            }
            
            rank--;
            if (rank < 0) {
                return false; // Too many ranks
            }
            
            file = 0;
            s = rank * 8;
        } else if (c >= '1' && c <= '8') {
            int emptySquares = c - '0';
            file += emptySquares;
            
            if (file > 8) {
                return false; // Rank too long
            }
            
            s += emptySquares;
        } else {
            // Should be a piece character
            Piece p = NO_PIECE;
            switch (c) {
                case 'P': p = WHITE_PAWN; break;
                case 'N': p = WHITE_KNIGHT; break;
                case 'B': p = WHITE_BISHOP; break;
                case 'R': p = WHITE_ROOK; break;
                case 'Q': p = WHITE_QUEEN; break;
                case 'K': p = WHITE_KING; break;
                case 'p': p = BLACK_PAWN; break;
                case 'n': p = BLACK_KNIGHT; break;
                case 'b': p = BLACK_BISHOP; break;
                case 'r': p = BLACK_ROOK; break;
                case 'q': p = BLACK_QUEEN; break;
                case 'k': p = BLACK_KING; break;
                default: return false;
            }
            
            if (s >= NUM_SQUARES) {
                return false; // Square out of bounds
            }
            
            // Don't allow pawns on back ranks (rank 1 for black pawns, rank 8 for white pawns)
            int currentRank = s / 8;
            if ((p == WHITE_PAWN && currentRank == 7) || (p == BLACK_PAWN && currentRank == 0)) {
                return false;
            }
            
            setPiece(s, p);
            file++;
            s++;
            
            if (file > 8) {
                return false; // Rank too long
            }
        }
    }
    
    // Validate we completed all 8 ranks
    return (rank == 0 && file == 8);
}

bool Board::parseCastlingRights(const std::string& castlingStr) {
    m_castlingRights = NO_CASTLING;
    
    if (castlingStr == "-") {
        return true;
    }
    
    for (char c : castlingStr) {
        switch (c) {
            case 'K': 
                if (m_castlingRights & WHITE_KINGSIDE) return false; // Duplicate
                m_castlingRights |= WHITE_KINGSIDE; 
                break;
            case 'Q': 
                if (m_castlingRights & WHITE_QUEENSIDE) return false; // Duplicate
                m_castlingRights |= WHITE_QUEENSIDE; 
                break;
            case 'k': 
                if (m_castlingRights & BLACK_KINGSIDE) return false; // Duplicate
                m_castlingRights |= BLACK_KINGSIDE; 
                break;
            case 'q': 
                if (m_castlingRights & BLACK_QUEENSIDE) return false; // Duplicate
                m_castlingRights |= BLACK_QUEENSIDE; 
                break;
            default: 
                return false; // Invalid character
        }
    }
    
    m_zobristKey ^= s_zobristCastling[m_castlingRights];
    return true;
}

bool Board::parseEnPassant(const std::string& epStr) {
    if (epStr == "-") {
        m_enPassantSquare = NO_SQUARE;
        return true;
    }
    
    Square epSquare = stringToSquare(epStr);
    if (epSquare == NO_SQUARE) {
        return false;
    }
    
    // En passant square must be on rank 3 (for white) or rank 6 (for black)
    int rank = rankOf(epSquare);
    if ((m_sideToMove == WHITE && rank != 5) || (m_sideToMove == BLACK && rank != 2)) {
        return false;
    }
    
    m_enPassantSquare = epSquare;
    m_zobristKey ^= s_zobristEnPassant[m_enPassantSquare];
    return true;
}

bool Board::validatePosition() const {
    return validatePieceCounts() && validateKings() && validateEnPassant() && validateCastlingRights();
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

bool Board::fromFEN(const std::string& fen) {
    if (fen.empty()) {
        return false;
    }
    
    clear();
    
    std::istringstream ss(fen);
    std::string board, stm, castling, ep;
    
    // Parse all 6 fields - need to handle negative numbers manually
    std::string halfmoveStr, fullmoveStr;
    if (!(ss >> board >> stm >> castling >> ep >> halfmoveStr >> fullmoveStr)) {
        return false;
    }
    
    // Parse and validate halfmove clock
    try {
        int halfmove = std::stoi(halfmoveStr);
        if (halfmove < 0) return false;
        m_halfmoveClock = static_cast<uint16_t>(halfmove);
    } catch (...) {
        return false;
    }
    
    // Parse and validate fullmove number
    try {
        int fullmove = std::stoi(fullmoveStr);
        if (fullmove < 1) return false;
        m_fullmoveNumber = static_cast<uint16_t>(fullmove);
    } catch (...) {
        return false;
    }
    
    // Validate basic format
    if (stm != "w" && stm != "b") {
        return false;
    }
    
    // Validate and parse board position
    if (!parseBoardPosition(board)) {
        return false;
    }
    
    // Set side to move
    m_sideToMove = (stm == "w") ? WHITE : BLACK;
    if (m_sideToMove == BLACK) {
        m_zobristKey ^= s_zobristSideToMove;
    }
    
    // Validate and parse castling rights
    if (!parseCastlingRights(castling)) {
        return false;
    }
    
    // Validate and parse en passant
    if (!parseEnPassant(ep)) {
        return false;
    }
    
    // Validate clocks
    if (m_halfmoveClock > 100 || m_fullmoveNumber < 1) {
        return false;
    }
    
    // Final position validation
    if (!validatePosition()) {
        return false;
    }
    
    return true;
}

std::string Board::toString() const {
    std::ostringstream ss;
    
    ss << "\n  +---+---+---+---+---+---+---+---+\n";
    for (int r = 7; r >= 0; --r) {
        ss << (char)('1' + r) << " |";
        for (File f = 0; f < 8; ++f) {
            Square s = makeSquare(f, r);
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

} // namespace seajay