#include "negamax.h"
#include "search_info.h"
#include "iterative_search_data.h"  // Stage 13 addition
#include "time_management.h"        // Stage 13, Deliverable 2.2a
#include "aspiration_window.h"       // Stage 13, Deliverable 3.2b
#include "window_growth_mode.h"      // Stage 13 Remediation Phase 4
#include "game_phase.h"              // Stage 13 Remediation Phase 4
#include "move_ordering.h"  // Stage 11: MVV-LVA ordering (always enabled)
#include "lmr.h"            // Stage 18: Late Move Reductions
#include "../core/board.h"
#include "../core/board_safety.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
#include "quiescence.h"  // Stage 14: Quiescence search
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <cassert>

namespace seajay::search {

// Simple move ordering without MVV-LVA (fallback)
template<typename MoveContainer>
inline void orderMovesSimple(MoveContainer& moves) noexcept {
    auto first = moves.begin();
    auto last = moves.end();
    auto partition_point = first;
    
    // Phase 1: Move all promotions to the front
    // Queen promotions have highest priority
    for (auto it = first; it != last; ++it) {
        if (isPromotion(*it)) {
            // If it's a queen promotion, move it to the very front
            if (promotionType(*it) == QUEEN) {
                if (it != first) {
                    // Move queen promotion to front
                    Move temp = *it;
                    std::move_backward(first, it, it + 1);
                    *first = temp;
                    partition_point = std::next(first);
                }
            } else if (it != partition_point) {
                // Other promotions go after queen promotions
                std::iter_swap(it, partition_point);
                ++partition_point;
            } else {
                ++partition_point;
            }
        }
    }
    
    // Phase 2: Move captures after promotions
    for (auto it = partition_point; it != last; ++it) {
        if (isCapture(*it) && !isPromotion(*it)) {  // Skip promotion-captures (already ordered)
            if (it != partition_point) {
                std::iter_swap(it, partition_point);
            }
            ++partition_point;
        }
    }
    
    // Quiet moves remain at the end (no need to explicitly order them)
}

// Move ordering function for alpha-beta pruning efficiency
// Orders moves in-place: TT move first, then promotions, then captures (MVV-LVA), then killers, then quiet moves
template<typename MoveContainer>
inline void orderMoves(const Board& board, MoveContainer& moves, Move ttMove = NO_MOVE, 
                      const SearchData* searchData = nullptr, int ply = 0) noexcept {
    // Sub-phase 4E: TT Move Ordering
    // If we have a TT move, put it first
    if (ttMove != NO_MOVE) {
        auto it = std::find(moves.begin(), moves.end(), ttMove);
        if (it != moves.end() && it != moves.begin()) {
            // Move TT move to front
            Move temp = *it;
            std::move_backward(moves.begin(), it, it + 1);
            *moves.begin() = temp;
        }
    }
    
    // Stage 11: Always use MVV-LVA for move ordering (remediated - no compile flag)
    // Stage 19, Phase A2: Use killer moves if available
    // Stage 20, Phase B2: Use history heuristic for quiet moves
    static MvvLvaOrdering mvvLva;
    if (searchData != nullptr) {
        // Use killer and history aware ordering
        mvvLva.orderMovesWithHistory(board, moves, searchData->killers, searchData->history, ply);
    } else {
        // Fallback to standard MVV-LVA
        mvvLva.orderMoves(board, moves);
    }
    
    // After ordering, ensure TT move is still first if it was valid
    if (ttMove != NO_MOVE) {
        auto it = std::find(moves.begin(), moves.end(), ttMove);
        if (it != moves.end() && it != moves.begin()) {
            // Move TT move back to front (ordering may have moved it)
            Move temp = *it;
            std::move_backward(moves.begin(), it, it + 1);
            *moves.begin() = temp;
        }
    }
}

// Mate score constants for TT integration
constexpr int MATE_SCORE = 30000;
constexpr int MATE_BOUND = 29000;

// Core negamax search implementation
eval::Score negamax(Board& board, 
                   int depth, 
                   int ply,
                   eval::Score alpha,
                   eval::Score beta,
                   SearchInfo& searchInfo,
                   SearchData& info,
                   const SearchLimits& limits,
                   TranspositionTable* tt,
                   bool isPvNode) {
    
    // Debug output at root
    if (ply == 0 && depth >= 4) {
        std::cerr << "Negamax: Starting search at depth " << depth << std::endl;
    }
    
    // Debug assertions
    assert(depth >= 0);
    assert(ply >= 0);
    assert(ply < 128);  // Prevent stack overflow
    
    // Debug assertion for valid search window
    assert(alpha < beta && "Alpha must be less than beta in negamax search");
    
    // In release mode, still handle invalid window gracefully
    if (alpha >= beta) {
        // This shouldn't happen in correct negamax, but handle it gracefully
        return alpha;  // Return alpha as a fail-soft bound
    }
    
    // Time check - only check periodically to reduce overhead
    if ((info.nodes & (SearchData::TIME_CHECK_INTERVAL - 1)) == 0 && info.checkTime()) {
        info.stopped = true;
        return eval::Score::zero();
    }
    
    // Increment node counter
    info.nodes++;
    
    // Phase P2: Store PV status in search stack
    searchInfo.setPvNode(ply, isPvNode);
    
    // Update selective depth (maximum depth reached)
    if (ply > info.seldepth) {
        info.seldepth = ply;
    }
    
    // Terminal node - enter quiescence search or return static evaluation
    if (depth <= 0) {
        // Stage 14: Quiescence search - ALWAYS compiled in, controlled by UCI option
        // This is a CORE FEATURE and should never be behind compile flags!
        if (info.useQuiescence) {
            // Candidate 9: Detect time pressure for panic mode
            // Panic mode activates when remaining time < 100ms
            bool inPanicMode = false;
            if (info.timeLimit != std::chrono::milliseconds::max()) {
                auto remainingTime = info.timeLimit - info.elapsed();
                inPanicMode = (remainingTime < std::chrono::milliseconds(100));
            }
            // Use quiescence search to resolve tactical sequences
            return quiescence(board, ply, alpha, beta, searchInfo, info, limits, *tt, 0, inPanicMode);
        }
        // Fallback: return static evaluation (only if quiescence disabled via UCI)
        return board.evaluate();
    }
    
    // Sub-phase 4B: Draw Detection Order
    // CRITICAL ORDER:
    // 1. Check for checkmate/stalemate FIRST (terminal conditions)
    // 2. Check for repetition SECOND
    // 3. Check for fifty-move rule THIRD
    // 4. Only THEN probe TT
    
    // Initialize TT move (used for move ordering later)
    Move ttMove = NO_MOVE;
    
    // Check if we're in check (needed for null move and other decisions)
    bool weAreInCheck = inCheck(board);
    
    // Generate all legal moves (needed for checkmate/stalemate check)
    MoveList moves = generateLegalMoves(board);
    
    // Check for checkmate or stalemate first (has priority over draws)
    if (moves.empty()) {
        if (inCheck(board)) {
            // Checkmate - return mate score (checkmate has priority over draws)
            return eval::Score(-32000 + ply);
        } else {
            // Stalemate - return draw score
            return eval::Score::draw();
        }
    }
    
    // Sub-phase 4B: Establish correct probe order
    // 1. Check repetition FIRST (fastest, most common draw in search)
    // 2. Check fifty-move rule SECOND (less common)
    // 3. Only THEN probe TT (after all draw conditions)
    
    // Never check draws or probe TT at root
    if (ply > 0) {
        // PERFORMANCE OPTIMIZATION: Strategic draw checking
        bool shouldCheckDraw = false;
        
        // Determine if we should check for draws based on position characteristics
        bool inCheckPosition = inCheck(board);
        
        // Get info about last move from search stack (if available)
        bool lastMoveWasCapture = false;
        if (ply > 0 && ply - 1 < 128) {
            Move lastMove = searchInfo.getStackEntry(ply - 1).move;
            if (lastMove != NO_MOVE) {
                lastMoveWasCapture = isCapture(lastMove);
            }
        }
        
        shouldCheckDraw = 
            inCheckPosition ||                      // Always after checks
            lastMoveWasCapture ||                   // After captures (50-move reset)
            (ply >= 4 && (ply & 3) == 0);          // Every 4th ply for repetitions
        
        // Check for draws when necessary
        if (shouldCheckDraw && board.isDrawInSearch(searchInfo, ply)) {
            return eval::Score::draw();  // Draw score
        }
        
        // NOW probe TT (after draw detection)
        TTEntry* ttEntry = nullptr;
        if (tt && tt->isEnabled()) {
            Hash zobristKey = board.zobristKey();
            ttEntry = tt->probe(zobristKey);
            info.ttProbes++;
            
            if (ttEntry && ttEntry->key32 == (zobristKey >> 32)) {
                info.ttHits++;
                
                // Sub-phase 4C: Use TT for Cutoffs
                // Check if the stored depth is sufficient
                if (ttEntry->depth >= depth) {
                    eval::Score ttScore(ttEntry->score);
                    Bound ttBound = ttEntry->bound();
                    
                    // Sub-phase 4D: Mate Score Adjustment
                    // Adjust mate scores relative to current ply
                    if (ttScore.value() >= MATE_BOUND) {
                        // Positive mate score - we're winning
                        // Adjust distance to mate from current position
                        ttScore = eval::Score(ttScore.value() - ply);
                    } else if (ttScore.value() <= -MATE_BOUND) {
                        // Negative mate score - we're losing
                        // Adjust distance to mate from current position
                        ttScore = eval::Score(ttScore.value() + ply);
                    }
                    
                    // Handle different bound types
                    if (ttBound == Bound::EXACT) {
                        // Exact score - we can return immediately
                        info.ttCutoffs++;
                        return ttScore;
                    } else if (ttBound == Bound::LOWER) {
                        // Lower bound (fail-high) - score >= ttScore
                        if (ttScore >= beta) {
                            info.ttCutoffs++;
                            return ttScore;  // Beta cutoff
                        }
                        // Update alpha if we have a better lower bound
                        if (ttScore > alpha) {
                            alpha = ttScore;
                        }
                    } else if (ttBound == Bound::UPPER) {
                        // Upper bound (fail-low) - score <= ttScore  
                        if (ttScore <= alpha) {
                            info.ttCutoffs++;
                            return ttScore;  // Alpha cutoff
                        }
                        // Update beta if we have a better upper bound
                        if (ttScore < beta) {
                            beta = ttScore;
                        }
                    }
                    
                    // Check if window is now invalid after adjustments
                    if (alpha >= beta) {
                        info.ttCutoffs++;
                        return alpha;  // Window closed
                    }
                }
                
                // Sub-phase 4E: Extract TT move for ordering
                // Even if depth is insufficient, we can still use the move
                ttMove = static_cast<Move>(ttEntry->move);
                if (ttMove != NO_MOVE) {
                    // Bug #013 fix: Validate TT move has valid squares before using
                    Square from = moveFrom(ttMove);
                    Square to = moveTo(ttMove);
                    if (from < 64 && to < 64 && from != to) {
                        info.ttMoveHits++;
                    } else {
                        // TT move is corrupted - likely from hash collision
                        ttMove = NO_MOVE;
                        info.ttCollisions++; // Track this as a collision
                        std::cerr << "WARNING: Corrupted TT move detected: " 
                                  << std::hex << ttEntry->move << std::dec << std::endl;
                    }
                }
            }
        }
    }
    
    // Stage 21 Phase A4: Null Move Pruning with Static Null Move and Tuning
    // Constants for null move pruning
    constexpr eval::Score ZUGZWANG_THRESHOLD = eval::Score(500 + 330);  // Rook + Bishop value (original)
    
    // Phase A4: Static null move pruning (reverse futility) for shallow depths
    // This is a lightweight check before the more expensive null move search
    if (!isPvNode && depth <= 3 && depth > 0 && !weAreInCheck && std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {
        // Only evaluate if we haven't already
        eval::Score staticEval = eval::Score::zero();
        
        // Try to get cached eval first
        if (ply > 0) {
            int cachedEval = searchInfo.getStackEntry(ply).staticEval;
            if (cachedEval != 0) {
                staticEval = eval::Score(cachedEval);
            }
        }
        
        // If no cached eval and we're likely to benefit, compute it
        if (staticEval == eval::Score::zero()) {
            // Only evaluate if we think we might get a cutoff
            // Quick material balance check first
            if (board.material().balance(board.sideToMove()).value() - beta.value() > -200) {
                staticEval = board.evaluate();
                searchInfo.setStaticEval(ply, staticEval);
                
                // Margin based on depth (tunable)
                eval::Score margin = eval::Score(limits.nullMoveStaticMargin * depth);
                
                if (staticEval - margin >= beta) {
                    info.nullMoveStats.staticCutoffs++;
                    return staticEval - margin;  // Return reduced score for safety
                }
            }
        }
    }
    
    // Regular null move pruning
    // Check if we can do null move
    bool canDoNull = !isPvNode                                  // Phase P2: No null in PV nodes!
                    && !weAreInCheck                              // Not in check
                    && depth >= 3                                // Minimum depth
                    && ply > 0                                   // Not at root
                    && !searchInfo.wasNullMove(ply - 1)         // No consecutive nulls
                    && board.nonPawnMaterial(board.sideToMove()) > ZUGZWANG_THRESHOLD  // Original detection
                    && std::abs(beta.value()) < MATE_BOUND - MAX_PLY;  // Not near mate
    
    if (canDoNull && limits.useNullMove) {
        info.nullMoveStats.attempts++;
        
        // Phase A3.1: Simple depth-based adaptive reduction (no expensive eval call)
        // R=2 for depth<6, R=3 for depth 6-11, R=4 for depth 12+
        int nullMoveReduction = 2 + (depth >= 6) + (depth >= 12);
        
        // Ensure we don't reduce too much
        nullMoveReduction = std::min(nullMoveReduction, depth - 1);
        
        // Make null move
        Board::UndoInfo nullUndo;
        board.makeNullMove(nullUndo);
        searchInfo.setNullMove(ply, true);
        
        // Search with adaptive reduction (null window around beta)
        eval::Score nullScore = -negamax(
            board,
            depth - 1 - nullMoveReduction,
            ply + 1,
            -beta,
            -beta + eval::Score(1),
            searchInfo,
            info,
            limits,
            tt,
            false  // Phase P2: Null move searches are never PV nodes
        );
        
        // Unmake null move
        board.unmakeNullMove(nullUndo);
        searchInfo.setNullMove(ply, false);
        
        // Check for cutoff
        if (nullScore >= beta) {
            info.nullMoveStats.cutoffs++;
            
            // Phase A3.2a: No verification search - rely on good zugzwang detection
            // Don't return mate scores from null search
            if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
                return nullScore;
            } else {
                // Mate score, return beta instead
                return beta;
            }
        }
    } else if (!canDoNull && ply > 0 && board.nonPawnMaterial(board.sideToMove()) <= ZUGZWANG_THRESHOLD) {
        // Track when we avoid null move due to zugzwang
        info.nullMoveStats.zugzwangAvoids++;
    }
    
    // In quiescence search (depth <= 0), handle differently
    if (depth <= 0) {
        // Get info about last move
        bool lastMoveWasCapture = false;
        if (ply > 0 && ply - 1 < 128) {
            Move lastMove = searchInfo.getStackEntry(ply - 1).move;
            if (lastMove != NO_MOVE) {
                lastMoveWasCapture = isCapture(lastMove);
            }
        }
        
        // Only check insufficient material after captures in qsearch
        if (lastMoveWasCapture && board.isInsufficientMaterial()) {
            return eval::Score::draw();
        }
    }
    
    // Order moves for better alpha-beta pruning
    // TT move first, then promotions (especially queen), then captures (MVV-LVA), then killers, then quiet moves
    orderMoves(board, moves, ttMove, &info, ply);
    
    // Debug output at root for deeper searches
    if (ply == 0 && depth >= 4) {
        std::cerr << "Root: generated " << moves.size() << " moves, depth=" << depth << "\n";
    }
    
    
    // Debug: Validate board state before search
#ifdef DEBUG
    Hash hashBefore = board.zobristKey();
    int pieceCountBefore = __builtin_popcountll(board.occupied());
#endif
    
    // Sub-phase 5A-5E: Store preparation
    // Store original alpha for bound determination
    eval::Score alphaOrig = alpha;
    
    // Initialize best score
    eval::Score bestScore = eval::Score::minus_infinity();
    Move bestMove = NO_MOVE;  // Track best move for all plies (needed for TT storage later)
    
    // Search all moves
    int moveCount = 0;
    
    // Stage 20 Phase B3 Fix: Track quiet moves for butterfly history update
    std::vector<Move> quietMoves;
    quietMoves.reserve(moves.size());
    
    for (const Move& move : moves) {
        moveCount++;
        info.totalMoves++;  // Track total moves examined
        
        // Track quiet moves for history update
        if (!isCapture(move) && !isPromotion(move)) {
            quietMoves.push_back(move);
        }
        
        // Push position to search stack BEFORE making the move
        searchInfo.pushSearchPosition(board.zobristKey(), move, ply);
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        // Phase P3: Principal Variation Search (PVS) with LMR integration
        eval::Score score;
        
        if (moveCount == 1) {
            // First move: search with full window as PV node
            score = -negamax(board, depth - 1, ply + 1,
                            -beta, -alpha, searchInfo, info, limits, tt,
                            isPvNode);  // Phase P3: First move inherits PV status
        } else {
            // Later moves: use PVS with LMR
            
            // Phase 3: Late Move Reductions (LMR)
            int reduction = 0;
            
            // Calculate LMR reduction (don't reduce at root)
            if (ply > 0 && info.lmrParams.enabled && depth >= info.lmrParams.minDepth) {
                // Determine move properties for LMR
                bool captureMove = isCapture(move);
                bool givesCheck = false;  // Phase 3: Skip gives-check detection for now
                bool pvNode = isPvNode;   // Phase P3: Use actual PV status
                
                // Check if we should reduce this move
                if (shouldReduceMove(depth, moveCount, captureMove, 
                                    weAreInCheck, givesCheck, pvNode, 
                                    info.lmrParams)) {
                    // Calculate reduction amount
                    reduction = getLMRReduction(depth, moveCount, info.lmrParams);
                    
                    // Track LMR statistics
                    info.lmrStats.totalReductions++;
                }
            }
            
            // Scout search (possibly reduced)
            info.pvsStats.scoutSearches++;
            score = -negamax(board, depth - 1 - reduction, ply + 1,
                            -(alpha + eval::Score(1)), -alpha, searchInfo, info, limits, tt,
                            false);  // Scout searches are not PV
            
            // If reduced scout fails high, re-search without reduction
            if (score > alpha && reduction > 0) {
                info.lmrStats.reSearches++;
                score = -negamax(board, depth - 1, ply + 1,
                                -(alpha + eval::Score(1)), -alpha, searchInfo, info, limits, tt,
                                false);  // Still a scout search
            }
            
            // If scout search fails high, do full PV re-search
            if (score > alpha) {
                info.pvsStats.reSearches++;
                score = -negamax(board, depth - 1, ply + 1,
                                -beta, -alpha, searchInfo, info, limits, tt,
                                isPvNode);  // CRITICAL: Re-search as PV node!
            } else if (reduction > 0 && score <= alpha) {
                // Reduction was successful (move was bad as expected)
                info.lmrStats.successfulReductions++;
            }
        }
        
        // Unmake the move
        board.unmakeMove(move, undo);
        
        // Debug: Validate board state is properly restored
#ifdef DEBUG
        assert(board.zobristKey() == hashBefore);
        assert(__builtin_popcountll(board.occupied()) == pieceCountBefore);
#endif
        
        // Check if search was interrupted
        if (info.stopped) {
            return bestScore;
        }
        
        // Update best score and move
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            
            // At root, store the best move in SearchInfo
            if (ply == 0) {
                info.bestMove = move;
                info.bestScore = score;
                
                // Debug output for root moves (removed in release builds)
                #ifndef NDEBUG
                if (depth >= 3) {
                    std::cerr << "Root: Move " << SafeMoveExecutor::moveToString(move) 
                              << " score=" << score.to_cp() << " cp" << std::endl;
                }
                #endif
            }
            
            // Update alpha (best score we can guarantee)
            if (score > alpha) {
                alpha = score;
                
                // Beta cutoff - prune remaining moves
                // This is a fail-high, meaning the opponent won't choose this position
                // Return bestScore for fail-soft alpha-beta
                if (score >= beta) [[unlikely]] {
                    info.betaCutoffs++;  // Track beta cutoffs
                    if (moveCount == 1) {
                        info.betaCutoffsFirst++;  // Track first-move cutoffs
                    }
                    
                    // Stage 19, Phase A3: Update killer moves for quiet moves that cause cutoffs
                    // Stage 20, Phase B3: Update history for quiet moves that cause cutoffs
                    if (!isCapture(move) && !isPromotion(move)) {
                        info.killers.update(ply, move);
                        
                        // Update history with depth-based bonus for cutoff move
                        Color side = board.sideToMove();
                        info.history.update(side, moveFrom(move), moveTo(move), depth);
                        
                        // Butterfly update: penalize quiet moves tried before the cutoff
                        for (const Move& quietMove : quietMoves) {
                            if (quietMove != move) {  // Don't penalize the cutoff move itself
                                info.history.updateFailed(side, moveFrom(quietMove), moveTo(quietMove), depth);
                            }
                        }
                    }
                    
                    break;  // Beta cutoff - no need to search more moves
                }
            }
        }
    }
    
