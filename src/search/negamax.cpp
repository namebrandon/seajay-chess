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

// Core negamax search implementation
eval::Score negamax(Board& board, 
                   int depth, 
                   int ply,
                   eval::Score alpha,
                   eval::Score beta,
                   SearchInfo& info) {
    
    // Debug output at root
    if (ply == 0 && depth >= 4) {
        std::cerr << "Negamax: Starting search at depth " << depth << std::endl;
    }
    
    // Debug assertions
    assert(depth >= 0);
    assert(ply >= 0);
    assert(ply < 128);  // Prevent stack overflow
    
    // Ensure valid search window
    // Note: Due to integer limits, -infinity negated is not exactly +infinity
    // This can cause alpha >= beta after negation in some edge cases
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
    
    // Generate all legal moves
    MoveList moves = generateLegalMoves(board);
    
    // Debug output at root for deeper searches
    if (ply == 0 && depth >= 4) {
        std::cerr << "Root: generated " << moves.size() << " moves, depth=" << depth << "\n";
    }
    
    // Check for checkmate or stalemate
    if (moves.empty()) {
        if (inCheck(board)) {
            // Checkmate - we are mated
            // Return negative mate score adjusted by ply
            // (Prefer shorter mates)
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
    Move bestMove;
    
    // Search all moves
    int moveCount = 0;
    for (const Move& move : moves) {
        moveCount++;
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        // Recursive search with negation and swapped window
        // Note: When negating, we swap alpha and beta
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
                    std::cerr << "Root: Move " << SafeMoveExecutor::moveToString(move) 
                              << " score=" << score.to_cp() << " cp" << std::endl;
                }
            }
            
            // Update alpha (best score we can guarantee)
            // This prepares the framework for alpha-beta pruning in Stage 8
            if (score > alpha) {
                alpha = score;
                
                // Beta cutoff check (will be activated in Stage 8)
                // For now, we don't break even if score >= beta
                // This ensures we search all moves in Stage 7
                if (score >= beta) {
                    // In Stage 8, we would: break;
                    // For now, continue searching
                }
            }
        }
    }
    
    return bestScore;
}

// Iterative deepening search controller
Move search(Board& board, const SearchLimits& limits) {
    // Debug output
    std::cerr << "Search: Starting with maxDepth=" << limits.maxDepth << std::endl;
    std::cerr << "Search: movetime=" << limits.movetime.count() << "ms" << std::endl;
    
    SearchInfo info;
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
        
        // Search with infinite window initially
        eval::Score score = negamax(board, depth, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   info);
        
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
void sendSearchInfo(const SearchInfo& info) {
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
    
    // Output principal variation (just the best move for now)
    if (info.bestMove != Move()) {
        std::cout << " pv " << SafeMoveExecutor::moveToString(info.bestMove);
    }
    
    std::cout << std::endl;
}

} // namespace seajay::search