#pragma once

#include "types.h"
#include "../core/types.h"
#include "../evaluation/types.h"

namespace seajay {
class Board;
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
//   info  - Search statistics and control information
eval::Score negamax(Board& board, 
                   int depth, 
                   int ply,
                   eval::Score alpha,
                   eval::Score beta,
                   SearchInfo& info);

// Iterative deepening search controller
// Returns the best move found within the given limits
// Parameters:
//   board  - The current board position
//   limits - Time and depth constraints for the search
Move search(Board& board, const SearchLimits& limits);

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
void sendSearchInfo(const SearchInfo& info);

} // namespace seajay::search