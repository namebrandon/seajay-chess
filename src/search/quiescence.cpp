#include "quiescence.h"
#include "../core/board.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
#include "../core/transposition_table.h"
#include "discovered_check.h"  // For discovered check detection
#include <chrono>  // For time management
#include "move_ordering.h"  // For MVV-LVA ordering and VICTIM_VALUES - ALWAYS NEEDED
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
    seajay::TranspositionTable& tt,
    int checkPly,
    bool inPanicMode)
{
    // REMOVED emergency cutoff - it was destroying tactical play
    // Accepting occasional time losses is better than constant tactical blindness
    // Candidate 1 proved this with 300+ ELO gain despite 1-2% time losses
    
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
    // Reverting to Candidate 1 frequency - the overhead of frequent checks was harmful
    if ((data.qsearchNodes & 1023) == 0) {
        if (data.stopped || data.checkTime()) {
            data.stopped = true;
            return eval::Score::zero();
        }
    }
    
    // Update selective depth
    if (ply > data.seldepth) {
        data.seldepth = ply;
    }
    
    // Deliverable 3.4: Detect endgame for delta pruning margin adjustment
    // Candidate 9: Add panic mode for time pressure (100cp margin)
    bool isEndgame = (board.material().value(WHITE).value() < 1300) && (board.material().value(BLACK).value() < 1300);
    int deltaMargin;
    if (inPanicMode) {
        deltaMargin = DELTA_MARGIN_PANIC;  // 100cp in panic mode
    } else {
        deltaMargin = isEndgame ? DELTA_MARGIN_ENDGAME : DELTA_MARGIN;
    }
    
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
    
    // Deliverable 3.2.1: Track check ply depth and limit extensions
    int newCheckPly = isInCheck ? checkPly + 1 : 0;
    
    // Stop extending checks after MAX_CHECK_PLY to prevent search explosion
    if (newCheckPly > MAX_CHECK_PLY) {
        // Return static evaluation when check depth limit reached
        return eval::evaluate(board);
    }
    
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
        
        // Deliverable 3.2.2: Escape Route Prioritization
        // Order evasion moves: King moves > Blocking > Capturing checker
        Square kingSquare = board.kingSquare(board.sideToMove());
        std::sort(moves.begin(), moves.end(), [kingSquare](Move a, Move b) {
            // King moves have highest priority
            bool aIsKingMove = (from(a) == kingSquare);
            bool bIsKingMove = (from(b) == kingSquare);
            if (aIsKingMove && !bIsKingMove) return true;
            if (!aIsKingMove && bIsKingMove) return false;
            
            // Then captures (which might capture the checking piece)
            bool aIsCapture = isCapture(a);
            bool bIsCapture = isCapture(b);
            if (aIsCapture && !bIsCapture) return true;
            if (!aIsCapture && bIsCapture) return false;
            
            // Rest maintain original order (blocks)
            return false;
        });
    } else {
        // Not in check: only search captures
        MoveGenerator::generateCaptures(board, moves);
    }
    
    // Phase 2.2: Enhanced move ordering with queen promotion prioritization
    // MVV-LVA is a CORE FEATURE - always compile it in!
    // Order: Queen Promotions → Discovered Checks → TT moves → Other captures → Quiet moves
    MvvLvaOrdering mvvLva;
    mvvLva.orderMoves(board, moves);
    
    // Phase 2.2 Missing Item 1: Queen Promotion Prioritization
    // Move queen promotions to the very front (before TT moves)
    auto queenPromoIt = moves.begin();
    for (auto it = moves.begin(); it != moves.end(); ++it) {
        if (isPromotion(*it) && promotionType(*it) == QUEEN) {
            if (it != queenPromoIt) {
                std::rotate(queenPromoIt, it, it + 1);
            }
            ++queenPromoIt;  // Move insertion point for next queen promotion
        }
    }
    
    // Deliverable 3.2.3: Discovered Check Detection
    // Prioritize captures that create discovered checks (after queen promos)
    if (!isInCheck) {  // Only for capture moves, not check evasions
        auto discoveredCheckIt = queenPromoIt;
        for (auto it = queenPromoIt; it != moves.end(); ++it) {
            if (isCapture(*it) && isDiscoveredCheck(board, *it)) {
                if (it != discoveredCheckIt) {
                    std::rotate(discoveredCheckIt, it, it + 1);
                }
                ++discoveredCheckIt;
            }
        }
    }
    
    // TT Move Ordering (after queen promotions)
    // If we have a TT move and it's not already a queen promotion, prioritize it
    if (ttMove != NO_MOVE) {
        auto ttMoveIt = std::find(queenPromoIt, moves.end(), ttMove);
        if (ttMoveIt != moves.end()) {
            // Move TT move to front of non-queen-promotion moves
            std::rotate(queenPromoIt, ttMoveIt, ttMoveIt + 1);
        }
    }
    
    // Search moves
    eval::Score bestScore = isInCheck ? eval::Score::minus_infinity() : alpha;
    int moveCount = 0;
    
    // Candidate 9: Use reduced capture limit in panic mode
    const int maxCaptures = inPanicMode ? MAX_CAPTURES_PANIC : MAX_CAPTURES_PER_NODE;
    
    for (const Move& move : moves) {
        // Limit moves per node to prevent explosion (except when in check)
        if (!isInCheck && ++moveCount > maxCaptures) {
            break;
        }
        
        // Deliverable 3.3 & 3.4: Per-move delta pruning with endgame safety
        // Skip bad captures that can't improve alpha even if successful
        if (!isInCheck && !isPromotion(move)) {
            // Use accurate victim value from MVV-LVA tables (always available)
            Piece capturedPiece = board.pieceAt(to(move));
            PieceType captured = (capturedPiece != NO_PIECE) ? typeOf(capturedPiece) : NO_PIECE_TYPE;
            int captureValue = (captured != NO_PIECE_TYPE) ? VICTIM_VALUES[captured] : 0;
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
        
        // Recursive quiescence search with check ply tracking and panic mode propagation
        eval::Score score = -quiescence(board, ply + 1, -beta, -alpha, 
                                       searchInfo, data, tt, newCheckPly, inPanicMode);
        
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