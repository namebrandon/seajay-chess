#include "king_safety.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

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

constexpr int kPawnAttackUnit = 1;
constexpr int kKnightAttackUnit = 2;
constexpr int kBishopAttackUnit = 2;
constexpr int kRookAttackUnit = 3;
constexpr int kQueenAttackUnit = 4;

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
    .fortressBonusScale = 100,
    .looseBonusScale = 30,
    .fortressPenaltyScale = 100,
    .loosePenaltyScale = 40,
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
    const bool kingInFortress = isReasonableKingPosition(kingSquare, side);

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

    constexpr int SCALE_BASE = 100;
    const int bonusScale = kingInFortress ? s_params.fortressBonusScale : s_params.looseBonusScale;
    const int penaltyScale = kingInFortress ? s_params.fortressPenaltyScale : s_params.loosePenaltyScale;

    int mgScore = 0;
    int egScore = 0;

    const auto scaleValue = [&](int value, int scale) {
        const int64_t product = static_cast<int64_t>(value) * scale;
        const int64_t rounding = product >= 0 ? SCALE_BASE / 2 : -SCALE_BASE / 2;
        return static_cast<int>((product + rounding) / SCALE_BASE);
    };

    auto applyBonus = [&](int mgValue, int egValue) {
        mgScore += scaleValue(mgValue, bonusScale);
        egScore += scaleValue(egValue, bonusScale);
    };
    auto applyPenalty = [&](int mgValue, int egValue) {
        mgScore -= scaleValue(mgValue, penaltyScale);
        egScore -= scaleValue(egValue, penaltyScale);
    };

    applyBonus(directCount * s_params.directShieldMg, directCount * s_params.directShieldEg);
    applyBonus(advancedCount * s_params.advancedShieldMg, advancedCount * s_params.advancedShieldEg);
    applyPenalty(missingDirect * s_params.missingDirectPenaltyMg,
                 missingDirect * s_params.missingDirectPenaltyEg);
    applyPenalty(missingAdvanced * s_params.missingAdvancedPenaltyMg,
                 missingAdvanced * s_params.missingAdvancedPenaltyEg);

    if (kingInFortress && hasAirSquares(board, side, kingSquare)) {
        applyBonus(s_params.airSquareBonusMg, s_params.airSquareBonusEg);
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
                applyPenalty(s_params.semiOpenFilePenaltyMg, s_params.semiOpenFilePenaltyEg);
            } else {
                applyPenalty(s_params.openFilePenaltyMg, s_params.openFilePenaltyEg);
            }
        }
    }

    const Bitboard kingRing = kingRingMask(kingSquare);
    Bitboard attackedMask = 0;
    Bitboard multiAttackedMask = 0;
    int ringAttackSquares = 0;
    int attackUnits = 0;

    auto registerAttacks = [&](Bitboard attacks, int unitWeight) {
        const Bitboard hits = attacks & kingRing;
        if (!hits) {
            return;
        }
        ringAttackSquares += popCount(hits);
        attackUnits += unitWeight;
        multiAttackedMask |= attackedMask & hits;
        attackedMask |= hits;
    };

    Bitboard enemyPawnAttacks = enemyPawns;
    while (enemyPawnAttacks) {
        const Square sq = popLsb(enemyPawnAttacks);
        registerAttacks(MoveGenerator::getPawnAttacks(sq, enemy), kPawnAttackUnit);
    }

    Bitboard enemyKnights = board.pieces(enemy, KNIGHT);
    while (enemyKnights) {
        const Square sq = popLsb(enemyKnights);
        registerAttacks(MoveGenerator::getKnightAttacks(sq), kKnightAttackUnit);
        if (seajay::distance(sq, kingSquare) <= 2) {
            applyPenalty(s_params.minorProximityPenaltyMg, s_params.minorProximityPenaltyEg);
        }
    }

    Bitboard enemyBishops = board.pieces(enemy, BISHOP);
    while (enemyBishops) {
        const Square sq = popLsb(enemyBishops);
        registerAttacks(MoveGenerator::getBishopAttacks(sq, occupancy), kBishopAttackUnit);
        if (seajay::distance(sq, kingSquare) <= 2) {
            applyPenalty(s_params.minorProximityPenaltyMg, s_params.minorProximityPenaltyEg);
        }

        const int fileDiff = std::abs(static_cast<int>(fileOf(sq)) - kingFileIndex);
        const int rankDiff = std::abs(static_cast<int>(rankOf(sq)) - static_cast<int>(rankOf(kingSquare)));
        if (fileDiff == rankDiff && fileDiff != 0 && isLineClear(sq, kingSquare, occupancy)) {
            applyPenalty(s_params.majorProximityPenaltyMg, s_params.majorProximityPenaltyEg);
        }
    }

    Bitboard enemyRooks = board.pieces(enemy, ROOK);
    while (enemyRooks) {
        const Square sq = popLsb(enemyRooks);
        registerAttacks(MoveGenerator::getRookAttacks(sq, occupancy), kRookAttackUnit);
        if (seajay::distance(sq, kingSquare) <= 3) {
            applyPenalty(s_params.majorProximityPenaltyMg, s_params.majorProximityPenaltyEg);
        }
        if (static_cast<int>(fileOf(sq)) == kingFileIndex && isLineClear(sq, kingSquare, occupancy)) {
            applyPenalty(s_params.rookOnOpenFilePenaltyMg, s_params.rookOnOpenFilePenaltyEg);
        }
    }

    Bitboard enemyQueens = board.pieces(enemy, QUEEN);
    while (enemyQueens) {
        const Square sq = popLsb(enemyQueens);
        registerAttacks(MoveGenerator::getQueenAttacks(sq, occupancy), kQueenAttackUnit);
        const int dist = seajay::distance(sq, kingSquare);
        if (dist <= 3) {
            applyPenalty(s_params.majorProximityPenaltyMg, s_params.majorProximityPenaltyEg);
        }
        if (dist <= 2) {
            applyPenalty(s_params.queenContactPenaltyMg, s_params.queenContactPenaltyEg);
        }
        if (static_cast<int>(fileOf(sq)) == kingFileIndex && isLineClear(sq, kingSquare, occupancy)) {
            applyPenalty(s_params.rookOnOpenFilePenaltyMg, s_params.rookOnOpenFilePenaltyEg);
        }
        const int fileDiff = std::abs(static_cast<int>(fileOf(sq)) - kingFileIndex);
        const int rankDiff = std::abs(static_cast<int>(rankOf(sq)) - static_cast<int>(rankOf(kingSquare)));
        if (fileDiff == rankDiff && fileDiff != 0 && isLineClear(sq, kingSquare, occupancy)) {
            applyPenalty(s_params.majorProximityPenaltyMg, s_params.majorProximityPenaltyEg);
        }
    }

    const Square enemyKing = board.kingSquare(enemy);
    if (isValidSquare(enemyKing)) {
        registerAttacks(MoveGenerator::getKingAttacks(enemyKing), kKnightAttackUnit);
    }

    const int multiAttackSquares = popCount(multiAttackedMask);
    const int effectiveRingHits = ringAttackSquares + attackUnits + multiAttackSquares;
    applyPenalty(effectiveRingHits * s_params.attackedRingPenaltyMg,
                 effectiveRingHits * s_params.attackedRingPenaltyEg);

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
