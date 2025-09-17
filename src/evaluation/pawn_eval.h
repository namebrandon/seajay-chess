#pragma once

#include "pawn_structure.h"
#include "material.h"
#include "types.h"
#include "../core/board.h"
#include "../core/bitboard.h"
#include "../search/game_phase.h"

#include <cstdint>

namespace seajay::eval {

struct PawnEvalSummary {
    Score total{Score::zero()};
    Score passed{Score::zero()};
    Score isolated{Score::zero()};
    Score doubled{Score::zero()};
    Score islands{Score::zero()};
    Score backward{Score::zero()};
    int whitePassedCount{0};
    int blackPassedCount{0};
};

// Build all cached pawn-structure bitboards for the given position.
void buildPawnEntry(const Board& board, PawnEntry& entry);

// Fetch an existing pawn entry or build a fresh one (storing it in the global cache).
const PawnEntry& getOrBuildPawnEntry(const Board& board, PawnEntry& scratch);

// Compute the pawn-structure score contributions using cached features.
PawnEvalSummary computePawnEval(const Board& board,
                                const PawnEntry& entry,
                                const Material& material,
                                search::GamePhase gamePhase,
                                bool isPureEndgame,
                                Color sideToMove,
                                Square whiteKingSquare,
                                Square blackKingSquare,
                                Bitboard whitePawns,
                                Bitboard blackPawns);

} // namespace seajay::eval

