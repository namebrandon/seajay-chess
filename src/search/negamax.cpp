#include "negamax.h"
#include "search_info.h"
#include "iterative_search_data.h"  // Stage 13 addition
#include "time_management.h"        // Stage 13, Deliverable 2.2a
#include "aspiration_window.h"       // Stage 13, Deliverable 3.2b
#include "window_growth_mode.h"      // Stage 13 Remediation Phase 4
#include "game_phase.h"              // Stage 13 Remediation Phase 4
#include "move_ordering.h"  // Stage 11: MVV-LVA ordering (always enabled)
#include "lmr.h"            // Stage 18: Late Move Reductions
#include "principal_variation.h"     // PV tracking infrastructure
#include "countermove_history.h"     // Phase 4.3.a: Counter-move history
#include "../core/board.h"
#include "../core/board_safety.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../core/engine_config.h"    // Phase 4: Runtime configuration
#include "../evaluation/evaluate.h"
#include "../uci/info_builder.h"     // Phase 5: Structured info building
#include "quiescence.h"  // Stage 14: Quiescence search
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <vector>
#include <memory>

namespace seajay::search {

// Phase 3.2: Helper for move generation with lazy legality checking option
inline MoveList generateLegalMoves(const Board& board) {
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    return moves;
}

inline MoveList generateMovesForSearch(const Board& board, bool checkLegal = false) {
    MoveList moves;
    MoveGenerator::generateMovesForSearch(board, moves, checkLegal);
    return moves;
}

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
                      const SearchData* searchData = nullptr, int ply = 0,
                      Move prevMove = NO_MOVE, int countermoveBonus = 0,
                      const SearchLimits* limits = nullptr, int depth = 0) noexcept {
    // Stage 11: Base MVV-LVA ordering (always available)
    // Stage 15: Optional SEE ordering for captures when enabled via UCI
    // Stage 19/20/23: Killers, history, countermoves for quiets
    static MvvLvaOrdering mvvLva;
    
    // Phase 4.3.a-fix2: Depth gating - only use CMH at depth >= 6 to avoid noise
    // At shallow depths, the CMH table is "cold" and adds more noise than signal
    // Increased from 4 to 6 for better performance at fast time controls
    // Optional SEE capture ordering first (prefix-only) when enabled
    if (search::g_seeMoveOrdering.getMode() != search::SEEMode::OFF) {
        // SEE policy orders only captures/promotions prefix; quiets preserved
        search::g_seeMoveOrdering.orderMoves(board, moves);
    } else {
        // Default capture ordering
        mvvLva.orderMoves(board, moves);
    }

    if (depth >= 6 && searchData != nullptr && searchData->killers && searchData->history && 
        searchData->counterMoves && searchData->counterMoveHistory) {
        // Use counter-move history for enhanced move ordering at sufficient depth
        // Get CMH weight from search limits - should always be available
        float cmhWeight = 1.5f;  // default fallback (shouldn't be hit)
        if (limits != nullptr) {
            cmhWeight = limits->counterMoveHistoryWeight;
        } else {
            // This shouldn't happen in normal search - log if in debug mode
            #ifdef DEBUG
            std::cerr << "Warning: SearchLimits not available in orderMoves, using default CMH weight" << std::endl;
            #endif
        }
        mvvLva.orderMovesWithHistory(board, moves, *searchData->killers, *searchData->history,
                                    *searchData->counterMoves, *searchData->counterMoveHistory,
                                    prevMove, ply, countermoveBonus, cmhWeight);
    } else if (searchData != nullptr && searchData->killers && searchData->history && searchData->counterMoves) {
        // Fallback to basic countermoves without history
        mvvLva.orderMovesWithHistory(board, moves, *searchData->killers, *searchData->history,
                                    *searchData->counterMoves, prevMove, ply, countermoveBonus);
    }

    // Sub-phase 4E: TT Move Ordering
    // Put TT move first AFTER all other ordering (only do this once!)
    if (ttMove != NO_MOVE) {
        auto it = std::find(moves.begin(), moves.end(), ttMove);
        if (it != moves.end() && it != moves.begin()) {
            // Move TT move to front
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
                   TriangularPV* pv,
                   bool isPvNode) {
    
    
    // Debug output at root for deeper searches
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
    
    
    // Phase 1 & 2, enhanced in Phase 6: Check for periodic UCI info updates with smart throttling
    // Check periodically regardless of ply depth (since we spend most time at deeper plies)
    if (info.nodes > 0 && (info.nodes & 0xFFF) == 0) {
        // Use virtual method instead of expensive dynamic_cast
        // This eliminates RTTI overhead in the hot path
        if (info.isIterativeSearch()) {
            auto* iterativeInfo = static_cast<IterativeSearchData*>(&info);
            if (iterativeInfo->shouldSendInfo(true)) {  // Phase 6: Check with score change flag
                // UCI Score Conversion FIX: Use root side-to-move stored in SearchData
                sendCurrentSearchInfo(*iterativeInfo, info.rootSideToMove, tt);
                iterativeInfo->recordInfoSent(iterativeInfo->bestScore);  // Phase 6: Pass current score
            }
        }
    }
    
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
            return quiescence(board, ply, 0, alpha, beta, searchInfo, info, limits, *tt, 0, inPanicMode);
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
    
    // Initialize TT move and info for singular extensions
    Move ttMove = NO_MOVE;
    eval::Score ttScore = eval::Score::zero();
    Bound ttBound = Bound::NONE;
    int ttDepth = -1;
    
    // Phase 4.2.c: Compute static eval early and preserve it for TT storage
    // This will be the true static evaluation, not the search score
    eval::Score staticEval = eval::Score::zero();
    bool staticEvalComputed = false;
    
    // Check if we're in check (needed for null move and other decisions)
    bool weAreInCheck = inCheck(board);
    
    // Check extension: Extend search by 1 ply when in check
    // This is a fundamental search extension present in all competitive engines
    // It helps the engine see through forcing sequences and avoid horizon effects
    if (weAreInCheck) {
        depth++;
    }
    
    // Phase 3.2: Generate pseudo-legal moves for lazy validation in search
    // We still need to know if we have any legal moves for checkmate/stalemate
    MoveList moves = generateMovesForSearch(board, false);  // Pseudo-legal moves
    
    // Note: We can't check for checkmate/stalemate here yet because we have pseudo-legal moves
    // We'll check after trying all moves below
    
    // Sub-phase 4B: Establish correct probe order
    // 1. Check repetition FIRST (fastest, most common draw in search)
    // 2. Check fifty-move rule SECOND (less common)
    // 3. Only THEN probe TT (after all draw conditions)
    
    // Never check draws or probe TT at root
    if (ply > 0) {
        // PERFORMANCE OPTIMIZATION: Strategic draw checking
        bool shouldCheckDraw = false;
        
        // Get info about last move from search stack (if available)
        bool lastMoveWasCapture = false;
        if (ply > 0 && ply - 1 < 128) {
            Move lastMove = searchInfo.getStackEntry(ply - 1).move;
            if (lastMove != NO_MOVE) {
                lastMoveWasCapture = isCapture(lastMove);
            }
        }
        
        shouldCheckDraw = 
            weAreInCheck ||                         // Always after checks (use cached value)
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
            tt->prefetch(zobristKey);  // Prefetch TT entry to hide memory latency
            ttEntry = tt->probe(zobristKey);
            info.ttProbes++;
            
            if (ttEntry && ttEntry->key32 == (zobristKey >> 32)) {
                info.ttHits++;
                
                // Sub-phase 4C: Use TT for Cutoffs
                // Check if the stored depth is sufficient
                if (ttEntry->depth >= depth) {
                    eval::Score ttScore(ttEntry->score);
                    ttBound = ttEntry->bound();  // Update outer variable, don't shadow
                    
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
                
                // Sub-phase 4E: Extract TT move and info for ordering and singular extensions
                // Even if depth is insufficient, we can still use the move
                ttMove = static_cast<Move>(ttEntry->move);
                if (ttMove != NO_MOVE) {
                    // Bug #013 fix: Validate TT move has valid squares before using
                    Square from = moveFrom(ttMove);
                    Square to = moveTo(ttMove);
                    if (from < 64 && to < 64 && from != to) {
                        info.ttMoveHits++;
                        // Save TT info for singular extensions
                        ttScore = eval::Score(ttEntry->score);
                        ttBound = ttEntry->bound();
                        ttDepth = ttEntry->depth;
                    } else {
                        // TT move is corrupted - likely from hash collision
                        ttMove = NO_MOVE;
                        info.ttCollisions++; // Track this as a collision
                        std::cerr << "WARNING: Corrupted TT move detected: " 
                                  << std::hex << ttEntry->move << std::dec << std::endl;
                    }
                }
                
                // Phase 4.2.c: Try to get static eval from TT if available
                // The evalScore field should contain the true static evaluation
                if (!staticEvalComputed && ttEntry->evalScore != TT_EVAL_NONE) {
                    staticEval = eval::Score(ttEntry->evalScore);
                    staticEvalComputed = true;
                }
            }
        }
    }
    
    // Phase 4.2.c: Compute static eval if not in check and haven't gotten it from TT
    // We need this for pruning decisions and to store in TT later
    if (!staticEvalComputed && !weAreInCheck) {
        staticEval = board.evaluate();
        staticEvalComputed = true;
        // Cache it in search stack for potential reuse
        searchInfo.setStaticEval(ply, staticEval);
    }
    
    // Stage 21 Phase A4: Null Move Pruning with Static Null Move and Tuning
    // Constants for null move pruning
    constexpr eval::Score ZUGZWANG_THRESHOLD = eval::Score(500 + 330);  // Rook + Bishop value (original)
    
    // Phase 1.2: Enhanced reverse futility pruning (static null move)
    // Extended to depth 8 with more aggressive shallow margins
    if (!isPvNode && depth <= 8 && depth > 0 && !weAreInCheck && std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {
        // Phase 4.2.c: Use our pre-computed static eval
        if (staticEvalComputed) {
            // More aggressive margin at shallow depths, conservative at deeper
            // depth 1: 85, 2: 170, 3: 255, 4: 320, 5: 375, 6: 420, 7: 455, 8: 480
            int baseMargin = 85;
            eval::Score margin;
            if (depth <= 3) {
                margin = eval::Score(baseMargin * depth);
            } else {
                // Slower growth for deeper depths
                margin = eval::Score(baseMargin * 3 + (depth - 3) * (baseMargin / 2));
            }
            
            if (staticEval - margin >= beta) {
                info.nullMoveStats.staticCutoffs++;
                return staticEval - margin;  // Return reduced score for safety
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
            nullptr,  // Phase PV1: Pass nullptr for now
            false  // Phase P2: Null move searches are never PV nodes
        );
        
        // Unmake null move
        board.unmakeNullMove(nullUndo);
        searchInfo.setNullMove(ply, false);
        
        // Check for cutoff
        if (nullScore >= beta) {
            info.nullMoveStats.cutoffs++;
            
            // Phase 1.5b: Deeper verification search at depth >= 10
            if (depth >= 10) {  // Lowered from 12 to match Laser's threshold
                // Verification search at depth - R (one ply deeper than null move)
                eval::Score verifyScore = negamax(
                    board,
                    depth - nullMoveReduction,  // depth - R (matches Stockfish/Laser)
                    ply,
                    beta - eval::Score(1),
                    beta,
                    searchInfo,
                    info,
                    limits,
                    tt,
                    nullptr,  // Phase PV1: Pass nullptr for now
                    false
                );
                
                if (verifyScore < beta) {
                    // Verification failed, don't trust null move
                    info.nullMoveStats.verificationFails++;
                    // Continue with normal search instead of returning
                } else {
                    // Verification passed, null move cutoff is valid
                    if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
                        return nullScore;
                    } else {
                        // Mate score, return beta instead
                        return beta;
                    }
                }
            } else {
                // Shallow depth, trust null move without verification
                if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
                    return nullScore;
                } else {
                    // Mate score, return beta instead
                    return beta;
                }
            }
        }
    } else if (!canDoNull && ply > 0 && board.nonPawnMaterial(board.sideToMove()) <= ZUGZWANG_THRESHOLD) {
        // Track when we avoid null move due to zugzwang
        info.nullMoveStats.zugzwangAvoids++;
    }
    
    // Phase 4.1: Razoring - DISABLED
    // Testing showed -39.45 Elo loss, likely due to:
    // 1. Interaction with our existing pruning techniques
    // 2. SeaJay's evaluation may not be accurate enough for razoring
    // 3. Our quiescence search implementation may differ from Laser's
    // Razoring requires very careful tuning and may not be suitable for SeaJay's current architecture
    
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
    
    // Get previous move for countermove lookup (CM3.2)
    Move prevMove = NO_MOVE;
    if (ply > 0) {
        prevMove = searchInfo.getStackEntry(ply - 1).move;
    }
    
    // CM4.1: Track countermove hit attempts
    if (prevMove != NO_MOVE && info.countermoveBonus > 0) {
        Move counterMove = info.counterMoves->getCounterMove(prevMove);
        if (counterMove != NO_MOVE) {
            // Check if countermove is in our move list
            if (std::find(moves.begin(), moves.end(), counterMove) != moves.end()) {
                info.counterMoveStats.hits++;  // We found a countermove
            }
        }
    }
    
    // Order moves for better alpha-beta pruning
    // TT move first, then promotions (especially queen), then captures (MVV-LVA), then killers, then quiet moves
    // CM3.3: Pass prevMove and bonus for countermove ordering
    // Phase 4.3.a-fix2: Pass depth for CMH gating
    orderMoves(board, moves, ttMove, &info, ply, prevMove, info.countermoveBonus, &limits, depth);
    
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
    int legalMoveCount = 0;  // Phase 3.2: Track legal moves for checkmate/stalemate
    
    // Stage 20 Phase B3 Fix: Track quiet moves for butterfly history update
    std::vector<Move> quietMoves;
    quietMoves.reserve(moves.size());
    
    for (const Move& move : moves) {
        // Skip excluded move (for singular extension search)
        if (searchInfo.isExcluded(ply, move)) {
            continue;
        }
        
        // Phase B1: Don't increment moveCount here - wait until we know move is legal
        info.totalMoves++;  // Track total moves examined
        
        // Phase 2: Track current root move for UCI currmove output
        if (ply == 0) {
            info.currentRootMove = move;
            info.currentRootMoveNumber = moveCount;
        }
        
        // Track quiet moves for history update
        if (!isCapture(move) && !isPromotion(move)) {
            quietMoves.push_back(move);
        }
        
        // Phase 2.1 FINAL: Futility Pruning - Now with UCI control (Phase 4 investigation)
        // Default settings match the tested optimal configuration (+37.63 ELO)
        // UCI options allow runtime adjustment for testing different depths and margins
        const auto& config = seajay::getConfig();
        if (config.useFutilityPruning && !isPvNode && depth <= config.futilityMaxDepth 
            && depth > 0 && !weAreInCheck && moveCount > 1
            && !isCapture(move) && !isPromotion(move)) {
            
            // Phase 4.2.c: Use our pre-computed static eval for consistency
            if (staticEvalComputed) {
                // Phase 1.1 FIXED: Capped margin growth for practical deep pruning
                // Linear scaling up to depth 4, then capped to prevent excessive margins
                // depth 1: 150, 2: 300, 3: 450, 4: 600, 5+: 600 (capped)
                // This allows pruning at deeper depths without being too aggressive
                int futilityMargin = config.futilityBase * std::min(depth, 4);
                
                // Prune if current position is so bad that even improving by margin won't help
                if (staticEval <= alpha - eval::Score(futilityMargin)) {
                    info.futilityPruned++;
                    int b = SearchData::PruneBreakdown::bucketForDepth(depth);
                    info.pruneBreakdown.futility[b]++;
                    continue;  // Skip this move
                }
            }
        }
        
        // Phase 3.1 CONSERVATIVE: Move Count Pruning (Late Move Pruning)
        // Only prune at depths 3+ to avoid tactical blindness at shallow depths
        // Much more conservative limits to avoid over-pruning
        if (limits.useMoveCountPruning && !isPvNode && !weAreInCheck && depth >= 3 && depth <= 8 && moveCount > 1
            && !isCapture(move) && !isPromotion(move) && !info.killers->isKiller(ply, move)) {
            
            // Phase 3.3: Countermove Consideration
            // Don't prune countermoves - they're often good responses
            if (prevMove != NO_MOVE && move == info.counterMoves->getCounterMove(prevMove)) {
                // This is a countermove, don't prune it
                // Skip the entire move count pruning block
            } else {
            
            // MODERATELY CONSERVATIVE depth-based move count limits
            // Starting at depth 3 to avoid shallow tactical issues
            // 25% less conservative than previous version for more pruning
            // Now configurable via UCI for SPSA tuning
            int moveCountLimit[9] = {
                999, // depth 0 (not used)
                999, // depth 1 (not used - too shallow)
                999, // depth 2 (not used - too shallow)  
                limits.moveCountLimit3,  // depth 3 - moderately conservative
                limits.moveCountLimit4,  // depth 4 - moderately conservative
                limits.moveCountLimit5,  // depth 5 - moderately conservative
                limits.moveCountLimit6,  // depth 6 - moderately conservative
                limits.moveCountLimit7,  // depth 7 - moderately conservative
                limits.moveCountLimit8   // depth 8 - moderately conservative
            };
            
            // Check if we're improving (compare to previous ply's eval)
            bool improving = false;
            if (ply >= 2) {
                int prevEval = searchInfo.getStackEntry(ply - 2).staticEval;
                int currEval = searchInfo.getStackEntry(ply).staticEval;
                if (prevEval != 0 && currEval != 0) {
                    improving = (currEval > prevEval);
                }
            }
            
            // Adjust limit based on improvement
            int limit = moveCountLimit[depth];
            if (!improving) {
                limit = (limit * limits.moveCountImprovingRatio) / 100;  // Configurable reduction ratio
            }
            
            // History-based adjustment (moderately conservative)
            int historyScore = info.history->getScore(board.sideToMove(), moveFrom(move), moveTo(move));
            if (historyScore > limits.moveCountHistoryThreshold) {  // Configurable threshold
                limit += limits.moveCountHistoryBonus;  // Configurable bonus
            }
            
            if (moveCount > limit) {
                info.moveCountPruned++;
                int b = SearchData::PruneBreakdown::bucketForDepth(depth);
                info.pruneBreakdown.moveCount[b]++;
                continue;  // Skip this move
            }
            } // End of else block for countermove check
        }
        
        // Singular Extension: DISABLED - Implementation needs redesign
        // The current implementation using excluded moves doesn't match how
        // successful engines (Laser, Stockfish) implement it. They iterate
        // through moves and test each one, rather than doing a single recursive
        // call with exclusion. This needs to be reimplemented properly.
        // 
        // For now, we'll just use check extensions which are proven to work.
        int extension = 0;
        
        // TODO: Reimplement singular extensions properly:
        // 1. Loop through all moves except TT move
        // 2. Search each at reduced depth with narrow window
        // 3. If all fail low, extend the TT move
        // See Laser engine for reference implementation
        
        #if 0  // DISABLED - needs proper implementation
        if (depth >= 7 && move == ttMove && ttBound == Bound::LOWER && 
            ttDepth >= depth - 3 && ply > 0 && !isPvNode &&
            std::abs(ttScore.value()) < MATE_BOUND - MAX_PLY) {
            // Implementation removed - was not working correctly
        }
        #endif
        
        // Track QxP and RxP attempts before making the move
        if (isCapture(move) && !isPromotion(move)) {
            Square fromSq = moveFrom(move);
            Square toSq = moveTo(move);
            Piece attacker = board.pieceAt(fromSq);
            Piece victim = board.pieceAt(toSq);
            PieceType attackerType = typeOf(attacker);
            PieceType victimType = typeOf(victim);
            
            if (victimType == PAWN) {
                if (attackerType == QUEEN) {
                    info.moveOrderingStats.qxpAttempts++;
                } else if (attackerType == ROOK) {
                    info.moveOrderingStats.rxpAttempts++;
                }
            }
        }
        
        // Track game phase
        int pieceCount = __builtin_popcountll(board.occupied());
        if (pieceCount > 28) {
            info.moveOrderingStats.openingNodes++;
        } else if (pieceCount >= 16) {
            info.moveOrderingStats.middlegameNodes++;
        } else {
            info.moveOrderingStats.endgameNodes++;
        }
        
        // Phase 3.2: Try to make the move with lazy legality checking
        Board::UndoInfo undo;
        if (!board.tryMakeMove(move, undo)) {
            // B0: Count illegal pseudo-legal moves
            if (legalMoveCount == 0) {
                info.illegalPseudoBeforeFirst++;
            }
            info.illegalPseudoTotal++;
            // Move is illegal (leaves king in check) - skip it
            continue;
        }
        
        // Move is legal
        legalMoveCount++;
        moveCount++;  // Phase B1: Increment moveCount only after confirming legality
        
        // Push position to search stack after we know move is legal
        searchInfo.pushSearchPosition(board.zobristKey(), move, ply);
        
        // Phase PV3: Create child PV for recursive calls
        // Only allocate if we're in a PV node and have a parent PV to update
        TriangularPV childPV;
        
        // Phase P3: Principal Variation Search (PVS) with LMR integration
        eval::Score score;
        
        // Phase B1: Use legalMoveCount to determine if this is the first LEGAL move
        if (legalMoveCount == 1) {
            // First move: search with full window as PV node (apply extension if any)
            // Pass childPV only if we're in a PV node and have a parent PV
            TriangularPV* firstMoveChildPV = (pv != nullptr && isPvNode) ? &childPV : nullptr;
            score = -negamax(board, depth - 1 + extension, ply + 1,
                            -beta, -alpha, searchInfo, info, limits, tt,
                            firstMoveChildPV,  // Phase PV3: Pass child PV for PV nodes
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
                
                // Check if we should reduce this move with improved conditions
                if (shouldReduceMove(move, depth, moveCount, captureMove, 
                                    weAreInCheck, givesCheck, pvNode, 
                                    *info.killers, *info.history, 
                                    *info.counterMoves, prevMove,
                                    ply, board.sideToMove(),
                                    info.lmrParams)) {
                    // Calculate reduction amount
                    // For now, assume not improving to be conservative
                    // (future enhancement: track eval history for proper improving detection)
                    bool improving = false; // Conservative: assume not improving
                    
                    reduction = getLMRReduction(depth, moveCount, info.lmrParams, pvNode, improving);
                    
                    // Track LMR statistics
                    info.lmrStats.totalReductions++;
                }
            }
            
            // Scout search (possibly reduced, with extension)
            info.pvsStats.scoutSearches++;
            score = -negamax(board, depth - 1 - reduction + extension, ply + 1,
                            -(alpha + eval::Score(1)), -alpha, searchInfo, info, limits, tt,
                            nullptr,  // Phase PV3: Scout searches don't need PV
                            false);  // Scout searches are not PV
            
            // If reduced scout fails high, re-search without reduction
            if (score > alpha && reduction > 0) {
                info.lmrStats.reSearches++;
                score = -negamax(board, depth - 1 + extension, ply + 1,
                                -(alpha + eval::Score(1)), -alpha, searchInfo, info, limits, tt,
                                nullptr,  // Phase PV3: Still a scout search
                                false);  // Still a scout search
            }
            
            // If scout search fails high, do full PV re-search
            if (score > alpha) {
                info.pvsStats.reSearches++;
                // For re-search, we need to collect PV if we're in a PV node
                TriangularPV* reSearchChildPV = (pv != nullptr && isPvNode) ? &childPV : nullptr;
                score = -negamax(board, depth - 1 + extension, ply + 1,
                                -beta, -alpha, searchInfo, info, limits, tt,
                                reSearchChildPV,  // Phase PV3: Re-search needs PV for PV nodes
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
            
            // Phase PV3: Update PV at all depths
            if (pv != nullptr && isPvNode) {
                // Update PV with best move and child's PV
                // childPV should have been populated by the successful search
                pv->updatePV(ply, move, &childPV);
            }
            
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
                    
                    // Track detailed move ordering statistics
                    auto& stats = info.moveOrderingStats;
                    
                    // Track which type of move caused cutoff
                    if (move == ttMove && ttMove != NO_MOVE) {
                        stats.ttMoveCutoffs++;
                    } else if (isCapture(move) || isPromotion(move)) {
                        if (moveCount == 1) {
                            stats.firstCaptureCutoffs++;
                        }
                        // Check if it's a bad capture (QxP or RxP)
                        Square fromSq = moveFrom(move);
                        Square toSq = moveTo(move);
                        Piece attacker = board.pieceAt(fromSq);
                        Piece victim = board.pieceAt(toSq);
                        PieceType attackerType = typeOf(attacker);
                        PieceType victimType = typeOf(victim);
                        
                        if (victimType == PAWN && (attackerType == QUEEN || attackerType == ROOK)) {
                            stats.badCaptureCutoffs++;
                            if (attackerType == QUEEN) {
                                stats.qxpCutoffs++;
                            } else {
                                stats.rxpCutoffs++;
                            }
                        }
                    } else if (info.killers->isKiller(ply, move)) {
                        stats.killerCutoffs++;
                    } else if (info.countermoveBonus > 0 && prevMove != NO_MOVE &&
                               move == info.counterMoves->getCounterMove(prevMove)) {
                        stats.counterMoveCutoffs++;
                    } else {
                        stats.quietCutoffs++;
                    }
                    
                    // Track cutoff by move index
                    int moveIndex = moveCount - 1;
                    if (moveIndex < 10) {
                        stats.cutoffsAtMove[moveIndex]++;
                    } else {
                        stats.cutoffsAfter10++;
                    }
                    
                    // Stage 19, Phase A3: Update killer moves for quiet moves that cause cutoffs
                    // Stage 20, Phase B3: Update history for quiet moves that cause cutoffs
                    // Stage 23, Phase CM2: Update countermoves (shadow mode - no ordering yet)
                    if (!isCapture(move) && !isPromotion(move)) {
                        info.killers->update(ply, move);
                        
                        // Update history with depth-based bonus for cutoff move
                        Color side = board.sideToMove();
                        info.history->update(side, moveFrom(move), moveTo(move), depth);
                        
                        // Butterfly update: penalize quiet moves tried before the cutoff
                        for (const Move& quietMove : quietMoves) {
                            if (quietMove != move) {  // Don't penalize the cutoff move itself
                                info.history->updateFailed(side, moveFrom(quietMove), moveTo(quietMove), depth);
                            }
                        }
                        
                        // Stage 23, Phase CM2: Update countermove table
                        // Shadow mode: update the table but don't use for ordering yet
                        if (ply > 0) {
                            Move prevMove = searchInfo.getStackEntry(ply - 1).move;
                            if (prevMove != NO_MOVE) {
                                info.counterMoves->update(prevMove, move);
                                info.counterMoveStats.updates++;  // Track shadow mode updates
                                
                                // Phase 4.3.a: Update counter-move history (simplified interface)
                                if (info.counterMoveHistory) {
                                    info.counterMoveHistory->update(prevMove, move, depth);
                                    
                                    // Penalize quiet moves that were tried but didn't cause cutoff
                                    for (const Move& quietMove : quietMoves) {
                                        if (quietMove != move) {  // Don't penalize the cutoff move itself
                                            info.counterMoveHistory->updateFailed(prevMove, quietMove, depth);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    break;  // Beta cutoff - no need to search more moves
                }
            }
        }
    }
    
    // Phase 3.2: Check for checkmate/stalemate after trying all moves
    if (legalMoveCount == 0) {
        if (weAreInCheck) {
            // Checkmate - return mate score
            return eval::Score(-32000 + ply);
        } else {
            // Stalemate - return draw score
            return eval::Score::draw();
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
            // Phase 4.2.c: Store the TRUE static eval, not the search score
            // This allows better eval reuse and improving position detection
            int16_t evalToStore = staticEvalComputed ? staticEval.value() : TT_EVAL_NONE;
            tt->store(zobristKey, bestMove, scoreToStore.value(), evalToStore, 
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
    
    // PERFORMANCE FIX: Allocate move ordering tables on stack
    // These were previously embedded in SearchData (42KB total)
    // Now allocated separately and pointed to (SearchData only 1KB)
    KillerMoves killerMoves;
    HistoryHeuristic historyHeuristic;
    CounterMoves counterMovesTable;
    
    // Phase 4.3.a: Allocate counter-move history on heap
    // Note: This is 32MB per thread, allocated on heap to avoid stack overflow
    std::unique_ptr<CounterMoveHistory> counterMoveHistoryPtr = std::make_unique<CounterMoveHistory>();
    
    // Connect pointers to stack-allocated objects
    info.killers = &killerMoves;
    info.history = &historyHeuristic;
    info.counterMoves = &counterMovesTable;
    info.counterMoveHistory = counterMoveHistoryPtr.get();
    
    // Phase PV1: Stack-allocate triangular PV array for future use
    // Currently passing nullptr to maintain existing behavior
    alignas(64) TriangularPV rootPV;
    
    // UCI Score Conversion FIX: Store root side-to-move for all UCI output
    // This MUST be used for all UCI output, not the changing board.sideToMove() during search
    info.rootSideToMove = board.sideToMove();
    
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
    
    // Stage 23, Phase CM2: Reset countermove statistics at search start
    info.counterMoveStats.reset();
    
    // Stage 23, Phase CM3.3: Initialize countermove bonus from limits
    info.countermoveBonus = limits.countermoveBonus;
    info.counterMoves->clear();  // Clear countermove table for fresh search
    
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
    
    // Advance TT generation for new search to enable proper replacement strategy
    if (tt) {
        tt->newSearch();
    }
    
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
        
        // Phase PV2: Pass rootPV to collect principal variation at root
        eval::Score score = negamax(board, depth, 0, alpha, beta, searchInfo, info, limits, tt,
                                   &rootPV);  // Phase PV2: Collect PV at root
        
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
                // Phase PV2: Pass rootPV for re-search at root
                score = negamax(board, depth, 0, alpha, beta, searchInfo, info, limits, tt,
                              &rootPV);  // Phase PV2: Collect PV at root re-search
                
                // Check if we're now using an infinite window
                if (window.isInfinite()) {
                    break;  // No point in further widening
                }
                
                // Update fail direction if needed
                failedHigh = (score >= beta);
            }
            // B0: Aggregate aspiration telemetry after re-search loop
            info.aspiration.attempts += static_cast<uint64_t>(window.attempts);
            if (window.failedLow) info.aspiration.failLow++;
            if (window.failedHigh) info.aspiration.failHigh++;
        }
        
        board.setSearchMode(false);
        
        if (!info.stopped) {
            bestMove = info.bestMove;
            
            // Stage 13, Deliverable 5.1a: Use enhanced UCI info output
            // Phase 6: Always send info at iteration completion and record it
            // Phase PV4: Pass rootPV for full principal variation display
            // UCI Score Conversion FIX: Use root side-to-move, not current position's side
            sendIterationInfo(info, info.rootSideToMove, tt, &rootPV);
            info.recordInfoSent(info.bestScore);  // Phase 6: Record to prevent immediate duplicate
            
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
            
            // Stage 23, Phase CM4.1: Enhanced countermove statistics
            if (info.counterMoveStats.updates > 0 || info.counterMoveStats.hits > 0) {
                double hitRate = info.counterMoveStats.updates > 0 
                    ? (100.0 * info.counterMoveStats.hits / info.counterMoveStats.updates) 
                    : 0.0;
                std::cout << "info string Countermoves: updates=" << info.counterMoveStats.updates 
                          << " hits=" << info.counterMoveStats.hits
                          << " hitRate=" << std::fixed << std::setprecision(1) << hitRate << "%"
                          << " bonus=" << info.countermoveBonus << std::endl;
            }
            
            // Output detailed move ordering statistics at depth 5 and 10
            if ((depth == 5 || depth == 10) && info.nodes > 1000) {
                auto& stats = info.moveOrderingStats;
                uint64_t totalCutoffs = stats.ttMoveCutoffs + stats.firstCaptureCutoffs + 
                                       stats.killerCutoffs + stats.counterMoveCutoffs +
                                       stats.quietCutoffs + stats.badCaptureCutoffs;
                
                // Always output stats for debugging
                std::cout << "info string MoveOrdering: totalCutoffs=" << totalCutoffs 
                          << " TT=" << stats.ttMoveCutoffs 
                          << " Cap=" << stats.firstCaptureCutoffs
                          << " Kill=" << stats.killerCutoffs 
                          << " nodes=" << info.nodes << std::endl;
                
                if (totalCutoffs > 0) {
                    std::cout << "info string MoveOrdering cutoffs: "
                              << std::fixed << std::setprecision(1)
                              << "TT=" << (100.0 * stats.ttMoveCutoffs / totalCutoffs) << "% "
                              << "1stCap=" << (100.0 * stats.firstCaptureCutoffs / totalCutoffs) << "% "
                              << "Killer=" << (100.0 * stats.killerCutoffs / totalCutoffs) << "% "
                              << "Counter=" << (100.0 * stats.counterMoveCutoffs / totalCutoffs) << "% "
                              << "Quiet=" << (100.0 * stats.quietCutoffs / totalCutoffs) << "% "
                              << "BadCap=" << (100.0 * stats.badCaptureCutoffs / totalCutoffs) << "%" << std::endl;
                    
                    // Output cutoff distribution by move index
                    std::cout << "info string Cutoff by index: ";
                    for (int i = 0; i < 5; i++) {
                        std::cout << "[" << i << "]=" << stats.cutoffDistribution(i) << "% ";
                    }
                    std::cout << "[5+]=" << (stats.cutoffDistribution(5) + stats.cutoffDistribution(6) + 
                                             stats.cutoffDistribution(7) + stats.cutoffDistribution(8) +
                                             stats.cutoffDistribution(9)) << "% "
                              << "[10+]=" << stats.cutoffDistribution(10) << "%" << std::endl;
                    
                    // Output QxP and RxP statistics
                    if (stats.qxpAttempts > 0 || stats.rxpAttempts > 0) {
                        std::cout << "info string Bad captures: "
                                  << "QxP=" << stats.qxpAttempts << " attempts (" 
                                  << (stats.qxpAttempts > 0 ? (100.0 * stats.qxpCutoffs / stats.qxpAttempts) : 0.0) << "% cutoff) "
                                  << "RxP=" << stats.rxpAttempts << " attempts ("
                                  << (stats.rxpAttempts > 0 ? (100.0 * stats.rxpCutoffs / stats.rxpAttempts) : 0.0) << "% cutoff)" << std::endl;
                    }
                    
                    // Output game phase distribution
                    uint64_t totalPhaseNodes = stats.openingNodes + stats.middlegameNodes + stats.endgameNodes;
                    if (totalPhaseNodes > 0) {
                        std::cout << "info string Phase distribution: "
                                  << "Opening=" << (100.0 * stats.openingNodes / totalPhaseNodes) << "% "
                                  << "Middle=" << (100.0 * stats.middlegameNodes / totalPhaseNodes) << "% "
                                  << "Endgame=" << (100.0 * stats.endgameNodes / totalPhaseNodes) << "%" << std::endl;
                    }
                    
                    // MO2a: Output killer validation statistics
                    if (stats.killerValidationAttempts > 0) {
                        std::cout << "info string Killer validation: "
                                  << stats.killerValidationAttempts << " attempts, "
                                  << stats.killerValidationFailures << " illegal ("
                                  << (100.0 * stats.killerValidationFailures / stats.killerValidationAttempts) << "% pollution rate)" << std::endl;
                    }
                }
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
            // Skip time management for infinite searches
            if (!limits.infinite && info.m_hardLimit > 0) {
                // Use actual elapsed time, not cached (for accurate time management)
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - info.startTime);
                bool stable = info.isPositionStable();
                
                // Check if we should stop based on new time management
                // Guard against max value issues
                TimeLimits checkLimits;
                checkLimits.soft = std::chrono::milliseconds(info.m_softLimit);
                checkLimits.optimum = std::chrono::milliseconds(info.m_optimumTime);
                // Protect against overflow when hardLimit is near max
                if (info.m_hardLimit >= std::chrono::milliseconds::max().count() - 1000) {
                    checkLimits.hard = std::chrono::milliseconds::max();
                } else {
                    checkLimits.hard = std::chrono::milliseconds(info.m_hardLimit);
                }
                
                if (shouldStopOnTime(checkLimits, elapsed, depth, stable)) {
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
    
    // B0: One-shot search summary (low overhead)
    if (limits.showSearchStats or limits.showPVSStats) {
        double ttHitRate = info.ttProbes > 0 ? (100.0 * info.ttHits / (double)info.ttProbes) : 0.0;
        double pvsReRate = info.pvsStats.scoutSearches > 0 ? (100.0 * info.pvsStats.reSearches / (double)info.pvsStats.scoutSearches) : 0.0;
        double nullRate = info.nullMoveStats.attempts > 0 ? (100.0 * info.nullMoveStats.cutoffs / (double)info.nullMoveStats.attempts) : 0.0;
        std::cout << "info string SearchStats: depth=" << info.depth
                  << " seldepth=" << info.seldepth
                  << " nodes=" << info.nodes
                  << " nps=" << info.nps()
                  << " tt: probes=" << info.ttProbes
                  << " hits=" << info.ttHits
                  << " hit%=" << std::fixed << std::setprecision(1) << ttHitRate
                  << " cutoffs=" << info.ttCutoffs
                  << " stores=" << info.ttStores
                  << " coll=" << info.ttCollisions
                  << " pvs: scout=" << info.pvsStats.scoutSearches
                  << " re%=" << std::fixed << std::setprecision(1) << pvsReRate
                  << " null: att=" << info.nullMoveStats.attempts
                  << " cut=" << info.nullMoveStats.cutoffs
                  << " cut%=" << std::fixed << std::setprecision(1) << nullRate
                  << " prune: fut=" << info.futilityPruned
                  << " mcp=" << info.moveCountPruned
                  << " asp: att=" << info.aspiration.attempts
                  << " low=" << info.aspiration.failLow
                  << " high=" << info.aspiration.failHigh
                  << " fut_b=[" << info.pruneBreakdown.futility[0] << "," << info.pruneBreakdown.futility[1]
                  << "," << info.pruneBreakdown.futility[2] << "," << info.pruneBreakdown.futility[3] << "]"
                  << " mcp_b=[" << info.pruneBreakdown.moveCount[0] << "," << info.pruneBreakdown.moveCount[1]
                  << "," << info.pruneBreakdown.moveCount[2] << "," << info.pruneBreakdown.moveCount[3] << "]"
                  << " illegal: first=" << info.illegalPseudoBeforeFirst
                  << " total=" << info.illegalPseudoTotal
                  << std::endl;
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
    
    // PERFORMANCE FIX: Allocate move ordering tables on stack
    // Same fix as in searchIterativeTest()
    KillerMoves killerMoves;
    HistoryHeuristic historyHeuristic;
    CounterMoves counterMovesTable;
    
    // Phase 4.3.a: Allocate counter-move history on heap
    std::unique_ptr<CounterMoveHistory> counterMoveHistoryPtr = std::make_unique<CounterMoveHistory>();
    
    // Connect pointers to stack-allocated objects
    info.killers = &killerMoves;
    info.history = &historyHeuristic;
    info.counterMoves = &counterMovesTable;
    info.counterMoveHistory = counterMoveHistoryPtr.get();
    
    // UCI Score Conversion FIX: Store root side-to-move for correct UCI output
    info.rootSideToMove = board.sideToMove();
    
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
    // Phase 1a: Store soft and hard limits for better time management
    auto softLimit = timeLimits.soft;
    auto hardLimit = timeLimits.hard;
    std::cerr << "Search: using NEW time management - optimum=" << info.timeLimit.count() << "ms" 
              << ", soft=" << softLimit.count() << "ms"
              << ", hard=" << hardLimit.count() << "ms" << std::endl;
    
    Move bestMove;
    
    // Debug: Show search parameters (removed in release)
    #ifndef NDEBUG
    std::cerr << "Search: maxDepth=" << limits.maxDepth 
              << " movetime=" << limits.movetime.count() 
              << "ms" << std::endl;
    #endif
    
    // Phase 1c: Track iteration times for EBF prediction
    std::chrono::milliseconds lastIterationTime(0);
    double lastEBF = 3.0;  // Conservative default EBF
    
    // Iterative deepening loop
    for (int depth = 1; depth <= limits.maxDepth; depth++) {
        info.depth = depth;
        
        // Track iteration start time
        auto iterationStart = std::chrono::steady_clock::now();
        
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
                                   searchInfo, info, limits, tt,
                                   nullptr);  // Phase PV1: Pass nullptr for now
        
        // Clear search mode after search
        board.setSearchMode(false);
        
        // Only update best move if search completed
        if (!info.stopped) {
            bestMove = info.bestMove;
            
            // Send UCI info about this iteration
            sendSearchInfo(info);
            
            // Phase 1c: Update iteration time and EBF for next iteration prediction
            auto iterationEnd = std::chrono::steady_clock::now();
            lastIterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                iterationEnd - iterationStart);
            
            // Get EBF from this iteration (if available)
            double currentEBF = info.effectiveBranchingFactor();
            if (currentEBF > 0) {
                lastEBF = currentEBF;
            }
            
            // Stop early if we found a mate
            if (score.is_mate_score()) {
                // If we found a mate, no need to search deeper
                break;
            }
            
            // Phase 1c: Use EBF prediction for time management
            // Replace Phase 1b's simple soft limit check with intelligent prediction
            if (hardLimit.count() > 0 && info.timeLimit != std::chrono::milliseconds::max()) {
                auto elapsed = info.elapsed();
                
                // Never exceed hard limit
                if (elapsed >= hardLimit) {
                    std::cerr << "[Time Management] Stopping at depth " << depth 
                              << " - reached hard limit (" << elapsed.count() << "ms >= " 
                              << hardLimit.count() << "ms)\n";
                    break;
                }
                
                // Use EBF prediction to decide if we have time for next iteration
                if (depth >= 2 && lastIterationTime.count() > 0) {
                    // Predict time for next iteration using our sophisticated prediction function
                    auto predictedNext = search::predictNextIterationTime(
                        lastIterationTime, lastEBF, depth);
                    
                    // Don't start an iteration we can't finish
                    if (elapsed + predictedNext > hardLimit) {
                        std::cerr << "[Time Management] Stopping at depth " << depth 
                                  << " - predicted next iteration (" << predictedNext.count() 
                                  << "ms) would exceed hard limit (elapsed: " << elapsed.count() 
                                  << "ms, hard: " << hardLimit.count() << "ms)\n";
                        break;
                    }
                    
                    // Use optimum time as a guideline, not a hard stop
                    // Allow going over optimum if we predict we can complete another iteration
                    if (elapsed > timeLimits.optimum && depth >= 8) {
                        // At reasonable depth and past optimum time
                        // Check if next iteration would take us well past optimum
                        if (elapsed + predictedNext > timeLimits.optimum * 2) {
                            std::cerr << "[Time Management] Stopping at depth " << depth 
                                      << " - sufficient depth reached and next iteration would exceed 2x optimum\n";
                            break;
                        }
                    }
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

// Phase 1: Send current search info during a search (periodic updates)
// Phase 5: Refactored to use InfoBuilder for cleaner construction
// UCI Score Conversion: Added Color parameter for White's perspective conversion
void sendCurrentSearchInfo(const IterativeSearchData& info, Color sideToMove, TranspositionTable* tt) {
    uci::InfoBuilder builder;
    
    // Basic depth info
    builder.appendDepth(info.depth, info.seldepth);
    
    // Score - now with UCI-compliant White's perspective conversion
    builder.appendScore(info.bestScore, sideToMove);
    
    // Node statistics
    builder.appendNodes(info.nodes)
           .appendNps(info.nps())
           .appendTime(info.elapsed().count());
    
    // Phase 4: Hashfull
    if (tt) {
        builder.appendHashfull(tt->hashfull());
    }
    
    // Phase 2: Currmove info for long searches
    if (info.currentRootMove != NO_MOVE && info.currentRootMoveNumber > 0) {
        // Force accurate time calculation for periodic updates
        auto now = std::chrono::steady_clock::now();
        auto actualElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.startTime);
        
        // Show currmove after 3 seconds
        if (actualElapsed.count() > 3000) {
            builder.appendCurrmove(info.currentRootMove, info.currentRootMoveNumber);
        }
    }
    
    // Principal variation
    if (info.bestMove != NO_MOVE) {
        builder.appendPv(info.bestMove);
    }
    
    std::cout << builder.build();
}

// Stage 13, Deliverable 5.1a: Enhanced UCI info output with iteration details
// Phase 5: Refactored to use InfoBuilder for cleaner construction
// UCI Score Conversion: Added Color parameter for White's perspective conversion
void sendIterationInfo(const IterativeSearchData& info, Color sideToMove, TranspositionTable* tt, const TriangularPV* pv) {
    uci::InfoBuilder builder;
    
    // Basic depth info
    builder.appendDepth(info.depth, info.seldepth);
    
    // Score - now with UCI-compliant White's perspective conversion
    builder.appendScore(info.bestScore, sideToMove);
    
    // Iteration-specific information
    if (info.hasIterations()) {
        const auto& lastIter = info.getLastIteration();
        
        // Stage 13, Deliverable 5.1b: Aspiration window reporting
        if (lastIter.depth >= AspirationConstants::MIN_DEPTH) {
            // Report window information
            if (lastIter.windowAttempts > 0) {
                // Window was used and there were re-searches
                if (lastIter.failedHigh) {
                    builder.appendString("fail-high(" + std::to_string(lastIter.windowAttempts) + ")");
                } else if (lastIter.failedLow) {
                    builder.appendString("fail-low(" + std::to_string(lastIter.windowAttempts) + ")");
                }
            }
            
            // Show window bounds for debugging (optional)
            if (lastIter.alpha != eval::Score::minus_infinity() || 
                lastIter.beta != eval::Score::infinity()) {
                std::string boundStr = "[";
                boundStr += (lastIter.alpha == eval::Score::minus_infinity() ? "-inf" : 
                            std::to_string(lastIter.alpha.to_cp()));
                boundStr += ",";
                boundStr += (lastIter.beta == eval::Score::infinity() ? "inf" : 
                            std::to_string(lastIter.beta.to_cp()));
                boundStr += "]";
                builder.appendCustom("bound", boundStr);
            }
        }
        
        // Stability indicator
        if (info.getIterationCount() >= 3) {
            if (info.isPositionStable()) {
                builder.appendString("stable");
            } else if (info.shouldExtendDueToInstability()) {
                builder.appendString("unstable");
            }
        }
        
        // Effective branching factor from sophisticated calculation
        double ebf = info.getSophisticatedEBF();
        if (ebf > 0) {
            builder.appendCustom("ebf", ebf);
        }
    }
    
    // Standard statistics
    builder.appendNodes(info.nodes)
           .appendNps(info.nps())
           .appendTime(info.elapsed().count());
    
    // Move ordering efficiency
    if (info.betaCutoffs > 0) {
        builder.appendCustom("moveeff", info.moveOrderingEfficiency());
    }
    
    // TT statistics
    if (info.ttProbes > 0) {
        double hitRate = (100.0 * info.ttHits) / info.ttProbes;
        builder.appendCustom("tthits", hitRate);
    }
    
    // Phase 4: Hashfull
    if (tt) {
        builder.appendHashfull(tt->hashfull());
    }
    
    // Phase PV4: Output full principal variation
    if (pv != nullptr && !pv->isEmpty(0)) {
        // Extract full PV from root
        std::vector<Move> pvMoves = pv->extractPV(0);
        if (!pvMoves.empty()) {
            // Validate moves and build clean PV
            std::vector<Move> validPvMoves;
            for (Move move : pvMoves) {
                // Validate each move before adding
                Square from = moveFrom(move);
                Square to = moveTo(move);
                if (from < 64 && to < 64 && from != to) {
                    validPvMoves.push_back(move);
                } else {
                    // Stop if we hit a corrupted move
                    break;
                }
            }
            // Pass entire PV vector to builder (it will add "pv" once)
            if (!validPvMoves.empty()) {
                builder.appendPv(validPvMoves);
            }
        }
    } else if (info.bestMove != Move()) {
        // Fallback: Output just the best move if no PV available
        // Bug #013 fix: Validate move is legal before outputting
        Square from = moveFrom(info.bestMove);
        Square to = moveTo(info.bestMove);
        if (from < 64 && to < 64 && from != to) {
            builder.appendPv(info.bestMove);
        } else {
            // Log warning about corrupted move but don't output it
            std::cerr << "WARNING: Corrupted bestMove detected in sendIterationInfo: " 
                      << std::hex << info.bestMove << std::dec << std::endl;
        }
    }
    
    std::cout << builder.build();
}

} // namespace seajay::search
