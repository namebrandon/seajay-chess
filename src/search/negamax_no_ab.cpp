#include "negamax.h"
#include "../core/board.h"
#include "../core/board_safety.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
#include <iostream>
#include <algorithm>
#include <cassert>

namespace seajay::search {

// Helper function to order moves - same as Stage 8 for fair comparison
template<typename MoveContainer>
void orderMoves(MoveContainer& moves) {
    auto first = moves.begin();
    auto last = moves.end();
    auto partition_point = first;
    
    // First partition: promotions
    for (auto it = first; it != last; ++it) {
        if (isPromotion(*it)) {
            // Queen promotions go first among promotions
            if (promotionType(*it) == QUEEN) {
                // Move queen promotions to the very front
                if (it != first) {
                    auto temp = *it;
                    std::move_backward(first, it, it + 1);
                    *first = temp;
                    ++partition_point;
                }
            } else if (it != partition_point) {
                std::iter_swap(it, partition_point);
                ++partition_point;
            }
        }
    }
    
    // Second partition: captures (after promotions)
    for (auto it = partition_point; it != last; ++it) {
        if (isCapture(*it) && !isPromotion(*it)) {
            if (it != partition_point) {
                std::iter_swap(it, partition_point);
                ++partition_point;
            }
        }
    }
}

// Core negamax search implementation WITHOUT alpha-beta pruning
// This is for Stage 7 baseline comparison
eval::Score negamax(Board& board, 
                   int depth, 
                   int ply,
                   eval::Score alpha,
                   eval::Score beta,
                   SearchInfo& info) {
    
    // Debug output at root
    if (ply == 0 && depth >= 4) {
        std::cerr << "Negamax (NO AB): Starting search at depth " << depth << std::endl;
    }
    
    // Debug assertions
    assert(depth >= 0);
    assert(ply >= 0);
    assert(ply < 128);  // Prevent stack overflow
    
    // Ensure valid search window (kept for compatibility but not used for pruning)
    if (alpha >= beta) {
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
    
    // Generate all legal moves
    MoveList moves = generateLegalMoves(board);
    
    // Apply move ordering for fair comparison (Stage 7 would have same ordering)
    orderMoves(moves);
    
    // Debug output at root for deeper searches
    if (ply == 0 && depth >= 4) {
        std::cerr << "Root (NO AB): generated " << moves.size() << " moves, depth=" << depth << "\n";
    }
    
    // Check for checkmate or stalemate
    if (moves.empty()) {
        if (inCheck(board)) {
            // Checkmate - we are mated
            return eval::Score(-32000 + ply);
        }
        // Stalemate - draw
        return eval::Score::draw();
    }
    
    // Debug: Validate board state before search
#ifdef DEBUG
    Hash hashBefore = board.zobristKey();
    int pieceCountBefore = __builtin_popcountll(board.occupied());
#endif
    
    // Initialize best score
    eval::Score bestScore = eval::Score::minus_infinity();
    Move bestMove;  // Only used at root (ply == 0) for tracking best move
    
    // Search all moves WITHOUT PRUNING
    int moveCount = 0;
    for (const Move& move : moves) {
        moveCount++;
        info.totalMoves++;  // Track total moves examined
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        // Recursive search with negation and swapped window
        eval::Score score = -negamax(board, depth - 1, ply + 1, 
                                    -beta, -alpha, info);
        
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
                    std::cerr << "Root (NO AB): Move " << SafeMoveExecutor::moveToString(move) 
                              << " score=" << score.to_cp() << " cp" << std::endl;
                }
            }
            
            // Update alpha (best score we can guarantee)
            // Keep updating alpha for proper window management
            // but DO NOT PRUNE - this is the key difference
            if (score > alpha) {
                alpha = score;
                
                // NO BETA CUTOFF - WE SEARCH ALL MOVES
                // This is what makes it Stage 7 (no pruning)
                if (score >= beta) {
                    // In Stage 8, we would break here
                    // In Stage 7, we continue searching
                    info.betaCutoffs++;  // Still track for statistics
                    if (moveCount == 1) {
                        info.betaCutoffsFirst++;
                    }
                    // DO NOT BREAK - search all moves
                }
            }
        }
    }
    
    return bestScore;
}

