#include "fast_evaluate.h"
#include "evaluate.h"
#include "pst.h"
#include "../core/engine_config.h"
#include "../search/game_phase.h"
#include <algorithm>

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

Score fastEvaluate(const Board& board) {
#ifndef NDEBUG
    g_fastEvalStats.fastEvalCalls++;
#endif
    
    // Phase 3B: Material-only evaluation
    // Optional early-out for insufficient material (cheap check)
    if (board.isInsufficientMaterial()) {
        return Score::draw();
    }
    
    // Return material balance from side-to-move perspective
    return board.material().balance(board.sideToMove());
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
        
        // Material with phase interpolation
        Score whiteMat = Score((material.valueMg(WHITE).value() * phase + 
                                material.valueEg(WHITE).value() * invPhase + 128) >> 8);
        Score blackMat = Score((material.valueMg(BLACK).value() * phase + 
                                material.valueEg(BLACK).value() * invPhase + 128) >> 8);
        
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