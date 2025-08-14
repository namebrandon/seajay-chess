#pragma once

#include "../core/types.h"
#include "../core/board.h"
#include "../evaluation/types.h"
#include "types.h"
#include "search_info.h"

namespace seajay::search {

// Safety constants for quiescence search
static constexpr int QSEARCH_MAX_PLY = 32;          // Maximum quiescence ply depth
static constexpr int TOTAL_MAX_PLY = 128;           // Combined main + quiescence depth
static constexpr uint64_t NODE_LIMIT_PER_POSITION = 10000;  // Per-position node limit
static constexpr int MAX_CAPTURES_PER_NODE = 32;    // Maximum captures to search per node

// Static assertions for safety verification
static_assert(QSEARCH_MAX_PLY > 0 && QSEARCH_MAX_PLY <= 64, 
              "Quiescence max ply must be reasonable");
static_assert(TOTAL_MAX_PLY >= QSEARCH_MAX_PLY, 
              "Total max ply must include quiescence depth");
static_assert(NODE_LIMIT_PER_POSITION > 0, 
              "Node limit must be positive");
static_assert(MAX_CAPTURES_PER_NODE > 0 && MAX_CAPTURES_PER_NODE <= 256,
              "Capture limit must be reasonable");

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