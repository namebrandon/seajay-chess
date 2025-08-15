#pragma once

#include "../core/types.h"
#include "../core/board.h"
#include "../evaluation/types.h"
#include "types.h"
#include "search_info.h"
#include <cstdint>  // For UINT64_MAX

namespace seajay::search {

// Safety constants for quiescence search
static constexpr int QSEARCH_MAX_PLY = 32;          // Maximum quiescence ply depth
static constexpr int TOTAL_MAX_PLY = 128;           // Combined main + quiescence depth
static constexpr int MAX_CHECK_PLY = 8;             // Maximum check extensions in quiescence

// Delta pruning constants
static constexpr int DELTA_MARGIN = 900;            // Conservative margin (queen value)
static constexpr int DELTA_MARGIN_ENDGAME = 600;    // Reduced margin for endgames

// Progressive limiter removal system
// This ensures we remember to remove limiters when transitioning phases
#ifdef QSEARCH_TESTING
    // Phase 1: Conservative testing with strict limits
    static constexpr uint64_t NODE_LIMIT_PER_POSITION = 10000;
    #pragma message("QSEARCH_TESTING mode: Node limit = 10,000 per position")
#elif defined(QSEARCH_TUNING)
    // Phase 2: Tuning with higher limits
    static constexpr uint64_t NODE_LIMIT_PER_POSITION = 100000;
    #pragma message("QSEARCH_TUNING mode: Node limit = 100,000 per position")
#else
    // Production: Unlimited nodes - tactical excellence requires freedom
    // Reverting to Candidate 1 - the "safety" limits destroyed performance
    static constexpr uint64_t NODE_LIMIT_PER_POSITION = UINT64_MAX;
    // No pragma message in production - silent operation
#endif

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
 * @param checkPly Number of consecutive check plies (default 0)
 * @return Evaluation score of the position
 */
eval::Score quiescence(
    Board& board,
    int ply,
    eval::Score alpha,
    eval::Score beta,
    seajay::SearchInfo& searchInfo,
    SearchData& data,
    seajay::TranspositionTable& tt,
    int checkPly = 0
);

} // namespace seajay::search