    // Sub-phase 5A: Basic Store Implementation
    // Store position in TT after search completes
    // Only store if we have a valid TT and didn't terminate early
    if (tt && tt->isEnabled() && !info.stopped && bestMove != NO_MOVE) {
        // Sub-phase 5D: Skip storing at root position
        if (ply > 0) {
            Hash zobristKey = board.zobristKey();
            
            // Sub-phase 5B: Determine bound type based on score relative to original window
            Bound bound;
            if (bestScore <= alphaOrig) {
                // Fail-low: All moves were worse than alpha
                bound = Bound::UPPER;
            } else if (bestScore >= beta) {
                // Fail-high: We found a move better than beta (beta cutoff)
                bound = Bound::LOWER;
            } else {
                // Exact: Score is within the original window
                bound = Bound::EXACT;
            }
            
            // Sub-phase 5C: Mate Score Adjustment for storing
            // Adjust mate scores to be relative to root (inverse of adjustMateScoreFromTT)
            eval::Score scoreToStore = bestScore;
            if (bestScore.value() >= MATE_BOUND) {
                // Positive mate score - we're winning
                // Store distance from root, not from current position
                scoreToStore = eval::Score(bestScore.value() + ply);
            } else if (bestScore.value() <= -MATE_BOUND) {
                // Negative mate score - we're losing
                // Store distance from root, not from current position
                scoreToStore = eval::Score(bestScore.value() - ply);
            }
            
            // Store the entry
            // For now, use the same score for both score and evalScore
            // In the future, we might want to store static eval separately
            tt->store(zobristKey, bestMove, scoreToStore.value(), scoreToStore.value(), 
                     static_cast<uint8_t>(depth), bound);
            info.ttStores++;
        }
    }
    
