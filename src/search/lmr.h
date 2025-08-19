#pragma once

#include "../core/types.h"
#include "types.h"

namespace seajay::search {

/**
 * Stage 18: Late Move Reductions (LMR)
 * 
 * Reduces search depth for moves late in move ordering based on the
 * observation that moves late in the ordering are unlikely to be best.
 * If a reduced move beats alpha, we re-search at full depth.
 */

/**
 * Calculate the depth reduction for a given move
 * 
 * @param depth Current search depth
 * @param moveNumber Move number in the ordered list (1-based)
 * @param params LMR parameters from UCI options
 * @return Depth reduction in plies (0 if no reduction should be applied)
 */
int getLMRReduction(int depth, int moveNumber, const SearchData::LMRParams& params);

/**
 * Check if a move is eligible for reduction
 * 
 * @param depth Current search depth
 * @param moveNumber Move number in the ordered list
 * @param isCapture Whether the move is a capture
 * @param inCheck Whether the side to move is in check
 * @param givesCheck Whether the move gives check
 * @param isPVNode Whether this is a PV node
 * @param params LMR parameters
 * @return true if the move should be reduced
 */
bool shouldReduceMove(int depth, int moveNumber, bool isCapture, 
                     bool inCheck, bool givesCheck, bool isPVNode,
                     const SearchData::LMRParams& params);

} // namespace seajay::search