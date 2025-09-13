#pragma once

#include "types.h"

namespace seajay {
class Board;
}

namespace seajay::eval {

// Forward declaration
struct EvalTrace;

// Normal evaluation - zero overhead
Score evaluate(const Board& board);

// Evaluation with detailed tracing
Score evaluateWithTrace(const Board& board, EvalTrace& trace);


#ifdef DEBUG
bool verifyMaterialIncremental(const Board& board);
#endif

#ifndef NDEBUG
// Phase 3C.0: Reference function for material + PST evaluation parity checking
Score refMaterialPST(const Board& board);
#endif

} // namespace seajay::eval