    return bestScore;
}

// Stage 13: Test wrapper for iterative deepening (Deliverable 1.2a)
// This function calls the existing search without modifications
// Used to verify that IterativeSearchData doesn't break anything
Move searchIterativeTest(Board& board, const SearchLimits& limits, TranspositionTable* tt) {
    // Stage 13, Deliverable 1.2c: Full iteration recording (all depths)
    // Stage 13, Deliverable 2.2a: Time management integration prep
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    
    IterativeSearchData info;  // Using new class instead of SearchData
    
    // Stage 14, Deliverable 1.8: Pass quiescence option to search
    info.useQuiescence = limits.useQuiescence;
    
    // Stage 14 Remediation: Parse SEE mode once at search start
    info.seePruningModeEnum = parseSEEPruningMode(limits.seePruningMode);
    
    // Stage 18: Initialize LMR parameters from limits
    info.lmrParams.enabled = limits.lmrEnabled;
    info.lmrParams.minDepth = limits.lmrMinDepth;
    info.lmrParams.minMoveNumber = limits.lmrMinMoveNumber;
    info.lmrParams.baseReduction = limits.lmrBaseReduction;
    info.lmrParams.depthFactor = limits.lmrDepthFactor;
    info.lmrStats.reset();
    
    // Stage 22 Phase P3.5: Reset PVS statistics at search start
    info.pvsStats.reset();
    
    // Stage 13 Remediation: Set configurable stability threshold
    // Phase 4: Adjust for game phase if enabled
    if (limits.usePhaseStability) {
        GamePhase phase = detectGamePhase(board);
        int adjustedThreshold = getPhaseStabilityThreshold(phase, 
                                                          limits.stabilityThreshold,
                                                          limits.openingStability,
                                                          limits.middlegameStability,
                                                          limits.endgameStability,
                                                          true);  // Use specific values
        info.setRequiredStability(adjustedThreshold);
        
        // Debug output for phase detection
        #ifndef NDEBUG
        const char* phaseName = (phase == GamePhase::OPENING) ? "Opening" :
                               (phase == GamePhase::MIDDLEGAME) ? "Middlegame" : "Endgame";
        std::cerr << "[Phase Stability] Game phase: " << phaseName 
                  << ", stability threshold: " << adjustedThreshold << std::endl;
        #endif
    } else {
        info.setRequiredStability(limits.stabilityThreshold);
    }
    
    // Stage 13, Deliverable 2.2b: Switch to new time management
    // Calculate initial time limits with neutral stability (1.0)
    TimeLimits timeLimits = calculateTimeLimits(limits, board, 1.0);
    info.m_softLimit = timeLimits.soft.count();
    info.m_hardLimit = timeLimits.hard.count();
    info.m_optimumTime = timeLimits.optimum.count();
    
    // Use NEW calculation for timeLimit (replacing old)
    info.timeLimit = timeLimits.optimum;
    
    // Debug logging for time management (Deliverable 2.2b)
    if (limits.movetime == std::chrono::milliseconds(0)) {  // Only log for non-fixed time
        std::cerr << "[Time Management] Initial limits: ";
        std::cerr << "soft=" << info.m_softLimit << "ms, ";
        std::cerr << "hard=" << info.m_hardLimit << "ms, ";
        std::cerr << "optimum=" << info.m_optimumTime << "ms\n";
    }
    
    Move bestMove;
    Move previousBestMove = NO_MOVE;  // Track best move from previous iteration
    eval::Score previousScore = eval::Score::zero();  // Track score for aspiration windows
    
    // Same iterative deepening loop as original search
    for (int depth = 1; depth <= limits.maxDepth; depth++) {
        info.depth = depth;
        board.setSearchMode(true);
        
        // Track start time for this iteration
        auto iterationStart = std::chrono::steady_clock::now();
        uint64_t nodesBeforeIteration = info.nodes;  // Save node count before iteration
        
        // Stage 13, Deliverable 3.2b: Use aspiration window for depth >= 4
        eval::Score alpha, beta;
        AspirationWindow window;
        
        // Stage 13 Remediation: Use configurable aspiration windows
        if (limits.useAspirationWindows && 
            depth >= AspirationConstants::MIN_DEPTH && 
            previousScore != eval::Score::zero()) {
            // Calculate aspiration window based on previous score with configurable delta
            window = calculateInitialWindow(previousScore, depth, limits.aspirationWindow);
            alpha = window.alpha;
            beta = window.beta;
        } else {
            // Use infinite window for shallow depths or when disabled
            alpha = eval::Score::minus_infinity();
            beta = eval::Score::infinity();
        }
        
        eval::Score score = negamax(board, depth, 0, alpha, beta, searchInfo, info, limits, tt);
        
        // Stage 13, Deliverable 3.2d: Progressive widening re-search
        if (depth >= AspirationConstants::MIN_DEPTH && (score <= alpha || score >= beta)) {
            // Progressive widening instead of full window
            bool failedHigh = (score >= beta);
            window.failedLow = !failedHigh;
            window.failedHigh = failedHigh;
            
            // Progressive widening loop
            while (score <= alpha || score >= beta) {
                // Widen the window progressively with configurable growth mode
                WindowGrowthMode growthMode = parseWindowGrowthMode(limits.aspirationGrowth);
                window = widenWindow(window, score, failedHigh, limits.aspirationMaxAttempts, growthMode);
                alpha = window.alpha;
                beta = window.beta;
                
                // Re-search with widened window
                score = negamax(board, depth, 0, alpha, beta, searchInfo, info, limits, tt);
                
                // Check if we're now using an infinite window
                if (window.isInfinite()) {
                    break;  // No point in further widening
                }
                
                // Update fail direction if needed
                failedHigh = (score >= beta);
            }
        }
        
        board.setSearchMode(false);
        
        if (!info.stopped) {
            bestMove = info.bestMove;
            
            // Stage 13, Deliverable 5.1a: Use enhanced UCI info output
            sendIterationInfo(info);
            
            // Record iteration data for ALL depths (full recording)
            auto iterationEnd = std::chrono::steady_clock::now();
            auto iterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                iterationEnd - iterationStart).count();
            
            // Ensure minimum iteration time of 1ms for very fast searches
            if (iterationTime == 0) {
                iterationTime = 1;
            }
            
            IterationInfo iter;
            iter.depth = depth;
            iter.score = score;
            iter.bestMove = info.bestMove;
            iter.nodes = info.nodes - nodesBeforeIteration;  // Nodes for this iteration only
            iter.elapsed = iterationTime;
            // Record actual window used
            iter.alpha = alpha;
            iter.beta = beta;
            iter.windowAttempts = window.attempts;
            iter.failedHigh = window.failedHigh;
            iter.failedLow = window.failedLow;
            
            // Track move changes and stability
            if (depth == 1) {
                iter.moveChanged = false;  // No previous iteration
                iter.moveStability = 1;    // First occurrence
            } else {
                iter.moveChanged = (info.bestMove != previousBestMove);
                if (iter.moveChanged) {
                    iter.moveStability = 1;  // Reset stability counter
                } else {
                    // Same move as previous iteration - increment stability
                    const IterationInfo& prevIter = info.getLastIteration();
                    iter.moveStability = prevIter.moveStability + 1;
                }
            }
            
            iter.firstMoveFailHigh = false;
            iter.failHighMoveIndex = -1;
            iter.secondBestScore = eval::Score::minus_infinity();
            
            // Calculate branching factor if not first iteration
            if (depth > 1 && info.hasIterations()) {
                const IterationInfo& prevIter = info.getLastIteration();
                if (prevIter.nodes > 0) {
                    iter.branchingFactor = static_cast<double>(iter.nodes) / prevIter.nodes;
                } else {
                    iter.branchingFactor = 0.0;
                }
            } else {
                iter.branchingFactor = 0.0;
            }
            
            info.recordIteration(iter);
            info.updateStability(iter);  // Update stability tracking (Deliverable 2.1e)
            previousBestMove = info.bestMove;  // Update for next iteration
            previousScore = score;  // Save score for next iteration's aspiration window
            
            // Stage 22 Phase P3.5: Output PVS statistics if requested
            if (limits.showPVSStats && info.pvsStats.scoutSearches > 0) {
                std::cout << "info string PVS scout searches: " << info.pvsStats.scoutSearches << std::endl;
                std::cout << "info string PVS re-searches: " << info.pvsStats.reSearches << std::endl;
                std::cout << "info string PVS re-search rate: " 
                          << std::fixed << std::setprecision(1)
                          << info.pvsStats.reSearchRate() << "%" << std::endl;
            }
            
            // Stage 13, Deliverable 2.2b: Dynamic time management
            // Recalculate time limits based on current stability
            double stabilityFactor = info.getStabilityFactor();
            if (depth > 2 && stabilityFactor != 1.0) {  // Only adjust after depth 2
                TimeLimits adjustedLimits = calculateTimeLimits(limits, board, stabilityFactor);
                info.m_softLimit = adjustedLimits.soft.count();
                // Don't change hard limit - it's a safety bound
                
                // Log stability-based adjustments
                if (depth == 3 || depth == 5) {  // Log at specific depths to avoid spam
                    std::cerr << "[Time Management] Depth " << depth 
                              << ": stability=" << std::fixed << std::setprecision(1) 
                              << stabilityFactor << ", adjusted soft=" 
                              << info.m_softLimit << "ms\n";
                }
            }
            
            if (score.is_mate_score()) {
                break;
            }
            
            // Stage 13, Deliverable 4.2b: Enhanced early termination logic
            if (info.m_hardLimit > 0) {
                // Use actual elapsed time, not cached (for accurate time management)
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - info.startTime);
                bool stable = info.isPositionStable();
                
                // Check if we should stop based on new time management
                if (shouldStopOnTime(TimeLimits{std::chrono::milliseconds(info.m_softLimit),
                                               std::chrono::milliseconds(info.m_hardLimit),
                                               std::chrono::milliseconds(info.m_optimumTime)},
                                   elapsed, depth, stable)) {
                    std::cerr << "[Time Management] Stopping at depth " << depth 
                              << " (elapsed=" << elapsed.count() << "ms, "
                              << (stable ? "stable" : "unstable") << ")\n";
                    break;
                }
                
                // Enhanced time prediction using sophisticated EBF
                double sophisticatedEBF = info.getSophisticatedEBF();
                if (sophisticatedEBF <= 0) {
                    // Fall back to simple EBF or default
                    sophisticatedEBF = iter.branchingFactor > 0 ? iter.branchingFactor : 5.0;
                }
                
                // Predict time for next iteration
                auto predictedTime = predictNextIterationTime(
                    std::chrono::milliseconds(iterationTime),
                    sophisticatedEBF,
                    depth + 1
                );
                
                // Early termination decision factors:
                // 1. Would we exceed soft limit?
                // 2. Is position stable (less need for deeper search)?
                // 3. Have we reached reasonable depth?
                // 4. Is predicted time reasonable?
                
                bool exceedsSoftLimit = (elapsed + predictedTime) > std::chrono::milliseconds(info.m_softLimit);
                bool exceedsHardLimit = (elapsed + predictedTime) > std::chrono::milliseconds(info.m_hardLimit);
                bool reasonableDepth = depth >= 6;  // Minimum reasonable depth
                bool veryStable = stable && info.m_stabilityCount >= 3;
                
                // Decision logic:
                if (exceedsHardLimit) {
                    // Never exceed hard limit
                    std::cerr << "[Time Management] No time for depth " << (depth + 1)
                              << " (would exceed hard limit)\n";
                    break;
                } else if (exceedsSoftLimit) {
                    // Decide whether to exceed soft limit based on position characteristics
                    if (veryStable || (stable && reasonableDepth)) {
                        // Stop if position is very stable or stable at reasonable depth
                        std::cerr << "[Time Management] No time for depth " << (depth + 1)
                                  << " (would exceed soft limit, position stable/deep)\n";
                        break;
                    } else if (!stable && depth < 6) {
                        // Continue if unstable and not too deep
                        // This allows searching deeper in tactical positions
                        std::cerr << "[Time Management] Continuing despite soft limit "
                                  << "(depth=" << depth << ", unstable)\n";
                    } else {
                        // Default: stop at soft limit
                        std::cerr << "[Time Management] No time for depth " << (depth + 1)
                                  << " (would exceed soft limit)\n";
                        break;
                    }
                }
                
                // Additional early termination for very stable positions
                if (veryStable && depth >= 8 && predictedTime > std::chrono::milliseconds(2000)) {
                    std::cerr << "[Time Management] Early termination at depth " << depth
                              << " (very stable, deep enough, next iteration expensive)\n";
                    break;
                }
            }
        } else {
            break;
        }
    }
    
    // Stage 15: Report final SEE pruning statistics after search completes
    // Only report once at the end, not per iteration
    if (info.seeStats.totalCaptures > 0) {
        std::cout << "info string SEE pruning final: "
                  << info.seeStats.seePruned << "/" << info.seeStats.totalCaptures
                  << " captures pruned (" << std::fixed << std::setprecision(1) 
                  << info.seeStats.pruneRate() << "%)";
        
        // Determine mode from statistics patterns
        if (info.seeStats.conservativePrunes > 0 && info.seeStats.aggressivePrunes == 0) {
            std::cout << " [Conservative mode]";
        } else if (info.seeStats.aggressivePrunes > 0) {
            std::cout << " [Aggressive mode]";
            if (info.seeStats.equalExchangePrunes > 0) {
                std::cout << ", equal exchanges: " << info.seeStats.equalExchangePrunes;
            }
        }
        std::cout << std::endl;
    }
    
    return bestMove;
}

