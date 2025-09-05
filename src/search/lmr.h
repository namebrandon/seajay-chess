#pragma once

#include "../core/types.h"
#include "types.h"
#include "killer_moves.h"
#include "history_heuristic.h"
#include "countermoves.h"
#include <cmath>

namespace seajay::search {

/**
 * Stage 18: Late Move Reductions (LMR)
 * 
 * Reduces search depth for moves late in move ordering based on the
 * observation that moves late in the ordering are unlikely to be best.
 * If a reduced move beats alpha, we re-search at full depth.
 */

/**
 * Initialize the logarithmic reduction table with parameters
 * @param baseReduction Base reduction value (100 = 1.0 in formula)
 * @param depthFactor Divisor for log formula (225 = 2.25 in formula)
 */
void initLMRTableWithParams(int baseReduction, int depthFactor);

/**
 * Initialize the logarithmic reduction table with default values
 * Should be called once at engine startup
 */
void initLMRTable();

/**
 * Calculate the depth reduction for a given move
 * 
 * @param depth Current search depth
 * @param moveNumber Move number in the ordered list (1-based)
 * @param params LMR parameters from UCI options
 * @param isPvNode Whether this is a principal variation node
 * @param improving Whether the position is improving
 * @return Depth reduction in plies (0 if no reduction should be applied)
 */
int getLMRReduction(int depth, int moveNumber, const SearchData::LMRParams& params,
                   bool isPvNode = false, bool improving = true);

/**
 * Check if a move is eligible for reduction
 * 
 * @param move The move to check
 * @param depth Current search depth
 * @param moveNumber Move number in the ordered list
 * @param isCapture Whether the move is a capture
 * @param inCheck Whether the side to move is in check
 * @param givesCheck Whether the move gives check
 * @param isPVNode Whether this is a PV node
 * @param killers Killer moves table
 * @param history History heuristic table
 * @param counterMoves Countermoves table
 * @param prevMove Previous move played
 * @param ply Current ply
 * @param sideToMove Side to move
 * @param params LMR parameters
 * @return true if the move should be reduced
 */
bool shouldReduceMove(Move move, int depth, int moveNumber, bool isCapture, 
                     bool inCheck, bool givesCheck, bool isPVNode,
                     const KillerMoves& killers, const HistoryHeuristic& history,
                     const CounterMoves& counterMoves, Move prevMove,
                     int ply, Color sideToMove,
                     const SearchData::LMRParams& params);

} // namespace seajay::search