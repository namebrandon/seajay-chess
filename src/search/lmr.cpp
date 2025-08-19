#include "lmr.h"
#include <algorithm>

namespace seajay::search {

int getLMRReduction(int depth, int moveNumber, const SearchData::LMRParams& params) {
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
    
    // Basic linear formula
    int reduction = params.baseReduction;
    
    // Add depth-based component
    reduction += (depth - params.minDepth) / params.depthFactor;
    
    // Additional reduction for very late moves (moves 9+)
    if (moveNumber > 8) {
        reduction += (moveNumber - 8) / 4;
    }
    
    // Cap reduction to leave at least 1 ply of search
    // Use max(1, depth-2) to ensure we always search at least depth 1
    reduction = std::min(reduction, std::max(1, depth - 2));
    
    // Ensure reduction is non-negative
    return std::max(0, reduction);
}

bool shouldReduceMove(int depth, int moveNumber, bool isCapture, 
                     bool inCheck, bool givesCheck, bool isPVNode,
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
    
    // Don't reduce moves that give check (Phase 2: simplified, may refine later)
    if (givesCheck) {
        return false;
    }
    
    // For Phase 2, we won't reduce PV nodes differently
    // This will be refined in Phase 5
    
    return true;
}

} // namespace seajay::search