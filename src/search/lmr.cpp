#include "lmr.h"
#include <algorithm>
#include <cmath>

namespace seajay::search {

// Logarithmic reduction table based on depth and move number
// Inspired by Stockfish and other strong engines
static int reductionTable[64][64];
static bool tableInitialized = false;

void initLMRTable() {
    // Only initialize once
    if (tableInitialized) {
        return;
    }
    
    // Initialize with logarithmic formula
    // This gives a smooth increase in reductions based on both depth and move number
    for (int depth = 1; depth < 64; depth++) {
        for (int moves = 1; moves < 64; moves++) {
            // Base logarithmic formula
            double reduction = 0.5 + std::log(depth) * std::log(moves) / 2.25;
            
            // Store as integer, capped at reasonable values
            reductionTable[depth][moves] = std::min(depth - 1, static_cast<int>(reduction));
            
            // Ensure non-negative
            if (reductionTable[depth][moves] < 0) {
                reductionTable[depth][moves] = 0;
            }
        }
    }
    
    // First few moves get no reduction regardless of depth
    for (int depth = 0; depth < 64; depth++) {
        reductionTable[depth][0] = 0;
        reductionTable[depth][1] = 0;
        reductionTable[depth][2] = 0;
    }
    
    tableInitialized = true;
}

int getLMRReduction(int depth, int moveNumber, const SearchData::LMRParams& params,
                   bool isPvNode, bool improving) {
    // Ensure table is initialized
    if (!tableInitialized) {
        initLMRTable();
    }
    
    // No reduction if LMR is disabled
    if (!params.enabled) {
        return 0;
    }
    
    // No reduction if depth is too shallow
    if (depth < params.minDepth) {
        return 0;
    }
    
    // No reduction for early moves in move ordering
    if (moveNumber < params.minMoveNumber) {
        return 0;
    }
    
    // Defensive programming: handle invalid inputs
    if (depth <= 1 || moveNumber <= 0) {
        return 0;
    }
    
    // Clamp to table bounds
    int d = std::min(63, depth);
    int m = std::min(63, moveNumber);
    
    // Get base reduction from logarithmic table
    int reduction = reductionTable[d][m];
    
    // Adjust based on PV node (reduce less in PV)
    if (isPvNode && reduction > 0) {
        reduction = std::max(1, reduction - 1);
    }
    
    // Adjust based on improving (reduce more when not improving)
    if (!improving && reduction < depth - 1) {
        reduction++;
    }
    
    // Apply user-configured base reduction adjustment
    if (params.baseReduction != 1) {
        reduction = reduction * params.baseReduction;
    }
    
    // Cap reduction to leave at least 1 ply of search
    reduction = std::min(reduction, std::max(1, depth - 2));
    
    // Ensure reduction is non-negative
    return std::max(0, reduction);
}

bool shouldReduceMove(Move move, int depth, int moveNumber, bool isCapture, 
                     bool inCheck, bool givesCheck, bool isPVNode,
                     const KillerMoves& killers, const HistoryHeuristic& history,
                     const CounterMoves& counterMoves, Move prevMove,
                     int ply, Color sideToMove,
                     const SearchData::LMRParams& params) {
    // No reduction if LMR is disabled
    if (!params.enabled) {
        return false;
    }
    
    // No reduction if depth is too shallow
    if (depth < params.minDepth) {
        return false;
    }
    
    // No reduction for early moves
    if (moveNumber < params.minMoveNumber) {
        return false;
    }
    
    // Don't reduce captures
    if (isCapture) {
        return false;
    }
    
    // Don't reduce when in check
    if (inCheck) {
        return false;
    }
    
    // Don't reduce moves that give check
    if (givesCheck) {
        return false;
    }
    
    // Don't reduce killer moves (they're historically good)
    if (killers.isKiller(ply, move)) {
        return false;
    }
    
    // Don't reduce countermoves (they're often good responses)
    if (prevMove != NO_MOVE && move == counterMoves.getCounterMove(prevMove)) {
        return false;
    }
    
    // Don't reduce moves with very high history scores
    // Consider top 15% of history moves as "good" and don't reduce them
    if (move != NO_MOVE && !isCapture && !isPromotion(move)) {
        Square from = moveFrom(move);
        Square to = moveTo(move);
        int historyScore = history.getScore(sideToMove, from, to);
        
        // Don't reduce if history score is in top tier (>75% of max)
        if (historyScore > HistoryHeuristic::HISTORY_MAX * 3 / 4) {
            return false;
        }
    }
    
    // All checks passed - this move can be reduced
    return true;
}

} // namespace seajay::search