// Iterative deepening search controller (original)
Move search(Board& board, const SearchLimits& limits, TranspositionTable* tt) {
    // Debug output (removed in release builds)
    #ifndef NDEBUG
    std::cerr << "Search: Starting with maxDepth=" << limits.maxDepth << std::endl;
    std::cerr << "Search: movetime=" << limits.movetime.count() << "ms" << std::endl;
    #endif
    
    // Initialize search tracking
    SearchInfo searchInfo;  // For repetition detection
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());  // Capture current game history size
    
    SearchData info;  // For search statistics
    
    // Stage 20, Phase B3 Fix: Clear history only at search start,
    // not between iterative deepening iterations
    // History accumulates across ID iterations for better move ordering
    
    // Stage 14, Deliverable 1.8: Pass quiescence option to search
    info.useQuiescence = limits.useQuiescence;
    
    // Stage 14 Remediation: Parse SEE mode once at search start to avoid hot path parsing
    info.seePruningModeEnum = parseSEEPruningMode(limits.seePruningMode);
    
    // Stage 18: Initialize LMR parameters from limits
    info.lmrParams.enabled = limits.lmrEnabled;
    info.lmrParams.minDepth = limits.lmrMinDepth;
    info.lmrParams.minMoveNumber = limits.lmrMinMoveNumber;
    info.lmrParams.baseReduction = limits.lmrBaseReduction;
    info.lmrParams.depthFactor = limits.lmrDepthFactor;
    info.lmrStats.reset();
    
    // Stage 13, Deliverable 2.2b: Use new time management in regular search too
    TimeLimits timeLimits = calculateTimeLimits(limits, board, 1.0);
    info.timeLimit = timeLimits.optimum;
    std::cerr << "Search: using NEW time management - optimum=" << info.timeLimit.count() << "ms" << std::endl;
    
    Move bestMove;
    
    // Debug: Show search parameters (removed in release)
    #ifndef NDEBUG
    std::cerr << "Search: maxDepth=" << limits.maxDepth 
              << " movetime=" << limits.movetime.count() 
              << "ms" << std::endl;
    #endif
    
    // Iterative deepening loop
    for (int depth = 1; depth <= limits.maxDepth; depth++) {
        info.depth = depth;
        
        // Set search mode at depth 1
        board.setSearchMode(true);
#ifdef DEBUG
        if (depth == 1) {
            Board::resetCounters();
        }
#endif
        
        // Search with infinite window initially
        eval::Score score = negamax(board, depth, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, info, limits, tt);
        
        // Clear search mode after search
        board.setSearchMode(false);
        
        // Only update best move if search completed
        if (!info.stopped) {
            bestMove = info.bestMove;
            
            // Send UCI info about this iteration
            sendSearchInfo(info);
            
            // Stop early if we found a mate
            if (score.is_mate_score()) {
                // If we found a mate, no need to search deeper
                break;
            }
            
            // Check if we have enough time for another iteration
            // Use a simple heuristic: if we've used more than 40% of our time,
            // don't start another iteration
            if (info.timeLimit != std::chrono::milliseconds::max()) {
                auto elapsed = info.elapsed();
                if (elapsed * 5 > info.timeLimit * 2) {  // elapsed > 40% of limit
                    break;
                }
            }
        } else {
            // Time ran out during this iteration
            break;
        }
    }
    
