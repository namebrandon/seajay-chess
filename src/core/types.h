#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <concepts>

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
    A8 = 56, B8, C8, D8, E8, F8, G8, H8
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
    return r * 8 + f;
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
    
    File f = str[0] - 'a';
    Rank r = str[1] - '1';
    
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

using Move = uint16_t;
constexpr Move NO_MOVE = 0;

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

constexpr Square from(Move m) {
    return m & 0x3F;
}

constexpr Square to(Move m) {
    return (m >> 6) & 0x3F;
}

constexpr uint8_t flags(Move m) {
    return m >> 12;
}

constexpr Move makeMove(Square from, Square to, uint8_t flags = 0) {
    return from | (to << 6) | (flags << 12);
}

enum MoveFlags : uint8_t {
    NORMAL = 0,
    PROMOTION = 8,
    EN_PASSANT = 4,
    CASTLING = 2,
    PROMO_KNIGHT = PROMOTION | 0,
    PROMO_BISHOP = PROMOTION | 1,
    PROMO_ROOK = PROMOTION | 2,
    PROMO_QUEEN = PROMOTION | 3
};

} // namespace seajay