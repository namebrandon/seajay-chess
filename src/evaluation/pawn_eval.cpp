#include "pawn_eval.h"

#include "../core/types.h"
#include "../search/game_phase.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace seajay::eval {

namespace {

constexpr int PASSED_PAWN_BONUS[8] = {
    0,   // Rank 1
    10,  // Rank 2
    17,  // Rank 3
    30,  // Rank 4
    60,  // Rank 5
    120, // Rank 6
    180, // Rank 7
    0    // Rank 8
};

constexpr int ISOLATED_PAWN_PENALTY[8] = {
    0, 15, 14, 12, 12, 10, 8, 0
};

constexpr int FILE_ADJUSTMENT[8] = {
    120, 105, 100, 80, 80, 100, 105, 120
};

constexpr int DOUBLED_PAWN_PENALTY_MG = -8;
constexpr int DOUBLED_PAWN_PENALTY_EG = -3;
constexpr int PAWN_ISLAND_PENALTY = 5;
constexpr int BACKWARD_PAWN_PENALTY = 8;

inline int manhattanDistance(Square a, Square b) {
    return std::abs(rankOf(a) - rankOf(b)) + std::abs(fileOf(a) - fileOf(b));
}

inline int kingDistanceToPromotion(Square king, Square promotionSquare) {
    return std::max(std::abs(rankOf(king) - rankOf(promotionSquare)),
                    std::abs(fileOf(king) - fileOf(promotionSquare)));
}

} // namespace

void buildPawnEntry(const Board& board, PawnEntry& entry) {
    Bitboard whitePawns = board.pieces(WHITE, PAWN);
    Bitboard blackPawns = board.pieces(BLACK, PAWN);

    entry.isolatedPawns[WHITE] = g_pawnStructure.getIsolatedPawns(WHITE, whitePawns);
    entry.isolatedPawns[BLACK] = g_pawnStructure.getIsolatedPawns(BLACK, blackPawns);
    entry.doubledPawns[WHITE] = g_pawnStructure.getDoubledPawns(WHITE, whitePawns);
    entry.doubledPawns[BLACK] = g_pawnStructure.getDoubledPawns(BLACK, blackPawns);
    entry.passedPawns[WHITE] = g_pawnStructure.getPassedPawns(WHITE, whitePawns, blackPawns);
    entry.passedPawns[BLACK] = g_pawnStructure.getPassedPawns(BLACK, blackPawns, whitePawns);
    entry.backwardPawns[WHITE] = g_pawnStructure.getBackwardPawns(WHITE, whitePawns, blackPawns);
    entry.backwardPawns[BLACK] = g_pawnStructure.getBackwardPawns(BLACK, blackPawns, whitePawns);
    entry.pawnIslands[WHITE] = static_cast<uint8_t>(PawnStructure::countPawnIslands(whitePawns));
    entry.pawnIslands[BLACK] = static_cast<uint8_t>(PawnStructure::countPawnIslands(blackPawns));
}

const PawnEntry& getOrBuildPawnEntry(const Board& board, PawnEntry& scratch) {
    const uint64_t pawnKey = board.pawnZobristKey();
    if (PawnEntry* cached = g_pawnStructure.probe(pawnKey)) {
        return *cached;
    }

    scratch = PawnEntry{};
    scratch.key = pawnKey;
    scratch.valid = true;
    buildPawnEntry(board, scratch);
    g_pawnStructure.store(pawnKey, scratch);
    if (PawnEntry* stored = g_pawnStructure.probe(pawnKey)) {
        return *stored;
    }
    return scratch;  // Fallback, should not happen in practice
}

