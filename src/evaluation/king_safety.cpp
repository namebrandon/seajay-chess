#include "king_safety.h"

#include <algorithm>
#include <cmath>

#include "../core/board.h"
#include "../core/bitboard.h"
#include "../core/move_generation.h"
#include "../search/game_phase.h"

namespace seajay::eval {

namespace {

Bitboard computeShieldZone(Square kingSquare, Color side, int forwardSteps) {
    if (!isValidSquare(kingSquare) || forwardSteps <= 0) {
        return 0;
    }

    const int direction = (side == WHITE) ? 1 : -1;
    const int kingFile = static_cast<int>(fileOf(kingSquare));
    const int kingRank = static_cast<int>(rankOf(kingSquare));
    const int targetRank = kingRank + direction * forwardSteps;

    if (targetRank < 0 || targetRank >= NUM_RANKS) {
        return 0;
    }

    Bitboard zone = 0;
    for (int df = -1; df <= 1; ++df) {
        const int file = kingFile + df;
        if (file < 0 || file >= NUM_FILES) {
            continue;
        }
        const Square square = makeSquare(static_cast<File>(file), static_cast<Rank>(targetRank));
        zone |= squareBB(square);
    }
    return zone;
}

Bitboard kingRingMask(Square kingSquare) {
    const Bitboard king = squareBB(kingSquare);
    Bitboard ring = 0;
    ring |= shift<NORTH>(king);
    ring |= shift<SOUTH>(king);
    ring |= shift<EAST>(king);
    ring |= shift<WEST>(king);
    ring |= shift<NORTH_EAST>(king);
    ring |= shift<NORTH_WEST>(king);
    ring |= shift<SOUTH_EAST>(king);
    ring |= shift<SOUTH_WEST>(king);
    return ring;
}

bool isLineClear(Square from, Square to, Bitboard occupied) {
    return (between(from, to) & occupied) == 0;
}

int countRingAttacks(const Board& board,
                     Color enemy,
                     Square kingSquare,
                     Bitboard kingRing,
                     Bitboard occupancy) {
    Bitboard attacked = 0;

    // Pawns
    Bitboard pawns = board.pieces(enemy, PAWN);
    while (pawns) {
        const Square sq = popLsb(pawns);
        attacked |= MoveGenerator::getPawnAttacks(sq, enemy);
    }

    // Knights
    Bitboard knights = board.pieces(enemy, KNIGHT);
    while (knights) {
        const Square sq = popLsb(knights);
        attacked |= MoveGenerator::getKnightAttacks(sq);
    }

    // Bishops
    Bitboard bishops = board.pieces(enemy, BISHOP);
    while (bishops) {
        const Square sq = popLsb(bishops);
        attacked |= MoveGenerator::getBishopAttacks(sq, occupancy);
    }

    // Rooks
    Bitboard rooks = board.pieces(enemy, ROOK);
    while (rooks) {
        const Square sq = popLsb(rooks);
        attacked |= MoveGenerator::getRookAttacks(sq, occupancy);
    }

    // Queens (combine rook + bishop vectors)
    Bitboard queens = board.pieces(enemy, QUEEN);
    while (queens) {
        const Square sq = popLsb(queens);
        attacked |= MoveGenerator::getQueenAttacks(sq, occupancy);
    }

    // King
    const Square enemyKing = board.kingSquare(enemy);
    if (isValidSquare(enemyKing)) {
        attacked |= MoveGenerator::getKingAttacks(enemyKing);
    }

    return popCount(attacked & kingRing);
}

}  // namespace

KingSafety::KingSafetyParams KingSafety::s_params = {
    .directShieldMg = 28,
    .directShieldEg = -8,
    .advancedShieldMg = 12,
    .advancedShieldEg = -3,
    .missingDirectPenaltyMg = 26,
    .missingDirectPenaltyEg = 6,
    .missingAdvancedPenaltyMg = 10,
    .missingAdvancedPenaltyEg = 3,
    .airSquareBonusMg = 4,
    .airSquareBonusEg = 1,
    .semiOpenFilePenaltyMg = 18,
    .semiOpenFilePenaltyEg = 4,
    .openFilePenaltyMg = 28,
    .openFilePenaltyEg = 6,
    .rookOnOpenFilePenaltyMg = 38,
    .rookOnOpenFilePenaltyEg = 10,
    .attackedRingPenaltyMg = 8,
    .attackedRingPenaltyEg = 3,
    .minorProximityPenaltyMg = 11,
    .minorProximityPenaltyEg = 4,
    .majorProximityPenaltyMg = 16,
    .majorProximityPenaltyEg = 7,
    .queenContactPenaltyMg = 20,
    .queenContactPenaltyEg = 8,
    .enableScoring = 1
};

Score KingSafety::evaluate(const Board& board, Color side) {
    if (s_params.enableScoring == 0) {
        return Score(0);
    }

    const Square kingSquare = board.kingSquare(side);
    if (!isValidSquare(kingSquare)) {
        return Score(0);
    }

    const Color enemy = static_cast<Color>(side ^ 1);
    const auto phase = search::detectGamePhase(board);

    const Bitboard friendlyPawns = board.pieces(side, PAWN);
    const Bitboard enemyPawns = board.pieces(enemy, PAWN);
    const Bitboard occupancy = board.occupied();

    const Bitboard directZone = computeShieldZone(kingSquare, side, 1);
    const Bitboard advancedZone = computeShieldZone(kingSquare, side, 2);

    const int directCount = popCount(friendlyPawns & directZone);
    const int advancedCount = popCount(friendlyPawns & advancedZone);
    const int expectedDirect = popCount(directZone);
    const int expectedAdvanced = popCount(advancedZone);
    const int missingDirect = std::max(0, expectedDirect - directCount);
    const int missingAdvanced = std::max(0, expectedAdvanced - advancedCount);

    int mgScore = 0;
    int egScore = 0;

    mgScore += directCount * s_params.directShieldMg;
    egScore += directCount * s_params.directShieldEg;
    mgScore += advancedCount * s_params.advancedShieldMg;
    egScore += advancedCount * s_params.advancedShieldEg;

    mgScore -= missingDirect * s_params.missingDirectPenaltyMg;
    egScore -= missingDirect * s_params.missingDirectPenaltyEg;
    mgScore -= missingAdvanced * s_params.missingAdvancedPenaltyMg;
    egScore -= missingAdvanced * s_params.missingAdvancedPenaltyEg;

    if (hasAirSquares(board, side, kingSquare)) {
        mgScore += s_params.airSquareBonusMg;
        egScore += s_params.airSquareBonusEg;
    }

    const int kingFileIndex = static_cast<int>(fileOf(kingSquare));
    for (int df = -1; df <= 1; ++df) {
        const int file = kingFileIndex + df;
        if (file < 0 || file >= NUM_FILES) {
            continue;
        }

        const Bitboard fileMask = fileBB(file);
        const bool friendlyOnFile = (friendlyPawns & fileMask) != 0;
        const bool enemyOnFile = (enemyPawns & fileMask) != 0;

        if (!friendlyOnFile) {
            if (enemyOnFile) {
                mgScore -= s_params.semiOpenFilePenaltyMg;
                egScore -= s_params.semiOpenFilePenaltyEg;
            } else {
                mgScore -= s_params.openFilePenaltyMg;
                egScore -= s_params.openFilePenaltyEg;
            }
        }
    }

    const Bitboard kingRing = kingRingMask(kingSquare);
    const int attackedRingSquares = countRingAttacks(board, enemy, kingSquare, kingRing, occupancy);
    mgScore -= attackedRingSquares * s_params.attackedRingPenaltyMg;
    egScore -= attackedRingSquares * s_params.attackedRingPenaltyEg;

    Bitboard enemyKnights = board.pieces(enemy, KNIGHT);
    while (enemyKnights) {
        const Square sq = popLsb(enemyKnights);
        if (seajay::distance(sq, kingSquare) <= 2) {
            mgScore -= s_params.minorProximityPenaltyMg;
            egScore -= s_params.minorProximityPenaltyEg;
        }
    }

    Bitboard enemyBishops = board.pieces(enemy, BISHOP);
    while (enemyBishops) {
        const Square sq = popLsb(enemyBishops);
        if (seajay::distance(sq, kingSquare) <= 2) {
            mgScore -= s_params.minorProximityPenaltyMg;
            egScore -= s_params.minorProximityPenaltyEg;
        }

        const int fileDiff = std::abs(static_cast<int>(fileOf(sq)) - kingFileIndex);
        const int rankDiff = std::abs(static_cast<int>(rankOf(sq)) - static_cast<int>(rankOf(kingSquare)));
        if (fileDiff == rankDiff && fileDiff != 0 && isLineClear(sq, kingSquare, occupancy)) {
            mgScore -= s_params.majorProximityPenaltyMg;
            egScore -= s_params.majorProximityPenaltyEg;
        }
    }

    Bitboard enemyRooks = board.pieces(enemy, ROOK);
    while (enemyRooks) {
        const Square sq = popLsb(enemyRooks);
        if (seajay::distance(sq, kingSquare) <= 3) {
            mgScore -= s_params.majorProximityPenaltyMg;
            egScore -= s_params.majorProximityPenaltyEg;
        }
        if (static_cast<int>(fileOf(sq)) == kingFileIndex && isLineClear(sq, kingSquare, occupancy)) {
            mgScore -= s_params.rookOnOpenFilePenaltyMg;
            egScore -= s_params.rookOnOpenFilePenaltyEg;
        }
    }

    Bitboard enemyQueens = board.pieces(enemy, QUEEN);
    while (enemyQueens) {
        const Square sq = popLsb(enemyQueens);
        const int dist = seajay::distance(sq, kingSquare);
        if (dist <= 3) {
            mgScore -= s_params.majorProximityPenaltyMg;
            egScore -= s_params.majorProximityPenaltyEg;
        }
        if (dist <= 2) {
            mgScore -= s_params.queenContactPenaltyMg;
            egScore -= s_params.queenContactPenaltyEg;
        }
        if (static_cast<int>(fileOf(sq)) == kingFileIndex && isLineClear(sq, kingSquare, occupancy)) {
            mgScore -= s_params.rookOnOpenFilePenaltyMg;
            egScore -= s_params.rookOnOpenFilePenaltyEg;
        }
        const int fileDiff = std::abs(static_cast<int>(fileOf(sq)) - kingFileIndex);
        const int rankDiff = std::abs(static_cast<int>(rankOf(sq)) - static_cast<int>(rankOf(kingSquare)));
        if (fileDiff == rankDiff && fileDiff != 0 && isLineClear(sq, kingSquare, occupancy)) {
            mgScore -= s_params.majorProximityPenaltyMg;
            egScore -= s_params.majorProximityPenaltyEg;
        }
    }

    int rawScore = 0;
    switch (phase) {
        case search::GamePhase::OPENING:
        case search::GamePhase::MIDDLEGAME:
            rawScore = mgScore;
            break;
        case search::GamePhase::ENDGAME:
            rawScore = egScore;
            break;
    }

    rawScore *= s_params.enableScoring;
    return Score(rawScore);
}

Bitboard KingSafety::getShieldPawns(const Board& board, Color side, Square kingSquare) {
    const Bitboard shieldZone = computeShieldZone(kingSquare, side, 1);
    return board.pieces(side, PAWN) & shieldZone;
}

Bitboard KingSafety::getAdvancedShieldPawns(const Board& board, Color side, Square kingSquare) {
    const Bitboard advancedZone = computeShieldZone(kingSquare, side, 2);
    return board.pieces(side, PAWN) & advancedZone;
}

bool KingSafety::isReasonableKingPosition(Square kingSquare, Color side) {
    if (!isValidSquare(kingSquare)) {
        return false;
    }
    const Bitboard kingBit = squareBB(kingSquare);
    if (side == WHITE) {
        return (kingBit & REASONABLE_KING_SQUARES_WHITE) != 0;
    }
    return (kingBit & REASONABLE_KING_SQUARES_BLACK) != 0;
}

Bitboard KingSafety::getShieldZone(Square kingSquare, Color side) {
    return computeShieldZone(kingSquare, side, 1);
}

bool KingSafety::hasAirSquares(const Board& board, Color side, Square kingSquare) {
    if (!isValidSquare(kingSquare)) {
        return false;
    }

    const Bitboard friendlyPawns = board.pieces(side, PAWN);
    const int direction = (side == WHITE) ? 1 : -1;
    const int kingFile = static_cast<int>(fileOf(kingSquare));
    const int kingRank = static_cast<int>(rankOf(kingSquare));

    const bool onHomeRank = (side == WHITE && kingRank == 0) || (side == BLACK && kingRank == 7);
    const int startStep = onHomeRank ? 2 : 1;

    for (int step = startStep; step <= 2; ++step) {
        const int targetRank = kingRank + direction * step;
        if (targetRank < 0 || targetRank >= NUM_RANKS) {
            continue;
        }
        for (int df = -1; df <= 1; ++df) {
            const int file = kingFile + df;
            if (file < 0 || file >= NUM_FILES) {
                continue;
            }
            const Square sq = makeSquare(static_cast<File>(file), static_cast<Rank>(targetRank));
            if (friendlyPawns & squareBB(sq)) {
                return true;
            }
        }
    }

    return false;
}

}  // namespace seajay::eval
