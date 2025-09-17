#include "fast_evaluate.h"
#include "evaluate.h"
#include "pst.h"
#include "pawn_eval.h"
#include "../core/engine_config.h"
#include "../search/game_phase.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <vector>

namespace seajay {

namespace eval {

#ifndef NDEBUG
thread_local FastEvalStats g_fastEvalStats;

namespace {

std::mutex g_fastEvalRegistryMutex;
std::vector<FastEvalStats*> g_fastEvalRegisteredStats;
thread_local bool g_fastEvalRegistered = false;

void ensureFastEvalThreadRegistered() {
    if (!g_fastEvalRegistered) {
        std::lock_guard<std::mutex> lock(g_fastEvalRegistryMutex);
        g_fastEvalRegisteredStats.push_back(&g_fastEvalStats);
        g_fastEvalRegistered = true;
    }
}

} // namespace

FastEvalStats snapshotFastEvalStats() {
    FastEvalStats aggregate;
    std::lock_guard<std::mutex> lock(g_fastEvalRegistryMutex);
    for (FastEvalStats* stats : g_fastEvalRegisteredStats) {
        if (!stats) {
            continue;
        }
        aggregate.fastEvalCalls += stats->fastEvalCalls;
        aggregate.fastEvalUsedInStandPat += stats->fastEvalUsedInStandPat;
        aggregate.fastEvalUsedInPruning += stats->fastEvalUsedInPruning;
        aggregate.pawnCacheShadowStores += stats->pawnCacheShadowStores;
        aggregate.pawnCacheShadowComputes += stats->pawnCacheShadowComputes;
        aggregate.pawnCacheHits += stats->pawnCacheHits;
        aggregate.pawnCacheMisses += stats->pawnCacheMisses;
        aggregate.pawnCacheParitySamples += stats->pawnCacheParitySamples;
        aggregate.pawnCacheParityNonZero += stats->pawnCacheParityNonZero;
        aggregate.pawnCacheParityMaxAbs = std::max(aggregate.pawnCacheParityMaxAbs,
                                                   stats->pawnCacheParityMaxAbs);
        aggregate.fastFutilityDepth1Used += stats->fastFutilityDepth1Used;

        aggregate.parityHist.totalSamples += stats->parityHist.totalSamples;
        aggregate.parityHist.nonZeroDiffCount += stats->parityHist.nonZeroDiffCount;
        aggregate.parityHist.maxAbsDiff = std::max(aggregate.parityHist.maxAbsDiff,
                                                   stats->parityHist.maxAbsDiff);
        for (int i = 0; i < ParityHistogram::NUM_BUCKETS; ++i) {
            aggregate.parityHist.buckets[i] += stats->parityHist.buckets[i];
            aggregate.pawnCacheParityHist.buckets[i] += stats->pawnCacheParityHist.buckets[i];
        }
        aggregate.pawnCacheParityHist.totalSamples += stats->pawnCacheParityHist.totalSamples;
        aggregate.pawnCacheParityHist.nonZeroDiffCount += stats->pawnCacheParityHist.nonZeroDiffCount;
        aggregate.pawnCacheParityHist.maxAbsDiff = std::max(aggregate.pawnCacheParityHist.maxAbsDiff,
                                                            stats->pawnCacheParityHist.maxAbsDiff);

        for (int i = 1; i < 9; ++i) {
            aggregate.pruningAudit.staticNullAttempts[i] += stats->pruningAudit.staticNullAttempts[i];
            aggregate.pruningAudit.staticNullWouldFlip[i] += stats->pruningAudit.staticNullWouldFlip[i];
        }
        for (int i = 1; i < 3; ++i) {
            aggregate.pruningAudit.razorAttempts[i] += stats->pruningAudit.razorAttempts[i];
            aggregate.pruningAudit.razorWouldFlip[i] += stats->pruningAudit.razorWouldFlip[i];
        }
        for (int i = 1; i < 7; ++i) {
            aggregate.pruningAudit.futilityAttempts[i] += stats->pruningAudit.futilityAttempts[i];
            aggregate.pruningAudit.futilityWouldFlip[i] += stats->pruningAudit.futilityWouldFlip[i];
        }
        for (int i = 1; i < 13; ++i) {
            aggregate.pruningAudit.nullMoveStaticAttempts[i] += stats->pruningAudit.nullMoveStaticAttempts[i];
            aggregate.pruningAudit.nullMoveStaticWouldFlip[i] += stats->pruningAudit.nullMoveStaticWouldFlip[i];
        }
    }
    return aggregate;
}

void resetFastEvalStats() {
    std::lock_guard<std::mutex> lock(g_fastEvalRegistryMutex);
    for (FastEvalStats* stats : g_fastEvalRegisteredStats) {
        if (stats) {
            stats->reset();
        }
    }
}
#endif

// Phase calculation (identical to evaluate.cpp for parity)
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

namespace {

struct PawnCacheContext {
    uint8_t sideToMove = 0;
    uint8_t gamePhase = 0;
    uint8_t isPureEndgame = 0;
    uint8_t whiteKingSquare = 0;
    uint8_t blackKingSquare = 0;
    uint8_t whiteBlockerCount = 0;
    std::array<uint8_t, 8> whiteBlockerSquares{};
    std::array<uint8_t, 8> whiteBlockerPieces{};
    uint8_t blackBlockerCount = 0;
    std::array<uint8_t, 8> blackBlockerSquares{};
    std::array<uint8_t, 8> blackBlockerPieces{};
};

struct alignas(64) PawnCacheEntry {
    uint64_t key = 0;
    Score score = Score::zero();
    bool valid = false;
    PawnCacheContext context{};
};

bool contextsEqual(const PawnCacheContext& cached, const PawnCacheContext& current) {
    if (cached.sideToMove != current.sideToMove ||
        cached.gamePhase != current.gamePhase ||
        cached.isPureEndgame != current.isPureEndgame ||
        cached.whiteKingSquare != current.whiteKingSquare ||
        cached.blackKingSquare != current.blackKingSquare ||
        cached.whiteBlockerCount != current.whiteBlockerCount ||
        cached.blackBlockerCount != current.blackBlockerCount) {
        return false;
    }

    for (uint8_t i = 0; i < cached.whiteBlockerCount; ++i) {
        if (cached.whiteBlockerSquares[i] != current.whiteBlockerSquares[i] ||
            cached.whiteBlockerPieces[i] != current.whiteBlockerPieces[i]) {
            return false;
        }
    }

    for (uint8_t i = 0; i < cached.blackBlockerCount; ++i) {
        if (cached.blackBlockerSquares[i] != current.blackBlockerSquares[i] ||
            cached.blackBlockerPieces[i] != current.blackBlockerPieces[i]) {
            return false;
        }
    }

    return true;
}

class FastEvalPawnCache {
public:
    static constexpr size_t SIZE = 512;  // 8KB table, per-thread

