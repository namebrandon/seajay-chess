#include "quiescence.h"
#include "../core/board.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
#include "../core/transposition_table.h"
#ifdef ENABLE_MVV_LVA
#include "move_ordering.h"  // For MVV-LVA ordering
#endif
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
    
    // Stage 14, Deliverable 1.9: Generate and search captures
    // Generate all captures
    MoveList captures;
    MoveGenerator::generateCaptures(board, captures);
    
    // Order captures using MVV-LVA for better pruning
#ifdef ENABLE_MVV_LVA
    MvvLvaOrdering mvvLva;
    mvvLva.orderMoves(board, captures);
#else
    // Simple ordering: promotions first, then by captured piece value
    std::sort(captures.begin(), captures.end(), [&board](Move a, Move b) {
        // Promotions first
        if (isPromotion(a) && !isPromotion(b)) return true;
        if (!isPromotion(a) && isPromotion(b)) return false;
        
        // Then by captured piece value (approximation without full board access)
        // This is a simplified ordering when MVV-LVA is not available
        return false;  // Keep original order
    });
#endif
    
    // Search captures
    eval::Score bestScore = alpha;
    int captureCount = 0;
    
    for (const Move& capture : captures) {
        // Limit captures per node to prevent explosion
        if (++captureCount > MAX_CAPTURES_PER_NODE) {
            break;
        }
        
        // Push position to search stack
        searchInfo.pushSearchPosition(board.zobristKey(), capture, ply);
        
        // Make the capture
        Board::UndoInfo undo;
        board.makeMove(capture, undo);
        
        // Recursive quiescence search
        eval::Score score = -quiescence(board, ply + 1, -beta, -alpha, 
                                       searchInfo, data, tt);
        
        // Unmake the capture
        board.unmakeMove(capture, undo);
        
        // Check if search was stopped
        if (data.stopped) {
            return bestScore;
        }
        
        // Update best score
        if (score > bestScore) {
            bestScore = score;
            
            // Update alpha
            if (score > alpha) {
                alpha = score;
                
                // Beta cutoff - prune remaining captures
                if (score >= beta) {
                    data.qsearchCutoffs++;
                    return score;
                }
            }
        }
    }
    
    return bestScore;
}

} // namespace seajay::search