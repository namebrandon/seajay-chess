#pragma once

#include "types.h"
#include "board_safety.h"  // Safety infrastructure
#include <string>
#include <string_view>
#include <array>

namespace seajay {

class Board {
public:
    Board();
    ~Board() = default;
    
    // Copy constructor and assignment (needed for validation)
    Board(const Board&) = default;
    Board& operator=(const Board&) = default;
    
    void clear();
    void setStartingPosition();
    
    Piece pieceAt(Square s) const noexcept { return m_mailbox[s]; }
    void setPiece(Square s, Piece p);
    void removePiece(Square s);
    void movePiece(Square from, Square to);
    
    Bitboard pieces(Color c) const noexcept { return m_colorBB[c]; }
    Bitboard pieces(PieceType pt) const noexcept { return m_pieceTypeBB[pt]; }
    Bitboard pieces(Color c, PieceType pt) const noexcept { return m_pieceBB[makePiece(c, pt)]; }
    Bitboard pieces(Piece p) const noexcept { return m_pieceBB[p]; }
    
    Bitboard occupied() const noexcept { return m_occupied; }
    Bitboard empty() const noexcept { return ~m_occupied; }
    
    Color sideToMove() const noexcept { return m_sideToMove; }
    void setSideToMove(Color c) noexcept { m_sideToMove = c; }
    
    uint8_t castlingRights() const noexcept { return m_castlingRights; }
    void setCastlingRights(uint8_t rights) noexcept { m_castlingRights = rights; }
    bool canCastle(uint8_t flag) const noexcept { return m_castlingRights & flag; }
    
    Square enPassantSquare() const noexcept { return m_enPassantSquare; }
    void setEnPassantSquare(Square s) noexcept { m_enPassantSquare = s; }
    
    uint16_t halfmoveClock() const noexcept { return m_halfmoveClock; }
    void setHalfmoveClock(uint16_t clock) noexcept { m_halfmoveClock = clock; }
    void incrementHalfmoveClock() noexcept { m_halfmoveClock++; }
    void resetHalfmoveClock() noexcept { m_halfmoveClock = 0; }
    
    uint16_t fullmoveNumber() const noexcept { return m_fullmoveNumber; }
    void setFullmoveNumber(uint16_t num) noexcept { m_fullmoveNumber = num; }
    void incrementFullmoveNumber() noexcept { m_fullmoveNumber++; }
    
    Hash zobristKey() const noexcept { return m_zobristKey; }
    
    std::string toFEN() const;
    bool fromFEN(const std::string& fen);  // Legacy interface
    FenResult parseFEN(const std::string& fen);  // New safe interface
    
    std::string toString() const;
    std::string debugDisplay() const;
    
    // Position hash for testing (separate from Zobrist)
    uint64_t positionHash() const;
    
public:
    // Static lookup tables for initialization
    static std::array<Piece, 256> PIECE_CHAR_LUT;
    static bool s_lutInitialized;
    
private:
    void updateBitboards(Square s, Piece p, bool add);
    void initZobrist();
    void updateZobristKey(Square s, Piece p);
    
    // New: Incremental zobrist updates without double-XOR issues
    void updateZobristForMove(Move move, Piece movingPiece, Piece capturedPiece);
    void updateZobristForCastling(Color us, bool kingside);
    void updateZobristForEnPassant(Square oldEP, Square newEP);
    void updateZobristForCastlingRights(uint8_t oldRights, uint8_t newRights);
    void updateZobristSideToMove();
    
    // FEN parsing helpers (new safe versions)
    FenResult parseBoardPosition(std::string_view boardStr);
    FenResult parseSideToMove(std::string_view stmStr);
    FenResult parseCastlingRights(std::string_view castlingStr);
    FenResult parseEnPassant(std::string_view epStr);
    FenResult parseHalfmoveClock(std::string_view clockStr);
    FenResult parseFullmoveNumber(std::string_view moveStr);
    
    // Legacy FEN parsing helpers (for backward compatibility)
    bool parseBoardPositionLegacy(const std::string& boardStr);
    bool parseCastlingRightsLegacy(const std::string& castlingStr);
    bool parseEnPassantLegacy(const std::string& epStr);
    
public:
    // Validation functions (public for testing)
    bool validatePosition() const;
    bool validatePieceCounts() const;
    bool validateKings() const;
    bool validateEnPassant() const;
    bool validateCastlingRights() const;
    bool validateNotInCheck() const;  // Critical: side not to move cannot be in check
    bool validateBitboardSync() const;  // Ensure bitboard/mailbox consistency
    bool validateZobrist() const;       // Zobrist key matches actual position
    
    // TODO(Stage4): En passant pin validation - expensive, deferred to move generation
    // See: deferred_items_tracker.md for details
    // bool validateEnPassantPins() const;
    
    // Helper functions
    void rebuildZobristKey();  // Rebuild Zobrist from scratch (used after FEN parsing)
    
    // Attack detection (needed for move generation)
    bool isAttacked(Square s, Color byColor) const;
    Square kingSquare(Color c) const;
    
    // Enhanced make/unmake with safety infrastructure
    // Legacy UndoInfo for backward compatibility
    struct UndoInfo {
        Piece capturedPiece;
        uint8_t castlingRights;
        Square enPassantSquare;
        uint16_t halfmoveClock;
        uint16_t fullmoveNumber;  // ADDED: Was missing!
        Hash zobristKey;
    };
    
    // Public interface with safety checks
    void makeMove(Move move, UndoInfo& undo);
    void unmakeMove(Move move, const UndoInfo& undo);
    
    // Enhanced interface with complete state tracking
    void makeMove(Move move, CompleteUndoInfo& undo);
    void unmakeMove(Move move, const CompleteUndoInfo& undo);
    
    // Internal implementation (called by safe wrappers)
    void makeMoveInternal(Move move, UndoInfo& undo);
    void unmakeMoveInternal(Move move, const UndoInfo& undo);
    void makeMoveInternal(Move move, CompleteUndoInfo& undo);
    void unmakeMoveInternal(Move move, const CompleteUndoInfo& undo);
    
private:
    
    std::array<Piece, 64> m_mailbox;
    
    std::array<Bitboard, NUM_PIECES> m_pieceBB;
    std::array<Bitboard, NUM_PIECE_TYPES> m_pieceTypeBB;
    std::array<Bitboard, NUM_COLORS> m_colorBB;
    Bitboard m_occupied;
    
    Color m_sideToMove;
    uint8_t m_castlingRights;
    Square m_enPassantSquare;
    uint16_t m_halfmoveClock;
    uint16_t m_fullmoveNumber;
    
    Hash m_zobristKey;
    
    static std::array<std::array<Hash, NUM_PIECES>, NUM_SQUARES> s_zobristPieces;
    static std::array<Hash, NUM_SQUARES> s_zobristEnPassant;
    static std::array<Hash, 16> s_zobristCastling;
    static Hash s_zobristSideToMove;
    static bool s_zobristInitialized;
    
#ifdef DEBUG
    // Debug validation macros - active in debug builds for Stage 4 debugging
    void validateSync() const;
    void validateZobristDebug() const;
    void validateStateIntegrity() const;  // Full state validation
#endif
    
    // Friend classes for safety infrastructure
    friend class BoardStateValidator;
    friend class ZobristKeyManager;
    friend class SafeMoveExecutor;
};

} // namespace seajay