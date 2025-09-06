#pragma once

#include "types.h"
#include "board_safety.h"  // Safety infrastructure
#include "../evaluation/material.h"
#include "../evaluation/types.h"
#include "../evaluation/pst.h"  // PST for Stage 9
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <bit>  // For std::popcount (Phase A3)

namespace seajay {

// Forward declaration for FEN parsing
struct FenParseData {
    std::array<Piece, 64> mailbox{};
    Color sideToMove = Color::WHITE;
    uint8_t castlingRights = 0;
    Square enPassantSquare = NO_SQUARE;
    uint16_t halfmoveClock = 0;
    uint16_t fullmoveNumber = 1;
};

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
    
    // Phase A3: Add pieceCount method for improved zugzwang detection
    int pieceCount(Color c, PieceType pt) const noexcept { 
        return std::popcount(pieces(c, pt)); 
    }
    
    Bitboard occupied() const noexcept { return m_occupied; }
    Bitboard empty() const noexcept { return ~m_occupied; }
    
    Color sideToMove() const noexcept { return m_sideToMove; }
    void setSideToMove(Color c) noexcept { 
        m_sideToMove = c; 
        m_evalCacheValid = false;  // Side to move affects evaluation
    }
    
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
    Hash pawnZobristKey() const noexcept { return m_pawnZobristKey; }  // For pawn hash table
    
    // Material evaluation
    const eval::Material& material() const noexcept { return m_material; }
    void recalculateMaterial();  // Recalculate material values after piece value changes
    eval::Score evaluate() const noexcept;
    
    // PST evaluation (Stage 9)
    const eval::MgEgScore& pstScore() const noexcept { return m_pstScore; }
    void recalculatePSTScore();
    
    // Rook evaluation helpers (Phase ROF1)
    bool isOpenFile(int file) const noexcept;
    bool isSemiOpenFile(int file, Color side) const noexcept;
    
    std::string toFEN() const;
    bool fromFEN(const std::string& fen);  // Legacy interface
    FenResult parseFEN(const std::string& fen);  // New safe interface
    
    std::string toString() const;
    std::string debugDisplay() const;
    
    // Position hash for testing (separate from Zobrist)
    uint64_t positionHash() const;
    
    // Stage 9b Repetition Detection methods
    void clearGameHistory();           // Clear position history for repetition detection  
    bool isRepetitionDraw() const;     // Check for threefold repetition
    bool isFiftyMoveRule() const;      // Check for fifty-move rule
    
    // Stage 9b: Complete draw detection
    bool isDraw() const;                    // Check all draw conditions
    bool isInsufficientMaterial() const;   // Check insufficient material
    
    // Stage 9b: Game History Management
    void pushGameHistory();                       // Push current position to history
    void clearHistoryBeforeIrreversible();       // Clear history before irreversible moves
    size_t gameHistorySize() const;              // Get current history size
    
    // Stage 9b Phase 2: Search-specific draw detection
    bool isRepetitionDrawInSearch(const class SearchInfo& searchInfo, int searchPly) const;
    bool isDrawInSearch(const class SearchInfo& searchInfo, int searchPly) const;
    Hash gameHistoryAt(size_t index) const;      // Get history entry at index
    
public:
    // Static lookup tables for initialization
    static std::array<Piece, 256> PIECE_CHAR_LUT;
    static bool s_lutInitialized;
    
private:
    // Force inline for hot path - called 60M times during search
    // Phase 2.3.b: Optimized with better memory access patterns
    inline void addPieceToBitboards(Square s, Piece p) {
#ifdef DEBUG
        if (!isValidSquare(s) || p >= NUM_PIECES) return;
        Color c = colorOf(p);
        PieceType pt = typeOf(p);
        if (c >= NUM_COLORS || pt >= NUM_PIECE_TYPES) return;
#else
        // Compute indices once, use multiple times
        const Color c = colorOf(p);
        const PieceType pt = typeOf(p);
#endif
        // Single bitboard computation
        const Bitboard bb = squareBB(s);
        
        // Update all bitboards - compiler can optimize this better
        // when operations are grouped together
        m_pieceBB[p] |= bb;
        m_pieceTypeBB[pt] |= bb;
        m_colorBB[c] |= bb;
        m_occupied |= bb;
    }
    
