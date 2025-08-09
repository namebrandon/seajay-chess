#pragma once

#include "../core/types.h"
#include "../evaluation/types.h"

namespace seajay {

class Board;

namespace search {

Move selectBestMove(Board& board);

Move selectRandomMove(Board& board);

} // namespace search
} // namespace seajay