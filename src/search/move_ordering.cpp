#include "move_ordering.h"
#include "../core/board_safety.h"  // For moveToString
#include "../core/move_generation.h"  // For MoveGenerator::isPseudoLegal (MO2a)
#include <algorithm>
#include <iostream>
#include <chrono>  // For timestamp in log file
#include <sstream>
#include <numeric>
#include <cassert>
#include <cstdint>
#include <optional>

// Debug output control
#ifdef DEBUG_MOVE_ORDERING
    static bool g_debugMoveOrdering = true;
#else
    static bool g_debugMoveOrdering = false;
#endif

namespace seajay::search {

#ifdef SEAJAY_DEBUG_MOVE_PICKING
namespace {
MovePickingStats g_movePickingStats;
}

bool movePickingStatsEnabled() noexcept {
    return true;
}

void resetMovePickingStats() noexcept {
    g_movePickingStats = MovePickingStats{};
}

void recordMoveListForStats(std::size_t totalMoves) noexcept {
    g_movePickingStats.moveLists++;
    g_movePickingStats.totalMoves += static_cast<uint64_t>(totalMoves);
}

void recordCapturePartitionForStats(std::size_t captureMoves) noexcept {
    if (captureMoves == 0) {
        return;
    }
    g_movePickingStats.capturePartitions++;
    g_movePickingStats.capturePartitionMoves += static_cast<uint64_t>(captureMoves);
}

static void accumulateSortCounters(MovePickingSortKind kind, std::size_t sortedMoves) {
    const uint64_t moveCount = static_cast<uint64_t>(sortedMoves);
    switch (kind) {
        case MovePickingSortKind::Captures:
            g_movePickingStats.captureStableSorts++;
            g_movePickingStats.captureStableSortMoves += moveCount;
            break;
        case MovePickingSortKind::Quiet:
            g_movePickingStats.quietStableSorts++;
            g_movePickingStats.quietStableSortMoves += moveCount;
            break;
        case MovePickingSortKind::RootQuiet:
            g_movePickingStats.rootQuietStableSorts++;
            g_movePickingStats.rootQuietStableSortMoves += moveCount;
            break;
        case MovePickingSortKind::Auxiliary:
        default:
            g_movePickingStats.auxiliaryStableSorts++;
            g_movePickingStats.auxiliaryStableSortMoves += moveCount;
            break;
    }
}

void recordStableSortForStats(MovePickingSortKind kind, std::size_t sortedMoves) noexcept {
    if (sortedMoves == 0) {
        return;
    }
    accumulateSortCounters(kind, sortedMoves);
}

MovePickingStats snapshotMovePickingStats() noexcept {
    return g_movePickingStats;
}

std::string formatMovePickingStats(const MovePickingStats& stats) {
    if (stats.moveLists == 0) {
        return {};
    }

    std::ostringstream oss;
    oss << "lists=" << stats.moveLists
        << " moves=" << stats.totalMoves;

    if (stats.capturePartitions > 0) {
        oss << " capturePartition=" << stats.capturePartitions
            << "(" << stats.capturePartitionMoves << ")";
    }
    if (stats.captureStableSorts > 0) {
        oss << " captureSorts=" << stats.captureStableSorts
            << "(" << stats.captureStableSortMoves << ")";
    }
    if (stats.quietStableSorts > 0) {
        oss << " quietSorts=" << stats.quietStableSorts
            << "(" << stats.quietStableSortMoves << ")";
    }
    if (stats.rootQuietStableSorts > 0) {
        oss << " rootQuietSorts=" << stats.rootQuietStableSorts
            << "(" << stats.rootQuietStableSortMoves << ")";
    }
    if (stats.auxiliaryStableSorts > 0) {
        oss << " auxSorts=" << stats.auxiliaryStableSorts
            << "(" << stats.auxiliaryStableSortMoves << ")";
    }

    return oss.str();
}

#else

bool movePickingStatsEnabled() noexcept {
    return false;
}

void resetMovePickingStats() noexcept {}
void recordMoveListForStats(std::size_t) noexcept {}
void recordCapturePartitionForStats(std::size_t) noexcept {}
void recordStableSortForStats(MovePickingSortKind, std::size_t) noexcept {}

MovePickingStats snapshotMovePickingStats() noexcept {
    return MovePickingStats{};
}

std::string formatMovePickingStats(const MovePickingStats&) {
    return {};
}

#endif

namespace {
struct CaptureScoreBuffer {
    std::array<Move, seajay::MAX_MOVES> moves{};
    std::array<int, seajay::MAX_MOVES> scores{};
    std::array<std::size_t, seajay::MAX_MOVES> order{};
};

inline bool isQuietMoveCandidate(Move move) noexcept {
    return !isPromotion(move) && !isCapture(move) && !isEnPassant(move);
}

struct QuietOrderingBuffers {
    std::array<Move, seajay::MAX_MOVES> moves{};
    std::array<int32_t, seajay::MAX_MOVES> scores{};
    std::array<uint8_t, seajay::MAX_MOVES> used{};
    std::array<Move, seajay::MAX_MOVES> ordered{};
};

MoveList::iterator findQuietRangeEnd(MoveList::iterator quietStart,
                                     MoveList::iterator listEnd) {
    return std::find_if(quietStart, listEnd, [](const Move& move) {
        return isPromotion(move) || isCapture(move) || isEnPassant(move);
    });
}

void applyOrderedQuietMoves(MoveList::iterator quietStart,
                            const QuietOrderingBuffers& buffers,
                            std::size_t quietCount) {
    auto dest = quietStart;
    for (std::size_t idx = 0; idx < quietCount; ++idx, ++dest) {
        *dest = buffers.ordered[idx];
    }
}

void populateQuietBuffers(const Board& board,
                          MoveList::iterator quietStart,
                          MoveList::iterator quietEnd,
                          QuietOrderingBuffers& buffers,
                          const HistoryHeuristic& history,
                          std::optional<int32_t> cmhNumerator,
                          const CounterMoveHistory* counterMoveHistory,
                          Move prevMove) {
    const std::size_t quietCount = static_cast<std::size_t>(std::distance(quietStart, quietEnd));
    Color side = board.sideToMove();

    for (std::size_t idx = 0; idx < quietCount; ++idx) {
        const Move move = *(quietStart + static_cast<std::ptrdiff_t>(idx));
        buffers.moves[idx] = move;
        buffers.used[idx] = 0;

        int32_t baseScore = static_cast<int32_t>(history.getScore(side, moveFrom(move), moveTo(move)));

        if (cmhNumerator && counterMoveHistory && prevMove != NO_MOVE) {
            constexpr int cmhDenominator = 2;
            int32_t cmhScore = static_cast<int32_t>(counterMoveHistory->getScore(prevMove, move));
            cmhScore = (cmhScore * *cmhNumerator) / cmhDenominator;
            buffers.scores[idx] = baseScore * 2 + cmhScore;
        } else if (cmhNumerator) {
            buffers.scores[idx] = baseScore * 2;
        } else {
            buffers.scores[idx] = baseScore;
        }
    }
}

bool emitIfPresent(const Board& board,
                   Move move,
                   std::size_t quietCount,
                   QuietOrderingBuffers& buffers,
                   std::size_t& writeIndex) {
    if (!isQuietMoveCandidate(move)) {
        return false;
    }

    Square from = moveFrom(move);
    Piece piece = board.pieceAt(from);
    if (piece == NO_PIECE || colorOf(piece) != board.sideToMove()) {
        return false;
    }

    for (std::size_t idx = 0; idx < quietCount; ++idx) {
        if (!buffers.used[idx] && buffers.moves[idx] == move) {
            buffers.used[idx] = 1;
            buffers.ordered[writeIndex++] = move;
            return true;
        }
    }
    return false;
}

void emitRemainingInOrder(QuietOrderingBuffers& buffers,
                          std::size_t quietCount,
                          std::size_t& writeIndex) {
    for (std::size_t idx = 0; idx < quietCount; ++idx) {
        if (!buffers.used[idx]) {
            buffers.used[idx] = 1;
            buffers.ordered[writeIndex++] = buffers.moves[idx];
        }
    }
}

void emitRemainingByScore(QuietOrderingBuffers& buffers,
                          std::size_t quietCount,
                          std::size_t& writeIndex) {
    std::vector<std::size_t> remaining;
    remaining.reserve(quietCount);
    for (std::size_t idx = 0; idx < quietCount; ++idx) {
        if (!buffers.used[idx]) {
            remaining.push_back(idx);
        }
    }

    std::stable_sort(remaining.begin(), remaining.end(),
        [&buffers](std::size_t lhs, std::size_t rhs) {
            if (buffers.scores[lhs] == buffers.scores[rhs]) {
                return lhs < rhs;  // Preserve generator order on ties
            }
            return buffers.scores[lhs] > buffers.scores[rhs];
        });

    for (std::size_t idx : remaining) {
        buffers.used[idx] = 1;
        buffers.ordered[writeIndex++] = buffers.moves[idx];
    }
}

void reorderQuietSectionBasic(const Board& board,
                              MoveList::iterator quietStart,
                              MoveList::iterator quietEnd,
                              const KillerMoves& killers,
                              const HistoryHeuristic& history,
                              const CounterMoves* counterMoves,
                              Move prevMove,
                              int ply,
                              int countermoveBonus,
                              QuietOrderingRequest request) {
    const std::size_t quietCount = static_cast<std::size_t>(std::distance(quietStart, quietEnd));
    if (quietCount == 0) {
        return;
    }

    if (request == QuietOrderingRequest::Full) {
        recordStableSortForStats(MovePickingSortKind::Quiet, quietCount);
    }

    QuietOrderingBuffers buffers;
    populateQuietBuffers(board, quietStart, quietEnd, buffers, history, std::nullopt, nullptr, NO_MOVE);

    std::size_t writeIndex = 0;
    for (int slot = 0; slot < 2; ++slot) {
        emitIfPresent(board, killers.getKiller(ply, slot), quietCount, buffers, writeIndex);
    }

    if (counterMoves && prevMove != NO_MOVE && countermoveBonus > 0) {
        emitIfPresent(board, counterMoves->getCounterMove(prevMove), quietCount, buffers, writeIndex);
    }

    if (request == QuietOrderingRequest::ChecksOnly) {
        emitRemainingInOrder(buffers, quietCount, writeIndex);
    } else {
        emitRemainingByScore(buffers, quietCount, writeIndex);
    }

#ifdef DEBUG
    assert(writeIndex == quietCount && "All quiet moves must be emitted");
#endif

    applyOrderedQuietMoves(quietStart, buffers, quietCount);
}

void reorderQuietSectionWithHistory(const Board& board,
                                    MoveList::iterator quietStart,
                                    MoveList::iterator quietEnd,
                                    const KillerMoves& killers,
                                    const HistoryHeuristic& history,
                                    const CounterMoves& counterMoves,
                                    const CounterMoveHistory& counterMoveHistory,
                                    Move prevMove,
                                    int ply,
                                    int countermoveBonus,
                                    QuietOrderingRequest request,
                                    float cmhWeight) {
    const std::size_t quietCount = static_cast<std::size_t>(std::distance(quietStart, quietEnd));
    if (quietCount == 0) {
        return;
    }

    if (request == QuietOrderingRequest::Full) {
        recordStableSortForStats(MovePickingSortKind::Auxiliary, quietCount);
    }

    QuietOrderingBuffers buffers;
    const int32_t cmhNumerator = static_cast<int32_t>(cmhWeight * 2.0f + 0.5f);
    populateQuietBuffers(board, quietStart, quietEnd, buffers, history, cmhNumerator,
                         &counterMoveHistory, prevMove);

    std::size_t writeIndex = 0;
    for (int slot = 0; slot < 2; ++slot) {
        emitIfPresent(board, killers.getKiller(ply, slot), quietCount, buffers, writeIndex);
    }

    if (countermoveBonus > 0 && prevMove != NO_MOVE) {
        emitIfPresent(board, counterMoves.getCounterMove(prevMove), quietCount, buffers, writeIndex);
    }

    if (request == QuietOrderingRequest::ChecksOnly) {
        emitRemainingInOrder(buffers, quietCount, writeIndex);
    } else {
        emitRemainingByScore(buffers, quietCount, writeIndex);
    }

#ifdef DEBUG
    assert(writeIndex == quietCount && "All quiet moves must be emitted");
#endif

    applyOrderedQuietMoves(quietStart, buffers, quietCount);
}
}

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

