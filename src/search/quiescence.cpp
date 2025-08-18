#include "quiescence.h"
#include "../core/board.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
#include "../core/transposition_table.h"
#include "discovered_check.h"  // For discovered check detection
#include <chrono>  // For time management
#include "move_ordering.h"  // For MVV-LVA ordering and VICTIM_VALUES - ALWAYS NEEDED
#include "../core/see.h"  // Stage 15 Day 6: For SEE-based pruning
#include <algorithm>
#include <iostream>

namespace seajay::search {

// Helper functions for SEE pruning mode
SEEPruningMode parseSEEPruningMode(const std::string& mode) {
    if (mode == "conservative") return SEEPruningMode::CONSERVATIVE;
    if (mode == "aggressive") return SEEPruningMode::AGGRESSIVE;
    return SEEPruningMode::OFF;
}

std::string seePruningModeToString(SEEPruningMode mode) {
    switch (mode) {
        case SEEPruningMode::CONSERVATIVE: return "conservative";
        case SEEPruningMode::AGGRESSIVE: return "aggressive";
        default: return "off";
    }
}

// Mate score bound for adjustment
constexpr int MATE_BOUND = 29000;

eval::Score quiescence(
    Board& board,
    int ply,
    eval::Score alpha,
    eval::Score beta,
    seajay::SearchInfo& searchInfo,
    SearchData& data,
    const SearchLimits& limits,
    seajay::TranspositionTable& tt,
    int checkPly,
    bool inPanicMode)
{
    // Store original alpha for correct TT bound classification
    const eval::Score originalAlpha = alpha;
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
            // Only accept depth 0 entries (quiescence-specific)
            // Don't accept deeper entries from main search (depth > 0)
            // This prevents main search entries from short-circuiting quiescence
            if (ttEntry->depth == 0) {
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
    
    // Check time limit periodically to prevent time losses
    // Using consistent interval with main search for uniformity
    if ((data.qsearchNodes & (SearchData::TIME_CHECK_INTERVAL - 1)) == 0) {
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
    
    // Safety check 2: enforce per-position node limit (if set)
    // This prevents search explosion in complex tactical positions
    if (limits.qsearchNodeLimit > 0 && 
        data.qsearchNodes - entryNodes > limits.qsearchNodeLimit) {
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
        
        // Deliverable 3.2 & 3.4: Basic delta pruning pre-check
        // If we're so far behind that even the best possible capture won't help, we can return early
        // This is a coarse filter - we still do per-move delta pruning later
        // Only do this aggressive pruning if we're really far behind (queen value + margin)
        if (staticEval + eval::Score(900 + deltaMargin) < alpha) {
            data.deltasPruned++;
            return staticEval;  // Position is hopeless even with best capture
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
    Move bestMove = NO_MOVE;  // Track the best move found for TT storage
    int moveCount = 0;
    
    // Candidate 9: Use reduced capture limit in panic mode
    const int maxCaptures = inPanicMode ? MAX_CAPTURES_PANIC : MAX_CAPTURES_PER_NODE;
    
    for (const Move& move : moves) {
        // Limit moves per node to prevent explosion (except when in check)
        if (!isInCheck && ++moveCount > maxCaptures) {
            break;
        }
        
        // Deliverable 3.3 & 3.4: Per-move delta pruning
        // Formula: static_eval + captured_piece_value + margin < alpha
        // This is the standard delta pruning used by all modern engines
        if (!isInCheck && !isPromotion(move)) {
            // Get the value of the piece we're capturing
            Piece capturedPiece = board.pieceAt(to(move));
            PieceType captured = (capturedPiece != NO_PIECE) ? typeOf(capturedPiece) : NO_PIECE_TYPE;
            int captureValue = (captured != NO_PIECE_TYPE) ? VICTIM_VALUES[captured] : 0;
            
            // Standard delta pruning formula: can this capture possibly improve alpha?
            if (staticEval + eval::Score(captureValue + deltaMargin) < alpha) {
                data.deltasPruned++;
                continue;  // Skip this capture - it can't improve our position enough
            }
        }
        
        // Stage 15 Day 6: SEE-based pruning
        // Only prune captures (not promotions or check evasions)
        // Stage 14 Remediation: Use pre-parsed mode from SearchData to avoid string parsing
        if (data.seePruningModeEnum != SEEPruningMode::OFF && !isInCheck && isCapture(move) && !isPromotion(move)) {
            data.seeStats.totalCaptures++;
            
            // Calculate SEE value for this capture
            SEEValue seeValue = see(board, move);
            data.seeStats.seeEvaluations++;
            
            // Determine pruning threshold based on mode, game phase, and depth
            int pruneThreshold;
            if (data.seePruningModeEnum == SEEPruningMode::CONSERVATIVE) {
                // Conservative: fixed threshold -100
                pruneThreshold = SEE_PRUNE_THRESHOLD_CONSERVATIVE;  // -100
            } else {  // AGGRESSIVE
                // Aggressive: depth-dependent and game-phase aware
                // Start with base threshold
                pruneThreshold = isEndgame ? SEE_PRUNE_THRESHOLD_ENDGAME : SEE_PRUNE_THRESHOLD_AGGRESSIVE;
                
                // Make pruning more aggressive deeper in quiescence
                // Each 2 plies deeper, increase pruning aggressiveness by 25cp
                int depthBonus = (ply / 2) * 25;
                pruneThreshold = std::min(pruneThreshold + depthBonus, 0);  // Don't prune winning captures
            }
            
            // Prune if SEE value is below threshold
            if (seeValue < pruneThreshold) {
                data.seeStats.seePruned++;
                
                // Track which threshold was used
                if (pruneThreshold == SEE_PRUNE_THRESHOLD_CONSERVATIVE) {
                    data.seeStats.conservativePrunes++;
                } else if (pruneThreshold == SEE_PRUNE_THRESHOLD_AGGRESSIVE) {
                    data.seeStats.aggressivePrunes++;
                } else {
                    data.seeStats.endgamePrunes++;
                }
                
                // Note: Debug logging removed from hot path - stats can be logged after search
                continue;  // Skip this capture
            }
            
            // Also consider pruning equal exchanges late in quiescence
            // The deeper we are, the more likely we prune equal exchanges
            if (data.seePruningModeEnum == SEEPruningMode::AGGRESSIVE && seeValue == 0) {
                // Prune equal exchanges based on depth
                // At ply 3-4: prune if position is quiet
                // At ply 5-6: prune more aggressively  
                // At ply 7+: always prune equal exchanges
                bool pruneEqual = false;
                if (ply >= 7) {
                    pruneEqual = true;  // Always prune deep in search
                } else if (ply >= 5) {
                    // Prune if we're not finding tactics
                    pruneEqual = (staticEval >= alpha - eval::Score(50));
                } else if (ply >= 3) {
                    // Only prune if position looks very quiet
                    pruneEqual = (staticEval >= alpha);
                }
                
                if (pruneEqual) {
                    data.seeStats.seePruned++;
                    data.seeStats.equalExchangePrunes++;
                    continue;
                }
            }
        }
        
        // Push position to search stack
        searchInfo.pushSearchPosition(board.zobristKey(), move, ply);
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        // Recursive quiescence search with check ply tracking and panic mode propagation
        eval::Score score = -quiescence(board, ply + 1, -beta, -alpha, 
                                       searchInfo, data, limits, tt, newCheckPly, inPanicMode);
        
        // Unmake the move
        board.unmakeMove(move, undo);
        
        // Check if search was stopped
        if (data.stopped) {
            return bestScore;
        }
        
        // Update best score
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;  // Always track which move produced the best score
            
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
                        // Note: 'move' is the best move that caused the beta cutoff
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
        // Correct TT bound classification using original alpha
        Bound bound;
        // bestMove already tracked during search - use it for TT storage
        
        if (bestScore >= beta) {
            // Beta cutoff - this is a lower bound (fail-high)
            bound = Bound::LOWER;
        } else if (bestScore > originalAlpha) {
            // We raised alpha - this is an exact score
            bound = Bound::EXACT;
        } else {
            // Failed low - this is an upper bound
            bound = Bound::UPPER;
        }
        
        // Adjust mate scores for storage
        eval::Score scoreToStore = bestScore;
        if (bestScore.value() >= MATE_BOUND) {
            scoreToStore = eval::Score(bestScore.value() + ply);
        } else if (bestScore.value() <= -MATE_BOUND) {
            scoreToStore = eval::Score(bestScore.value() - ply);
        }
        
        // Store with depth 0 for quiescence and the best move found
        // Even for UPPER bounds (fail-low), storing the best move helps move ordering
        tt.store(board.zobristKey(), bestMove, scoreToStore.value(),
                staticEval.value(), 0, bound);
    }
    
    return bestScore;
}

} // namespace seajay::search