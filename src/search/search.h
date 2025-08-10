#pragma once

#include "../core/types.h"
#include "../evaluation/types.h"
#include "types.h"
#include "negamax.h"

namespace seajay {

class Board;

namespace search {

// Legacy interface - now uses negamax search internally
Move selectBestMove(Board& board);

// Random move selection for testing
Move selectRandomMove(Board& board);

} // namespace search
} // namespace seajay