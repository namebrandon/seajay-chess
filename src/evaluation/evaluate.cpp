#include "evaluate.h"
#include "eval_trace.h"  // For evaluation tracing
#include "material.h"
#include "pst.h"  // Stage 9: Include PST header
#include "pawn_structure.h"  // Phase PP2: Passed pawn evaluation
#include "king_safety.h"  // Phase KS2: King safety evaluation
#include "../core/board.h"
#include "../core/bitboard.h"
#include "../core/move_generation.h"  // MOB2: For mobility calculation
#include "../core/simd_utils.h"  // Phase 2.5.e-3: SIMD optimizations
#include "../core/engine_config.h"  // PST Phase Interpolation: For UCI options
#include "../search/game_phase.h"  // Phase PP2: For phase scaling
#include <cstdlib>  // PP3b: For std::abs
#include <algorithm>  // For std::clamp
#include <array>

namespace seajay::eval {

namespace {

// Lightweight cache for promotion-path attack lookups. We amortize the cost of
// computing attackers to a given square by storing both colors' results on the
// first query. Passed-pawn evaluation may touch the same squares multiple
// times (stop square, promotion path, telemetry hooks), so this keeps the
// walker from hammering MoveGenerator::isSquareAttacked repeatedly.
class PromotionPathAttackCache {
public:
    explicit PromotionPathAttackCache(const Board& board) noexcept
        : m_occupied(board.occupied()) {
        m_colorBitboards[WHITE] = board.pieces(WHITE);
        m_colorBitboards[BLACK] = board.pieces(BLACK);
        m_pawns[WHITE] = board.pieces(WHITE, PAWN);
        m_pawns[BLACK] = board.pieces(BLACK, PAWN);
        m_knights = board.pieces(WHITE, KNIGHT) | board.pieces(BLACK, KNIGHT);
        m_bishops = board.pieces(WHITE, BISHOP) | board.pieces(BLACK, BISHOP);
        m_rooks = board.pieces(WHITE, ROOK) | board.pieces(BLACK, ROOK);
        m_queens = board.pieces(WHITE, QUEEN) | board.pieces(BLACK, QUEEN);
        m_kings = board.pieces(WHITE, KING) | board.pieces(BLACK, KING);

        for (auto& perColor : m_cache) {
            perColor.fill(kUnknown);
        }
    }

    bool isAttacked(Color color, Square square) {
        const int colorIndex = static_cast<int>(color);
        const size_t sqIndex = static_cast<size_t>(square);
        int8_t state = m_cache[colorIndex][sqIndex];
        if (state == kUnknown) {
            populateCache(square);
            state = m_cache[colorIndex][sqIndex];
        }
        return state == kTrue;
    }

private:
    static constexpr int8_t kUnknown = -1;
    static constexpr int8_t kFalse = 0;
    static constexpr int8_t kTrue = 1;

    void populateCache(Square square) {
        const size_t sqIndex = static_cast<size_t>(square);
        const Bitboard attackers = attackersTo(square);
        m_cache[WHITE][sqIndex] = (attackers & m_colorBitboards[WHITE]) ? kTrue : kFalse;
        m_cache[BLACK][sqIndex] = (attackers & m_colorBitboards[BLACK]) ? kTrue : kFalse;
    }

    Bitboard attackersTo(Square square) const {
        Bitboard attackers = 0ULL;
        const int file = fileOf(square);
        const int rank = rankOf(square);
        const int sqIdx = static_cast<int>(square);

        if (rank > 0) {
            if (file > 0) {
                attackers |= squareBB(static_cast<Square>(sqIdx - 9)) & m_pawns[WHITE];
            }
            if (file < 7) {
                attackers |= squareBB(static_cast<Square>(sqIdx - 7)) & m_pawns[WHITE];
            }
        }

        if (rank < 7) {
            if (file > 0) {
                attackers |= squareBB(static_cast<Square>(sqIdx + 7)) & m_pawns[BLACK];
            }
            if (file < 7) {
                attackers |= squareBB(static_cast<Square>(sqIdx + 9)) & m_pawns[BLACK];
            }
        }

        attackers |= MoveGenerator::getKnightAttacks(square) & m_knights;
        attackers |= MoveGenerator::getKingAttacks(square) & m_kings;
        attackers |= MoveGenerator::getBishopAttacks(square, m_occupied) & (m_bishops | m_queens);
        attackers |= MoveGenerator::getRookAttacks(square, m_occupied) & (m_rooks | m_queens);

        return attackers;
    }

