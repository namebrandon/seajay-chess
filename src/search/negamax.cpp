#include "negamax.h"
#include "search_info.h"
#include "iterative_search_data.h"  // Stage 13 addition
#ifdef ENABLE_MVV_LVA
#include "move_ordering.h"  // MVV-LVA ordering
#endif
#include "../core/board.h"
#include "../core/board_safety.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
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
// Orders moves in-place: TT move first, then promotions, then captures (MVV-LVA), then quiet moves
template<typename MoveContainer>
inline void orderMoves(const Board& board, MoveContainer& moves, Move ttMove = NO_MOVE) noexcept {
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
    
#ifdef ENABLE_MVV_LVA
    // Use MVV-LVA ordering for remaining moves
    // MVV-LVA will handle the entire list, including TT move positioning
    MvvLvaOrdering mvvLvaOrdering;
    mvvLvaOrdering.orderMoves(board, moves);
    
    // After MVV-LVA, ensure TT move is still first if it was valid
    if (ttMove != NO_MOVE) {
        auto it = std::find(moves.begin(), moves.end(), ttMove);
        if (it != moves.end() && it != moves.begin()) {
            // Move TT move back to front (MVV-LVA may have moved it)
            Move temp = *it;
            std::move_backward(moves.begin(), it, it + 1);
            *moves.begin() = temp;
        }
    }
#else
    // Fallback to simple ordering without MVV-LVA
    (void)board; // Unused in simple ordering
    if (ttMove != NO_MOVE && !moves.empty() && moves[0] == ttMove) {
        // Order everything except the TT move
        auto first = moves.begin() + 1;
        auto last = moves.end();
        auto partition_point = first;
        
        // Phase 1: Move all promotions to the front (after TT move)
        for (auto it = first; it != last; ++it) {
            if (isPromotion(*it)) {
                if (promotionType(*it) == QUEEN) {
                    if (it != first) {
                        Move temp = *it;
                        std::move_backward(first, it, it + 1);
                        *first = temp;
                        partition_point = std::next(first);
                    }
                } else if (it != partition_point) {
                    std::iter_swap(it, partition_point);
                    ++partition_point;
                } else {
                    ++partition_point;
                }
            }
        }
        
        // Phase 2: Move captures after promotions
        for (auto it = partition_point; it != last; ++it) {
            if (isCapture(*it) && !isPromotion(*it)) {
                if (it != partition_point) {
                    std::iter_swap(it, partition_point);
                }
                ++partition_point;
            }
        }
    } else {
        orderMovesSimple(moves);
    }
#endif
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
                   TranspositionTable* tt) {
    
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
    
    // Time check - only check every 4096 nodes to reduce overhead
    if ((info.nodes & 0xFFF) == 0 && info.checkTime()) {
        info.stopped = true;
        return eval::Score::zero();
    }
    
    // Increment node counter
    info.nodes++;
    
    // Update selective depth (maximum depth reached)
    if (ply > info.seldepth) {
        info.seldepth = ply;
    }
    
    // Terminal node - return static evaluation
    if (depth <= 0) {
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
                    info.ttMoveHits++;
                }
            }
        }
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
    // TT move first, then promotions (especially queen), then captures (MVV-LVA), then quiet moves
    orderMoves(board, moves, ttMove);
    
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
    
    for (const Move& move : moves) {
        moveCount++;
        info.totalMoves++;  // Track total moves examined
        
        // Push position to search stack BEFORE making the move
        searchInfo.pushSearchPosition(board.zobristKey(), move, ply);
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        // Recursive search with negation and swapped window
        // Note: When negating, we swap alpha and beta
        eval::Score score = -negamax(board, depth - 1, ply + 1, 
                                    -beta, -alpha, searchInfo, info, tt);
        
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
                
                // Debug output for root moves
                if (depth >= 3) {
                    std::cerr << "Root: Move " << SafeMoveExecutor::moveToString(move) 
                              << " score=" << score.to_cp() << " cp" << std::endl;
                }
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
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    
    IterativeSearchData info;  // Using new class instead of SearchData
    info.timeLimit = calculateTimeLimit(limits, board);
    
    Move bestMove;
    Move previousBestMove = NO_MOVE;  // Track best move from previous iteration
    
    // Same iterative deepening loop as original search
    for (int depth = 1; depth <= limits.maxDepth; depth++) {
        info.depth = depth;
        board.setSearchMode(true);
        
        // Track start time for this iteration
        auto iterationStart = std::chrono::steady_clock::now();
        uint64_t nodesBeforeIteration = info.nodes;  // Save node count before iteration
        
        eval::Score score = negamax(board, depth, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, info, tt);
        
        board.setSearchMode(false);
        
        if (!info.stopped) {
            bestMove = info.bestMove;
            sendSearchInfo(info);
            
            // Record iteration data for ALL depths (full recording)
            auto iterationEnd = std::chrono::steady_clock::now();
            auto iterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                iterationEnd - iterationStart).count();
            
            IterationInfo iter;
            iter.depth = depth;
            iter.score = score;
            iter.bestMove = info.bestMove;
            iter.nodes = info.nodes - nodesBeforeIteration;  // Nodes for this iteration only
            iter.elapsed = iterationTime;
            iter.alpha = eval::Score::minus_infinity();
            iter.beta = eval::Score::infinity();
            iter.windowAttempts = 0;
            iter.failedHigh = false;
            iter.failedLow = false;
            
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
            previousBestMove = info.bestMove;  // Update for next iteration
            
            if (score.is_mate_score()) {
                break;
            }
            
            if (info.timeLimit != std::chrono::milliseconds::max()) {
                auto elapsed = info.elapsed();
                if (elapsed * 5 > info.timeLimit * 2) {
                    break;
                }
            }
        } else {
            break;
        }
    }
    
    return bestMove;
}

// Iterative deepening search controller (original)
Move search(Board& board, const SearchLimits& limits, TranspositionTable* tt) {
    // Debug output
    std::cerr << "Search: Starting with maxDepth=" << limits.maxDepth << std::endl;
    std::cerr << "Search: movetime=" << limits.movetime.count() << "ms" << std::endl;
    
    // Initialize search tracking
    SearchInfo searchInfo;  // For repetition detection
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());  // Capture current game history size
    
    SearchData info;  // For search statistics
    info.timeLimit = calculateTimeLimit(limits, board);
    std::cerr << "Search: calculated timeLimit=" << info.timeLimit.count() << "ms" << std::endl;
    
    Move bestMove;
    
    // Debug: Show search parameters
    #ifdef DEBUG
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
                                   searchInfo, info, tt);
        
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
    if (info.bestMove != Move()) {
        std::cout << " pv " << SafeMoveExecutor::moveToString(info.bestMove);
    }
    
    std::cout << std::endl;
}

} // namespace seajay::search