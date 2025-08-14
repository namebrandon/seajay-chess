#pragma once

#include "types.h"
#include "search_info.h"
#include "../core/types.h"
#include "../core/transposition_table.h"
#include "../evaluation/types.h"

namespace seajay {
class Board;
class TranspositionTable;
}

namespace seajay::search {

// Core negamax search function
// Returns the best score for the current position
// Parameters:
//   board - The current board position
//   depth - Remaining search depth (in plies)
//   ply   - Current distance from root (for mate score adjustment)
//   alpha - Lower bound of the search window
//   beta  - Upper bound of the search window
//   searchInfo - Search stack for repetition detection
//   info  - Search statistics and control information
//   tt    - Transposition table (can be nullptr)
eval::Score negamax(Board& board, 
                   int depth, 
                   int ply,
                   eval::Score alpha,
                   eval::Score beta,
                   SearchInfo& searchInfo,
                   SearchData& info,
                   TranspositionTable* tt = nullptr);

// Iterative deepening search controller
// Returns the best move found within the given limits
// Parameters:
//   board  - The current board position
//   limits - Time and depth constraints for the search
//   tt     - Transposition table (can be nullptr)
Move search(Board& board, const SearchLimits& limits, TranspositionTable* tt = nullptr);

// Stage 13: Test wrapper using IterativeSearchData (Deliverable 1.2a)
// Identical behavior to search() but uses new data structure
Move searchIterativeTest(Board& board, const SearchLimits& limits, TranspositionTable* tt = nullptr);

// Calculate time allocation for a move
// Returns the time to allocate for this search
// Parameters:
//   limits - Search constraints from UCI
//   board  - Current position (for move number, etc.)
std::chrono::milliseconds calculateTimeLimit(const SearchLimits& limits, 
                                            const Board& board);

// Send UCI info output during search
// Parameters:
//   info - Current search statistics
void sendSearchInfo(const SearchData& info);

} // namespace seajay::search