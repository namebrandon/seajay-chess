#include "lmr.h"
#include <algorithm>
#include <cmath>

namespace seajay::search {

// Logarithmic reduction table based on depth and move number
// Inspired by Stockfish and other strong engines
static int reductionTable[64][64];
static bool tableInitialized = false;
static int currentBaseReduction = -1;
static int currentDepthFactor = -1;

void initLMRTableWithParams(int baseReduction, int depthFactor) {
    // Update tracked parameters
    currentBaseReduction = baseReduction;
    currentDepthFactor = depthFactor;
    
    // Convert parameters to decimal values
    // baseReduction: 100 = 1.0, 50 = 0.5, etc.
    // depthFactor: 225 = 2.25, 300 = 3.0, etc.
    double base = baseReduction / 100.0;
    double divisor = depthFactor / 100.0;
    
    // Initialize with logarithmic formula using parameters
    for (int depth = 1; depth < 64; depth++) {
        for (int moves = 1; moves < 64; moves++) {
            // Parameterized logarithmic formula
            double reduction = base + std::log(depth) * std::log(moves) / divisor;
            
            // Store as integer, capped at reasonable values
            reductionTable[depth][moves] = std::min(depth - 1, static_cast<int>(reduction));
            
            // Ensure non-negative
            reductionTable[depth][moves] = std::max(0, reductionTable[depth][moves]);
        }
    }
    
    // First few moves get no reduction regardless of depth
    for (int depth = 0; depth < 64; depth++) {
        reductionTable[depth][0] = 0;
        reductionTable[depth][1] = 0;
        reductionTable[depth][2] = 0;
    }
}

void initLMRTable() {
    // Only initialize once
    if (tableInitialized) {
        return;
    }
    
    // Initialize with default values: base=0.5 (50), divisor=2.25 (225)
    initLMRTableWithParams(50, 225);
    
    tableInitialized = true;
}

int getLMRReduction(int depth, int moveNumber, const SearchData::LMRParams& params,
                   bool isPvNode, bool improving) {
    // Reinitialize table ONLY if parameters have actually changed
    // This prevents constant reinitialization during search
    if (!tableInitialized || 
        (params.baseReduction != currentBaseReduction) || 
        (params.depthFactor != currentDepthFactor)) {
        initLMRTableWithParams(params.baseReduction, params.depthFactor);
        tableInitialized = true;
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
        reduction = std::max(1, reduction - params.pvReduction);
    }
    
    // Adjust based on improving (reduce more when not improving)
    if (!improving && reduction < depth - 1) {
        reduction += params.nonImprovingBonus;
    }
    
    // Note: baseReduction and depthFactor are now applied in the table initialization
    // No need for additional scaling here
    
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
    // Consider top 25% of history moves as "good" and don't reduce them
    if (move != NO_MOVE && !isCapture && !isPromotion(move)) {
        Square from = moveFrom(move);
        Square to = moveTo(move);
        int historyScore = history.getScore(sideToMove, from, to);
        
        // Don't reduce if history score is above threshold (UCI configurable)
        int threshold = (HistoryHeuristic::HISTORY_MAX * params.historyThreshold) / 100;
        if (historyScore > threshold) {
            return false;
        }
    }
    
    // All checks passed - this move can be reduced
    return true;
}

} // namespace seajay::search