#ifdef DEBUG
    // Print instrumentation counters at end of search - only in debug builds
    Board::printCounters();
#endif
    
    // Return the best move found
    // If no iterations completed, bestMove will be invalid
    return bestMove;
}

// Calculate time allocation for this move
std::chrono::milliseconds calculateTimeLimit(const SearchLimits& limits, 
                                            const Board& board) {
    // Fixed move time takes priority
    if (limits.movetime > std::chrono::milliseconds(0)) {
        return limits.movetime;
    }
    
    // Infinite analysis mode
    if (limits.infinite) {
        return std::chrono::milliseconds::max();
    }
    
    // Game time management
    Color stm = board.sideToMove();
    auto remaining = limits.time[stm];
    
    // If no time specified, use a default
    if (remaining == std::chrono::milliseconds(0)) {
        return std::chrono::milliseconds(5000);  // 5 seconds default
    }
    
    // Simple time allocation:
    // Use 5% of remaining time + 75% of increment
    auto base = remaining / 20;  // 5% of remaining
    auto increment = limits.inc[stm] * 3 / 4;  // 75% of increment
    auto allocated = base + increment;
    
    // Apply safety bounds
    // Minimum 5ms to do something
    allocated = std::max(allocated, std::chrono::milliseconds(5));
    
    // Never use more than 25% of remaining time in one move
    auto maxTime = remaining / 4;
    allocated = std::min(allocated, maxTime);
    
    // Keep at least 50ms buffer
    if (remaining > std::chrono::milliseconds(100)) {
        allocated = std::min(allocated, remaining - std::chrono::milliseconds(50));
    }
    
    return allocated;
}

