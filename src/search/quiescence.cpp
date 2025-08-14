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
    
    // Check time limit periodically (every 1024 nodes)
    if ((data.qsearchNodes & 1023) == 0) {
        if (data.stopped || data.checkTime()) {
            data.stopped = true;
            return eval::Score::zero();  // Return neutral score when stopped
        }
    }
    
    // Update selective depth
    if (ply > data.seldepth) {
        data.seldepth = ply;
    }
    
    // Safety check: prevent stack overflow
    if (ply >= TOTAL_MAX_PLY) {
        return eval::evaluate(board);
    }
    
    // Critical: Check for repetition to prevent infinite loops
    // This MUST come before any evaluation or move generation
    if (searchInfo.isRepetitionInSearch(board.zobristKey(), ply)) {
        return eval::Score::zero();  // Draw score
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