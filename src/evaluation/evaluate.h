#pragma once

#include "types.h"

namespace seajay {
class Board;
}

namespace seajay::eval {

Score evaluate(const Board& board);


#ifdef DEBUG
bool verifyMaterialIncremental(const Board& board);
#endif

} // namespace seajay::eval