// Send UCI info output
void sendSearchInfo(const SearchData& info) {
    std::cout << "info"
              << " depth " << info.depth
              << " seldepth " << info.seldepth;
    
    // Output score
    if (info.bestScore.is_mate_score()) {
        // Mate score - calculate moves to mate
        int mateIn = 0;
        if (info.bestScore > eval::Score::zero()) {
            // We have a mate
            mateIn = (eval::Score::mate().value() - info.bestScore.value() + 1) / 2;
        } else {
            // We are being mated
            mateIn = -(eval::Score::mate().value() + info.bestScore.value()) / 2;
        }
        std::cout << " score mate " << mateIn;
    } else {
        std::cout << " score cp " << info.bestScore.to_cp();
    }
    
    std::cout << " nodes " << info.nodes
              << " nps " << info.nps()
              << " time " << info.elapsed().count();
    
    // Add search efficiency statistics (only if we have beta cutoffs)
    if (info.betaCutoffs > 0) {
        std::cout << " ebf " << std::fixed << std::setprecision(2) 
                  << info.effectiveBranchingFactor()
                  << " moveeff " << std::fixed << std::setprecision(1)
                  << info.moveOrderingEfficiency() << "%";
    }
    
    // Add TT statistics if we have probes
    if (info.ttProbes > 0) {
        double hitRate = (100.0 * info.ttHits) / info.ttProbes;
        std::cout << " tthits " << std::fixed << std::setprecision(1) << hitRate << "%";
        if (info.ttCutoffs > 0) {
            std::cout << " ttcuts " << info.ttCutoffs;
        }
        if (info.ttStores > 0) {
            std::cout << " ttstores " << info.ttStores;
        }
    }
    
    // Output principal variation (just the best move for now)
    // Bug #013 fix: Validate move is legal before outputting to prevent illegal PV moves
    if (info.bestMove != Move()) {
        // Quick validation - check that from and to squares are valid
        Square from = moveFrom(info.bestMove);
        Square to = moveTo(info.bestMove);
        if (from < 64 && to < 64 && from != to) {
            std::cout << " pv " << SafeMoveExecutor::moveToString(info.bestMove);
        } else {
            // Log warning about corrupted move but don't output it
            std::cerr << "WARNING: Corrupted bestMove detected in sendUCIInfo: " 
                      << std::hex << info.bestMove << std::dec << std::endl;
        }
    }
    
    std::cout << std::endl;
}

