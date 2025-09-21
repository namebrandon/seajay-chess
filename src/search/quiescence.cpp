#include "quiescence.h"
#include "node_explosion_stats.h"  // Node explosion diagnostics
#include "../core/board.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
#include "../core/transposition_table.h"
#include "discovered_check.h"  // For discovered check detection
#include <chrono>  // For time management
#include "move_ordering.h"  // For MVV-LVA ordering and VICTIM_VALUES - ALWAYS NEEDED
#include "ranked_move_picker.h"  // Phase 2a: Ranked move picker
#include "../core/see.h"  // Stage 15 Day 6: For SEE-based pruning
#include <cassert>
#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>  // For stack-allocated RankedMovePickerQS

namespace seajay::search {

// Helper functions for SEE pruning mode
SEEPruningMode parseSEEPruningMode(const std::string& mode) {
    if (mode == "conservative") return SEEPruningMode::CONSERVATIVE;
    if (mode == "moderate") return SEEPruningMode::MODERATE;
    if (mode == "aggressive") return SEEPruningMode::AGGRESSIVE;
    return SEEPruningMode::OFF;
}

std::string seePruningModeToString(SEEPruningMode mode) {
    switch (mode) {
        case SEEPruningMode::CONSERVATIVE: return "conservative";
        case SEEPruningMode::MODERATE: return "moderate";
        case SEEPruningMode::AGGRESSIVE: return "aggressive";
        default: return "off";
    }
}

// Mate score bound for adjustment
constexpr int MATE_BOUND = 29000;

eval::Score quiescence(
    Board& board,
    NodeContext context,
    int ply,
    int qply,
    eval::Score alpha,
    eval::Score beta,
    seajay::SearchInfo& searchInfo,
    SearchData& data,
    const SearchLimits& limits,
    seajay::TranspositionTable& tt,
    int checkPly,
    bool inPanicMode)
{
#ifdef DEBUG
    assert(!context.hasExcludedMove() && "Quiescence search cannot operate with an excluded move");
#endif
    [[maybe_unused]] const bool isPvNode = context.isPv();

    // Store original alpha for correct TT bound classification
    const eval::Score originalAlpha = alpha;
    // REMOVED emergency cutoff - it was destroying tactical play
    // Accepting occasional time losses is better than constant tactical blindness
    // Candidate 1 proved this with 300+ ELO gain despite 1-2% time losses
    
    // Record entry node count for per-position limit enforcement
    const uint64_t entryNodes = data.qsearchNodes;
    
    // Track nodes
    data.qsearchNodes++;
    
    // Node explosion diagnostics: Track quiescence nodes
    if (limits.nodeExplosionDiagnostics) {
        g_nodeExplosionStats.recordQuiescenceNode(qply);
    }
    
    // Deliverable 2.1: TT Probing in Quiescence
    // Probe transposition table at the start of quiescence
    TTEntry* ttEntry = nullptr;
    eval::Score ttScore = eval::Score::zero();
    Move ttMove = NO_MOVE;  // Deliverable 2.3: Track TT move for ordering
    
    if (tt.isEnabled()) {
        Hash zobristKey = board.zobristKey();
        tt.prefetch(zobristKey);  // Prefetch TT entry to hide memory latency
        ttEntry = tt.probe(zobristKey);
    }
    
    if (ttEntry != nullptr) {
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
    
    // Stop extending checks after maxCheckPly to prevent search explosion
    if (newCheckPly > limits.maxCheckPly) {
        // Return static evaluation when check depth limit reached
        return eval::evaluate(board);
    }
    
    // Stand-pat evaluation (skip if in check - must make a move)
    eval::Score staticEval;
    bool staticEvalComputed = false;  // Track if we have a real static eval
    if (!isInCheck) {
        staticEval = eval::evaluate(board);
        staticEvalComputed = true;
        
        // Beta cutoff on stand-pat
        if (staticEval >= beta) {
            data.standPatCutoffs++;
            return staticEval;
        }
        
        // Phase 1.3: Enhanced delta pruning pre-check
        // More aggressive pruning when position is hopeless
        // Use queen value (975) as base, adjusted for game phase
        const int QUEEN_VALUE = 975;
        int coarseDeltaMargin = QUEEN_VALUE;
        
        // Be more aggressive in endgame where evaluations are more accurate
        if (isEndgame) {
            coarseDeltaMargin = 600;  // Rook + minor piece
        }
        
        if (staticEval + eval::Score(coarseDeltaMargin) < alpha) {
            data.deltasPruned++;
            return alpha;  // Fail-hard alpha cutoff
        }
        
        // Update alpha with stand-pat score
        // In quiet positions, we can choose to not make any move
        alpha = std::max(alpha, staticEval);
    } else {
        // In check: no stand-pat, must make a move
        staticEval = eval::Score::minus_infinity();
    }
    
    // Phase 2a: Use RankedMovePickerQS for non-check positions
    // Use optional to avoid dynamic allocation (stack allocation instead)
    std::optional<RankedMovePickerQS> rankedPickerQS;
    MoveList& moves = data.acquireMoveList(ply);
    
    // Phase 2a.7: Verify RankedMovePicker is NOT used in quiescence
    // This path should never be taken in Phase 2a
    if (false && limits.useRankedMovePicker && !isInCheck) {
#ifdef DEBUG
        // Phase 2a.7: Assert this path is never taken
        assert(false && "RankedMovePicker should not be used in quiescence during Phase 2a");
#endif
        // One-time log to confirm no ranked QS path (should never print)
        static bool loggedOnce = false;
        if (!loggedOnce) {
            std::cerr << "ERROR: RankedMovePicker path hit in quiescence (should not happen in Phase 2a)" << std::endl;
            loggedOnce = true;
        }
        // Use ranked move picker for captures/promotions only
        rankedPickerQS.emplace(board, ttMove);
    }
    
    // Generate moves based on check status
    if (isInCheck) {
        // In check: must generate ALL legal moves (not just captures)
        // Don't use RankedMovePicker in check positions for Phase 2a
        moves = generateLegalMoves(board);
        
        // Check for checkmate/stalemate
        if (moves.empty()) {
            // Checkmate (we're in check with no legal moves)
            return eval::Score(-32000 + ply);
        }
        
        // QSearch Fix Option A: Reverse evasion move ordering
        // Prioritize Captures > Blocks > King moves (except in double check)
        Square kingSquare = board.kingSquare(board.sideToMove());
        
        // Handle double check case where only king moves are legal
        bool isDoubleCheck = false;
        if (moves.size() > 0) {
            isDoubleCheck = std::all_of(moves.begin(), moves.end(), 
                [kingSquare](Move m) { return from(m) == kingSquare; });
        }
        
        if (!isDoubleCheck) {
            // Single check: Reorder to deprioritize king moves
            std::sort(moves.begin(), moves.end(), [kingSquare](Move a, Move b) {
                bool aIsKingMove = (from(a) == kingSquare);
                bool bIsKingMove = (from(b) == kingSquare);
                
                // King moves have LOWEST priority (reversed from original)
                if (aIsKingMove && !bIsKingMove) return false;
                if (!aIsKingMove && bIsKingMove) return true;
                
                // Among non-king moves: captures before blocks
                bool aIsCapture = isCapture(a);
                bool bIsCapture = isCapture(b);
                if (aIsCapture && !bIsCapture) return true;
                if (!aIsCapture && bIsCapture) return false;
                
                return false;
            });
        }
    } else if (!rankedPickerQS) {
        // Not in check and not using RankedMovePicker: only search captures
        MoveGenerator::generateCaptures(board, moves);
        if (limits.nodeExplosionDiagnostics) {
            g_nodeExplosionStats.qsearchExplosion.capturesGenerated += moves.size();
        }
    }
    
    // Phase 2.2: Enhanced move ordering with queen promotion prioritization
    // Order: Queen Promotions → Discovered Checks → TT moves → Other captures → Quiet moves
    // Use SEE-based capture ordering when enabled for non-check nodes
    // Skip ordering if using RankedMovePicker (it handles ordering internally)
    if (!rankedPickerQS) {
        if (!isInCheck && search::g_seeMoveOrdering.getMode() != search::SEEMode::OFF) {
            search::g_seeMoveOrdering.orderMoves(board, moves);
        } else {
            MvvLvaOrdering mvvLva;
            mvvLva.orderMoves(board, moves);
        }
    }
    
    // Phase 2.2 Missing Item 1: Queen Promotion Prioritization
    // Move queen promotions to the very front (before TT moves)
    // Skip if using RankedMovePicker (it handles ordering internally)
    if (!rankedPickerQS) {
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
    }
    
    // Search moves
    eval::Score bestScore = isInCheck ? eval::Score::minus_infinity() : alpha;
    Move bestMove = NO_MOVE;  // Track the best move found for TT storage
    int moveCount = 0;
    int legalMoveCount = 0;
    
    // Candidate 9: Use reduced capture limit in panic mode
    const int maxCaptures = inPanicMode ? MAX_CAPTURES_PANIC : (limits.qsearchMaxCaptures > 0 ? limits.qsearchMaxCaptures : MAX_CAPTURES_PER_NODE);
    
    // Phase 2a: Iterate using RankedMovePickerQS or traditional move list
    Move move;
    size_t moveIndex = 0;
    while (true) {
        if (rankedPickerQS) {
#ifdef DEBUG
            // Phase 2a.7: This should never be reached in Phase 2a
            assert(false && "RankedPickerQS should not be active in Phase 2a");
#endif
            move = rankedPickerQS->next();
            if (move == NO_MOVE) break;
        } else {
            if (moveIndex >= moves.size()) break;
            move = moves[moveIndex++];
        }
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
            
            // Phase 1.3: Enhanced per-move delta pruning
            // More aggressive margin, especially for minor captures
            int adjustedMargin = deltaMargin;
            if (captureValue <= 325) {  // Minor piece or less
                adjustedMargin = deltaMargin / 2;  // Tighter margin for small captures
            }
            
            if (staticEval + eval::Score(captureValue + adjustedMargin) < alpha) {
                data.deltasPruned++;
                continue;  // Skip this capture - it can't improve our position enough
            }
        }
        
        // Stage 15 Day 6: SEE-based pruning
        // Only prune captures (not promotions or check evasions)
        // Stage 14 Remediation: Use pre-parsed mode from SearchData to avoid string parsing
        if (data.seePruningModeEnumQ != SEEPruningMode::OFF && !isInCheck && isCapture(move) && !isPromotion(move)) {
            data.seeStats.totalCaptures++;
            
            // Calculate SEE value for this capture
            SEEValue seeValue = see(board, move);
            data.seeStats.seeEvaluations++;
            
            // Determine pruning threshold based on mode, game phase, and depth
            int pruneThreshold;
            if (data.seePruningModeEnumQ == SEEPruningMode::CONSERVATIVE) {
                // Conservative: fixed threshold -100
                pruneThreshold = SEE_PRUNE_THRESHOLD_CONSERVATIVE;  // -100
            } else if (data.seePruningModeEnumQ == SEEPruningMode::MODERATE) {
                // Moderate-lite: gentler than previous moderate
                // Base thresholds
                pruneThreshold = isEndgame ? -65 : SEE_PRUNE_THRESHOLD_MODERATE; // endgame softened from -50 -> -65
                // Smaller, later depth ramp: only from qply>=6
                int depthBonus = (qply >= 8 ? 15 : (qply >= 6 ? 10 : 0));
                pruneThreshold = std::min(pruneThreshold + depthBonus, 0);  // Never prune winning captures
            } else {  // AGGRESSIVE
                // Aggressive: depth-dependent and game-phase aware
                // Start with base threshold
                pruneThreshold = isEndgame ? SEE_PRUNE_THRESHOLD_ENDGAME : SEE_PRUNE_THRESHOLD_AGGRESSIVE;
                
                // Make pruning more aggressive deeper in quiescence
                // Each 2 plies deeper, increase pruning aggressiveness by 25cp (use qply)
                int depthBonus = (qply / 2) * 25;
                pruneThreshold = std::min(pruneThreshold + depthBonus, 0);  // Don't prune winning captures
            }
            
            // Prune if SEE value is below threshold
            if (seeValue < pruneThreshold) {
                data.seeStats.seePruned++;
                // Node explosion diagnostics: Track SEE pruning
                if (limits.nodeExplosionDiagnostics) {
                    g_nodeExplosionStats.recordSEEPrune(qply, seeValue);
                    // Track bad captures (SEE < 0)
                    if (seeValue < 0) {
                        g_nodeExplosionStats.recordBadCapture(qply);
                    }
                }
                
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
            if (seeValue == 0) {
                bool applyEqualPrune = false;
                if (data.seePruningModeEnumQ == SEEPruningMode::AGGRESSIVE) {
                    // Aggressive profile (existing behavior)
                    if (qply >= 7) applyEqualPrune = true;
                    else if (qply >= 5) applyEqualPrune = (staticEval >= alpha - eval::Score(50));
                    else if (qply >= 3) applyEqualPrune = (staticEval >= alpha);
                } else if (data.seePruningModeEnumQ == SEEPruningMode::MODERATE) {
                    // Moderate-lite: only at deepest qplies and with stricter guard
                    if (qply >= 8) {
                        applyEqualPrune = (staticEval >= alpha + eval::Score(25));
                        if (applyEqualPrune) {
                            // Victim-aware exception: do not prune equal exchanges on pieces (only allow equal pawn trades)
                            Piece capturedPieceEE = board.pieceAt(to(move));
                            if (capturedPieceEE != NO_PIECE) {
                                PieceType victimEE = typeOf(capturedPieceEE);
                                if (victimEE != PAWN) {
                                    applyEqualPrune = false;
                                }
                            }
                        }
                    }
                }
                if (applyEqualPrune) {
                    data.seeStats.seePruned++;
                    data.seeStats.equalExchangePrunes++;
                    continue;
                }
            }
        }
        
        // Record capture searched after pruning on non-check nodes
        if (limits.nodeExplosionDiagnostics && !isInCheck && isCapture(move)) {
            g_nodeExplosionStats.qsearchExplosion.capturesSearched++;
        }
        // Push position to search stack
        searchInfo.pushSearchPosition(board.zobristKey(), move, ply);
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        legalMoveCount++;

        bool childIsPv = context.isPv() && (legalMoveCount == 1);
        NodeContext childContext = makeChildContext(context, childIsPv);

        // Recursive quiescence search with check ply tracking and panic mode propagation
        eval::Score score = -quiescence(board, childContext, ply + 1, qply + 1, -beta, -alpha,
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
                        // Only store real static eval, not minus_infinity when in check
                        int16_t evalToStore = staticEvalComputed ? staticEval.value() : TT_EVAL_NONE;
                        tt.store(board.zobristKey(), move, scoreToStore.value(), 
                                evalToStore, 0, Bound::LOWER);
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
        // Only store real static eval, not minus_infinity when in check
        int16_t evalToStore = staticEvalComputed ? staticEval.value() : TT_EVAL_NONE;
        tt.store(board.zobristKey(), bestMove, scoreToStore.value(),
                evalToStore, 0, bound);
    }
    
    return bestScore;
}

} // namespace seajay::search