PawnEvalSummary computePawnEval(const Board& board,
                                const PawnEntry& entry,
                                const Material& material,
                                search::GamePhase gamePhase,
                                bool isPureEndgame,
                                Color sideToMove,
                                Square whiteKingSquare,
                                Square blackKingSquare,
                                Bitboard whitePawns,
                                Bitboard blackPawns) {
    PawnEvalSummary summary{};

    Bitboard whitePassedPawns = entry.passedPawns[WHITE];
    Bitboard blackPassedPawns = entry.passedPawns[BLACK];
    summary.whitePassedCount = popCount(whitePassedPawns);
    summary.blackPassedCount = popCount(blackPassedPawns);

    int passedPawnValue = 0;

    Bitboard whitePassers = whitePassedPawns;
    while (whitePassers) {
        Square sq = popLsb(whitePassers);
        int relRank = PawnStructure::relativeRank(WHITE, sq);
        int bonus = PASSED_PAWN_BONUS[relRank];

        Bitboard protectingSquares = 0ULL;
        if (rankOf(sq) > 0) {
            if (fileOf(sq) > 0) protectingSquares |= (1ULL << (sq - 9));
            if (fileOf(sq) < 7) protectingSquares |= (1ULL << (sq - 7));
        }
        if (protectingSquares & whitePawns) {
            bonus = (bonus * 12) / 10;
        }

        Square blockSquare = Square(sq + 8);
        if (blockSquare <= SQ_H8) {
            Piece blocker = board.pieceAt(blockSquare);
            if (blocker != NO_PIECE && colorOf(blocker) == BLACK) {
                int blockPenalty = 0;
                switch (typeOf(blocker)) {
                    case KNIGHT: blockPenalty = bonus / 8; break;
                    case BISHOP: blockPenalty = bonus / 4; break;
                    case ROOK:   blockPenalty = bonus / 6; break;
                    case QUEEN:  blockPenalty = bonus / 5; break;
                    case KING:   blockPenalty = bonus / 6; break;
                    default: break;
                }
                bonus -= blockPenalty;
            }
        }

        if (gamePhase == search::GamePhase::ENDGAME) {
            int friendlyKingDist = manhattanDistance(sq, whiteKingSquare);
            int enemyKingDist = manhattanDistance(sq, blackKingSquare);
            int kingProximityBonus = (8 - friendlyKingDist) * 2;
            int kingProximityPenalty = (8 - enemyKingDist) * 3;
            bonus += kingProximityBonus;
            bonus -= kingProximityPenalty;

            if (relRank >= 4 && isPureEndgame) {
                int pawnDistToPromotion = 7 - rankOf(sq);
                Square promotionSquare = Square(fileOf(sq) + 56);  // rank 8 same file
                int kingDistToPromotion = kingDistanceToPromotion(blackKingSquare, promotionSquare);
                int moveAdvantage = (sideToMove == WHITE) ? 1 : 0;
                if (kingDistToPromotion > pawnDistToPromotion + moveAdvantage) {
                    bonus += 300;
                }
            }
        }

        int file = fileOf(sq);
        Bitboard adjacentFiles = 0ULL;
        if (file > 0) adjacentFiles |= FILE_A_BB << (file - 1);
        if (file < 7) adjacentFiles |= FILE_A_BB << (file + 1);
        Bitboard otherPassed = whitePassedPawns & ~(1ULL << sq);
        Bitboard adjacentPassed = otherPassed & adjacentFiles;
        while (adjacentPassed) {
            Square adjSq = popLsb(adjacentPassed);
            int rankDiff = std::abs(rankOf(sq) - rankOf(adjSq));
            if (rankDiff <= 1 && rankOf(sq) > rankOf(adjSq)) {
                bonus = (bonus * 12) / 10;
                break;
            }
        }

        passedPawnValue += bonus;
    }

    Bitboard blackPassers = blackPassedPawns;
    while (blackPassers) {
        Square sq = popLsb(blackPassers);
        int relRank = PawnStructure::relativeRank(BLACK, sq);
        int bonus = PASSED_PAWN_BONUS[relRank];

        Bitboard protectingSquares = 0ULL;
        if (rankOf(sq) < 7) {
            if (fileOf(sq) > 0) protectingSquares |= (1ULL << (sq + 7));
            if (fileOf(sq) < 7) protectingSquares |= (1ULL << (sq + 9));
        }
        if (protectingSquares & blackPawns) {
            bonus = (bonus * 12) / 10;
        }

        Square blockSquare = Square(sq - 8);
        if (blockSquare >= SQ_A1) {
            Piece blocker = board.pieceAt(blockSquare);
            if (blocker != NO_PIECE && colorOf(blocker) == WHITE) {
                int blockPenalty = 0;
                switch (typeOf(blocker)) {
                    case KNIGHT: blockPenalty = bonus / 8; break;
                    case BISHOP: blockPenalty = bonus / 4; break;
                    case ROOK:   blockPenalty = bonus / 6; break;
                    case QUEEN:  blockPenalty = bonus / 5; break;
                    case KING:   blockPenalty = bonus / 6; break;
                    default: break;
                }
                bonus -= blockPenalty;
            }
        }

        if (gamePhase == search::GamePhase::ENDGAME) {
            int friendlyKingDist = manhattanDistance(sq, blackKingSquare);
            int enemyKingDist = manhattanDistance(sq, whiteKingSquare);
            int kingProximityBonus = (8 - friendlyKingDist) * 2;
            int kingProximityPenalty = (8 - enemyKingDist) * 3;
            bonus += kingProximityBonus;
            bonus -= kingProximityPenalty;

            if (relRank >= 4 && isPureEndgame) {
                int pawnDistToPromotion = rankOf(sq);
                Square promotionSquare = Square(fileOf(sq));
                int kingDistToPromotion = kingDistanceToPromotion(whiteKingSquare, promotionSquare);
                int moveAdvantage = (sideToMove == BLACK) ? 1 : 0;
                if (kingDistToPromotion > pawnDistToPromotion + moveAdvantage) {
                    bonus += 300;
                }
            }
        }

        int file = fileOf(sq);
        Bitboard adjacentFiles = 0ULL;
        if (file > 0) adjacentFiles |= FILE_A_BB << (file - 1);
        if (file < 7) adjacentFiles |= FILE_A_BB << (file + 1);
        Bitboard otherPassed = blackPassedPawns & ~(1ULL << sq);
        Bitboard adjacentPassed = otherPassed & adjacentFiles;
        while (adjacentPassed) {
            Square adjSq = popLsb(adjacentPassed);
            int rankDiff = std::abs(rankOf(sq) - rankOf(adjSq));
            if (rankDiff <= 1 && rankOf(sq) < rankOf(adjSq)) {
                bonus = (bonus * 12) / 10;
                break;
            }
        }

        passedPawnValue -= bonus;
    }

    switch (gamePhase) {
        case search::GamePhase::OPENING:
            passedPawnValue = passedPawnValue / 2;
            break;
        case search::GamePhase::MIDDLEGAME:
            passedPawnValue = (passedPawnValue * 3) / 4;
            break;
        case search::GamePhase::ENDGAME:
            passedPawnValue = (passedPawnValue * 3) / 2;
            break;
    }

    summary.passed = Score(passedPawnValue);

    int isolatedValue = 0;
    Bitboard whiteIsolani = entry.isolatedPawns[WHITE];
    while (whiteIsolani) {
        Square sq = popLsb(whiteIsolani);
        int rank = rankOf(sq);
        int file = fileOf(sq);
        int penalty = ISOLATED_PAWN_PENALTY[rank];
        penalty = (penalty * FILE_ADJUSTMENT[file]) / 100;
        isolatedValue -= penalty;
    }

    Bitboard blackIsolani = entry.isolatedPawns[BLACK];
    while (blackIsolani) {
        Square sq = popLsb(blackIsolani);
        int rank = 7 - rankOf(sq);
        int file = fileOf(sq);
        int penalty = ISOLATED_PAWN_PENALTY[rank];
        penalty = (penalty * FILE_ADJUSTMENT[file]) / 100;
        isolatedValue += penalty;
    }

    if (gamePhase == search::GamePhase::ENDGAME) {
        isolatedValue /= 2;
    }

    summary.isolated = Score(isolatedValue);

    int whiteDoubledCount = popCount(entry.doubledPawns[WHITE]);
    int blackDoubledCount = popCount(entry.doubledPawns[BLACK]);
    int doubledPenalty = (gamePhase == search::GamePhase::ENDGAME)
        ? std::abs(DOUBLED_PAWN_PENALTY_EG)
        : std::abs(DOUBLED_PAWN_PENALTY_MG);
    int doubledValue = (blackDoubledCount - whiteDoubledCount) * doubledPenalty;
    summary.doubled = Score(doubledValue);

    int whiteIslandPenalty = (entry.pawnIslands[WHITE] > 1)
        ? (entry.pawnIslands[WHITE] - 1) * PAWN_ISLAND_PENALTY
        : 0;
    int blackIslandPenalty = (entry.pawnIslands[BLACK] > 1)
        ? (entry.pawnIslands[BLACK] - 1) * PAWN_ISLAND_PENALTY
        : 0;
    summary.islands = Score(blackIslandPenalty - whiteIslandPenalty);

    int whiteBackwardCount = popCount(entry.backwardPawns[WHITE]);
    int blackBackwardCount = popCount(entry.backwardPawns[BLACK]);
    int backwardValue = (blackBackwardCount - whiteBackwardCount) * BACKWARD_PAWN_PENALTY;
    summary.backward = Score(backwardValue);

    summary.total = summary.passed + summary.isolated + summary.doubled + summary.islands + summary.backward;
    return summary;
}

} // namespace seajay::eval