// Stage 13, Deliverable 5.1a: Enhanced UCI info output with iteration details
void sendIterationInfo(const IterativeSearchData& info) {
    // Basic info
    std::cout << "info"
              << " depth " << info.depth
              << " seldepth " << info.seldepth;
    
    // Score output
    if (info.bestScore.is_mate_score()) {
        int mateIn = 0;
        if (info.bestScore > eval::Score::zero()) {
            mateIn = (eval::Score::mate().value() - info.bestScore.value() + 1) / 2;
        } else {
            mateIn = -(eval::Score::mate().value() + info.bestScore.value()) / 2;
        }
        std::cout << " score mate " << mateIn;
    } else {
        std::cout << " score cp " << info.bestScore.to_cp();
    }
    
    // Iteration-specific information
    if (info.hasIterations()) {
        const auto& lastIter = info.getLastIteration();
        
        // Stage 13, Deliverable 5.1b: Aspiration window reporting
        if (lastIter.depth >= AspirationConstants::MIN_DEPTH) {
            // Report window information
            if (lastIter.windowAttempts > 0) {
                // Window was used and there were re-searches
                if (lastIter.failedHigh) {
                    std::cout << " string fail-high(" << lastIter.windowAttempts << ")";
                } else if (lastIter.failedLow) {
                    std::cout << " string fail-low(" << lastIter.windowAttempts << ")";
                }
            }
            
            // Show window bounds for debugging (optional)
            if (lastIter.alpha != eval::Score::minus_infinity() || 
                lastIter.beta != eval::Score::infinity()) {
                std::cout << " bound [" 
                          << (lastIter.alpha == eval::Score::minus_infinity() ? "-inf" : 
                              std::to_string(lastIter.alpha.to_cp()))
                          << "," 
                          << (lastIter.beta == eval::Score::infinity() ? "inf" : 
                              std::to_string(lastIter.beta.to_cp()))
                          << "]";
            }
        }
        
        // Stability indicator
        if (info.getIterationCount() >= 3) {
            if (info.isPositionStable()) {
                std::cout << " string stable";
            } else if (info.shouldExtendDueToInstability()) {
                std::cout << " string unstable";
            }
        }
        
        // Effective branching factor from sophisticated calculation
        double ebf = info.getSophisticatedEBF();
        if (ebf > 0) {
            std::cout << " ebf " << std::fixed << std::setprecision(2) << ebf;
        }
    }
    
    // Standard statistics
    std::cout << " nodes " << info.nodes
              << " nps " << info.nps()
              << " time " << info.elapsed().count();
    
    // Move ordering efficiency
    if (info.betaCutoffs > 0) {
        std::cout << " moveeff " << std::fixed << std::setprecision(1)
                  << info.moveOrderingEfficiency() << "%";
    }
    
    // TT statistics
    if (info.ttProbes > 0) {
        double hitRate = (100.0 * info.ttHits) / info.ttProbes;
        std::cout << " tthits " << std::fixed << std::setprecision(1) << hitRate << "%";
    }
    
    // Principal variation
    // Bug #013 fix: Validate move is legal before outputting to prevent illegal PV moves
    if (info.bestMove != Move()) {
        // Quick validation - check that from and to squares are valid
        Square from = moveFrom(info.bestMove);
        Square to = moveTo(info.bestMove);
        if (from < 64 && to < 64 && from != to) {
            std::cout << " pv " << SafeMoveExecutor::moveToString(info.bestMove);
        } else {
            // Log warning about corrupted move but don't output it
            std::cerr << "WARNING: Corrupted bestMove detected in sendIterationInfo: " 
                      << std::hex << info.bestMove << std::dec << std::endl;
        }
    }
    
    std::cout << std::endl;
}

} // namespace seajay::search