#include "lmr.h"
#include <algorithm>

namespace seajay::search {

int getLMRReduction(int depth, int moveNumber, const SearchData::LMRParams& params) {
    // Early exit for disabled LMR or invalid inputs
    if (!params.enabled || depth < params.minDepth || moveNumber < 1) {
        return 0;
    }
    
    // Don't reduce early moves in the move ordering
    if (moveNumber < params.minMoveNumber) {
        return 0;
    }
    
    // Basic linear formula: baseReduction + (depth - minDepth) / depthFactor
    // Integer division provides natural rounding down
    int reduction = params.baseReduction;
    
    // Add depth-based component
    // Using depthFactor=100 means every 100 ply of remaining depth adds 1 to reduction
    // This is very conservative (100 is high), which is appropriate for Phase 2
    if (params.depthFactor > 0) {
        reduction += (depth - params.minDepth) / params.depthFactor;
    }
    
    // Additional reduction for very late moves (moveNumber > 8)
    // This reflects that moves very late in ordering are extremely unlikely to be best
    if (moveNumber > 8) {
        // Add (moveNumber - 8) / 4, so moves 9-12 get +0, 13-16 get +1, etc.
        // Integer division naturally handles this
        reduction += (moveNumber - 8) / 4;
    }
    
    // Cap reduction to leave at least 1 ply of search
    // depth - reduction >= 1, so reduction <= depth - 1
    // But we also want to leave at least 2 plies for meaningful search
    // So cap at depth - 2
    reduction = std::min(reduction, std::max(1, depth - 2));
    
    // Ensure non-negative (defensive programming)
    return std::max(0, reduction);
}

bool shouldReduceMove(int depth, int moveNumber, bool isCapture, 
                      bool inCheck, bool givesCheck, bool isPVNode,
                      const SearchData::LMRParams& params) {
    // Don't reduce if LMR is disabled
    if (!params.enabled) {
        return false;
    }
    
    // Don't reduce at shallow depths
    if (depth < params.minDepth) {
        return false;
    }
    
    // Don't reduce early moves
    if (moveNumber < params.minMoveNumber) {
        return false;
    }
    
    // Phase 2: Conservative approach - don't reduce in these cases:
    
    // Don't reduce when in check (need to escape)
    if (inCheck) {
        return false;
    }
    
    // Don't reduce moves that give check (often forcing)
    if (givesCheck) {
        return false;
    }
    
    // Don't reduce captures (tactical moves)
    // In Phase 3, we might reduce bad captures, but not in Phase 2
    if (isCapture) {
        return false;
    }
    
    // Don't reduce in PV nodes (Phase 2 conservative approach)
    // Phase 3 might allow small reductions in PV nodes
    if (isPVNode) {
        return false;
    }
    
    // All conditions passed - this quiet, late move can be reduced
    return true;
}

} // namespace seajay::search