#include "fast_evaluate.h"
#include "evaluate.h"
#include "pst.h"
#include "pawn_eval.h"
#include "../core/engine_config.h"
#include "../search/game_phase.h"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace seajay {

namespace eval {

#ifndef NDEBUG
thread_local FastEvalStats g_fastEvalStats;
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

struct alignas(64) PawnCacheEntry {
    uint64_t key = 0;
    Score score = Score::zero();
    bool valid = false;
};

class FastEvalPawnCache {
public:
    static constexpr size_t SIZE = 256;  // 4KB table, per-thread

    void clear() {
        for (auto& entry : m_entries) {
            entry.key = 0;
            entry.score = Score::zero();
            entry.valid = false;
        }
    }

    bool probe(uint64_t key, Score& outScore) const {
        const auto& entry = m_entries[index(key)];
        if (entry.valid && entry.key == key) {
            outScore = entry.score;
            return true;
        }
        return false;
    }

    void store(uint64_t key, Score score) {
        auto& entry = m_entries[index(key)];
        entry.key = key;
        entry.score = score;
        entry.valid = true;
    }

private:
    static constexpr size_t index(uint64_t key) noexcept {
        return static_cast<size_t>(key & (SIZE - 1));
    }

    std::array<PawnCacheEntry, SIZE> m_entries{};
};

thread_local FastEvalPawnCache g_fastEvalPawnCache;

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

Score computePawnScoreFresh(const Board& board) {
    PawnEntry scratchEntry;
    const PawnEntry& pawnEntry = getOrBuildPawnEntry(board, scratchEntry);
    const Material& material = board.material();
    search::GamePhase gamePhase = search::detectGamePhase(board);
    const bool pureEndgame = isPureEndgame(board);

    return computePawnEval(
               board,
               pawnEntry,
               material,
               gamePhase,
               pureEndgame,
               board.sideToMove(),
               board.kingSquare(WHITE),
               board.kingSquare(BLACK),
               board.pieces(WHITE, PAWN),
               board.pieces(BLACK, PAWN))
        .total;
}

} // namespace

Score fastEvaluate(const Board& board) {
#ifndef NDEBUG
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
    
    // Phase 3D.1: Shadow-fill pawn cache (store fresh computations, ignore cached value)
    const uint64_t pawnKey = board.pawnZobristKey();
    Score cachedPawnScore;
    [[maybe_unused]] bool cacheHit = g_fastEvalPawnCache.probe(pawnKey, cachedPawnScore);

    Score pawnScore = computePawnScoreFresh(board);

#ifndef NDEBUG
    g_fastEvalStats.pawnCacheShadowComputes++;
    if (!cacheHit) {
        g_fastEvalStats.pawnCacheShadowStores++;
    } else {
        g_fastEvalStats.pawnCacheParitySamples++;

        const int32_t diff = pawnScore.value() - cachedPawnScore.value();
        if (diff != 0) {
            g_fastEvalStats.pawnCacheParityNonZero++;
            g_fastEvalStats.pawnCacheParityMaxAbs = std::max(
                g_fastEvalStats.pawnCacheParityMaxAbs,
                std::abs(diff));
        }

        // Sample 1/64 of hits to build a histogram without high overhead
        if ((g_fastEvalStats.pawnCacheParitySamples & 63ULL) == 0) {
            g_fastEvalStats.pawnCacheParityHist.record(diff);
        }
    }
#endif

    g_fastEvalPawnCache.store(pawnKey, pawnScore);

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

} // namespace eval

} // namespace seajay