    recordMoveListForStats(moves.size());
    
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

    const std::size_t captureCount = static_cast<std::size_t>(std::distance(moves.begin(), captureEnd));
    recordCapturePartitionForStats(captureCount);

    // Only sort the captures/promotions portion if there are any
    if (captureCount != 0) {
        CaptureScoreBuffer cache;
        std::array<std::size_t, seajay::MAX_MOVES> goodIndices{};
        std::array<std::size_t, seajay::MAX_MOVES> badIndices{};
        std::size_t goodCount = 0;
        std::size_t badCount = 0;

        auto it = moves.begin();
        for (std::size_t idx = 0; idx < captureCount; ++idx, ++it) {
            const Move move = *it;
            cache.moves[idx] = move;
            cache.scores[idx] = scoreMove(board, move);
            cache.order[idx] = idx;

            const bool isPromotionMove = isPromotion(move);
            bool isGoodCapture = isPromotionMove;
            if (!isGoodCapture) {
                if (cache.scores[idx] >= 0) {
                    isGoodCapture = true;
                } else {
                    isGoodCapture = seajay::seeGE(board, move, 0);
                }
            }

            if (isGoodCapture) {
                goodIndices[goodCount++] = idx;
            } else {
                badIndices[badCount++] = idx;
            }
        }

        auto stableSelect = [&cache](std::array<std::size_t, seajay::MAX_MOVES>& indices,
                                     std::size_t count) {
            for (std::size_t i = 0; i < count; ++i) {
                std::size_t best = i;
                for (std::size_t j = i + 1; j < count; ++j) {
                    const int scoreBest = cache.scores[indices[best]];
                    const int scoreCandidate = cache.scores[indices[j]];
                    if (scoreCandidate > scoreBest) {
                        best = j;
                    }
                }
                if (best != i) {
                    std::swap(indices[i], indices[best]);
                }
            }
        };

        if (goodCount > 0) {
            recordStableSortForStats(MovePickingSortKind::Captures, goodCount);
            stableSelect(goodIndices, goodCount);
        }
        if (badCount > 0) {
            recordStableSortForStats(MovePickingSortKind::Auxiliary, badCount);
            stableSelect(badIndices, badCount);
        }

        std::array<Move, seajay::MAX_MOVES> reordered{};
        std::size_t writeIndex = 0;

        for (std::size_t i = 0; i < goodCount; ++i) {
            reordered[writeIndex++] = cache.moves[goodIndices[i]];
        }

        for (auto quietIt = captureEnd; quietIt != moves.end(); ++quietIt) {
            reordered[writeIndex++] = *quietIt;
        }

        for (std::size_t i = 0; i < badCount; ++i) {
            reordered[writeIndex++] = cache.moves[badIndices[i]];
        }

        assert(writeIndex == moves.size());

        auto dest = moves.begin();
        for (std::size_t i = 0; i < writeIndex; ++i, ++dest) {
            *dest = reordered[i];
        }
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
                                          const HistoryHeuristic& history, int ply,
                                          QuietOrderingRequest request) const {
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
    
    auto quietEnd = findQuietRangeEnd(quietStart, moves.end());
    if (quietEnd == quietStart) {
        return;
    }

    reorderQuietSectionBasic(board,
                              quietStart,
                              quietEnd,
                              killers,
                              history,
                              nullptr,
                              NO_MOVE,
                              ply,
                              0,
                              request);
}

// Order moves with killers, history, and countermoves (Stage 23, CM3.3)
void MvvLvaOrdering::orderMovesWithHistory(const Board& board, MoveList& moves,
                                          const KillerMoves& killers,
                                          const HistoryHeuristic& history,
                                          const CounterMoves& counterMoves,
                                          Move prevMove, int ply, int countermoveBonus,
                                          QuietOrderingRequest request) const {
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
    
    auto quietEnd = findQuietRangeEnd(quietStart, moves.end());
    if (quietEnd == quietStart) {
        return;
    }

    reorderQuietSectionBasic(board,
                              quietStart,
                              quietEnd,
                              killers,
                              history,
                              &counterMoves,
                              prevMove,
                              ply,
                              countermoveBonus,
                              request);
}

// Phase 4.3.a: Order moves with counter-move history
void MvvLvaOrdering::orderMovesWithHistory(const Board& board, MoveList& moves,
                                          const KillerMoves& killers, 
                                          const HistoryHeuristic& history,
                                          const CounterMoves& counterMoves,
                                          const CounterMoveHistory& counterMoveHistory,
                                          Move prevMove, int ply, int countermoveBonus,
                                          float cmhWeight,
                                          QuietOrderingRequest request) const {
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

    auto quietEnd = findQuietRangeEnd(quietStart, moves.end());
    if (quietEnd == quietStart) {
        return;
    }

    reorderQuietSectionWithHistory(board,
                                    quietStart,
                                    quietEnd,
                                    killers,
                                    history,
                                    counterMoves,
                                    counterMoveHistory,
                                    prevMove,
                                    ply,
                                    countermoveBonus,
                                    request,
                                    cmhWeight);
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
    recordCapturePartitionForStats(static_cast<std::size_t>(std::distance(moves.begin(), captureEnd)));

    // Sort captures by SEE value
    if (captureEnd != moves.begin()) {
        recordStableSortForStats(MovePickingSortKind::Captures,
                                 static_cast<std::size_t>(std::distance(moves.begin(), captureEnd)));
        std::stable_sort(moves.begin(), captureEnd,
            [this, &board](const Move& a, const Move& b) {
                SEEValue seeA = m_see.see(board, a);
                SEEValue seeB = m_see.see(board, b);
                
                // Log values in testing mode
                if (g_debugMoveOrdering) {
                    std::cout << "  " << SafeMoveExecutor::moveToString(a) << ": SEE=" << seeA << "\n";
                    std::cout << "  " << SafeMoveExecutor::moveToString(b) << ": SEE=" << seeB << "\n";
                }
                
                // Phase 2a.5a: Preserve legacy behavior with stable ordering
                // Primary: SEE value (higher is better)
                if (seeA != seeB) {
                    return seeA > seeB;
                }
                
                // Secondary: MVV-LVA score (preserve existing fallback)
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
        recordStableSortForStats(MovePickingSortKind::Auxiliary,
                                 static_cast<std::size_t>(std::distance(seeOrdered.begin(), seeCaptureEnd)));
        std::stable_sort(seeOrdered.begin(), seeCaptureEnd,
            [this, &board](const Move& a, const Move& b) {
                SEEValue seeA = m_see.see(board, a);
                SEEValue seeB = m_see.see(board, b);
                
                // Phase 2a.5a: Preserve legacy behavior
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
    recordCapturePartitionForStats(static_cast<std::size_t>(std::distance(moves.begin(), captureEnd)));

    // Sort captures by SEE value
    if (captureEnd != moves.begin()) {
        recordStableSortForStats(MovePickingSortKind::Captures,
                                 static_cast<std::size_t>(std::distance(moves.begin(), captureEnd)));
        std::stable_sort(moves.begin(), captureEnd,
            [this, &board](const Move& a, const Move& b) {
                // Get SEE values without exceptions (SEE is noexcept)
                SEEValue seeA = m_see.see(board, a);
                SEEValue seeB = m_see.see(board, b);

                // Fallback to MVV-LVA if SEE returned invalid
                if (seeA == SEE_INVALID) seeA = MvvLvaOrdering::scoreMove(board, a);
                if (seeB == SEE_INVALID) seeB = MvvLvaOrdering::scoreMove(board, b);

                // Phase 2a.5a: Preserve legacy behavior
                // Primary: SEE value (higher is better)
                if (seeA != seeB) return seeA > seeB;

                // Secondary: MVV-LVA score for stability (preserve existing fallback)
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