    inline void removePieceFromBitboards(Square s, Piece p) {
#ifdef DEBUG
        if (!isValidSquare(s) || p >= NUM_PIECES) return;
        Color c = colorOf(p);
        PieceType pt = typeOf(p);
        if (c >= NUM_COLORS || pt >= NUM_PIECE_TYPES) return;
#else
        // Compute indices once, use multiple times
        const Color c = colorOf(p);
        const PieceType pt = typeOf(p);
#endif
        // Single bitboard computation with complement
        const Bitboard notBB = ~squareBB(s);
        
        // Update all bitboards with single mask
        // Compiler can potentially vectorize these operations
        m_pieceBB[p] &= notBB;
        m_pieceTypeBB[pt] &= notBB;
        m_colorBB[c] &= notBB;
        m_occupied &= notBB;
    }
    
    // Phase 2.3.b: Specialized versions for moves (most common operation)
    // These avoid some redundant updates when moving pieces
    inline void movePieceInBitboards(Square from, Square to, Piece p) {
        const Color c = colorOf(p);
        const PieceType pt = typeOf(p);
        const Bitboard fromToBB = squareBB(from) | squareBB(to);
        
        // Prefetch bitboards to L1 cache for better performance
        __builtin_prefetch(&m_pieceBB[p], 1, 3);
        __builtin_prefetch(&m_pieceTypeBB[pt], 1, 3);
        __builtin_prefetch(&m_colorBB[c], 1, 3);
        
        // XOR toggles both squares at once
        m_pieceBB[p] ^= fromToBB;
        m_pieceTypeBB[pt] ^= fromToBB;
        m_colorBB[c] ^= fromToBB;
        // Occupied doesn't change for non-captures
    }
    
    // Keep original interface for compatibility
    inline void updateBitboards(Square s, Piece p, bool add) {
        if (add) {
            addPieceToBitboards(s, p);
        } else {
            removePieceFromBitboards(s, p);
        }
    }
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
    
    // Direct parsing helpers (atomic parsing approach)
    FenResult parseBoardPositionDirect(std::string_view boardStr, std::array<Piece, 64>& mailbox);
    FenResult parseSideToMoveDirect(std::string_view stmStr, Color& sideToMove);
    FenResult parseCastlingRightsDirect(std::string_view castlingStr, uint8_t& castlingRights);
    FenResult parseEnPassantDirect(std::string_view epStr, Square& enPassantSquare);
    FenResult parseHalfmoveClockDirect(std::string_view clockStr, uint16_t& halfmoveClock);
    FenResult parseFullmoveNumberDirect(std::string_view moveStr, uint16_t& fullmoveNumber);
    
