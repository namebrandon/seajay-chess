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
    
    // Stage 14, Deliverable 1.10: Handle check evasion
    // Check if we're in check - requires different handling
    bool isInCheck = inCheck(board);
    
    // Stand-pat evaluation (skip if in check - must make a move)
    eval::Score staticEval;
    if (!isInCheck) {
        staticEval = eval::evaluate(board);
        
        // Beta cutoff on stand-pat
        if (staticEval >= beta) {
            data.standPatCutoffs++;
            return staticEval;
        }
        
        // Update alpha with stand-pat score
        // In quiet positions, we can choose to not make any move
        alpha = std::max(alpha, staticEval);
    } else {
        // In check: no stand-pat, must make a move
        staticEval = eval::Score::minus_infinity();
    }
    
    // Generate moves based on check status
    MoveList moves;
    if (isInCheck) {
        // In check: must generate ALL legal moves (not just captures)
        moves = generateLegalMoves(board);
        
        // Check for checkmate/stalemate
        if (moves.empty()) {
            // Checkmate (we're in check with no legal moves)
            return eval::Score(-32000 + ply);
        }
    } else {
        // Not in check: only search captures
        MoveGenerator::generateCaptures(board, moves);
    }
    
    // Order moves for better pruning
#ifdef ENABLE_MVV_LVA
    MvvLvaOrdering mvvLva;
    mvvLva.orderMoves(board, moves);
#else
    // Simple ordering: promotions first, then captures
    if (!isInCheck) {
        std::sort(moves.begin(), moves.end(), [&board](Move a, Move b) {
            // Promotions first
            if (isPromotion(a) && !isPromotion(b)) return true;
            if (!isPromotion(a) && isPromotion(b)) return false;
            
            // Then by captured piece value (approximation without full board access)
            // This is a simplified ordering when MVV-LVA is not available
            return false;  // Keep original order
        });
    }
    // When in check, keep move generation order (usually good enough)
#endif
    
    // Search moves
    eval::Score bestScore = isInCheck ? eval::Score::minus_infinity() : alpha;
    int moveCount = 0;
    
    for (const Move& move : moves) {
        // Limit moves per node to prevent explosion (except when in check)
        if (!isInCheck && ++moveCount > MAX_CAPTURES_PER_NODE) {
            break;
        }
        
        // Push position to search stack
        searchInfo.pushSearchPosition(board.zobristKey(), move, ply);
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        // Recursive quiescence search
        eval::Score score = -quiescence(board, ply + 1, -beta, -alpha, 
                                       searchInfo, data, tt);
        
        // Unmake the move
        board.unmakeMove(move, undo);
        
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