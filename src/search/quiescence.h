#pragma once

#include "../core/types.h"
#include "../core/board.h"
#include "../evaluation/types.h"
#include "types.h"
#include "search_info.h"
#include <cstdint>  // For UINT64_MAX
#include <atomic>   // For SEE pruning statistics
#include <string>

namespace seajay::search {

// Safety constants for quiescence search
static constexpr int QSEARCH_MAX_PLY = 32;          // Maximum quiescence ply depth
static constexpr int TOTAL_MAX_PLY = 128;           // Combined main + quiescence depth
static constexpr int MAX_CHECK_PLY = 6;             // Balanced check extensions (was 8, now 6 for better time management)

// Delta pruning margins - Standard chess engine practice
// These are positional margins only - piece values are added separately in the pruning formula
static constexpr int DELTA_MARGIN = 200;            // Standard positional margin (most engines use 175-225)
static constexpr int DELTA_MARGIN_ENDGAME = 100;    // Tighter margin in endgame (fewer pieces = more accurate eval)
static constexpr int DELTA_MARGIN_PANIC = 50;       // Very aggressive when far behind (we're already losing)

// Stage 14 Remediation: Node limits now controlled via UCI at runtime
// Default is unlimited (0) for production use
// Testing can set to 10000, tuning to 100000 via UCI option

static constexpr int MAX_CAPTURES_PER_NODE = 32;    // Maximum captures to search per node
static constexpr int MAX_CAPTURES_PANIC = 8;        // Reduced captures in panic mode

// Stage 15 Day 8: SEE-based pruning thresholds (TUNED)
static constexpr int SEE_PRUNE_THRESHOLD_CONSERVATIVE = -100;  // Conservative: only clearly bad captures
static constexpr int SEE_PRUNE_THRESHOLD_AGGRESSIVE = -75;     // Aggressive: balanced pruning (tuned)
static constexpr int SEE_PRUNE_THRESHOLD_ENDGAME = -25;        // Even more aggressive in endgame

// SEE pruning modes are now defined in types.h to avoid circular dependency

// Parse string to SEE pruning mode
SEEPruningMode parseSEEPruningMode(const std::string& mode);

// Convert SEE pruning mode to string
std::string seePruningModeToString(SEEPruningMode mode);

// Static assertions for safety verification
static_assert(QSEARCH_MAX_PLY > 0 && QSEARCH_MAX_PLY <= 64, 
              "Quiescence max ply must be reasonable");
static_assert(TOTAL_MAX_PLY >= QSEARCH_MAX_PLY, 
              "Total max ply must include quiescence depth");
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
 * @param limits Search limits including quiescence node limit
 * @param tt Transposition table
 * @param checkPly Number of consecutive check plies (default 0)
 * @param inPanicMode True if time pressure requires aggressive pruning
 * @param staticEval Cached static evaluation (pass minus_infinity() to force re-evaluation)
 * @return Evaluation score of the position
 */
eval::Score quiescence(
    Board& board,
    int ply,
    eval::Score alpha,
    eval::Score beta,
    seajay::SearchInfo& searchInfo,
    SearchData& data,
    const SearchLimits& limits,
    seajay::TranspositionTable& tt,
    int checkPly = 0,
    bool inPanicMode = false,
    eval::Score cachedStaticEval = eval::Score::minus_infinity()
);

} // namespace seajay::search