    void clear() {
        for (auto& entry : m_entries) {
            entry = PawnCacheEntry{};
        }
    }

    bool probe(uint64_t key, const PawnCacheContext& context, Score& outScore) const {
        const auto& entry = m_entries[index(key)];
        if (!entry.valid || entry.key != key) {
            return false;
        }
        if (!contextsEqual(entry.context, context)) {
            return false;
        }
        outScore = entry.score;
        return true;
    }

    void store(uint64_t key, Score score, const PawnCacheContext& context) {
        auto& entry = m_entries[index(key)];
        entry.key = key;
        entry.score = score;
        entry.valid = true;
        entry.context = context;
    }

private:
    static constexpr size_t index(uint64_t key) noexcept {
        return static_cast<size_t>(key & (SIZE - 1));
    }

    std::array<PawnCacheEntry, SIZE> m_entries{};
};

thread_local FastEvalPawnCache g_fastEvalPawnCache;

struct BlockerInfo {
    uint8_t count = 0;
    std::array<uint8_t, 8> squares{};
    std::array<uint8_t, 8> pieces{};
};

bool isPureEndgame(const Board& board) {
    return board.pieces(WHITE, KNIGHT) == 0 &&
           board.pieces(WHITE, BISHOP) == 0 &&
           board.pieces(WHITE, ROOK) == 0 &&
           board.pieces(WHITE, QUEEN) == 0 &&
           board.pieces(BLACK, KNIGHT) == 0 &&
           board.pieces(BLACK, BISHOP) == 0 &&
           board.pieces(BLACK, ROOK) == 0 &&
           board.pieces(BLACK, QUEEN) == 0;
}

BlockerInfo gatherBlockerInfo(const Board& board, const PawnEntry& entry, Color color) {
    BlockerInfo info;
    Bitboard passers = entry.passedPawns[color];

    while (passers) {
        Square sq = popLsb(passers);
        Square blockSq = (color == WHITE) ? Square(static_cast<int>(sq) + 8)
                                          : Square(static_cast<int>(sq) - 8);

        if (blockSq < SQ_A1 || blockSq > SQ_H8) {
            continue;
        }

        if (info.count < 8) {
            info.squares[info.count] = static_cast<uint8_t>(blockSq);
            Piece blocker = board.pieceAt(blockSq);
            info.pieces[info.count] = static_cast<uint8_t>(blocker);
            ++info.count;
        }
    }

    return info;
}

PawnCacheContext buildPawnCacheContext(const Board& board, const PawnEntry& entry) {
    PawnCacheContext context;
    context.sideToMove = static_cast<uint8_t>(board.sideToMove());
    context.gamePhase = static_cast<uint8_t>(search::detectGamePhase(board));
    context.isPureEndgame = isPureEndgame(board) ? 1 : 0;
    context.whiteKingSquare = static_cast<uint8_t>(board.kingSquare(WHITE));
    context.blackKingSquare = static_cast<uint8_t>(board.kingSquare(BLACK));

    BlockerInfo whiteBlockers = gatherBlockerInfo(board, entry, WHITE);
    BlockerInfo blackBlockers = gatherBlockerInfo(board, entry, BLACK);

    context.whiteBlockerCount = whiteBlockers.count;
    context.whiteBlockerSquares = whiteBlockers.squares;
    context.whiteBlockerPieces = whiteBlockers.pieces;
    context.blackBlockerCount = blackBlockers.count;
    context.blackBlockerSquares = blackBlockers.squares;
    context.blackBlockerPieces = blackBlockers.pieces;

    return context;
}

Score computePawnScore(const Board& board,
                       const PawnEntry& pawnEntry,
                       const PawnCacheContext& context) {
    const Material& material = board.material();
    const search::GamePhase gamePhase = static_cast<search::GamePhase>(context.gamePhase);
    const bool pureEndgame = context.isPureEndgame != 0;
    const Color sideToMove = static_cast<Color>(context.sideToMove);
    const Square whiteKingSq = static_cast<Square>(context.whiteKingSquare);
    const Square blackKingSq = static_cast<Square>(context.blackKingSquare);
    const Bitboard whitePawns = board.pieces(WHITE, PAWN);
    const Bitboard blackPawns = board.pieces(BLACK, PAWN);

    PawnEvalSummary pawnSummary = computePawnEval(
        board,
        pawnEntry,
        material,
        gamePhase,
        pureEndgame,
        sideToMove,
        whiteKingSq,
        blackKingSq,
        whitePawns,
        blackPawns);

    return pawnSummary.total;
}

} // namespace

Score fastEvaluate(const Board& board) {
#ifndef NDEBUG
    ensureFastEvalThreadRegistered();
    g_fastEvalStats.fastEvalCalls++;
#endif
    
    // Phase 3C.1: Material + PST evaluation with phase interpolation
    // Uses board's incremental values for O(1) computation
    
    // Check for insufficient material draws
    if (board.isInsufficientMaterial()) {
        return Score::draw();
    }
    
    const Material& material = board.material();
    const MgEgScore& pstScore = board.pstScore();
    const Color stm = board.sideToMove();
    
    Score totalScore;
    
    if (seajay::getConfig().usePSTInterpolation) {
        // Phase interpolation (identical math to evaluate.cpp)
        int phase = phase0to256(board);  // 256 = full MG, 0 = full EG
        int invPhase = 256 - phase;      // Inverse phase for endgame weight
        
        // Material with phase interpolation (no rounding, matches evaluate.cpp)
        Score whiteMat = Score((material.valueMg(WHITE).value() * phase + 
                                material.valueEg(WHITE).value() * invPhase) / 256);
        Score blackMat = Score((material.valueMg(BLACK).value() * phase + 
                                material.valueEg(BLACK).value() * invPhase) / 256);
        
        // PST with phase interpolation (with rounding, matches evaluate.cpp)
        int blendedPst = (pstScore.mg.value() * phase + pstScore.eg.value() * invPhase + 128) >> 8;
        Score pstValue = Score(blendedPst);
        
        // Combine material and PST from side-to-move perspective
        // PST is stored from white's perspective, material needs balance calculation
        Score materialBalance = stm == WHITE ? whiteMat - blackMat : blackMat - whiteMat;
        Score pstFromStm = stm == WHITE ? pstValue : -pstValue;
        
        totalScore = materialBalance + pstFromStm;
    } else {
        // No interpolation - use only middlegame values
        Score materialBalance = material.balanceMg(stm);
        Score pstFromStm = stm == WHITE ? pstScore.mg : -pstScore.mg;
        
        totalScore = materialBalance + pstFromStm;
    }
    
    // Phase 3D.3: Use pawn cache when fast-eval toggles are enabled
    const bool usePawnTerm = seajay::getConfig().useFastEvalForQsearch ||
                             seajay::getConfig().useFastEvalForPruning;

    if (usePawnTerm) {
        const uint64_t pawnKey = board.pawnZobristKey();
        PawnEntry scratchEntry;
        const PawnEntry& pawnEntry = getOrBuildPawnEntry(board, scratchEntry);
        PawnCacheContext context = buildPawnCacheContext(board, pawnEntry);
        Score pawnScore = Score::zero();
        const bool cacheHit = g_fastEvalPawnCache.probe(pawnKey, context, pawnScore);

#ifndef NDEBUG
        if (cacheHit) {
            g_fastEvalStats.pawnCacheHits++;
        } else {
            g_fastEvalStats.pawnCacheMisses++;
        }
#endif

        if (cacheHit) {
#ifndef NDEBUG
            g_fastEvalStats.pawnCacheParitySamples++;

            // Sample 1/64 of hits to verify cache correctness without large overhead
            if ((g_fastEvalStats.pawnCacheParitySamples & 63ULL) == 0) {
                Score freshSample = computePawnScore(board, pawnEntry, context);
                g_fastEvalStats.pawnCacheShadowComputes++;

                const int32_t diff = freshSample.value() - pawnScore.value();
                if (diff != 0) {
                    g_fastEvalStats.pawnCacheParityNonZero++;
                    g_fastEvalStats.pawnCacheParityMaxAbs = std::max(
                        g_fastEvalStats.pawnCacheParityMaxAbs,
                        std::abs(diff));
                }

                g_fastEvalStats.pawnCacheParityHist.record(diff);
                if (diff != 0) {
                    g_fastEvalStats.pawnCacheShadowStores++;
                    g_fastEvalPawnCache.store(pawnKey, freshSample, context);
                    pawnScore = freshSample;
                }
            }
#endif
        } else {
            pawnScore = computePawnScore(board, pawnEntry, context);
            g_fastEvalPawnCache.store(pawnKey, pawnScore, context);

#ifndef NDEBUG
            g_fastEvalStats.pawnCacheShadowComputes++;
            g_fastEvalStats.pawnCacheShadowStores++;
#endif
        }

        totalScore += pawnScore;
    }

    return totalScore;
}

Score fastEvaluateMatPST(const Board& board) {
    // Phase 3C.0: Fast material + PST evaluation
    // Uses board's incremental values for O(1) computation
    
    // Check for insufficient material draws
    if (board.isInsufficientMaterial()) {
        return Score::draw();
    }
    
    const Material& material = board.material();
    const MgEgScore& pstScore = board.pstScore();
    const Color stm = board.sideToMove();
    
    Score totalScore;
    
    if (seajay::getConfig().usePSTInterpolation) {
        // Phase interpolation (identical math to evaluate.cpp)
        int phase = phase0to256(board);  // 256 = full MG, 0 = full EG
        int invPhase = 256 - phase;      // Inverse phase for endgame weight
        
        // Material with phase interpolation (no rounding, matches evaluate.cpp)
        Score whiteMat = Score((material.valueMg(WHITE).value() * phase + 
                                material.valueEg(WHITE).value() * invPhase) / 256);
        Score blackMat = Score((material.valueMg(BLACK).value() * phase + 
                                material.valueEg(BLACK).value() * invPhase) / 256);
        
        // PST with phase interpolation (identical to evaluate.cpp)
        int blendedPst = (pstScore.mg.value() * phase + pstScore.eg.value() * invPhase + 128) >> 8;
        Score pstValue = Score(blendedPst);
        
        // Combine material and PST
        // PST is stored from white's perspective, material needs balance calculation
        Score materialBalance = stm == WHITE ? whiteMat - blackMat : blackMat - whiteMat;
        Score pstFromStm = stm == WHITE ? pstValue : -pstValue;
        
        totalScore = materialBalance + pstFromStm;
    } else {
        // No interpolation - use only middlegame values
        Score materialBalance = material.balanceMg(stm);
        Score pstFromStm = stm == WHITE ? pstScore.mg : -pstScore.mg;
        
        totalScore = materialBalance + pstFromStm;
    }
    
    return totalScore;
}

#ifndef NDEBUG
Score fastEvaluatePawnOnly(const Board& board) {
    PawnEntry scratchEntry;
    const PawnEntry& pawnEntry = getOrBuildPawnEntry(board, scratchEntry);
    PawnCacheContext context = buildPawnCacheContext(board, pawnEntry);
    return computePawnScore(board, pawnEntry, context);
}
#endif

} // namespace eval

} // namespace seajay
