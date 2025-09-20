#pragma once

#include "../core/board.h"
#include "../core/types.h"
#include "../core/bitboard.h"
#include "../core/move_generation.h"
#include "../core/magic_bitboards.h"

namespace seajay::search {

namespace detail {

struct DiscoveredCheckContext {
    bool valid = false;
    bool discovered = false;
    Color us = WHITE;
    Square kingSquare = NO_SQUARE;
    Square fromSquare = NO_SQUARE;
    Square toSquare = NO_SQUARE;
    PieceType movedPieceType = NO_PIECE_TYPE;
    Bitboard sliderSquares = 0;
    Bitboard postOccupied = 0;
};

inline Square enPassantCaptureSquare(Move move, Square fromSq, Square toSq) {
    if (!isEnPassant(move)) {
        return NO_SQUARE;
    }
    return makeSquare(fileOf(toSq), rankOf(fromSq));
}

inline void updateSlidingSetsForMove(PieceType before, PieceType after,
                                     Bitboard fromBB, Bitboard toBB,
                                     Bitboard& rookLike, Bitboard& bishopLike) {
    if (before == ROOK || before == QUEEN) {
        rookLike &= ~fromBB;
    }
    if (before == BISHOP || before == QUEEN) {
        bishopLike &= ~fromBB;
    }

    if (after == ROOK || after == QUEEN) {
        rookLike |= toBB;
    }
    if (after == BISHOP || after == QUEEN) {
        bishopLike |= toBB;
    }
}

inline DiscoveredCheckContext computeDiscoveredContext(const Board& board, Move move) {
    DiscoveredCheckContext ctx{};

    Square fromSq = from(move);
    Square toSq = to(move);
    Piece movingPiece = board.pieceAt(fromSq);
    if (movingPiece == NO_PIECE) {
        return ctx;
    }

    ctx.us = colorOf(movingPiece);
    Color them = ~ctx.us;
    ctx.kingSquare = board.kingSquare(them);
    if (ctx.kingSquare == NO_SQUARE) {
        return ctx;
    }

    ctx.valid = true;
    ctx.fromSquare = fromSq;
    ctx.toSquare = toSq;

    magic_v2::ensureMagicsInitialized();

    const Bitboard fromBB = squareBB(fromSq);
    const Bitboard toBB = squareBB(toSq);

    PieceType beforeType = typeOf(movingPiece);
    PieceType afterType = beforeType;
    if (isPromotion(move)) {
        afterType = promotionType(move);
    }
    ctx.movedPieceType = afterType;

    Bitboard capturedMask = 0;
    if (isEnPassant(move)) {
        Square capturedSq = enPassantCaptureSquare(move, fromSq, toSq);
        if (capturedSq != NO_SQUARE) {
            capturedMask = squareBB(capturedSq);
        }
    } else {
        Piece captured = board.pieceAt(toSq);
        if (captured != NO_PIECE) {
            capturedMask = toBB;
        }
    }

    const Bitboard originalOccupied = board.occupied();
    Bitboard postOccupied = originalOccupied;
    postOccupied &= ~fromBB;
    if (capturedMask) {
        postOccupied &= ~capturedMask;
    }
    postOccupied |= toBB;
    ctx.postOccupied = postOccupied;

    Bitboard rookLikeOriginal = board.pieces(ctx.us, ROOK) | board.pieces(ctx.us, QUEEN);
    Bitboard bishopLikeOriginal = board.pieces(ctx.us, BISHOP) | board.pieces(ctx.us, QUEEN);
    Bitboard rookLikePost = rookLikeOriginal;
    Bitboard bishopLikePost = bishopLikeOriginal;

    updateSlidingSetsForMove(beforeType, afterType, fromBB, toBB, rookLikePost, bishopLikePost);

    Bitboard rookRayPost = MoveGenerator::getRookAttacks(ctx.kingSquare, postOccupied);
    Bitboard bishopRayPost = MoveGenerator::getBishopAttacks(ctx.kingSquare, postOccupied);
    Bitboard sliderCandidates = (rookRayPost & rookLikePost) | (bishopRayPost & bishopLikePost);
    sliderCandidates &= ~toBB;

    if (!sliderCandidates) {
        return ctx;
    }

    Bitboard rookRayPre = MoveGenerator::getRookAttacks(ctx.kingSquare, originalOccupied);
    Bitboard bishopRayPre = MoveGenerator::getBishopAttacks(ctx.kingSquare, originalOccupied);
    Bitboard preAttacking = (rookRayPre & rookLikeOriginal) | (bishopRayPre & bishopLikeOriginal);

    Bitboard capturedExclusion = capturedMask;
    Bitboard validSliders = 0;
    Bitboard tmp = sliderCandidates;
    while (tmp) {
        Square sliderSq = popLsb(tmp);
        Bitboard path = seajay::between(sliderSq, ctx.kingSquare);

        if (!(path & fromBB)) {
            continue;
        }

        if (preAttacking & squareBB(sliderSq)) {
            continue;
        }

        Bitboard blockersBefore = path & (originalOccupied & ~(fromBB | capturedExclusion));
        if (blockersBefore) {
            continue;
        }

        validSliders |= squareBB(sliderSq);
    }

    ctx.sliderSquares = validSliders;
    ctx.discovered = (validSliders != 0);
    return ctx;
}

inline Bitboard attacksFromPiece(PieceType type, Square sq, Color color,
                                 Bitboard occupied) {
    switch (type) {
        case PAWN:
            return MoveGenerator::getPawnAttacks(sq, color);
        case KNIGHT:
            return MoveGenerator::getKnightAttacks(sq);
        case BISHOP:
            return MoveGenerator::getBishopAttacks(sq, occupied);
        case ROOK:
            return MoveGenerator::getRookAttacks(sq, occupied);
        case QUEEN:
            return MoveGenerator::getQueenAttacks(sq, occupied);
        case KING:
            return MoveGenerator::getKingAttacks(sq);
        default:
            return 0;
    }
}

}  // namespace detail

inline bool isDiscoveredCheck(const Board& board, Move move) {
    auto ctx = detail::computeDiscoveredContext(board, move);
    return ctx.discovered;
}

inline bool isDoubleCheckAfterMove(const Board& board, Move move) {
    auto ctx = detail::computeDiscoveredContext(board, move);
    if (!ctx.discovered || !ctx.valid) {
        return false;
    }

    if (ctx.kingSquare == NO_SQUARE || ctx.toSquare == NO_SQUARE || ctx.movedPieceType == NO_PIECE_TYPE) {
        return false;
    }

    Bitboard attacks = detail::attacksFromPiece(ctx.movedPieceType, ctx.toSquare,
                                                ctx.us, ctx.postOccupied);
    bool movingPieceChecks = attacks & squareBB(ctx.kingSquare);

    return movingPieceChecks && ctx.sliderSquares;
}

} // namespace seajay::search

