#include "quiescence.h"
#include "../core/board.h"
#include "../evaluation/evaluate.h"
#include "../core/transposition_table.h"
#include <algorithm>

namespace seajay::search {

eval::Score quiescence(
    Board& board,
    int ply,
    eval::Score alpha,
    eval::Score beta,
    seajay::SearchInfo& searchInfo,
    SearchData& data,
    seajay::TranspositionTable& tt)
{
    // Track nodes
    data.qsearchNodes++;
    
    // Update selective depth
    if (ply > data.seldepth) {
        data.seldepth = ply;
    }
    
    // Safety check: prevent stack overflow
    if (ply >= TOTAL_MAX_PLY) {
        return eval::evaluate(board);
    }
    
    // Stand-pat evaluation: static evaluation of the position
    eval::Score staticEval = eval::evaluate(board);
    
    // Beta cutoff on stand-pat
    if (staticEval >= beta) {
        data.standPatCutoffs++;
        return staticEval;
    }
    
    // Update alpha with stand-pat score
    // In quiet positions, we can choose to not make any move
    alpha = std::max(alpha, staticEval);
    
    // For now, just return the stand-pat score
    // (Will add capture search in next deliverables)
    return alpha;
}

} // namespace seajay::search