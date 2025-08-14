#pragma once

#include "../core/types.h"
#include "../core/board.h"
#include "../evaluation/types.h"
#include "types.h"
#include "search_info.h"

namespace seajay::search {

// Forward declarations
class TranspositionTable;
struct SearchInfo;

/**
 * @brief Quiescence search to resolve tactical sequences
 * 
 * Searches only captures and check evasions to avoid horizon effect.
 * Returns when position becomes "quiet" (no more forcing moves).
 * 
 * @param board Current board position
 * @param ply Current search ply from root
 * @param alpha Lower bound of search window
 * @param beta Upper bound of search window
 * @param searchInfo Global search information
 * @param data Search statistics
 * @param tt Transposition table
 * @return Evaluation score of the position
 */
eval::Score quiescence(
    Board& board,
    int ply,
    eval::Score alpha,
    eval::Score beta,
    SearchInfo& searchInfo,
    SearchData& data,
    TranspositionTable& tt
);

} // namespace seajay::search