    // FEN data validation and application
    bool isValidFenData(const FenParseData& parseData) const;
    void applyFenData(const FenParseData& parseData);
    
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
        Hash pawnZobristKey;  // Pawn-only hash for pawn structure cache
        eval::MgEgScore pstScore;  // Stage 9: PST score backup
    };
    
    // Public interface with safety checks
    void makeMove(Move move, UndoInfo& undo);
    void unmakeMove(Move move, const UndoInfo& undo);
    
    // Enhanced interface with complete state tracking
    void makeMove(Move move, CompleteUndoInfo& undo);
    void unmakeMove(Move move, const CompleteUndoInfo& undo);
    
    // Phase 3.2: Lazy legality checking version for search
    // Returns false if move leaves king in check (illegal)
    bool tryMakeMove(Move move, UndoInfo& undo);
    
    // Null move methods for null move pruning
    void makeNullMove(UndoInfo& undo);
    void unmakeNullMove(const UndoInfo& undo);
    
    // Calculate non-pawn material for zugzwang detection
    eval::Score nonPawnMaterial(Color c) const;
    
    // Endgame detection for pruning decisions (NMR Phase 1)
    bool isEndgame(eval::Score npmThreshold = eval::Score(1300)) const;
    
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
    Hash m_pawnZobristKey;  // Separate hash for pawns only (for pawn hash table)
    
    // Material tracking
    eval::Material m_material;
    mutable eval::Score m_evalCache{eval::Score::zero()};
    mutable bool m_evalCacheValid{false};
    
    // Draw detection caching (performance optimization)
    mutable bool m_insufficientMaterialCached = false;
    mutable bool m_insufficientMaterialValue = false;
    
    // PST tracking (Stage 9)
    eval::MgEgScore m_pstScore{};  // Incremental PST score
    
    static std::array<std::array<Hash, NUM_PIECES>, NUM_SQUARES> s_zobristPieces;
    static std::array<Hash, NUM_SQUARES> s_zobristEnPassant;
    static std::array<Hash, 16> s_zobristCastling;
    static std::array<Hash, 100> s_zobristFiftyMove;  // Fifty-move counter keys
    static Hash s_zobristSideToMove;
    static std::array<std::array<Hash, NUM_SQUARES>, 2> s_zobristPawns;  // Separate zobrist for pawns only [color][square]
    static bool s_zobristInitialized;
    
#ifdef DEBUG
    // Debug validation macros - active in debug builds for Stage 4 debugging
    void validateSync() const;
    void validateZobristDebug() const;
    void validateStateIntegrity() const;  // Full state validation
#endif
    
    // Helper to compute insufficient material (internal implementation)
    bool computeInsufficientMaterial() const;
    
    // Friend classes for safety infrastructure
    friend class BoardStateValidator;
    friend class ZobristKeyManager;
    friend class SafeMoveExecutor;
    
    // Stage 9b: Position History for Draw Detection
    static constexpr size_t MAX_GAME_HISTORY = 1024;  // Support long games
    std::vector<Hash> m_gameHistory;              // Game position history  
    size_t m_lastIrreversiblePly;                 // Last pawn/capture move
    
    // Performance optimization: Skip history tracking during search
    bool m_inSearch = false;                      // Flag to disable history tracking in search
    
public:
    // Control search mode (disables game history tracking for performance)
    void setSearchMode(bool inSearch) { 
        m_inSearch = inSearch;
#ifdef DEBUG
        if (inSearch) g_searchModeSets++;
        else g_searchModeClears++;
#endif
    }
    bool isInSearch() const { return m_inSearch; }
    
#ifdef DEBUG
    // Debug instrumentation counters - only available in debug builds
    static size_t g_searchMoves;      // Moves made during search
    static size_t g_gameMoves;        // Moves made outside search
    static size_t g_historyPushes;    // Times pushGameHistory called
    static size_t g_historyPops;      // Times pop_back called
    static size_t g_searchModeSets;   // Times setSearchMode(true) called
    static size_t g_searchModeClears; // Times setSearchMode(false) called
    
    static void resetCounters() {
        g_searchMoves = 0;
        g_gameMoves = 0;
        g_historyPushes = 0;
        g_historyPops = 0;
        g_searchModeSets = 0;
        g_searchModeClears = 0;
    }
    
    static void printCounters() {
        std::cout << "info string === Board Operation Counters ===" << std::endl;
        std::cout << "info string Search moves: " << g_searchMoves << std::endl;
        std::cout << "info string Game moves: " << g_gameMoves << std::endl;
        std::cout << "info string History pushes: " << g_historyPushes << std::endl;
        std::cout << "info string History pops: " << g_historyPops << std::endl;
        std::cout << "info string Search mode sets: " << g_searchModeSets << std::endl;
        std::cout << "info string Search mode clears: " << g_searchModeClears << std::endl;
        std::cout << "info string ================================" << std::endl;
    }
#endif // DEBUG
};

} // namespace seajay