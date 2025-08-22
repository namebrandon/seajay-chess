#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <concepts>
#include <variant>

namespace seajay {

using Bitboard = uint64_t;
using Square = uint8_t;
using File = uint8_t;
using Rank = uint8_t;
using Hash = uint64_t;

constexpr Square NO_SQUARE = 64;
constexpr int NUM_SQUARES = 64;

// Common squares for convenience
enum Squares : Square {
    A1 = 0, B1, C1, D1, E1, F1, G1, H1,
    A2 = 8, B2, C2, D2, E2, F2, G2, H2,
    A3 = 16, B3, C3, D3, E3, F3, G3, H3,
    A4 = 24, B4, C4, D4, E4, F4, G4, H4,
    A5 = 32, B5, C5, D5, E5, F5, G5, H5,
    A6 = 40, B6, C6, D6, E6, F6, G6, H6,
    A7 = 48, B7, C7, D7, E7, F7, G7, H7,
    A8 = 56, B8, C8, D8, E8, F8, G8, H8,
    SQ_A1 = A1, SQ_H8 = H8, SQ_A8 = A8,  // Additional aliases
    SQ_E1 = E1, SQ_E4 = E4, SQ_E5 = E5, SQ_E8 = E8, SQ_H1 = H1
};
constexpr int NUM_FILES = 8;
constexpr int NUM_RANKS = 8;
constexpr int NUM_PIECES = 12;
constexpr int NUM_PIECE_TYPES = 6;
constexpr int NUM_COLORS = 2;

enum Color : uint8_t {
    WHITE = 0,
    BLACK = 1,
    NUM_COLORS_ENUM = 2
};

enum PieceType : uint8_t {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5,
    NO_PIECE_TYPE = 6
};

enum Piece : uint8_t {
    WHITE_PAWN = 0,
    WHITE_KNIGHT = 1,
    WHITE_BISHOP = 2,
    WHITE_ROOK = 3,
    WHITE_QUEEN = 4,
    WHITE_KING = 5,
    BLACK_PAWN = 6,
    BLACK_KNIGHT = 7,
    BLACK_BISHOP = 8,
    BLACK_ROOK = 9,
    BLACK_QUEEN = 10,
    BLACK_KING = 11,
    NO_PIECE = 12
};

enum CastlingRights : uint8_t {
    NO_CASTLING = 0,
    WHITE_KINGSIDE = 1,
    WHITE_QUEENSIDE = 2,
    BLACK_KINGSIDE = 4,
    BLACK_QUEENSIDE = 8,
    WHITE_BOTH = WHITE_KINGSIDE | WHITE_QUEENSIDE,
    BLACK_BOTH = BLACK_KINGSIDE | BLACK_QUEENSIDE,
    ALL_CASTLING = WHITE_BOTH | BLACK_BOTH
};

enum Direction : int8_t {
    NORTH = 8,
    EAST = 1,
    SOUTH = -8,
    WEST = -1,
    NORTH_EAST = NORTH + EAST,
    NORTH_WEST = NORTH + WEST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST
};

constexpr Square makeSquare(File f, Rank r) {
    return static_cast<Square>(r * 8 + f);
}

constexpr File fileOf(Square s) {
    return s & 7;
}

constexpr Rank rankOf(Square s) {
    return s >> 3;
}

constexpr Color operator~(Color c) {
    return static_cast<Color>(c ^ 1);
}

constexpr Piece makePiece(Color c, PieceType pt) {
    return static_cast<Piece>(c * 6 + pt);
}

constexpr PieceType typeOf(Piece p) {
    return static_cast<PieceType>(p % 6);
}

constexpr Color colorOf(Piece p) {
    return static_cast<Color>(p / 6);
}

constexpr bool isValidSquare(Square s) {
    return s < NUM_SQUARES;
}

// Phase PP3: Distance helpers for passed pawn evaluation
constexpr int distance(Square s1, Square s2) {
    int file_dist = std::abs(fileOf(s1) - fileOf(s2));
    int rank_dist = std::abs(rankOf(s1) - rankOf(s2));
    return std::max(file_dist, rank_dist);  // Chebyshev distance
}

// Get bitboard for a specific file
constexpr Bitboard fileBB(int file) {
    return 0x0101010101010101ULL << file;
}

// Get pawn attacks for a square
constexpr Bitboard pawnAttacks(Color c, Square s) {
    Bitboard bb = 1ULL << s;
    if (c == WHITE) {
        return ((bb & ~fileBB(0)) << 7) | ((bb & ~fileBB(7)) << 9);
    } else {
        return ((bb & ~fileBB(0)) >> 9) | ((bb & ~fileBB(7)) >> 7);
    }
}

constexpr const char* SQUARE_NAMES[64] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

constexpr char PIECE_CHARS[13] = {
    'P', 'N', 'B', 'R', 'Q', 'K',
    'p', 'n', 'b', 'r', 'q', 'k',
    '.'
};

inline std::string squareToString(Square s) {
    if (!isValidSquare(s)) return "-";
    return SQUARE_NAMES[s];
}

inline Square stringToSquare(const std::string& str) {
    if (str.length() != 2) return NO_SQUARE;
    
    File f = static_cast<File>(str[0] - 'a');
    Rank r = static_cast<Rank>(str[1] - '1');
    
    if (f >= NUM_FILES || r >= NUM_RANKS) return NO_SQUARE;
    
    return makeSquare(f, r);
}

constexpr Bitboard squareBB(Square s) {
    return 1ULL << s;
}

constexpr Bitboard rankBB(Rank r) {
    return 0xFFULL << (r * 8);
}

constexpr Bitboard fileBB(File f) {
    return 0x0101010101010101ULL << f;
}

// Move representation using 16-bit encoding
// Bits 0-5:   From square (0-63)
// Bits 6-11:  To square (0-63) 
// Bits 12-15: Move flags (special moves)
using Move = uint16_t;
constexpr Move NO_MOVE = 0;

// Move flag encoding (bits 12-15)
enum MoveFlags : uint8_t {
    NORMAL = 0,           // Normal move
    DOUBLE_PAWN = 1,      // Double pawn push
    CASTLING = 2,         // Castling move
    CAPTURE = 4,          // Capture flag
    EN_PASSANT = 5,       // En passant capture
    PROMOTION = 8,        // Promotion flag (bit 3)
    PROMO_KNIGHT = PROMOTION | 0,  // Promote to knight
    PROMO_BISHOP = PROMOTION | 1,  // Promote to bishop
    PROMO_ROOK = PROMOTION | 2,    // Promote to rook
    PROMO_QUEEN = PROMOTION | 3,   // Promote to queen
    PROMO_KNIGHT_CAPTURE = PROMOTION | CAPTURE | 0,  // Promote to knight with capture
    PROMO_BISHOP_CAPTURE = PROMOTION | CAPTURE | 1,  // Promote to bishop with capture
    PROMO_ROOK_CAPTURE = PROMOTION | CAPTURE | 2,    // Promote to rook with capture
    PROMO_QUEEN_CAPTURE = PROMOTION | CAPTURE | 3    // Promote to queen with capture
};

// Move accessor functions
constexpr Square moveFrom(Move m) noexcept {
    return static_cast<Square>(m & 0x3F);
}

constexpr Square moveTo(Move m) noexcept {
    return static_cast<Square>((m >> 6) & 0x3F);
}

constexpr uint8_t moveFlags(Move m) noexcept {
    return static_cast<uint8_t>(m >> 12);
}

constexpr PieceType promotionType(Move m) noexcept {
    return static_cast<PieceType>(((m >> 12) & 0x3) + 1);
}

constexpr bool isPromotion(Move m) noexcept {
    return (m >> 12) & PROMOTION;
}

constexpr bool isCapture(Move m) noexcept {
    uint8_t flags = moveFlags(m);
    return (flags & CAPTURE) || (flags == EN_PASSANT);
}

constexpr bool isEnPassant(Move m) noexcept {
    return moveFlags(m) == EN_PASSANT;
}

constexpr bool isCastling(Move m) noexcept {
    return moveFlags(m) == CASTLING;
}

constexpr bool isDoublePawnPush(Move m) noexcept {
    return moveFlags(m) == DOUBLE_PAWN;
}

// Move construction functions
constexpr Move makeMove(Square from, Square to, uint8_t flags = NORMAL) noexcept {
    return static_cast<Move>(from | (to << 6) | (flags << 12));
}

constexpr Move makePromotionMove(Square from, Square to, PieceType promoteTo) noexcept {
    return makeMove(from, to, PROMOTION | static_cast<uint8_t>(promoteTo - 1));
}

constexpr Move makeCastlingMove(Square from, Square to) noexcept {
    return makeMove(from, to, CASTLING);
}

constexpr Move makeEnPassantMove(Square from, Square to) noexcept {
    return makeMove(from, to, EN_PASSANT);
}

constexpr Move makeDoublePawnMove(Square from, Square to) noexcept {
    return makeMove(from, to, DOUBLE_PAWN);
}

constexpr Move makeCaptureMove(Square from, Square to) noexcept {
    return makeMove(from, to, CAPTURE);
}

constexpr Move makePromotionCaptureMove(Square from, Square to, PieceType promoteTo) noexcept {
    return makeMove(from, to, PROMOTION | CAPTURE | static_cast<uint8_t>(promoteTo - 1));
}

// Legacy function aliases for compatibility
constexpr Square from(Move m) noexcept { return moveFrom(m); }
constexpr Square to(Move m) noexcept { return moveTo(m); }
constexpr uint8_t flags(Move m) noexcept { return moveFlags(m); }

// C++20 Concepts for type safety
template<typename T>
concept ValidSquare = requires(T s) {
    { static_cast<Square>(s) } -> std::convertible_to<Square>;
    requires s >= 0 && s < NUM_SQUARES;
};

template<typename T>
concept ValidPiece = requires(T p) {
    { static_cast<Piece>(p) } -> std::convertible_to<Piece>;
    requires p >= WHITE_PAWN && p <= NO_PIECE;
};

template<typename T>
concept ValidMove = requires(T m) {
    { static_cast<Move>(m) } -> std::convertible_to<Move>;
    { moveFrom(m) } -> std::convertible_to<Square>;
    { moveTo(m) } -> std::convertible_to<Square>;
};

// Forward declarations for move generation
class MoveList;
class Board;

// Result<T,E> type for C++20 (std::expected is C++23)
template<typename T, typename E>
class Result {
public:
    using ValueType = T;
    using ErrorType = E;
    
private:
    std::variant<T, E> m_value;
    
public:
    // Constructors
    constexpr Result(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : m_value(value) {}
    
    constexpr Result(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : m_value(std::move(value)) {}
    
    constexpr Result(const E& error) noexcept(std::is_nothrow_copy_constructible_v<E>)
        : m_value(error) {}
    
    constexpr Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>)
        : m_value(std::move(error)) {}
    
    // Query functions
    constexpr bool hasValue() const noexcept {
        return std::holds_alternative<T>(m_value);
    }
    
    constexpr bool hasError() const noexcept {
        return std::holds_alternative<E>(m_value);
    }
    
    constexpr explicit operator bool() const noexcept {
        return hasValue();
    }
    
    // Access functions
    constexpr T& value() & {
        return std::get<T>(m_value);
    }
    
    constexpr const T& value() const & {
        return std::get<T>(m_value);
    }
    
    constexpr T&& value() && {
        return std::get<T>(std::move(m_value));
    }
    
    constexpr const T&& value() const && {
        return std::get<T>(std::move(m_value));
    }
    
    constexpr E& error() & {
        return std::get<E>(m_value);
    }
    
    constexpr const E& error() const & {
        return std::get<E>(m_value);
    }
    
    constexpr E&& error() && {
        return std::get<E>(std::move(m_value));
    }
    
    constexpr const E&& error() const && {
        return std::get<E>(std::move(m_value));
    }
    
    // Value or default
    template<typename U>
    constexpr T valueOr(U&& defaultValue) const & {
        return hasValue() ? value() : static_cast<T>(std::forward<U>(defaultValue));
    }
    
    template<typename U>
    constexpr T valueOr(U&& defaultValue) && {
        return hasValue() ? std::move(value()) : static_cast<T>(std::forward<U>(defaultValue));
    }
};

// FEN parsing error types
enum class FenError : uint8_t {
    InvalidFormat,        // Wrong number of fields
    InvalidBoard,         // Board position parsing failed
    InvalidSideToMove,    // Not 'w' or 'b'
    InvalidCastling,      // Invalid castling rights format
    InvalidEnPassant,     // Invalid en passant square
    InvalidClocks,        // Invalid halfmove/fullmove values
    PositionValidationFailed,  // Position fails chess rules
    BoardOverflow,        // Square index overflow during parsing
    InvalidPieceChar,     // Invalid piece character
    IncompleteRank,       // Rank doesn't have 8 squares
    PawnOnBackRank,       // Pawn on 1st or 8th rank
    TooManyRanks,         // More than 8 ranks
    KingNotFound,         // Missing king(s)
    KingsAdjacent,        // Kings are adjacent
    SideNotToMoveInCheck, // Side not to move is in check
    BitboardDesync,       // Bitboard/mailbox out of sync
    ZobristMismatch       // Zobrist key doesn't match position
};

struct FenErrorInfo {
    FenError error;
    std::string message;
    size_t position;      // Character position in FEN string
    
    FenErrorInfo(FenError err, std::string msg, size_t pos = 0)
        : error(err), message(std::move(msg)), position(pos) {}
};

// Convenience type aliases
using FenResult = Result<bool, FenErrorInfo>;

// Helper function to create error results
inline FenResult makeFenError(FenError error, const std::string& message, size_t position = 0) {
    return FenErrorInfo{error, message, position};
}

} // namespace seajay