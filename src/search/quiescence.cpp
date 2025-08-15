#include "quiescence.h"
#include "../core/board.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
#include "../core/transposition_table.h"
#ifdef ENABLE_MVV_LVA
#include "move_ordering.h"  // For MVV-LVA ordering and VICTIM_VALUES
#endif
#include <algorithm>

namespace seajay::search {

// Mate score bound for adjustment
constexpr int MATE_BOUND = 29000;

eval::Score quiescence(
    Board& board,
    int ply,
    eval::Score alpha,
    eval::Score beta,
    seajay::SearchInfo& searchInfo,
    SearchData& data,
    seajay::TranspositionTable& tt)
{
    // Record entry node count for per-position limit enforcement
    const uint64_t entryNodes = data.qsearchNodes;
    
    // Track nodes
    data.qsearchNodes++;
    
    // Deliverable 2.1: TT Probing in Quiescence
    // Probe transposition table at the start of quiescence
    TTEntry* ttEntry = nullptr;
    eval::Score ttScore = eval::Score::zero();
    Move ttMove = NO_MOVE;  // Deliverable 2.3: Track TT move for ordering
    
    if (tt.isEnabled() && (ttEntry = tt.probe(board.zobristKey())) != nullptr) {
        // We have a TT hit
        if (!ttEntry->isEmpty() && ttEntry->key32 == (board.zobristKey() >> 32)) {
            // Depth 0 is used for quiescence entries
            // Accept any depth >= 0 (all quiescence entries have depth 0)
            if (ttEntry->depth >= 0) {
                ttScore = eval::Score(ttEntry->score);
                
                // Adjust mate scores relative to current ply
                if (ttScore.value() >= MATE_BOUND) {
                    // Positive mate score - adjust distance from current position
                    ttScore = eval::Score(ttScore.value() - ply);
                } else if (ttScore.value() <= -MATE_BOUND) {
                    // Negative mate score - adjust distance from current position
                    ttScore = eval::Score(ttScore.value() + ply);
                }
                
                // Check bound type and use TT score if applicable
                Bound bound = ttEntry->bound();
                if (bound == Bound::EXACT) {
                    // Exact score - we can return immediately
                    data.qsearchTTHits++;  // Track TT hits in quiescence
                    return ttScore;
                } else if (bound == Bound::LOWER && ttScore >= beta) {
                    // Lower bound (fail-high) - score is at least ttScore
                    data.qsearchTTHits++;  // Track TT hits in quiescence
                    return ttScore;
                } else if (bound == Bound::UPPER && ttScore <= alpha) {
                    // Upper bound (fail-low) - score is at most ttScore
                    data.qsearchTTHits++;  // Track TT hits in quiescence
                    return ttScore;
                }
                
                // Deliverable 2.3: Save TT move for ordering even if we don't return
                ttMove = static_cast<Move>(ttEntry->move);
            }
        }
    }
    
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
    
    // Deliverable 3.4: Detect endgame for delta pruning margin adjustment
    bool isEndgame = (board.material().value(WHITE).value() < 1300) && (board.material().value(BLACK).value() < 1300);
    int deltaMargin = isEndgame ? DELTA_MARGIN_ENDGAME : DELTA_MARGIN;
    
    // Safety check 1: prevent stack overflow
    if (ply >= TOTAL_MAX_PLY) {
        return eval::evaluate(board);
    }
    
    // Safety check 2: enforce per-position node limit
    // This prevents search explosion in complex tactical positions
    if (data.qsearchNodes - entryNodes > NODE_LIMIT_PER_POSITION) {
        data.qsearchNodesLimited++;  // Track when we hit limits (for debugging)
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
        
        // Deliverable 3.2 & 3.4: Basic delta pruning with endgame safety
        // If we're so far behind that even winning a queen won't help, prune
        eval::Score futilityBase = staticEval + eval::Score(deltaMargin);
        if (futilityBase < alpha) {
            data.deltasPruned++;
            return staticEval;  // Position is hopeless
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
    
    // Deliverable 2.3: TT Move Ordering
    // If we have a TT move, try it first (only if it's in the move list)
    if (ttMove != NO_MOVE) {
        // Find the TT move in the list and move it to the front
        auto ttMoveIt = std::find(moves.begin(), moves.end(), ttMove);
        if (ttMoveIt != moves.end()) {
            // Move TT move to the front while preserving other moves' order
            std::rotate(moves.begin(), ttMoveIt, ttMoveIt + 1);
        }
    }
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
    
    // Deliverable 2.3: TT Move Ordering (non-MVV_LVA case)
    if (ttMove != NO_MOVE) {
        auto ttMoveIt = std::find(moves.begin(), moves.end(), ttMove);
        if (ttMoveIt != moves.end()) {
            std::rotate(moves.begin(), ttMoveIt, ttMoveIt + 1);
        }
    }
#endif
    
    // Search moves
    eval::Score bestScore = isInCheck ? eval::Score::minus_infinity() : alpha;
    int moveCount = 0;
    
    for (const Move& move : moves) {
        // Limit moves per node to prevent explosion (except when in check)
        if (!isInCheck && ++moveCount > MAX_CAPTURES_PER_NODE) {
            break;
        }
        
        // Deliverable 3.3 & 3.4: Per-move delta pruning with endgame safety
        // Skip bad captures that can't improve alpha even if successful
        if (!isInCheck && !isPromotion(move)) {
#ifdef ENABLE_MVV_LVA
            // Use accurate victim value from MVV-LVA tables
            PieceType captured = board.pieceTypeAt(move.to());
            int captureValue = (captured != NO_PIECE_TYPE) ? VICTIM_VALUES[captured] : 0;
#else
            // Conservative estimate when MVV-LVA not available
            int captureValue = 100;  // Assume at least a pawn
#endif
            // Use endgame-appropriate margin (already calculated above)
            if (staticEval + eval::Score(captureValue) + eval::Score(deltaMargin) < alpha) {
                data.deltasPruned++;
                continue;  // Skip this capture
            }
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
                    
                    // Deliverable 2.2: Store in TT with LOWER bound (fail-high)
                    if (tt.isEnabled()) {
                        // Adjust mate scores for storage (relative to root)
                        eval::Score scoreToStore = score;
                        if (score.value() >= MATE_BOUND) {
                            scoreToStore = eval::Score(score.value() + ply);
                        } else if (score.value() <= -MATE_BOUND) {
                            scoreToStore = eval::Score(score.value() - ply);
                        }
                        
                        // Store with depth 0 (quiescence) and LOWER bound
                        tt.store(board.zobristKey(), move, scoreToStore.value(), 
                                staticEval.value(), 0, Bound::LOWER);
                    }
                    
                    return score;
                }
            }
        }
    }
    
    // Deliverable 2.2: Store final result in TT
    if (tt.isEnabled()) {
        // Determine bound type based on search outcome
        Bound bound;
        Move bestMove = NO_MOVE;  // No best move tracked in current quiescence
        
        if (bestScore > alpha) {
            // We improved alpha - EXACT bound if not at original alpha
            // But be careful: if bestScore came from stand-pat, it's an UPPER bound
            if (!moves.empty() && !isInCheck) {
                // We searched moves and improved alpha - EXACT
                bound = Bound::EXACT;
            } else if (moves.empty() && !isInCheck) {
                // No captures available, stand-pat value - UPPER bound
                bound = Bound::UPPER;
            } else {
                // In check or found better move - EXACT
                bound = Bound::EXACT;
            }
        } else {
            // Failed low - UPPER bound
            bound = Bound::UPPER;
        }
        
        // Adjust mate scores for storage
        eval::Score scoreToStore = bestScore;
        if (bestScore.value() >= MATE_BOUND) {
            scoreToStore = eval::Score(bestScore.value() + ply);
        } else if (bestScore.value() <= -MATE_BOUND) {
            scoreToStore = eval::Score(bestScore.value() - ply);
        }
        
        // Store with depth 0 for quiescence
        tt.store(board.zobristKey(), bestMove, scoreToStore.value(),
                staticEval.value(), 0, bound);
    }
    
    return bestScore;
}

} // namespace seajay::search