    const Bitboard m_occupied;
    std::array<Bitboard, NUM_COLORS> m_colorBitboards{};
    std::array<Bitboard, NUM_COLORS> m_pawns{};
    Bitboard m_knights{};
    Bitboard m_bishops{};
    Bitboard m_rooks{};
    Bitboard m_queens{};
    Bitboard m_kings{};
    std::array<std::array<int8_t, 64>, NUM_COLORS> m_cache{};
};

}  // namespace

// PST Phase Interpolation - Phase calculation (continuous, fast)
// Returns phase value from 0 (pure endgame) to 256 (pure middlegame)
inline int phase0to256(const Board& board) noexcept {
    // Material weights for phase calculation
    // P=0 (pawns don't affect phase), N=1, B=1, R=2, Q=4, K=0
    constexpr int PHASE_WEIGHT[6] = { 0, 1, 1, 2, 4, 0 };
    
    // Maximum phase = 2*(N+N+B+B+R+R+Q) = 2*(1+1+1+1+2+2+4) = 24
    constexpr int TOTAL_PHASE = 24;
    
    // Count non-pawn material
    int phase = 0;
    phase += popCount(board.pieces(KNIGHT)) * PHASE_WEIGHT[KNIGHT];
    phase += popCount(board.pieces(BISHOP)) * PHASE_WEIGHT[BISHOP];
    phase += popCount(board.pieces(ROOK))   * PHASE_WEIGHT[ROOK];
    phase += popCount(board.pieces(QUEEN))  * PHASE_WEIGHT[QUEEN];
    
    // Scale to [0,256] with rounding
    // 256 = full middlegame, 0 = pure endgame
    return std::clamp((phase * 256 + TOTAL_PHASE/2) / TOTAL_PHASE, 0, 256);
}
// Template implementation for evaluation with optional tracing
template<bool Traced>
Score evaluateImpl(const Board& board, EvalTrace* trace = nullptr) {
    // Reset trace if provided
    if constexpr (Traced) {
        if (trace) trace->reset();
    }
    
    // Get material from board
    const Material& material = board.material();
    
    // Check for insufficient material draws
    if (material.isInsufficientMaterial()) {
        return Score::draw();
    }
    
    // Check for same-colored bishops (KB vs KB)
    // This is a special case where we need board position info
    if (material.count(WHITE, BISHOP) == 1 && 
        material.count(BLACK, BISHOP) == 1 &&
        material.count(WHITE, PAWN) == 0 && 
        material.count(BLACK, PAWN) == 0 &&
        material.count(WHITE, KNIGHT) == 0 && 
        material.count(BLACK, KNIGHT) == 0 &&
        material.count(WHITE, ROOK) == 0 && 
        material.count(BLACK, ROOK) == 0 &&
        material.count(WHITE, QUEEN) == 0 && 
        material.count(BLACK, QUEEN) == 0) {
        
        // Find the bishops and check if they're on same color squares
        Bitboard whiteBishops = board.pieces(WHITE, BISHOP);
        Bitboard blackBishops = board.pieces(BLACK, BISHOP);
        
        if (whiteBishops && blackBishops) {
            Square wbSq = lsb(whiteBishops);
            Square bbSq = lsb(blackBishops);
            
            // Check if both bishops are on same color squares
            // (rank + file) % 2 == 0 means dark square
            bool wbDark = ((wbSq / 8) + (wbSq % 8)) % 2 == 0;
            bool bbDark = ((bbSq / 8) + (bbSq % 8)) % 2 == 0;
            
            if (wbDark == bbDark) {
                return Score::draw();  // Same colored bishops = draw
            }
        }
    }
    
    // Get PST score (Stage 9)
    // PST score is stored from white's perspective in the board
    const MgEgScore& pstScore = board.pstScore();
    
    Score pstValue;
    if (seajay::getConfig().usePSTInterpolation) {
        // PST Phase Interpolation - calculate phase and interpolate mg/eg values
        int phase = phase0to256(board);  // 256 = full MG, 0 = full EG
        int invPhase = 256 - phase;      // Inverse phase for endgame weight
        
        // Trace phase and PST components
        if constexpr (Traced) {
            if (trace) {
                trace->phase256 = phase;
                trace->pstMg = pstScore.mg;
                trace->pstEg = pstScore.eg;
            }
        }
        
        // Fixed-point blend with rounding (shift by 8 to divide by 256)
        int blendedPst = (pstScore.mg.value() * phase + pstScore.eg.value() * invPhase + 128) >> 8;
        pstValue = Score(blendedPst);
    } else {
        // Original behavior - use only middlegame PST values (no tapering)
        pstValue = pstScore.mg;
    }
    
    // Trace PST value
    if constexpr (Traced) {
        if (trace) trace->pst = pstValue;
    }
    
    // Phase PP2: Passed pawn evaluation
    // Rank-based bonuses (indexed by relative rank)
    static constexpr int PASSED_PAWN_BONUS[8] = {
        0,    // Rank 1 (no bonus for pawns on first rank)
        10,   // Rank 2
        17,   // Rank 3
        30,   // Rank 4
        60,   // Rank 5
        120,  // Rank 6
        180,  // Rank 7
        0     // Rank 8 (promoted, handled elsewhere)
    };
    
    // Get pawn bitboards
    Bitboard whitePawns = board.pieces(WHITE, PAWN);
    Bitboard blackPawns = board.pieces(BLACK, PAWN);
    
    // Phase 2.5.b: Cache frequently used values at the start
    // Cache game phase once (was being called 3 times)
    search::GamePhase gamePhase = search::detectGamePhase(board);
    
    // Cache king squares once (was being fetched multiple times)
    Square whiteKingSquare = board.kingSquare(WHITE);
    Square blackKingSquare = board.kingSquare(BLACK);
    
    // Cache side to move (accessed 4 times)
    Color sideToMove = board.sideToMove();
    
    // Cache pure endgame check once (was calculated twice with identical logic)
    bool isPureEndgame = board.pieces(WHITE, KNIGHT) == 0 &&
                        board.pieces(WHITE, BISHOP) == 0 &&
                        board.pieces(WHITE, ROOK) == 0 &&
                        board.pieces(WHITE, QUEEN) == 0 &&
                        board.pieces(BLACK, KNIGHT) == 0 &&
                        board.pieces(BLACK, BISHOP) == 0 &&
                        board.pieces(BLACK, ROOK) == 0 &&
                        board.pieces(BLACK, QUEEN) == 0;
    
    // PPH2: Get cached pawn structure evaluation early
    uint64_t pawnKey = board.pawnZobristKey();
    PawnEntry* pawnEntry = g_pawnStructure.probe(pawnKey);

    if constexpr (Traced) {
        if (trace) {
            trace->pawnKey = pawnKey;
            trace->pawnCacheHit = (pawnEntry != nullptr);
        }
    }
    
    Bitboard whiteIsolated, blackIsolated, whiteDoubled, blackDoubled;
    Bitboard whitePassedPawns, blackPassedPawns;
    Bitboard whiteBackward, blackBackward;  // BP2: Track backward pawns
    uint8_t whiteIslands, blackIslands;  // PI2: Track island counts
    
    if (!pawnEntry) {
        // Cache miss - compute pawn structure
        PawnEntry newEntry;
        newEntry.key = pawnKey;
        newEntry.valid = true;
        
        // Compute all pawn structure features
        newEntry.isolatedPawns[WHITE] = g_pawnStructure.getIsolatedPawns(WHITE, whitePawns);
        newEntry.isolatedPawns[BLACK] = g_pawnStructure.getIsolatedPawns(BLACK, blackPawns);
        newEntry.doubledPawns[WHITE] = g_pawnStructure.getDoubledPawns(WHITE, whitePawns);
        newEntry.doubledPawns[BLACK] = g_pawnStructure.getDoubledPawns(BLACK, blackPawns);
        newEntry.passedPawns[WHITE] = g_pawnStructure.getPassedPawns(WHITE, whitePawns, blackPawns);
        newEntry.passedPawns[BLACK] = g_pawnStructure.getPassedPawns(BLACK, blackPawns, whitePawns);
        // BP2: Compute and cache backward pawns
        newEntry.backwardPawns[WHITE] = g_pawnStructure.getBackwardPawns(WHITE, whitePawns, blackPawns);
        newEntry.backwardPawns[BLACK] = g_pawnStructure.getBackwardPawns(BLACK, blackPawns, whitePawns);
        // PI2: Compute and cache pawn island counts
        newEntry.pawnIslands[WHITE] = PawnStructure::countPawnIslands(whitePawns);
        newEntry.pawnIslands[BLACK] = PawnStructure::countPawnIslands(blackPawns);
        
        // Store in cache
        g_pawnStructure.store(pawnKey, newEntry);
        
        // Use computed values
        whiteIsolated = newEntry.isolatedPawns[WHITE];
        blackIsolated = newEntry.isolatedPawns[BLACK];
        whiteDoubled = newEntry.doubledPawns[WHITE];
        blackDoubled = newEntry.doubledPawns[BLACK];
        whitePassedPawns = newEntry.passedPawns[WHITE];
        blackPassedPawns = newEntry.passedPawns[BLACK];
        whiteBackward = newEntry.backwardPawns[WHITE];  // BP2
        blackBackward = newEntry.backwardPawns[BLACK];  // BP2
        whiteIslands = newEntry.pawnIslands[WHITE];  // PI2
        blackIslands = newEntry.pawnIslands[BLACK];  // PI2
    } else {
        // Cache hit - use stored values
        whiteIsolated = pawnEntry->isolatedPawns[WHITE];
        blackIsolated = pawnEntry->isolatedPawns[BLACK];
        whiteDoubled = pawnEntry->doubledPawns[WHITE];
        blackDoubled = pawnEntry->doubledPawns[BLACK];
        whitePassedPawns = pawnEntry->passedPawns[WHITE];
        blackPassedPawns = pawnEntry->passedPawns[BLACK];
        whiteBackward = pawnEntry->backwardPawns[WHITE];  // BP2
        blackBackward = pawnEntry->backwardPawns[BLACK];  // BP2
        whiteIslands = pawnEntry->pawnIslands[WHITE];  // PI2
        blackIslands = pawnEntry->pawnIslands[BLACK];  // PI2
    }
    
    // Calculate passed pawn score
    int passedPawnValue = 0;

    struct PasserTelemetry {
        int count = 0;
        int totalBonus = 0;
        int maxRank = 0;
        int friendlyKingDist = 0;
        int enemyKingDist = 0;
        bool protectedPawn = false;
        bool connected = false;
        bool blockaded = false;
        bool unstoppable = false;
        bool pathFree = false;
        bool stopDefended = false;
        bool rookSupport = false;
    };

    PasserTelemetry whiteTelemetry;
    PasserTelemetry blackTelemetry;

    auto chebyshevDistance = [](Square a, Square b) -> int {
        return std::max(std::abs(rankOf(a) - rankOf(b)), std::abs(fileOf(a) - fileOf(b)));
    };

    static constexpr int PASSED_FILE_BONUS[8] = { 0, 4, 6, 8, 8, 6, 4, 0 };
    static constexpr int PASSER_NONLINEAR_BONUS[8] = {
        0,  // Rank 1
        0,  // Rank 2 (no additional bonus beyond base table)
        1,  // Rank 3
        4,  // Rank 4
        8,  // Rank 5
        14, // Rank 6
        24, // Rank 7
        0   // Rank 8
    };

    static constexpr int PASSER_BLOCKED_DEFENDED_PENALTY = 6;

    const auto& config = seajay::getConfig();
    const int PASSER_PATH_FREE_BONUS = config.passerPathFreeBonus;
    const int PASSER_PATH_SAFE_BONUS = config.passerPathSafeBonus;
    const int PASSER_PATH_DEFENDED_BONUS = config.passerPathDefendedBonus;
    const int PASSER_PATH_ATTACKED_PENALTY = config.passerPathAttackedPenalty;
    const int PASSER_STOP_DEFENDED_BONUS = config.passerStopDefendedBonus;
    const int PASSER_STOP_ATTACKED_PENALTY = config.passerStopAttackedPenalty;
    const int PASSER_ROOK_SUPPORT_BONUS = config.passerRookSupportBonus;
    const int PASSER_ENEMY_ROOK_BEHIND_PENALTY = config.passerEnemyRookBehindPenalty;
    const int PASSER_KING_DISTANCE_SCALE = config.passerKingDistanceScale;

    const bool usePasserPhaseP4 = config.usePasserPhaseP4;
    PromotionPathAttackCache pathAttackCache(board);

    auto processPassers = [&](Color color, Bitboard passersBase, PasserTelemetry& telemetry) {
        Bitboard passers = passersBase;
        const Color enemy = (color == WHITE) ? BLACK : WHITE;
        const Bitboard ownPawns = (color == WHITE) ? whitePawns : blackPawns;
        const Square friendlyKing = (color == WHITE) ? whiteKingSquare : blackKingSquare;
        const Square enemyKing = (color == WHITE) ? blackKingSquare : whiteKingSquare;
        const int forward = (color == WHITE) ? 8 : -8;
        const Bitboard sameColorPassers = passersBase;

        while (passers) {
            Square sq = popLsb(passers);
            telemetry.count++;

            const int relRank = PawnStructure::relativeRank(color, sq);
            int bonus = PASSED_PAWN_BONUS[relRank];
            const int file = fileOf(sq);

            bool isProtected = false;
            bool hasBlocker = false;
            bool friendlyRookBehind = false;
            bool stopDefended = false;
            bool pathFree = false;
            bool defendedStopTier = false;

            int friendlyKingDist = 0;
            int enemyKingDist = 0;

            Bitboard protectingSquares = 0ULL;
            if (color == WHITE) {
                if (rankOf(sq) > 0) {
                    if (file > 0) protectingSquares |= (1ULL << (sq - 9));
                    if (file < 7) protectingSquares |= (1ULL << (sq - 7));
                }
            } else {
                if (rankOf(sq) < 7) {
                    if (file > 0) protectingSquares |= (1ULL << (sq + 7));
                    if (file < 7) protectingSquares |= (1ULL << (sq + 9));
                }
            }
            Bitboard pawnSupport = protectingSquares & ownPawns;
            if (pawnSupport) {
                bonus = (bonus * 12) / 10;
                isProtected = true;
            }

            int blockIndex = static_cast<int>(sq) + forward;
            if (blockIndex >= 0 && blockIndex < 64) {
                Piece blocker = board.pieceAt(static_cast<Square>(blockIndex));
                if (blocker != NO_PIECE && colorOf(blocker) == enemy) {
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
                    hasBlocker = true;
                }
            }

            if (!usePasserPhaseP4 && gamePhase == search::GamePhase::ENDGAME) {
                int friendlyKingDistLegacy = std::abs(rankOf(sq) - rankOf(friendlyKing)) +
                                             std::abs(fileOf(sq) - fileOf(friendlyKing));
                int enemyKingDistLegacy = std::abs(rankOf(sq) - rankOf(enemyKing)) +
                                          std::abs(fileOf(sq) - fileOf(enemyKing));
                bonus += (8 - friendlyKingDistLegacy) * 2;
                bonus -= (8 - enemyKingDistLegacy) * 3;
            }

            Bitboard pathSquares = 0ULL;
            Square stopSquare = NO_SQUARE;
            bool validStop = false;

            if (usePasserPhaseP4) {
                bonus += PASSED_FILE_BONUS[file];
                int p4Adjust = PASSER_NONLINEAR_BONUS[relRank];

                const int rankWeight = std::max(0, relRank - 2);

                int idx = static_cast<int>(sq) + forward;
                if (idx >= 0 && idx < 64) {
                    stopSquare = static_cast<Square>(idx);
                    validStop = true;
                }

                int pathIdx = idx;
                while (pathIdx >= 0 && pathIdx < 64) {
                    pathSquares |= (1ULL << pathIdx);
                    pathIdx += forward;
                }

                pathFree = ((pathSquares & board.occupied()) == 0);
                bool pathEnemyControl = false;
                bool pathOwnControl = true;
                Bitboard temp = pathSquares;
                while (temp && (!pathEnemyControl || pathOwnControl)) {
                    Square pathSq = popLsb(temp);
                    if (!pathEnemyControl && pathAttackCache.isAttacked(enemy, pathSq)) {
                        pathEnemyControl = true;
                    }
                    if (pathOwnControl && !pathAttackCache.isAttacked(color, pathSq)) {
                        pathOwnControl = false;
                    }
                }

                bool stopEnemyControl = false;
                if (validStop) {
                    stopEnemyControl = pathAttackCache.isAttacked(enemy, stopSquare);
                    stopDefended = pathAttackCache.isAttacked(color, stopSquare);
                } else {
                    stopDefended = false;
                }

                const bool freeStop = pathFree && validStop && !stopEnemyControl;
                const bool pathFullyDefended = pathFree && pathOwnControl;
                const bool pathSafe = pathFree && !pathEnemyControl;
                const bool localDefendedStop = freeStop && pathFullyDefended && stopDefended;
                defendedStopTier = localDefendedStop;

                // Laser-style promotion path tiers: unlock bonuses only when
                // each successive condition (free path → free stop → fully
                // defended → defended stop) is satisfied.
                if (rankWeight > 0 && pathFree) {
                    p4Adjust += rankWeight * PASSER_PATH_FREE_BONUS;

                    if (freeStop) {
                        p4Adjust += rankWeight * PASSER_PATH_SAFE_BONUS;

                        if (pathFullyDefended) {
                            p4Adjust += rankWeight * PASSER_PATH_DEFENDED_BONUS;
                        }

                        if (localDefendedStop) {
                            p4Adjust += PASSER_STOP_DEFENDED_BONUS;
                        }
                    }

                    if (!pathSafe) {
                        p4Adjust -= rankWeight * PASSER_PATH_ATTACKED_PENALTY;
                    }
                }

                if (stopDefended && hasBlocker) {
                    int blockedPenalty = std::max(1, rankWeight) * PASSER_BLOCKED_DEFENDED_PENALTY;
                    bonus -= blockedPenalty;
                }

                if (validStop && stopEnemyControl) {
                    bonus -= PASSER_STOP_ATTACKED_PENALTY;
                }

                int behindIdx = static_cast<int>(sq) - forward;
                while (behindIdx >= 0 && behindIdx < 64) {
                    Square behindSq = static_cast<Square>(behindIdx);
                    Piece piece = board.pieceAt(behindSq);
                    if (piece != NO_PIECE) {
                        if (colorOf(piece) == color && (typeOf(piece) == ROOK || typeOf(piece) == QUEEN)) {
                            friendlyRookBehind = true;
                        } else if (colorOf(piece) == enemy && (typeOf(piece) == ROOK || typeOf(piece) == QUEEN)) {
                            p4Adjust -= PASSER_ENEMY_ROOK_BEHIND_PENALTY;
                        }
                        break;
                    }
                    behindIdx -= forward;
                }
                if (friendlyRookBehind) {
                    p4Adjust += PASSER_ROOK_SUPPORT_BONUS;
                }

                Square promotionSquare = (color == WHITE) ? static_cast<Square>(file + 56) : static_cast<Square>(file);
                friendlyKingDist = chebyshevDistance(friendlyKing, promotionSquare);
                enemyKingDist = chebyshevDistance(enemyKing, promotionSquare);
                if (relRank >= 5) {
                    const int kingWeight = relRank - 4;
                    const int friendlyTerm = friendlyKingDist * friendlyKingDist;
                    const int enemyTerm = enemyKingDist * enemyKingDist;
                    const int distanceDiff = enemyTerm - friendlyTerm;
                    const int scaledRamp = (distanceDiff * PASSER_KING_DISTANCE_SCALE * kingWeight) / 4;
                    p4Adjust += scaledRamp;
                }

                // Damp Phase P4 adjustment to avoid overshooting while coefficients are tuned
                p4Adjust = (p4Adjust * 3) / 8;
                p4Adjust = std::clamp(p4Adjust, -80, 80);

                bonus += p4Adjust;
            }

            Bitboard adjacentFiles = 0ULL;
            if (file > 0) adjacentFiles |= FILE_A_BB << (file - 1);
            if (file < 7) adjacentFiles |= FILE_A_BB << (file + 1);
            Bitboard otherPassed = sameColorPassers & ~(1ULL << sq);
            Bitboard adjacentPassed = otherPassed & adjacentFiles;
            bool hasConnectedPasser = false;
            while (adjacentPassed && !hasConnectedPasser) {
                Square adjSq = popLsb(adjacentPassed);
                int adjRank = PawnStructure::relativeRank(color, adjSq);
                if (std::abs(adjRank - relRank) <= 1 && adjRank > relRank) {
                    hasConnectedPasser = true;
                    bonus = (bonus * 12) / 10;
                }
            }

            bool unstoppable = false;
            if (relRank >= 4 && isPureEndgame) {
                int pawnDistToPromotion = (color == WHITE) ? (7 - rankOf(sq)) : rankOf(sq);
                Square promotionSquare = (color == WHITE) ? static_cast<Square>(file + 56) : static_cast<Square>(file);
                int kingDistToPromotion = chebyshevDistance(enemyKing, promotionSquare);
                int moveAdvantage = (sideToMove == color) ? 1 : 0;
                if (kingDistToPromotion > pawnDistToPromotion + moveAdvantage + 1) {
                    unstoppable = true;
                }
            }

            if (bonus < 0) {
                bonus = 0;
            }

            telemetry.totalBonus += bonus;
            telemetry.protectedPawn |= isProtected;
            telemetry.blockaded |= hasBlocker;
            telemetry.connected |= hasConnectedPasser;
            telemetry.unstoppable |= unstoppable;
            telemetry.pathFree |= (usePasserPhaseP4 && pathFree);
            telemetry.stopDefended |= (usePasserPhaseP4 && defendedStopTier);
            telemetry.rookSupport |= (usePasserPhaseP4 && friendlyRookBehind);
            if (usePasserPhaseP4 && relRank >= telemetry.maxRank) {
                telemetry.maxRank = relRank;
                telemetry.friendlyKingDist = friendlyKingDist;
                telemetry.enemyKingDist = enemyKingDist;
            }

            if (color == WHITE) {
                passedPawnValue += bonus;
            } else {
                passedPawnValue -= bonus;
            }
        }
    };

    processPassers(WHITE, whitePassedPawns, whiteTelemetry);
    processPassers(BLACK, blackPassedPawns, blackTelemetry);

// Phase scaling: passed pawns are more valuable in endgame
    // Using cached gamePhase from Phase 2.5.b
    
    // Scale passed pawn bonus based on phase
    // Opening: 50% value, Middlegame: 75% value, Endgame: 150% value
    switch (gamePhase) {
        case search::GamePhase::OPENING:
            passedPawnValue = passedPawnValue / 2;  // 50%
            break;
        case search::GamePhase::MIDDLEGAME:
            passedPawnValue = (passedPawnValue * 3) / 4;  // 75%
            break;
        case search::GamePhase::ENDGAME:
            passedPawnValue = (passedPawnValue * 3) / 2;  // 150%
            break;
    }
    
    // Convert passed pawn value to Score
    Score passedPawnScore(passedPawnValue);
    
    // Trace passed pawn score
    if constexpr (Traced) {
        if (trace) {
            trace->passedPawns = passedPawnScore;
            trace->passedDetail.whiteCount = whiteTelemetry.count;
            trace->passedDetail.blackCount = blackTelemetry.count;
            trace->passedDetail.whiteBonus = Score(whiteTelemetry.totalBonus);
            trace->passedDetail.blackBonus = Score(-blackTelemetry.totalBonus);
            trace->passedDetail.whiteHasProtected = whiteTelemetry.protectedPawn;
            trace->passedDetail.blackHasProtected = blackTelemetry.protectedPawn;
            trace->passedDetail.whiteHasBlockaded = whiteTelemetry.blockaded;
            trace->passedDetail.blackHasBlockaded = blackTelemetry.blockaded;
            trace->passedDetail.whiteHasConnected = whiteTelemetry.connected;
            trace->passedDetail.blackHasConnected = blackTelemetry.connected;
            trace->passedDetail.whiteHasUnstoppable = whiteTelemetry.unstoppable;
            trace->passedDetail.blackHasUnstoppable = blackTelemetry.unstoppable;
            trace->passedDetail.whitePathFree = whiteTelemetry.pathFree;
            trace->passedDetail.blackPathFree = blackTelemetry.pathFree;
            trace->passedDetail.whiteStopDefended = whiteTelemetry.stopDefended;
            trace->passedDetail.blackStopDefended = blackTelemetry.stopDefended;
            trace->passedDetail.whiteRookSupport = whiteTelemetry.rookSupport;
            trace->passedDetail.blackRookSupport = blackTelemetry.rookSupport;
            trace->passedDetail.whiteMaxRank = whiteTelemetry.maxRank;
            trace->passedDetail.blackMaxRank = blackTelemetry.maxRank;
            trace->passedDetail.whiteFriendlyKingDist = whiteTelemetry.friendlyKingDist;
            trace->passedDetail.whiteEnemyKingDist = whiteTelemetry.enemyKingDist;
            trace->passedDetail.blackFriendlyKingDist = blackTelemetry.friendlyKingDist;
            trace->passedDetail.blackEnemyKingDist = blackTelemetry.enemyKingDist;
        }
    }
    
    // Phase IP2: Isolated pawn evaluation
    // Base penalties by rank (white perspective) - conservative values
    static constexpr int ISOLATED_PAWN_PENALTY[8] = {
        0,   // Rank 1 - no pawns here
        15,  // Rank 2 - most vulnerable
        14,  // Rank 3
        12,  // Rank 4 - standard
        12,  // Rank 5
        10,  // Rank 6
        8,   // Rank 7 - less weak when advanced
        0    // Rank 8 - promoted
    };
    
    // Phase IP3a: File-based adjustments (percentage multipliers)
    static constexpr int FILE_ADJUSTMENT[8] = {
        120,  // a-file (edge pawn penalty)
        105,  // b-file
        100,  // c-file (standard)
        80,   // d-file (central bonus - controls center)
        80,   // e-file (central bonus - controls center)
        100,  // f-file (standard)
        105,  // g-file
        120   // h-file (edge pawn penalty)
    };
    
    // Calculate isolated pawn penalties (using cached values computed earlier)
    int isolatedPawnPenalty = 0;
    
    // Evaluate white isolated pawns (penalty)
    Bitboard whiteIsolani = whiteIsolated;
    while (whiteIsolani) {
        Square sq = popLsb(whiteIsolani);
        int rank = rankOf(sq);
        int file = fileOf(sq);
        int penalty = ISOLATED_PAWN_PENALTY[rank];
        
        // IP3a: Apply file adjustment
        penalty = (penalty * FILE_ADJUSTMENT[file]) / 100;
        
        isolatedPawnPenalty -= penalty;
    }
    
    // Evaluate black isolated pawns (bonus for white)
    Bitboard blackIsolani = blackIsolated;
    while (blackIsolani) {
        Square sq = popLsb(blackIsolani);
        int rank = 7 - rankOf(sq);  // Convert to black's perspective
        int file = fileOf(sq);
        int penalty = ISOLATED_PAWN_PENALTY[rank];
        
        // IP3a: Apply file adjustment
        penalty = (penalty * FILE_ADJUSTMENT[file]) / 100;
        
        isolatedPawnPenalty += penalty;
    }
    
    // Phase scaling for isolated pawns - less penalty in endgame
    switch (gamePhase) {
        case search::GamePhase::OPENING:
        case search::GamePhase::MIDDLEGAME:
            // Full penalty in opening and middlegame
            break;
        case search::GamePhase::ENDGAME:
            isolatedPawnPenalty = isolatedPawnPenalty / 2;  // 50% penalty in endgame
            break;
    }
    
    // Convert isolated pawn penalty to Score
    Score isolatedPawnScore(isolatedPawnPenalty);
    
    // Trace isolated pawn score
    if constexpr (Traced) {
        if (trace) trace->isolatedPawns = isolatedPawnScore;
    }
    
    // DP3: Doubled pawn penalties enabled
    // Reduced penalties after testing showed -15/-6 was too harsh
    static constexpr int DOUBLED_PAWN_PENALTY_MG = -8;   // Middlegame penalty (reduced from -15)
    static constexpr int DOUBLED_PAWN_PENALTY_EG = -3;   // Endgame penalty (reduced from -6)
    
    // Calculate doubled pawn penalties
    int doubledPawnPenalty = 0;
    
    // Count doubled pawns (now using cached values from above)
    int whiteDoubledCount = popCount(whiteDoubled);
    int blackDoubledCount = popCount(blackDoubled);
    
    // Apply penalties - positive values since we subtract black's penalty
    int penalty = (gamePhase == search::GamePhase::ENDGAME) ? 
                  std::abs(DOUBLED_PAWN_PENALTY_EG) : std::abs(DOUBLED_PAWN_PENALTY_MG);
    
    // White is penalized for its doubled pawns, black's doubled pawns help white
    doubledPawnPenalty = (blackDoubledCount - whiteDoubledCount) * penalty;
    
    // Convert doubled pawn penalty to Score
    Score doubledPawnScore(doubledPawnPenalty);
    
    // Trace doubled pawn score
    if constexpr (Traced) {
        if (trace) trace->doubledPawns = doubledPawnScore;
    }
    
    // PI2: Pawn islands evaluation - fewer islands is better
    // Conservative penalties to start (will tune in PI3)
    static constexpr int PAWN_ISLAND_PENALTY = 5;  // Per island beyond the first
    
    // Calculate penalty (having 1 island is ideal, penalize additional islands)
    int whiteIslandPenalty = (whiteIslands > 1) ? (whiteIslands - 1) * PAWN_ISLAND_PENALTY : 0;
    int blackIslandPenalty = (blackIslands > 1) ? (blackIslands - 1) * PAWN_ISLAND_PENALTY : 0;
    
    // From white's perspective: penalize white's islands, reward black's islands
    int pawnIslandValue = blackIslandPenalty - whiteIslandPenalty;
    Score pawnIslandScore(pawnIslandValue);
    
    // Trace pawn island score
    if constexpr (Traced) {
        if (trace) trace->pawnIslands = pawnIslandScore;
    }
    
    // BP3: Backward pawn evaluation enabled
    // Reduced penalty after initial test showed 18cp was too high
    static constexpr int BACKWARD_PAWN_PENALTY = 8;  // Centipawns penalty per backward pawn
    int whiteBackwardCount = popCount(whiteBackward);
    int blackBackwardCount = popCount(blackBackward);
    
    // From white's perspective: penalize white's backward pawns, reward black's backward pawns
    int backwardPawnValue = (blackBackwardCount - whiteBackwardCount) * BACKWARD_PAWN_PENALTY;
    Score backwardPawnScore(backwardPawnValue);
    
    // Trace backward pawn score
    if constexpr (Traced) {
        if (trace) trace->backwardPawns = backwardPawnScore;
    }
    
    // BPB2: Bishop pair bonus integration (Phase 2 - conservative values)
    // Detect if each side has the bishop pair
    bool whiteBishopPair = (material.count(WHITE, BISHOP) >= 2);
    bool blackBishopPair = (material.count(BLACK, BISHOP) >= 2);
    
    // Phase 2: Apply conservative bishop pair bonus
    // Starting with 20cp midgame, 50cp endgame (will tune in Phase 3)
    static constexpr int BISHOP_PAIR_BONUS_MG = 20;
    static constexpr int BISHOP_PAIR_BONUS_EG = 50;
    
    int bishopPairValue = 0;
    
    // Apply phase-scaled bonus
    int bonus = 0;
    switch (gamePhase) {
        case search::GamePhase::OPENING:
        case search::GamePhase::MIDDLEGAME:
            bonus = BISHOP_PAIR_BONUS_MG;
            break;
        case search::GamePhase::ENDGAME:
            bonus = BISHOP_PAIR_BONUS_EG;
            break;
    }
    
    // Apply bonus for white, penalty for black (from white's perspective)
    if (whiteBishopPair) bishopPairValue += bonus;
    if (blackBishopPair) bishopPairValue -= bonus;
    
    Score bishopPairScore(bishopPairValue);
    
    // Trace bishop pair score
    if constexpr (Traced) {
        if (trace) trace->bishopPair = bishopPairScore;
    }
    
    // MOB2: Piece mobility evaluation (Phase 2 - basic integration)
    // Count pseudo-legal moves for pieces, excluding squares attacked by enemy pawns
    
    // Get all occupied squares
    Bitboard occupied = board.occupied();
    
    // Calculate squares attacked by pawns (for safer mobility calculation)
    Bitboard whitePawnAttacks = 0;
    Bitboard blackPawnAttacks = 0;
    
    // Calculate pawn attacks for each side
    Bitboard wp = whitePawns;
    while (wp) {
        Square sq = popLsb(wp);
        whitePawnAttacks |= pawnAttacks(WHITE, sq);
    }
    
    Bitboard bp = blackPawns;
    while (bp) {
        Square sq = popLsb(bp);
        blackPawnAttacks |= pawnAttacks(BLACK, sq);
    }
    
    // Calculate pawn attack spans - all squares pawns could ever attack
    // For white pawns: fill forward and expand diagonally
    // For black pawns: fill backward and expand diagonally
    auto calculatePawnAttackSpan = [](Bitboard pawns, Color color) -> Bitboard {
        Bitboard span = 0;
        
        if (color == WHITE) {
            // Fill all squares forward of white pawns
            Bitboard filled = pawns;
            for (int i = 0; i < 6; i++) {  // Max 6 ranks to advance
                filled |= (filled << 8) & ~RANK_8_BB;  // Move forward one rank
            }
            // Expand to diagonals (squares that could be attacked)
            span = ((filled & ~FILE_A_BB) << 7) | ((filled & ~FILE_H_BB) << 9);
            // Remove rank 1 and 2 (pawns can't attack backwards)
            span &= ~(RANK_1_BB | RANK_2_BB);
        } else {
            // Fill all squares backward of black pawns  
            Bitboard filled = pawns;
            for (int i = 0; i < 6; i++) {  // Max 6 ranks to advance
                filled |= (filled >> 8) & ~RANK_1_BB;  // Move backward one rank
            }
            // Expand to diagonals (squares that could be attacked)
            span = ((filled & ~FILE_H_BB) >> 7) | ((filled & ~FILE_A_BB) >> 9);
            // Remove rank 7 and 8 (pawns can't attack backwards)
            span &= ~(RANK_7_BB | RANK_8_BB);
        }
        
        return span;
    };
    
    Bitboard whitePawnAttackSpan = calculatePawnAttackSpan(whitePawns, WHITE);
    Bitboard blackPawnAttackSpan = calculatePawnAttackSpan(blackPawns, BLACK);
    
    // Get knight bitboards for outpost evaluation
    Bitboard whiteKnights = board.pieces(WHITE, KNIGHT);
    Bitboard blackKnights = board.pieces(BLACK, KNIGHT);
    
    // Knight outpost evaluation (simplified to match Stash engine)
    // An outpost is a square where a knight:
    // 1. Cannot be attacked by enemy pawns (ever - using attack spans)
    // 2. Is protected by friendly pawns
    // 3. Is in enemy territory (ranks 4-6 for white, ranks 3-5 for black)
    
    // Define outpost ranks for each side
    constexpr Bitboard WHITE_OUTPOST_RANKS = RANK_4_BB | RANK_5_BB | RANK_6_BB;
    constexpr Bitboard BLACK_OUTPOST_RANKS = RANK_3_BB | RANK_4_BB | RANK_5_BB;
    
    // Simple outpost bonus (similar to Stash's 31cp, we use 35cp)
    constexpr int KNIGHT_OUTPOST_BONUS = 35;
    
    // Find potential outpost squares (safe from enemy pawn attack spans and protected by friendly pawns)
    // Now using attack spans for proper detection of squares that can never be attacked by pawns
    Bitboard whiteOutpostSquares = WHITE_OUTPOST_RANKS & ~blackPawnAttackSpan & whitePawnAttacks;
    Bitboard blackOutpostSquares = BLACK_OUTPOST_RANKS & ~whitePawnAttackSpan & blackPawnAttacks;
    
    // Find knights on outpost squares
    Bitboard whiteKnightOutposts = whiteKnights & whiteOutpostSquares;
    Bitboard blackKnightOutposts = blackKnights & blackOutpostSquares;
    
    // Count outposts and apply bonus
    int knightOutpostValue = 0;
    knightOutpostValue += popCount(whiteKnightOutposts) * KNIGHT_OUTPOST_BONUS;
    knightOutpostValue -= popCount(blackKnightOutposts) * KNIGHT_OUTPOST_BONUS;
    
    // Create score from total outpost value
    Score knightOutpostScore(knightOutpostValue);
    
    // Trace knight outpost score
    if constexpr (Traced) {
        if (trace) trace->knightOutposts = knightOutpostScore;
    }
    
    // Phase 2: Conservative mobility bonuses per move count
    // Phase 2.5.e-3: Using SIMD-optimized batched mobility calculation
    static constexpr int MOBILITY_BONUS_PER_MOVE = 2;  // 2 centipawns per available move
    
    int whiteMobilityScore = 0;
    int blackMobilityScore = 0;
    
    // Count white piece mobility with batched processing for better ILP
    // Knights - process in batches for better cache usage
    Bitboard wn = whiteKnights;
    {
        // Extract up to 4 knight positions for parallel processing
        Square knightSqs[4];
        int knightCount = 0;
        Bitboard tempKnights = wn;
        while (tempKnights && knightCount < 4) {
            knightSqs[knightCount++] = popLsb(tempKnights);
        }
        
        // Process all knights in parallel (compiler may vectorize)
        for (int i = 0; i < knightCount; ++i) {
            Bitboard attacks = MoveGenerator::getKnightAttacks(knightSqs[i]);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            int moveCount = popCount(attacks);
            whiteMobilityScore += moveCount * MOBILITY_BONUS_PER_MOVE;
            if constexpr (Traced) {
                if (trace) trace->mobilityDetail.whiteKnightMoves += moveCount;
            }
        }
        
        // Process remaining knights if any
        while (tempKnights) {
            Square sq = popLsb(tempKnights);
            Bitboard attacks = MoveGenerator::getKnightAttacks(sq);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
    }
    
    // Bishops - batch process for better ILP
    Bitboard whiteBishops = board.pieces(WHITE, BISHOP);
    Bitboard wb = whiteBishops;
    {
        // Process bishops in groups of 3 for register pressure balance
        Square bishopSqs[3];
        int bishopCount = 0;
        Bitboard tempBishops = wb;
        
        while (tempBishops && bishopCount < 3) {
            bishopSqs[bishopCount++] = popLsb(tempBishops);
        }
        
        // Calculate all bishop attacks in parallel
        for (int i = 0; i < bishopCount; ++i) {
            Bitboard attacks = MoveGenerator::getBishopAttacks(bishopSqs[i], occupied);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
        
        // Process remaining bishops
        while (tempBishops) {
            Square sq = popLsb(tempBishops);
            Bitboard attacks = MoveGenerator::getBishopAttacks(sq, occupied);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
    }
    
    // Rooks - batch process with file bonus tracking
    Bitboard whiteRooks = board.pieces(WHITE, ROOK);
    Bitboard wr = whiteRooks;
    // Phase ROF2: Track rook open file bonus separately
    int whiteRookFileBonus = 0;
    {
        // Batch process rooks for better ILP
        Square rookSqs[3];
        int rookCount = 0;
        Bitboard tempRooks = wr;
        
        while (tempRooks && rookCount < 3) {
            rookSqs[rookCount++] = popLsb(tempRooks);
        }
        
        // Process rooks in parallel
        for (int i = 0; i < rookCount; ++i) {
            Bitboard attacks = MoveGenerator::getRookAttacks(rookSqs[i], occupied);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
            
            // Phase ROF2: Add open/semi-open file bonuses
            int file = fileOf(rookSqs[i]);
            if (board.isOpenFile(file)) {
                whiteRookFileBonus += 25;
            } else if (board.isSemiOpenFile(file, WHITE)) {
                whiteRookFileBonus += 15;
            }
        }
        
        // Process remaining rooks
        while (tempRooks) {
            Square sq = popLsb(tempRooks);
            Bitboard attacks = MoveGenerator::getRookAttacks(sq, occupied);
            attacks &= ~board.pieces(WHITE);
            attacks &= ~blackPawnAttacks;
            whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
            
            int file = fileOf(sq);
            if (board.isOpenFile(file)) {
                whiteRookFileBonus += 25;
            } else if (board.isSemiOpenFile(file, WHITE)) {
                whiteRookFileBonus += 15;
            }
        }
    }
    
    // Queens - batch process (usually fewer queens)
    Bitboard whiteQueens = board.pieces(WHITE, QUEEN);
    Bitboard wq = whiteQueens;
    while (wq) {
        Square sq = popLsb(wq);
        // Queens use combined bishop and rook attacks
        Bitboard attacks = MoveGenerator::getQueenAttacks(sq, occupied);
        attacks &= ~board.pieces(WHITE);
        attacks &= ~blackPawnAttacks;
        whiteMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
    }
    
    // Count black piece mobility with batched processing
    // Knights - batch process
    Bitboard bn = blackKnights;
    {
        Square knightSqs[4];
        int knightCount = 0;
        Bitboard tempKnights = bn;
        
        while (tempKnights && knightCount < 4) {
            knightSqs[knightCount++] = popLsb(tempKnights);
        }
        
        for (int i = 0; i < knightCount; ++i) {
            Bitboard attacks = MoveGenerator::getKnightAttacks(knightSqs[i]);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
        
        while (tempKnights) {
            Square sq = popLsb(tempKnights);
            Bitboard attacks = MoveGenerator::getKnightAttacks(sq);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
    }
    
    // Bishops - batch process
    Bitboard blackBishops = board.pieces(BLACK, BISHOP);
    Bitboard bb = blackBishops;
    {
        Square bishopSqs[3];
        int bishopCount = 0;
        Bitboard tempBishops = bb;
        
        while (tempBishops && bishopCount < 3) {
            bishopSqs[bishopCount++] = popLsb(tempBishops);
        }
        
        for (int i = 0; i < bishopCount; ++i) {
            Bitboard attacks = MoveGenerator::getBishopAttacks(bishopSqs[i], occupied);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
        
        while (tempBishops) {
            Square sq = popLsb(tempBishops);
            Bitboard attacks = MoveGenerator::getBishopAttacks(sq, occupied);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
        }
    }
    
    // Rooks - batch process
    Bitboard blackRooks = board.pieces(BLACK, ROOK);
    Bitboard br = blackRooks;
    int blackRookFileBonus = 0;
    {
        Square rookSqs[3];
        int rookCount = 0;
        Bitboard tempRooks = br;
        
        while (tempRooks && rookCount < 3) {
            rookSqs[rookCount++] = popLsb(tempRooks);
        }
        
        for (int i = 0; i < rookCount; ++i) {
            Bitboard attacks = MoveGenerator::getRookAttacks(rookSqs[i], occupied);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
            
            int file = fileOf(rookSqs[i]);
            if (board.isOpenFile(file)) {
                blackRookFileBonus += 25;
            } else if (board.isSemiOpenFile(file, BLACK)) {
                blackRookFileBonus += 15;
            }
        }
        
        while (tempRooks) {
            Square sq = popLsb(tempRooks);
            Bitboard attacks = MoveGenerator::getRookAttacks(sq, occupied);
            attacks &= ~board.pieces(BLACK);
            attacks &= ~whitePawnAttacks;
            blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
            
            int file = fileOf(sq);
            if (board.isOpenFile(file)) {
                blackRookFileBonus += 25;
            } else if (board.isSemiOpenFile(file, BLACK)) {
                blackRookFileBonus += 15;
            }
        }
    }
    
    // Queens - usually fewer, process directly
    Bitboard blackQueens = board.pieces(BLACK, QUEEN);
    Bitboard bq = blackQueens;
    while (bq) {
        Square sq = popLsb(bq);
        Bitboard attacks = MoveGenerator::getQueenAttacks(sq, occupied);
        attacks &= ~board.pieces(BLACK);
        attacks &= ~whitePawnAttacks;
        blackMobilityScore += popCount(attacks) * MOBILITY_BONUS_PER_MOVE;
    }
    
    // Phase 2: Apply mobility difference to evaluation
    int mobilityValue = whiteMobilityScore - blackMobilityScore;
    Score mobilityScore(mobilityValue);
    
    // Trace mobility score
    if constexpr (Traced) {
        if (trace) trace->mobility = mobilityScore;
    }
    
    // Phase KS2: King safety evaluation (integrated but returns 0 score)
    // Evaluate for both sides and combine
    Score whiteKingSafety = KingSafety::evaluate(board, WHITE);
    Score blackKingSafety = KingSafety::evaluate(board, BLACK);
    
    // King safety is from each side's perspective, so we subtract black's from white's
    // Note: In Phase KS2, both will return 0 since enableScoring = 0
    Score kingSafetyScore = whiteKingSafety - blackKingSafety;

    int kingAttackScale = seajay::getConfig().kingAttackScale;
    if (kingAttackScale != 0) {
        int scaledValue = kingSafetyScore.value() * (100 + kingAttackScale) / 100;
        kingSafetyScore = Score(scaledValue);
    }
    
    // Trace king safety score
    if constexpr (Traced) {
        if (trace) trace->kingSafety = kingSafetyScore;
    }
    
    // Phase ROF2: Calculate rook file bonus score
    Score rookFileScore = Score(whiteRookFileBonus - blackRookFileBonus);
    
    // Trace rook file score
    if constexpr (Traced) {
        if (trace) trace->rookFiles = rookFileScore;
    }

    // Phase A3: Tiny bonus for king proximity to own rook in endgames only
    Score rookKingProximityScore = Score(0);
    if (gamePhase == search::GamePhase::ENDGAME) {
        auto kingDist = [](Square k, Square r) {
            int dr = std::abs(rankOf(k) - rankOf(r));
            int df = std::abs(fileOf(k) - fileOf(r));
            return dr + df;  // Manhattan distance
        };
        // White
        int wBonus = 0;
        Bitboard wrBB = board.pieces(WHITE, ROOK);
        if (wrBB) {
            int best = 99;
            Bitboard t = wrBB;
            while (t) {
                Square rsq = popLsb(t);
                best = std::min(best, kingDist(whiteKingSquare, rsq));
            }
            // Tiny bonus: closer king-rook gets up to ~6 cp
            wBonus = std::max(0, 6 - best);
        }
        // Black
        int bBonus = 0;
        Bitboard brBB = board.pieces(BLACK, ROOK);
        if (brBB) {
            int best = 99;
            Bitboard t = brBB;
            while (t) {
                Square rsq = popLsb(t);
                best = std::min(best, kingDist(blackKingSquare, rsq));
            }
            bBonus = std::max(0, 6 - best);
        }
        rookKingProximityScore = Score(wBonus - bBonus);
    }
    
    // Trace rook-king proximity score
    if constexpr (Traced) {
        if (trace) trace->rookKingProximity = rookKingProximityScore;
    }
    
    // Calculate total evaluation from white's perspective
    // Material difference + PST score + passed pawn score + isolated pawn score + doubled pawn score + island score + backward score + bishop pair + mobility + king safety + rook files + knight outposts
    
    // Phase-interpolated material evaluation
    Score materialDiff;
    if (seajay::getConfig().usePSTInterpolation) {
        // Use phase interpolation for material values (same phase as PST)
        int phase = phase0to256(board);  // 256 = full MG, 0 = full EG
        int invPhase = 256 - phase;      // Inverse phase for endgame weight
        
        // Get MG and EG material differences
        Score materialMg = material.valueMg(WHITE) - material.valueMg(BLACK);
        Score materialEg = material.valueEg(WHITE) - material.valueEg(BLACK);

        if constexpr (Traced) {
            if (trace) {
                trace->materialMg = materialMg;
                trace->materialEg = materialEg;
            }
        }
        
        // Interpolate between mg and eg values
        int interpolated = (materialMg.value() * phase + materialEg.value() * invPhase) / 256;
        materialDiff = Score(interpolated);
    } else {
        // Use pure middlegame values (backward compatibility)
        materialDiff = material.value(WHITE) - material.value(BLACK);

        if constexpr (Traced) {
            if (trace) {
                trace->materialMg = materialDiff;
                trace->materialEg = materialDiff;
            }
        }
    }
    
    // Trace material
    if constexpr (Traced) {
        if (trace) trace->material = materialDiff;
    }
    
    Score totalWhite = materialDiff + pstValue + passedPawnScore + isolatedPawnScore + doubledPawnScore + pawnIslandScore + backwardPawnScore + bishopPairScore + mobilityScore + kingSafetyScore + rookFileScore + rookKingProximityScore + knightOutpostScore;
    
    // Return from side-to-move perspective
    if (sideToMove == WHITE) {
        return totalWhite;
    } else {
        return -totalWhite;
    }
}

// Public interfaces for evaluation
Score evaluate(const Board& board) {
    // Normal evaluation with no tracing - zero overhead
    return evaluateImpl<false>(board, nullptr);
}

Score evaluateWithTrace(const Board& board, EvalTrace& trace) {
    // Evaluation with detailed tracing
    return evaluateImpl<true>(board, &trace);
}

#ifdef DEBUG
bool verifyMaterialIncremental(const Board& board) {
    // Recount material from scratch
    Material scratch;
    
    for (int i = 0; i < 64; ++i) {
        Square s = static_cast<Square>(i);
        Piece p = board.pieceAt(s);
        if (p != NO_PIECE) {
            scratch.add(p);
        }
    }
    
    // Compare with board's incremental material
    return scratch == board.material();
}
#endif

} // namespace seajay::eval