// Main search function - entry point for UCI
Move search(Board& board, const SearchLimits& limits) {
    // Enable search mode to skip game history tracking for performance
    board.setSearchMode(true);
    
    SearchInfo info;
    info.limits = limits;
    info.startTime = std::chrono::steady_clock::now();
    
    // Set time limit based on limits
    if (limits.movetime != std::chrono::milliseconds::zero()) {
        info.timeLimit = limits.movetime;
    } else if (limits.time[board.sideToMove()] != std::chrono::milliseconds::zero()) {
        // Simple time management: use 5% of remaining time
        auto ourTime = limits.time[board.sideToMove()];
        auto increment = limits.inc[board.sideToMove()];
        
        // Calculate time for this move (simplified)
        auto timeForMove = ourTime / 20 + increment * 4 / 5;
        
        // Apply minimum and maximum bounds
        timeForMove = std::max(timeForMove, std::chrono::milliseconds(10));
        timeForMove = std::min(timeForMove, ourTime - std::chrono::milliseconds(50));
        
        info.timeLimit = timeForMove;
    } else {
        // Default: 5 seconds if no time control specified
        info.timeLimit = std::chrono::milliseconds(5000);
    }
    
    std::cerr << "Search (NO AB): Starting with maxDepth=" << limits.maxDepth << std::endl;
    std::cerr << "Search (NO AB): movetime=" << limits.movetime.count() << "ms" << std::endl;
    std::cerr << "Search (NO AB): calculated timeLimit=" << info.timeLimit.count() << "ms" << std::endl;
    
    Move bestMove;
    eval::Score bestScore = eval::Score::minus_infinity();
    
    // Iterative deepening
    for (int depth = 1; depth <= limits.maxDepth; ++depth) {
        info.currentDepth = depth;
        info.seldepth = 0;
        
        // Search at current depth
        eval::Score score = negamax(board, depth, 0, 
                                   eval::Score::minus_infinity(), 
                                   eval::Score::infinity(), 
                                   info);
        
        // Check if search was stopped
        if (info.stopped) {
            break;
        }
        
        // Update best move from this iteration
        bestMove = info.bestMove;
        bestScore = score;
        
        // Calculate time elapsed
        auto elapsed = std::chrono::steady_clock::now() - info.startTime;
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        if (elapsedMs == 0) elapsedMs = 1;  // Avoid division by zero
        
        // Output UCI info
        std::cout << "info depth " << depth 
                  << " seldepth " << info.seldepth
                  << " score cp " << bestScore.to_cp()
                  << " nodes " << info.nodes
                  << " nps " << (info.nodes * 1000 / elapsedMs)
                  << " time " << elapsedMs;
        
        // Add performance metrics for depths > 1
        if (depth > 1) {
            double ebf = info.effectiveBranchingFactor();
            double moveEff = info.moveOrderingEfficiency();
            std::cout << " ebf " << std::fixed << std::setprecision(2) << ebf
                     << " moveeff " << std::fixed << std::setprecision(1) << moveEff << "%";
        }
        
        // Add PV (just the best move for now)
        std::cout << " pv " << SafeMoveExecutor::moveToString(bestMove)
                  << std::endl;
        
        // Time management: stop if we've used significant time
        if (elapsedMs > info.timeLimit.count() * 2 / 5) {
            // Used 40% of allocated time, probably won't finish next iteration
            if (depth < limits.maxDepth) {
                std::cerr << "Search (NO AB): Stopping early due to time (40% used)" << std::endl;
                break;
            }
        }
    }
    
    // Disable search mode - restore normal game history tracking
    board.setSearchMode(false);
    
    return bestMove;
}

} // namespace seajay::search