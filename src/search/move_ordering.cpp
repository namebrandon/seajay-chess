#include "move_ordering.h"
#include "../core/board_safety.h"  // For moveToString
#include "../core/move_generation.h"  // For MoveGenerator::isPseudoLegal (MO2a)
#include <algorithm>
#include <iostream>
#include <chrono>  // For timestamp in log file

// Debug output control
#ifdef DEBUG_MOVE_ORDERING
    static bool g_debugMoveOrdering = true;
#else
    static bool g_debugMoveOrdering = false;
#endif

namespace seajay::search {

// MVV-LVA scoring uses the simple formula:
// score = VICTIM_VALUES[victim] - ATTACKER_VALUES[attacker]
// This gives us scores like PxQ=899, QxP=91, PxP=99
// Note: Promotions are handled separately with PROMOTION_BASE_SCORE
// This ensures promotions are searched before captures

// Score a single move using MVV-LVA heuristic
int MvvLvaOrdering::scoreMove(const Board& board, Move move) noexcept {
    auto& stats = getStatistics();
    
    // Handle promotions first (highest priority)
    if (isPromotion(move)) {
        stats.promotions_scored++;
        
        // Get the promotion piece type (KNIGHT=1, BISHOP=2, ROOK=3, QUEEN=4)
        PieceType promoType = promotionType(move);
        int baseScore = PROMOTION_BASE_SCORE;
        
        // Add promotion type bonus (Queen > Rook > Bishop > Knight)
        if (promoType >= KNIGHT && promoType <= QUEEN) {
            baseScore += PROMOTION_TYPE_BONUS[promoType - KNIGHT];
        }
        
        // If it's also a capture, add MVV-LVA score
        // CRITICAL: Attacker is always PAWN for promotions, not the promoted piece!
        if (isCapture(move)) {
            Square toSq = moveTo(move);
            Piece capturedPiece = board.pieceAt(toSq);
            
            if (capturedPiece != NO_PIECE) {
                PieceType victim = typeOf(capturedPiece);
                // Use MVV-LVA formula (attacker is always PAWN for promotions)
                baseScore += VICTIM_VALUES[victim] - ATTACKER_VALUES[PAWN];
            }
        }
        
        return baseScore;
    }
    
    // Handle en passant captures (special case)
    if (isEnPassant(move)) {
        stats.en_passants_scored++;
        // En passant is always PxP (pawn captures pawn)
        return VICTIM_VALUES[PAWN] - ATTACKER_VALUES[PAWN];  // 100 - 1 = 99
    }
    
    // Handle regular captures
    if (isCapture(move)) {
        stats.captures_scored++;
        
        Square fromSq = moveFrom(move);
        Square toSq = moveTo(move);
        
        Piece attackingPiece = board.pieceAt(fromSq);
        Piece capturedPiece = board.pieceAt(toSq);
        
        // Validate pieces
        MVV_LVA_ASSERT(attackingPiece != NO_PIECE, "No attacking piece at from square");
        MVV_LVA_ASSERT(capturedPiece != NO_PIECE, "No captured piece at to square");
        
        if (attackingPiece == NO_PIECE || capturedPiece == NO_PIECE) {
            return 0;  // Safety fallback
        }
        
        PieceType attacker = typeOf(attackingPiece);
        PieceType victim = typeOf(capturedPiece);
        
        // King captures should never happen in legal chess
        MVV_LVA_ASSERT(victim != KING, "Attempting to capture king!");
        
        // Use the simple MVV-LVA formula
        return VICTIM_VALUES[victim] - ATTACKER_VALUES[attacker];
    }
    
    // Quiet moves get zero score (will be ordered last)
    stats.quiet_moves++;
    return 0;
}

// Order moves using MVV-LVA scoring - OPTIMIZED VERSION
// CRITICAL: Only sorts captures, preserves quiet move order from generator
void MvvLvaOrdering::orderMoves(const Board& board, MoveList& moves) const {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
#ifdef DEBUG_MOVE_ORDERING
    if (g_debugMoveOrdering) {
        std::cout << "[MVV-LVA] Ordering " << moves.size() << " moves (optimized)" << std::endl;
    }
#endif
    
    // CRITICAL OPTIMIZATION: In-place partition to separate captures from quiet moves
    // This preserves the natural order of quiet moves from the generator
    auto captureEnd = std::stable_partition(moves.begin(), moves.end(),
        [&board](const Move& move) {
            // Promotions and captures go to the front
            return isPromotion(move) || isCapture(move) || isEnPassant(move);
        });
    
    // Only sort the captures/promotions portion if there are any
    if (captureEnd != moves.begin()) {
        // Sort captures by MVV-LVA score (higher scores first)
        // Use stable_sort to maintain relative order of equal scores
        std::stable_sort(moves.begin(), captureEnd,
            [this, &board](const Move& a, const Move& b) {
                // Use scoreMove function for consistency and maintainability
                // Modern compilers will inline this anyway
                int scoreA = scoreMove(board, a);
                int scoreB = scoreMove(board, b);
                
                return scoreA > scoreB;  // Higher scores first
            });
    }
    
    // Quiet moves remain at the end in their original order (castling first, etc.)
}

// Order moves with killer move integration (Stage 19, Phase A2)
void MvvLvaOrdering::orderMovesWithKillers(const Board& board, MoveList& moves, 
                                           const KillerMoves& killers, int ply) const {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
    // First do standard MVV-LVA ordering for captures
    orderMoves(board, moves);
    
    // Now insert killer moves after captures but before other quiet moves
    // Find where quiet moves start (after captures/promotions)
    auto quietStart = std::find_if(moves.begin(), moves.end(),
        [](const Move& move) {
            return !isPromotion(move) && !isCapture(move) && !isEnPassant(move);
        });
    
    if (quietStart == moves.end()) {
        // No quiet moves, nothing more to do
        return;
    }
    
    // Try to move killer moves to the front of quiet moves
    for (int slot = 0; slot < 2; ++slot) {
        Move killer = killers.getKiller(ply, slot);
        if (killer != NO_MOVE && !isCapture(killer) && !isPromotion(killer)) {
            // Phase 4.1.b: Fast-path validation to skip obviously stale killers
            // This avoids searching for moves that can't possibly be valid
            Square from = moveFrom(killer);
            Piece piece = board.pieceAt(from);
            if (piece == NO_PIECE || colorOf(piece) != board.sideToMove()) {
                continue;  // Skip stale killer - wrong color or no piece
            }
            
            // Find this killer in the quiet moves section
            // If it's found, it's already valid (generated by move generator)
            auto it = std::find(quietStart, moves.end(), killer);
            if (it != moves.end() && it != quietStart) {
                // Move killer to front of quiet moves
                std::rotate(quietStart, it, it + 1);
                ++quietStart;  // Next killer goes after this one
            }
        }
    }
}

// Order moves with both killers and history (Stage 20, Phase B2)
void MvvLvaOrdering::orderMovesWithHistory(const Board& board, MoveList& moves,
                                          const KillerMoves& killers, 
                                          const HistoryHeuristic& history, int ply) const {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
    // First do standard MVV-LVA ordering for captures
    orderMoves(board, moves);
    
    // Find where quiet moves start (after captures/promotions)
    auto quietStart = std::find_if(moves.begin(), moves.end(),
        [](const Move& move) {
            return !isPromotion(move) && !isCapture(move) && !isEnPassant(move);
        });
    
    if (quietStart == moves.end()) {
        // No quiet moves, nothing more to do
        return;
    }
    
    // First, move killer moves to the front of quiet moves
    auto killerEnd = quietStart;
    for (int slot = 0; slot < 2; ++slot) {
        Move killer = killers.getKiller(ply, slot);
        if (killer != NO_MOVE && !isCapture(killer) && !isPromotion(killer)) {
            // Phase 4.1.b: Fast-path validation to skip obviously stale killers
            Square from = moveFrom(killer);
            Piece piece = board.pieceAt(from);
            if (piece == NO_PIECE || colorOf(piece) != board.sideToMove()) {
                continue;  // Skip stale killer - wrong color or no piece
            }
            
            // Find this killer in the quiet moves section
            // If it's found, it's already valid (generated by move generator)
            auto it = std::find(killerEnd, moves.end(), killer);
            if (it != moves.end() && it != killerEnd) {
                // Move killer to front of quiet moves
                std::rotate(killerEnd, it, it + 1);
                ++killerEnd;  // Next killer goes after this one
            }
        }
    }
    
    // Now sort the remaining quiet moves by history score
    // (moves after killerEnd are non-killer quiet moves)
    if (killerEnd != moves.end()) {
        Color side = board.sideToMove();
        std::stable_sort(killerEnd, moves.end(),
            [&history, side](const Move& a, const Move& b) {
                // Get history scores for both moves
                int scoreA = history.getScore(side, moveFrom(a), moveTo(a));
                int scoreB = history.getScore(side, moveFrom(b), moveTo(b));
                return scoreA > scoreB;  // Higher scores first
            });
    }
}

// Order moves with killers, history, and countermoves (Stage 23, CM3.3)
void MvvLvaOrdering::orderMovesWithHistory(const Board& board, MoveList& moves,
                                          const KillerMoves& killers, 
                                          const HistoryHeuristic& history,
                                          const CounterMoves& counterMoves,
                                          Move prevMove, int ply, int countermoveBonus) const {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
    // First do standard MVV-LVA ordering for captures
    orderMoves(board, moves);
    
    // Find where quiet moves start (after captures/promotions)
    auto quietStart = std::find_if(moves.begin(), moves.end(),
        [](const Move& move) {
            return !isPromotion(move) && !isCapture(move) && !isEnPassant(move);
        });
    
    if (quietStart == moves.end()) {
        // No quiet moves, nothing more to do
        return;
    }
    
    // First, move killer moves to the front of quiet moves
    auto killerEnd = quietStart;
    for (int slot = 0; slot < 2; ++slot) {
        Move killer = killers.getKiller(ply, slot);
        if (killer != NO_MOVE && !isCapture(killer) && !isPromotion(killer)) {
            // Phase 4.1.b: Fast-path validation to skip obviously stale killers
            Square from = moveFrom(killer);
            Piece piece = board.pieceAt(from);
            if (piece == NO_PIECE || colorOf(piece) != board.sideToMove()) {
                continue;  // Skip stale killer - wrong color or no piece
            }
            
            // Find this killer in the quiet moves section
            // If it's found, it's already valid (generated by move generator)
            auto it = std::find(killerEnd, moves.end(), killer);
            if (it != moves.end() && it != killerEnd) {
                // Move killer to front of quiet moves
                std::rotate(killerEnd, it, it + 1);
                ++killerEnd;  // Next killer goes after this one
            }
        }
    }
    
    // CM4.1: Position countermove with hit tracking
    if (countermoveBonus > 0 && prevMove != NO_MOVE) {
        Move counterMove = counterMoves.getCounterMove(prevMove);
        
        if (counterMove != NO_MOVE && !isCapture(counterMove) && !isPromotion(counterMove)) {
            // Phase 4.1.b: Fast-path validation for countermove
            Square from = moveFrom(counterMove);
            Piece piece = board.pieceAt(from);
            
            // Only proceed if piece exists and is the right color
            if (piece != NO_PIECE && colorOf(piece) == board.sideToMove()) {
                // Find the countermove in the remaining quiet moves
                // If it's found, it's already valid (generated by move generator)
                auto it = std::find(killerEnd, moves.end(), counterMove);
                if (it != moves.end() && it != killerEnd) {
                    // Move countermove right after killers
                    std::rotate(killerEnd, it, it + 1);
                    ++killerEnd;  // History moves go after countermove
                    
                    // CM4.1: Track that we found and used a countermove
                    // This will be picked up by SearchData stats later
                }
            }
        }
    }
    
    // Now sort the remaining quiet moves by history score
    // (moves after killerEnd are non-killer, non-countermove quiet moves)
    if (killerEnd != moves.end()) {
        Color side = board.sideToMove();
        std::stable_sort(killerEnd, moves.end(),
            [&history, side](const Move& a, const Move& b) {
                // Get history scores for both moves
                int scoreA = history.getScore(side, moveFrom(a), moveTo(a));
                int scoreB = history.getScore(side, moveFrom(b), moveTo(b));
                return scoreA > scoreB;  // Higher scores first
            });
    }
}

// Phase 4.3.a: Order moves with counter-move history
void MvvLvaOrdering::orderMovesWithHistory(const Board& board, MoveList& moves,
                                          const KillerMoves& killers, 
                                          const HistoryHeuristic& history,
                                          const CounterMoves& counterMoves,
                                          const CounterMoveHistory& counterMoveHistory,
                                          Move prevMove, int ply, int countermoveBonus,
                                          float cmhWeight) const {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
    // First do standard MVV-LVA ordering for captures
    orderMoves(board, moves);
    
    // Find where quiet moves start (after captures/promotions)
    auto quietStart = std::find_if(moves.begin(), moves.end(),
        [](const Move& move) {
            return !isPromotion(move) && !isCapture(move) && !isEnPassant(move);
        });
    
    if (quietStart == moves.end()) {
        // No quiet moves, nothing more to do
        return;
    }
    
    // First, move killer moves to the front of quiet moves
    auto killerEnd = quietStart;
    for (int slot = 0; slot < 2; ++slot) {
        Move killer = killers.getKiller(ply, slot);
        if (killer != NO_MOVE && !isCapture(killer) && !isPromotion(killer)) {
            // Phase 4.1.b: Fast-path validation to skip obviously stale killers
            Square from = moveFrom(killer);
            Piece piece = board.pieceAt(from);
            if (piece == NO_PIECE || colorOf(piece) != board.sideToMove()) {
                continue;  // Skip stale killer - wrong color or no piece
            }
            
            // Find this killer in the quiet moves section
            auto it = std::find(killerEnd, moves.end(), killer);
            if (it != moves.end() && it != killerEnd) {
                // Move killer to front of quiet moves
                std::rotate(killerEnd, it, it + 1);
                ++killerEnd;  // Next killer goes after this one
            }
        }
    }
    
    // Position countermove after killers
    if (countermoveBonus > 0 && prevMove != NO_MOVE) {
        Move counterMove = counterMoves.getCounterMove(prevMove);
        
        if (counterMove != NO_MOVE && !isCapture(counterMove) && !isPromotion(counterMove)) {
            // Fast-path validation for countermove
            Square from = moveFrom(counterMove);
            Piece piece = board.pieceAt(from);
            
            if (piece != NO_PIECE && colorOf(piece) == board.sideToMove()) {
                // Find this countermove in the quiet moves section (after killers)
                auto it = std::find(killerEnd, moves.end(), counterMove);
                if (it != moves.end() && it != killerEnd) {
                    // Move countermove to position after killers
                    std::rotate(killerEnd, it, it + 1);
                    ++killerEnd;  // Update end of special moves
                }
            }
        }
    }
    
    // Now sort the remaining quiet moves by combined history scores
    // Regular history + counter-move history with weight
    if (killerEnd != moves.end()) {
        Color side = board.sideToMove();
        
        std::stable_sort(killerEnd, moves.end(),
            [&history, &counterMoveHistory, side, prevMove, cmhWeight](const Move& a, const Move& b) {
                // Get regular history scores
                float scoreA = static_cast<float>(history.getScore(side, moveFrom(a), moveTo(a)));
                float scoreB = static_cast<float>(history.getScore(side, moveFrom(b), moveTo(b)));
                
                // Add weighted counter-move history scores (simplified interface)
                scoreA += counterMoveHistory.getScore(prevMove, a) * cmhWeight;
                scoreB += counterMoveHistory.getScore(prevMove, b) * cmhWeight;
                
                return scoreA > scoreB;  // Higher scores first
            });
    }
}

// Template implementation for integrating with existing code
// OPTIMIZED: No heap allocation, in-place sorting
template<typename MoveContainer>
void orderMovesWithMvvLva(const Board& board, MoveContainer& moves) noexcept {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
    // Use the optimized in-place implementation
    static MvvLvaOrdering ordering;
    ordering.orderMoves(board, moves);
}

// Explicit instantiation for common container types
template void orderMovesWithMvvLva<MoveList>(const Board& board, MoveList& moves) noexcept;

// Debug helper function to print move ordering (optional)
#ifdef DEBUG_MOVE_ORDERING
void printMoveOrdering(const Board& board, const MoveList& moves) {
    std::cout << "Move Ordering:\n";
    for (Move move : moves) {
        int score = MvvLvaOrdering::scoreMove(board, move);
        std::cout << "  " << SafeMoveExecutor::moveToString(move) 
                  << " score=" << score;
        
        if (isPromotion(move)) {
            std::cout << " (promotion";
            if (isCapture(move)) std::cout << "-capture";
            std::cout << ")";
        } else if (isEnPassant(move)) {
            std::cout << " (en passant)";
        } else if (isCapture(move)) {
            Square fromSq = moveFrom(move);
            Square toSq = moveTo(move);
            Piece attacker = board.pieceAt(fromSq);
            Piece victim = board.pieceAt(toSq);
            std::cout << " (" << PIECE_CHARS[attacker] 
                      << "x" << PIECE_CHARS[victim] << ")";
        }
        std::cout << "\n";
    }
}
#endif

// ================================================================================
// Stage 15 Day 5: SEE Integration Implementation
// ================================================================================

// Global SEE move ordering instance
SEEMoveOrdering g_seeMoveOrdering;

SEEMoveOrdering::SEEMoveOrdering() : m_see() {
    // Open log file in testing mode
    if (m_mode == SEEMode::TESTING) {
        m_logFile.open("see_discrepancies.log", std::ios::app);
        if (m_logFile.is_open()) {
            m_logFile << "\n=== New Session Started ===\n";
            m_logFile << "Time: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";
        }
    }
}

// Day 5.1: Parallel scoring infrastructure
std::vector<ParallelScore> SEEMoveOrdering::scoreMovesParallel(const Board& board, const MoveList& moves) const {
    std::vector<ParallelScore> results;
    results.reserve(moves.size());
    
    auto& stats = getStats();
    
    for (Move move : moves) {
        ParallelScore score;
        score.move = move;
        
        // Always calculate MVV-LVA score
        score.mvvLvaScore = MvvLvaOrdering::scoreMove(board, move);
        
        // Calculate SEE value for captures and promotions
        if (isCapture(move) || isPromotion(move) || isEnPassant(move)) {
            stats.capturesProcessed.fetch_add(1, std::memory_order_relaxed);
            if (isPromotion(move)) {
                stats.promotionsProcessed.fetch_add(1, std::memory_order_relaxed);
            }
            
            // Calculate SEE value
            stats.seeCalculations.fetch_add(1, std::memory_order_relaxed);
            score.seeValue = m_see.see(board, move);
            
            // Track that we did a SEE calculation
            // Cache hits are tracked internally by SEE
            const auto& seeStats = m_see.statistics();
            if (seeStats.cacheHits > 0) {
                // We can track cache hits if needed
                stats.seeCacheHits.store(seeStats.cacheHits.load(), std::memory_order_relaxed);
            }
        } else {
            // Quiet moves have SEE value of 0
            score.seeValue = 0;
        }
        
        // Determine if they agree on ordering
        // For simplicity, we consider them to agree if:
        // 1. Both consider the move good (positive score)
        // 2. Both consider the move bad (negative/zero score)
        // 3. Both give exactly the same evaluation
        
        bool mvvLvaPositive = score.mvvLvaScore > 0;
        bool seePositive = score.seeValue > 0;
        
        score.agree = (mvvLvaPositive == seePositive);
        
        if (score.mvvLvaScore == score.seeValue) {
            stats.equalScores.fetch_add(1, std::memory_order_relaxed);
            score.agree = true;
        }
        
        stats.totalComparisons.fetch_add(1, std::memory_order_relaxed);
        if (score.agree) {
            stats.agreements.fetch_add(1, std::memory_order_relaxed);
        } else {
            // Track which method preferred this move
            if (score.seeValue > score.mvvLvaScore) {
                stats.seePreferred.fetch_add(1, std::memory_order_relaxed);
            } else {
                stats.mvvLvaPreferred.fetch_add(1, std::memory_order_relaxed);
            }
            
            // Log discrepancy in testing mode
            if (m_mode == SEEMode::TESTING) {
                logDiscrepancy(board, move, score.mvvLvaScore, score.seeValue);
            }
        }
        
        results.push_back(score);
    }
    
    return results;
}

// Score a move with SEE
SEEValue SEEMoveOrdering::scoreMoveWithSEE(const Board& board, Move move) const {
    if (isCapture(move) || isPromotion(move) || isEnPassant(move)) {
        return m_see.see(board, move);
    }
    return 0;  // Quiet moves have SEE value of 0
}

// Compare SEE and MVV-LVA ordering for a pair of moves
bool SEEMoveOrdering::compareOrdering(const Board& board, Move a, Move b) const {
    int mvvA = MvvLvaOrdering::scoreMove(board, a);
    int mvvB = MvvLvaOrdering::scoreMove(board, b);
    
    SEEValue seeA = scoreMoveWithSEE(board, a);
    SEEValue seeB = scoreMoveWithSEE(board, b);
    
    // Check if they agree on ordering
    bool mvvLvaPrefers = mvvA > mvvB;
    bool seePrefers = seeA > seeB;
    
    return mvvLvaPrefers == seePrefers;
}

// Log discrepancies to file
void SEEMoveOrdering::logDiscrepancy(const Board& board, Move move, int mvvScore, SEEValue seeValue) const {
    if (!m_logFile.is_open()) return;
    
    m_logFile << "Discrepancy detected:\n";
    m_logFile << "  Move: " << SafeMoveExecutor::moveToString(move) << "\n";
    m_logFile << "  FEN: " << board.toFEN() << "\n";
    m_logFile << "  MVV-LVA Score: " << mvvScore << "\n";
    m_logFile << "  SEE Value: " << seeValue << "\n";
    
    // Add move details
    if (isCapture(move)) {
        Square from = moveFrom(move);
        Square to = moveTo(move);
        Piece attacker = board.pieceAt(from);
        Piece victim = board.pieceAt(to);
        m_logFile << "  Capture: " << PIECE_CHARS[attacker] << "x" << PIECE_CHARS[victim] << "\n";
    }
    if (isPromotion(move)) {
        m_logFile << "  Promotion to: " << promotionType(move) << "\n";
    }
    if (isEnPassant(move)) {
        m_logFile << "  En passant capture\n";
    }
    
    m_logFile << "\n";
    m_logFile.flush();
}

// Determine if a move should use SEE
bool SEEMoveOrdering::shouldUseSEE(Move move) const {
    return isCapture(move) || isPromotion(move) || isEnPassant(move);
}

// Main ordering function that dispatches based on mode
void SEEMoveOrdering::orderMoves(const Board& board, MoveList& moves) const {
    if (moves.size() <= 1) return;
    
    switch (m_mode) {
        case SEEMode::OFF:
            // Use MVV-LVA only
            {
                MvvLvaOrdering mvvLva;
                mvvLva.orderMoves(board, moves);
            }
            break;
            
        case SEEMode::TESTING:
            orderMovesTestingMode(board, moves);
            break;
            
        case SEEMode::SHADOW:
            orderMovesShadowMode(board, moves);
            break;
            
        case SEEMode::PRODUCTION:
            orderMovesWithSEE(board, moves);
            break;
    }
}

// Day 5.2: Testing mode - use SEE but log everything
void SEEMoveOrdering::orderMovesTestingMode(const Board& board, MoveList& moves) const {
    // First, get parallel scores for analysis
    auto parallelScores = scoreMovesParallel(board, moves);
    
    // Log summary statistics
    if (g_debugMoveOrdering) {
        std::cout << "[SEE Testing Mode] Ordering " << moves.size() << " moves\n";
        std::cout << "  Agreement rate: " << std::fixed << std::setprecision(1) 
                  << getStats().agreementRate() << "%\n";
    }
    
    // Now order using SEE for captures, MVV-LVA for quiet moves
    auto captureEnd = std::stable_partition(moves.begin(), moves.end(),
        [this](const Move& move) {
            return shouldUseSEE(move);
        });
    
    // Sort captures by SEE value
    if (captureEnd != moves.begin()) {
        std::sort(moves.begin(), captureEnd,
            [this, &board](const Move& a, const Move& b) {
                SEEValue seeA = m_see.see(board, a);
                SEEValue seeB = m_see.see(board, b);
                
                // Log values in testing mode
                if (g_debugMoveOrdering) {
                    std::cout << "  " << SafeMoveExecutor::moveToString(a) << ": SEE=" << seeA << "\n";
                    std::cout << "  " << SafeMoveExecutor::moveToString(b) << ": SEE=" << seeB << "\n";
                }
                
                // Order by SEE value (higher is better)
                // If equal SEE, fall back to MVV-LVA
                if (seeA != seeB) {
                    return seeA > seeB;
                }
                return MvvLvaOrdering::scoreMove(board, a) > MvvLvaOrdering::scoreMove(board, b);
            });
    }
    
    // Quiet moves remain in original order
}

// Day 5.3: Shadow mode - calculate both but use MVV-LVA
void SEEMoveOrdering::orderMovesShadowMode(const Board& board, MoveList& moves) const {
    // Calculate parallel scores to track agreement
    auto parallelScores = scoreMovesParallel(board, moves);
    
    // Create a copy of moves to see what SEE ordering would be
    MoveList seeOrdered = moves;
    
    // Order the copy with SEE
    auto seeCaptureEnd = std::stable_partition(seeOrdered.begin(), seeOrdered.end(),
        [this](const Move& move) {
            return shouldUseSEE(move);
        });
    
    if (seeCaptureEnd != seeOrdered.begin()) {
        std::sort(seeOrdered.begin(), seeCaptureEnd,
            [this, &board](const Move& a, const Move& b) {
                SEEValue seeA = m_see.see(board, a);
                SEEValue seeB = m_see.see(board, b);
                if (seeA != seeB) {
                    return seeA > seeB;
                }
                return MvvLvaOrdering::scoreMove(board, a) > MvvLvaOrdering::scoreMove(board, b);
            });
    }
    
    // Now actually order with MVV-LVA
    MvvLvaOrdering mvvLva;
    mvvLva.orderMoves(board, moves);
    
    // Compare the two orderings and log differences
    int differences = 0;
    for (size_t i = 0; i < std::min(size_t(10), moves.size()); ++i) {
        if (moves[i] != seeOrdered[i]) {
            differences++;
            if (g_debugMoveOrdering) {
                std::cout << "[SEE Shadow] Position " << i << " differs:\n";
                std::cout << "  MVV-LVA: " << SafeMoveExecutor::moveToString(moves[i]) << "\n";
                std::cout << "  SEE would pick: " << SafeMoveExecutor::moveToString(seeOrdered[i]) << "\n";
            }
        }
    }
    
    if (g_debugMoveOrdering && differences > 0) {
        std::cout << "[SEE Shadow] Total ordering differences: " << differences 
                  << " in top 10 moves\n";
    }
}

// Day 5.4: Production mode - use SEE for real
void SEEMoveOrdering::orderMovesWithSEE(const Board& board, MoveList& moves) const {
    // Partition captures/promotions from quiet moves
    auto captureEnd = std::stable_partition(moves.begin(), moves.end(),
        [this](const Move& move) {
            return shouldUseSEE(move);
        });
    
    // Sort captures by SEE value
    if (captureEnd != moves.begin()) {
        std::sort(moves.begin(), captureEnd,
            [this, &board](const Move& a, const Move& b) {
                // Get SEE values with error handling
                SEEValue seeA = SEE_INVALID;
                SEEValue seeB = SEE_INVALID;
                
                try {
                    seeA = m_see.see(board, a);
                } catch (...) {
                    // Fall back to MVV-LVA if SEE fails
                    seeA = MvvLvaOrdering::scoreMove(board, a);
                }
                
                try {
                    seeB = m_see.see(board, b);
                } catch (...) {
                    // Fall back to MVV-LVA if SEE fails
                    seeB = MvvLvaOrdering::scoreMove(board, b);
                }
                
                // Handle invalid SEE values
                if (seeA == SEE_INVALID) {
                    seeA = MvvLvaOrdering::scoreMove(board, a);
                }
                if (seeB == SEE_INVALID) {
                    seeB = MvvLvaOrdering::scoreMove(board, b);
                }
                
                // Order by SEE value (higher is better)
                if (seeA != seeB) {
                    return seeA > seeB;
                }
                
                // If equal SEE, fall back to MVV-LVA for stability
                return MvvLvaOrdering::scoreMove(board, a) > MvvLvaOrdering::scoreMove(board, b);
            });
    }
    
    // Quiet moves remain in original order (preserving castling first, etc.)
}

// Helper function to parse SEE mode from string
SEEMode parseSEEMode(const std::string& mode) {
    if (mode == "off" || mode == "OFF") return SEEMode::OFF;
    if (mode == "testing" || mode == "TESTING") return SEEMode::TESTING;
    if (mode == "shadow" || mode == "SHADOW") return SEEMode::SHADOW;
    if (mode == "production" || mode == "PRODUCTION") return SEEMode::PRODUCTION;
    return SEEMode::OFF;  // Default to off if unknown
}

// Helper function to convert SEE mode to string
std::string seeModeToString(SEEMode mode) {
    switch (mode) {
        case SEEMode::OFF: return "off";
        case SEEMode::TESTING: return "testing";
        case SEEMode::SHADOW: return "shadow";
        case SEEMode::PRODUCTION: return "production";
        default: return "unknown";
    }
}

} // namespace seajay::search