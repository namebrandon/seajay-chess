#include "ranked_move_picker.h"
#include "move_ordering.h"
#include <algorithm>
#include <cassert>
#include <cstring>  // For std::fill
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <unordered_set>
#ifdef DEBUG
#include <iostream> // DEV one-shot logging (debug builds only)
#endif

#include "../core/board_safety.h"
#include "../core/see.h"

namespace seajay {
namespace search {

const char* movePickerBucketName(MovePickerBucket bucket) noexcept {
    switch (bucket) {
        case MovePickerBucket::TT:
            return "TT";
        case MovePickerBucket::GoodCapture:
            return "GoodCapture";
        case MovePickerBucket::BadCapture:
            return "BadCapture";
        case MovePickerBucket::Promotion:
            return "Promotion";
        case MovePickerBucket::Killer:
            return "Killer";
        case MovePickerBucket::CounterMove:
            return "Counter";
        case MovePickerBucket::QuietCounterHistory:
            return "QuietCMH";
        case MovePickerBucket::QuietHistory:
            return "QuietHist";
        case MovePickerBucket::QuietFallback:
            return "QuietOther";
        case MovePickerBucket::Other:
        default:
            return "Other";
    }
}

namespace {
constexpr int HISTORY_GATING_DEPTH = 2;

struct DumpMoveOrderConfig {
    bool enabled = false;
    int plyLimit = 0;
    int countLimit = 0;
};

const DumpMoveOrderConfig& dumpMoveOrderConfig() {
    static const DumpMoveOrderConfig cfg = [] {
        DumpMoveOrderConfig config;
        const char* flag = std::getenv("MOVE_ORDER_DUMP");
        if (!flag || flag[0] == '\0') {
            return config;
        }
        config.enabled = true;
        config.plyLimit = 2;
        config.countLimit = 64;
        int parsedPly = 0;
        int parsedCount = 0;
        if (std::sscanf(flag, "%d:%d", &parsedPly, &parsedCount) >= 1) {
            if (parsedPly > 0) config.plyLimit = parsedPly;
            if (parsedCount > 0) config.countLimit = parsedCount;
        }
        return config;
    }();
    return cfg;
}

bool dumpMoveOrderEnabled() {
    return dumpMoveOrderConfig().enabled;
}

struct StageLogConfig {
    bool enabled = false;
    int plyLimit = 2;
    int countLimit = 64;
};

const StageLogConfig& stageLogConfig() {
    static const StageLogConfig cfg = [] {
        StageLogConfig config;
        const char* flag = std::getenv("MOVE_PICKER_STAGE_LOG");
        if (!flag || flag[0] == '\0') {
            return config;
        }
        config.enabled = true;
        int parsedPly = 0;
        int parsedCount = 0;
        if (std::sscanf(flag, "%d:%d", &parsedPly, &parsedCount) >= 1) {
            if (parsedPly > 0) config.plyLimit = parsedPly;
            if (parsedCount > 0) config.countLimit = parsedCount;
        }
        return config;
    }();
    return cfg;
}

bool stageLogEnabled() {
    return stageLogConfig().enabled;
}

}

// Use the MVV-LVA constants from move_ordering.h
// VICTIM_VALUES and ATTACKER_VALUES are already defined there

// Phase 2a.3: Scoring constants
static constexpr int16_t PROMOTION_BONUS[4] = {
    200,  // Knight promotion
    200,  // Bishop promotion  
    500,  // Rook promotion
    900   // Queen promotion
};

static constexpr int16_t KILLER_BONUS = 1000;
static constexpr int16_t COUNTERMOVE_BONUS = 500;
static constexpr int16_t REFUTATION_BONUS = 300;
static constexpr int16_t CHECK_BONUS = 50;
static constexpr int16_t HISTORY_MULTIPLIER = 2;

/**
 * Compute MVV-LVA score for a capture move
 * Higher scores = better captures (e.g., PxQ > QxP)
 */
int16_t RankedMovePicker::computeMvvLvaScore(Move move) const {
    // Only score captures (including capture-promotions)
    if (!isCapture(move) && !isEnPassant(move)) {
        return 0;
    }
    
    // Handle en passant (always PxP)
    if (isEnPassant(move)) {
        return VICTIM_VALUES[PAWN] - ATTACKER_VALUES[PAWN];  // 100 - 1 = 99
    }
    
    // Get attacker and victim pieces
    Square fromSq = moveFrom(move);
    Square toSq = moveTo(move);
    
    Piece attackingPiece = m_board.pieceAt(fromSq);
    Piece capturedPiece = m_board.pieceAt(toSq);
    
    if (attackingPiece == NO_PIECE || capturedPiece == NO_PIECE) {
        return 0;  // Safety fallback
    }
    
    PieceType attacker = typeOf(attackingPiece);
    PieceType victim = typeOf(capturedPiece);
    
    // For promotions, attacker is always PAWN
    if (isPromotion(move)) {
        attacker = PAWN;
    }
    
    // MVV-LVA formula: victim_value - attacker_value
    return VICTIM_VALUES[victim] - ATTACKER_VALUES[attacker];
}

/**
 * Compute score for a quiet move
 * Based on killers, history, countermoves, and CMH
 */
int16_t RankedMovePicker::computeQuietScore(Move move) const {
    int16_t score = 0;
    
    // History score (can be negative)
    if (m_history) {
        score += m_history->getScore(m_board.sideToMove(), moveFrom(move), moveTo(move)) * HISTORY_MULTIPLIER;
    }
    
    // Killer move bonus
    // Fix: Correct parameter order is (ply, move) not (move, ply)
    if (m_killers && m_killers->isKiller(m_ply, move)) {
        score += KILLER_BONUS;
    }
    
    // Countermove bonus
    if (m_counterMoves && m_prevMove != NO_MOVE) {
        if (m_counterMoves->getCounterMove(m_prevMove) == move) {
            score += COUNTERMOVE_BONUS;
        }
    }
    
    // Counter-move history bonus (if available)
    if (m_counterMoveHistory && m_prevMove != NO_MOVE) {
        score += m_counterMoveHistory->getScore(m_prevMove, move) / 2;  // Scale down CMH
    }
    
    // Small check bonus (cheap detection - could add if needed)
    // For now, omit as it requires extra computation
    
    return score;
}

/**
 * Compute score for a non-capture promotion
 * Base quiet score plus promotion bonus
 */
int16_t RankedMovePicker::computePromotionScore(Move move) const {
    if (!isPromotion(move)) {
        return 0;
    }
    
    // Start with base quiet score
    int16_t score = computeQuietScore(move);
    
    // Add promotion bonus based on piece type
    PieceType promoType = promotionType(move);
    if (promoType >= KNIGHT && promoType <= QUEEN) {
        score += PROMOTION_BONUS[promoType - KNIGHT];
    }
    
    return score;
}

/**
 * Insert a move into the shortlist if it's good enough
 * Maintains top-K moves sorted by score
 */
void RankedMovePicker::insertIntoShortlist(Move move, int16_t score) {
#ifdef DEBUG
    // Phase 2a.5b: Pre-condition checks
    assert(m_shortlistSize >= 0 && m_shortlistSize <= MAX_SHORTLIST_SIZE);
#endif
    
    // If shortlist not full, always insert
    if (m_shortlistSize < MAX_SHORTLIST_SIZE) {
        // Find insertion position (keep sorted, highest scores first)
        int insertPos = m_shortlistSize;
        for (int i = 0; i < m_shortlistSize; i++) {
            if (score > m_shortlistScores[i]) {
                insertPos = i;
                break;
            }
        }
        
        // Shift elements to make room
        for (int i = m_shortlistSize; i > insertPos; i--) {
#ifdef DEBUG
            // Phase 2a.5b: Assert array bounds
            assert(i > 0 && i <= MAX_SHORTLIST_SIZE);
            assert(i-1 >= 0 && i-1 < MAX_SHORTLIST_SIZE);
#endif
            m_shortlist[i] = m_shortlist[i-1];
            m_shortlistScores[i] = m_shortlistScores[i-1];
        }
        
        // Insert the new move
#ifdef DEBUG
        assert(insertPos >= 0 && insertPos < MAX_SHORTLIST_SIZE);
        assert(m_shortlistSize < MAX_SHORTLIST_SIZE);
#endif
        m_shortlist[insertPos] = move;
        m_shortlistScores[insertPos] = score;
        m_shortlistSize++;
#ifdef DEBUG
        assert(m_shortlistSize <= MAX_SHORTLIST_SIZE);
#endif
    }
    // If shortlist is full, check if this move is better than the worst
    else if (score > m_shortlistScores[MAX_SHORTLIST_SIZE - 1]) {
        // Find insertion position
        int insertPos = MAX_SHORTLIST_SIZE - 1;
        for (int i = 0; i < MAX_SHORTLIST_SIZE - 1; i++) {
            if (score > m_shortlistScores[i]) {
                insertPos = i;
                break;
            }
        }
        
        // Shift elements (dropping the worst)
        for (int i = MAX_SHORTLIST_SIZE - 1; i > insertPos; i--) {
#ifdef DEBUG
            // Phase 2a.5b: Assert array bounds  
            assert(i > 0 && i < MAX_SHORTLIST_SIZE);
            assert(i-1 >= 0 && i-1 < MAX_SHORTLIST_SIZE);
#endif
            m_shortlist[i] = m_shortlist[i-1];
            m_shortlistScores[i] = m_shortlistScores[i-1];
        }
        
        // Insert the new move
#ifdef DEBUG
        assert(insertPos >= 0 && insertPos < MAX_SHORTLIST_SIZE);
#endif
        m_shortlist[insertPos] = move;
        m_shortlistScores[insertPos] = score;
    }
}

/**
 * Check if a move is in the shortlist
 */
bool RankedMovePicker::isInShortlist(Move move) const {
    for (int i = 0; i < m_shortlistSize; i++) {
        if (m_shortlist[i] == move) {
            return true;
        }
    }
    return false;
}

/**
 * Phase 2a.4: In-check parity implementation
 */
RankedMovePicker::RankedMovePicker(const Board& board,
                                   Move ttMove,
                                   const KillerMoves* killers,
                                   const HistoryHeuristic* history,
                                   const CounterMoves* counterMoves,
                                   const CounterMoveHistory* counterMoveHistory,
                                   Move prevMove,
                                   int ply,
                                   int depth,
                                   int countermoveBonus,
                                   const SearchLimits* limits,
                                   SearchData* searchData)
    : m_board(board)
    , m_killers(killers)
    , m_history(history)
    , m_counterMoves(counterMoves)
    , m_counterMoveHistory(counterMoveHistory)
    , m_ttMove(ttMove)
    , m_prevMove(prevMove)
    , m_ply(ply)
    , m_depth(depth)
    , m_countermoveBonus(countermoveBonus)
    , m_limits(limits)
    , m_searchData(searchData)
    , m_shortlistSize(0)
    , m_shortlistIndex(0)
    , m_effectiveShortlistSize(0)  // Will be set based on depth
    , m_inCheck(board.isAttacked(board.kingSquare(board.sideToMove()), ~board.sideToMove()))
    , m_moveIndex(0)
    , m_captureScanIndex(0)
    , m_badCaptureCount(0)
    , m_badCaptureIndex(0)
    , m_ttMoveYielded(false)
    , m_yieldIndex(0)  // Phase 2b.2-fix: Always initialize
#ifdef DEBUG
    , m_generatedCount(0)
    , m_yieldedCount(0)
#endif
{
    // Initialize shortlist map to all false
    std::fill(std::begin(m_inShortlistMap), std::end(m_inShortlistMap), false);
    
    // Phase 2a.5b: Initialize arrays to prevent UB
    std::fill(std::begin(m_shortlist), std::end(m_shortlist), NO_MOVE);
    std::fill(std::begin(m_shortlistScores), std::end(m_shortlistScores), 0);
    std::fill(std::begin(m_badCaptures), std::end(m_badCaptures), NO_MOVE);
    
    // Calculate depth-based shortlist size (K)
    // Shallow depths: less overhead, fewer moves in shortlist
    // Deeper depths: full shortlist for better move ordering
    if (depth < 3) {
        m_effectiveShortlistSize = 0;  // No shortlist at very shallow depths
    } else if (depth < 6) {
        m_effectiveShortlistSize = 4;  // Small shortlist at shallow depths
    } else {
        m_effectiveShortlistSize = 8;  // Full shortlist at deeper depths
    }
    
#ifdef DEBUG
    // Phase 2a.5b: Assert shortlist size bounds
    assert(m_effectiveShortlistSize >= 0 && m_effectiveShortlistSize <= MAX_SHORTLIST_SIZE);
    assert(m_shortlistSize == 0 && "Shortlist size must be initialized to 0");
    assert(m_shortlistIndex == 0 && "Shortlist index must be initialized to 0");
#endif
    
    // Phase 2a.4: When in check, use optimized check evasion generation
    if (m_inCheck) {
        // Use the same generator as legacy (generateMovesForSearch calls generateCheckEvasions internally)
        // This is much more efficient than generateLegalMoves
        MoveGenerator::generateMovesForSearch(board, m_moves, false);
        
#ifdef DEBUG
        m_generatedCount = m_moves.size();
        // Phase 2a.8a: Assert that we only have evasions when in check
        // All moves generated should be check evasions
        // Defensive: in some integration branches Board may not expose isCheckmate().
        // Avoid relying on it here; generation should have produced evasions when in check.
        // In DEBUG builds, we could assert on m_generatedCount > 0, but keep release safe.
        (void)m_generatedCount; // no-op to avoid unused warning in some builds
#endif
        
        // Phase 2a.8b: Class-based ordering for check evasions
        if (limits && limits->useInCheckClassOrdering) {
            // Apply legacy ordering first for parity within classes
            static MvvLvaOrdering mvvLva;
            if (g_seeMoveOrdering.getMode() != SEEMode::OFF) {
                g_seeMoveOrdering.orderMoves(board, m_moves);
            } else {
                mvvLva.orderMoves(board, m_moves);
            }
            if (depth >= HISTORY_GATING_DEPTH && killers && history && counterMoves && counterMoveHistory) {
                float cmhWeight = limits ? limits->counterMoveHistoryWeight : 1.5f;
                mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                            *counterMoves, *counterMoveHistory,
                                            prevMove, ply, countermoveBonus, cmhWeight);
            } else if (killers && history && counterMoves) {
                mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                            *counterMoves, prevMove, ply, countermoveBonus);
            }

            // Now stably bring captures-of-checker to the front; leave remainder in legacy order
            const Color stm = board.sideToMove();
            const Square kingSq = board.kingSquare(stm);
            const Color oppSide = ~stm;
            Bitboard checkers = 0;
            checkers |= ::seajay::pawnAttacks(stm, kingSq) & board.pieces(oppSide, PAWN);
            checkers |= MoveGenerator::getKnightAttacks(kingSq) & board.pieces(oppSide, KNIGHT);
            checkers |= ::seajay::bishopAttacks(kingSq, board.occupied()) & (board.pieces(oppSide, BISHOP) | board.pieces(oppSide, QUEEN));
            checkers |= ::seajay::rookAttacks(kingSq, board.occupied()) & (board.pieces(oppSide, ROOK) | board.pieces(oppSide, QUEEN));
            const int numCheckers = popCount(checkers);
            const Square checkerSq = (numCheckers == 1) ? lsb(checkers) : NO_SQUARE;
            Bitboard blockMask = 0;
            if (numCheckers == 1) {
                const Piece checkerPiece = board.pieceAt(checkerSq);
                const PieceType checkerType = typeOf(checkerPiece);
                if (checkerType == BISHOP || checkerType == ROOK || checkerType == QUEEN) {
                    blockMask = ::seajay::between(checkerSq, kingSq);
                }
            }
            const int epDelta = (stm == WHITE) ? -8 : 8;  // Precompute once

            // Perf: predicate avoids expensive work on early rejects
            auto isCaptureOfChecker = [&](const Move& mv) -> bool {
                if (numCheckers != 1) return false;
                const Square to = moveTo(mv);
                // Fast path: en passant capture of checking pawn
                if (isEnPassant(mv)) {
                    const Square capturedSq = static_cast<Square>(to + epDelta);
                    return capturedSq == checkerSq;
                }
                // Regular capture must land on checker square
                if (to != checkerSq) return false;
                if (!isCapture(mv)) return false;
                // Exclude king moves (generator should not produce, but keep safe)
                const Square from = moveFrom(mv);
                return typeOf(board.pieceAt(from)) != KING;
            };
            auto class1End = std::stable_partition(m_moves.begin(), m_moves.end(), isCaptureOfChecker);

#ifdef DEBUG
            // 2a.8e: Safety checks and one-shot DEV log (debug builds only)
            // Double-check → generator should provide only king moves
            if (numCheckers > 1) {
                size_t nonKing = 0;
                for (const Move& mv : m_moves) {
                    if (typeOf(board.pieceAt(moveFrom(mv))) != KING) nonKing++;
                }
                assert(nonKing == 0 && "Double check should produce only king moves");
            }

            // Verify partition correctness: all before class1End satisfy predicate; none after do
            for (auto it = m_moves.begin(); it != class1End; ++it) {
                assert(isCaptureOfChecker(*it) && "Front segment must be captures-of-checker");
            }
            for (auto it = class1End; it != m_moves.end(); ++it) {
                assert(!isCaptureOfChecker(*it) && "Back segment must exclude captures-of-checker");
            }

            // Verify class targeting: any classified block move targets blockMask; captures target checkerSq
            size_t c1 = 0, c2 = 0, c3 = 0;
            for (const Move& mv : m_moves) {
                const Square from = moveFrom(mv);
                const Square to = moveTo(mv);
                const bool isK = (typeOf(board.pieceAt(from)) == KING);
                const bool isC1 = isCaptureOfChecker(mv);
                const bool isC2 = (!isK && numCheckers == 1 && blockMask && !isCapture(mv) && !isEnPassant(mv) && testBit(blockMask, to));
                if (isC1) {
                    // Ensure target matches checkerSq (EP aware already in predicate)
                    if (!isEnPassant(mv)) {
                        assert(to == checkerSq && "Class 1 capture must land on checker square");
                    }
                    c1++;
                } else if (isC2) {
                    // Ensure block squares are on blockMask
                    assert(testBit(blockMask, to) && "Block move must target block mask");
                    c2++;
                } else {
                    c3++;
                }
            }

            // One-shot DEV log for a few positions when toggle is enabled
            static int logCount = 0;
            if (logCount < 5) {
                std::cerr << "info string InCheckClassOrdering: checkers=" << numCheckers
                          << " c1(capture-checker)=" << c1
                          << " c2(block)=" << c2
                          << " c3(king/other)=" << c3 << std::endl;
                ++logCount;
            }
#endif

            // No shortlist when in check - we'll iterate evasions directly
#ifdef DEBUG
            assert(m_shortlistSize == 0 && "No shortlist when in check");
#endif
            return;  // Early return
        }
        
        // Apply legacy ordering to the evasions with history (same as non-check path)
        static MvvLvaOrdering mvvLva;
        
        if (g_seeMoveOrdering.getMode() != SEEMode::OFF) {
            g_seeMoveOrdering.orderMoves(board, m_moves);
        } else {
            mvvLva.orderMoves(board, m_moves);
        }
        
        // Apply history heuristics to evasions for better move ordering
        // This matches legacy behavior and should improve tactical positions
            if (depth >= HISTORY_GATING_DEPTH && killers && history && counterMoves && counterMoveHistory) {
            float cmhWeight = limits ? limits->counterMoveHistoryWeight : 1.5f;
            mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                        *counterMoves, *counterMoveHistory,
                                        prevMove, ply, countermoveBonus, cmhWeight);
        } else if (killers && history && counterMoves) {
            mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                        *counterMoves, prevMove, ply, countermoveBonus);
        }
        
        // No shortlist when in check - we'll iterate evasions directly
        
#ifdef DEBUG
        // Phase 2a.5b: When in check, assert no shortlist
        assert(m_shortlistSize == 0 && "No shortlist when in check");
#endif
    }
    else {
        // Not in check: normal pseudo-legal generation and shortlist building
        MoveGenerator::generateMovesForSearch(board, m_moves, false);
        
#ifdef DEBUG
        m_generatedCount = m_moves.size();
#endif
        
        // Phase 2a.3d: Order moves using legacy ordering FIRST
        // This ensures perfect alignment with existing behavior
        static MvvLvaOrdering mvvLva;
        
        // Apply legacy ordering to all moves (with ttMove=NO_MOVE to avoid special handling)
        // This gives us the exact legacy order
        if (g_seeMoveOrdering.getMode() != SEEMode::OFF) {
            g_seeMoveOrdering.orderMoves(board, m_moves);
        } else {
            mvvLva.orderMoves(board, m_moves);
        }

        // Apply history heuristics for quiet moves if available
        if (depth >= HISTORY_GATING_DEPTH && killers && history && counterMoves && counterMoveHistory) {
            float cmhWeight = limits ? limits->counterMoveHistoryWeight : 1.5f;
            mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                        *counterMoves, *counterMoveHistory,
                                        prevMove, ply, countermoveBonus, cmhWeight);
        } else if (killers && history && counterMoves) {
            mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                        *counterMoves, prevMove, ply, countermoveBonus);
        }
        
        // Phase 2a.3d: Extract first K captures from legacy-ordered list as shortlist
        // Only build shortlist if K > 0 (depth >= 3)
        if (m_effectiveShortlistSize > 0) {
            // Walk the legacy-ordered list and take the first K captures
            for (size_t i = 0; i < m_moves.size(); ++i) {
            const Move& move = m_moves[i];
            
            // Skip TT move (will be yielded first)
            if (move == m_ttMove) {
                continue;
            }
            
            // Take captures and promotions for the shortlist (no quiets)
            // This ensures non-capture promotions aren't delayed
            if ((isCapture(move) || isEnPassant(move) || isPromotion(move)) && m_shortlistSize < m_effectiveShortlistSize) {
#ifdef DEBUG
                // Phase 2a.5b: Assert bounds before array write
                assert(m_shortlistSize >= 0 && m_shortlistSize < MAX_SHORTLIST_SIZE);
                assert(i < ::seajay::MAX_MOVES && "Move index out of bounds for shortlist map");
#endif
                
#ifdef SEARCH_STATS
                // Phase 2a.6b: Track captures observed (only when stats are requested)
                if (m_limits && m_limits->showMovePickerStats) {
                    if (m_searchData && (isCapture(move) || isEnPassant(move))) {
                        m_searchData->movePickerStats.capturesTotal++;
                    }
                }
#endif
                
                m_shortlist[m_shortlistSize] = move;
                m_shortlistScores[m_shortlistSize] = 0; // Not used, but initialize
                m_inShortlistMap[i] = true;  // Mark this index as in shortlist
                m_shortlistSize++;
            }
            
            // Stop once we have K captures/promotions (using depth-based K)
            if (m_shortlistSize >= m_effectiveShortlistSize) {
                break;
            }
            }
        }
    }

    if (dumpMoveOrderEnabled() && m_ply <= dumpMoveOrderConfig().plyLimit) {
        static int dumpCount = 0;
        static std::unordered_set<uint64_t> seen;
        uint64_t taggedKey = board.zobristKey() ^ (static_cast<uint64_t>(m_ply) << 48);
        if (dumpCount < dumpMoveOrderConfig().countLimit && seen.insert(taggedKey).second) {
            std::ostringstream oss;
            oss << "info string PickerOrder ply=" << m_ply
                << " hash=" << board.zobristKey()
                << " moves:";
            for (const Move& move : m_moves) {
                oss << ' ' << SafeMoveExecutor::moveToString(move);
            }
            std::cout << oss.str() << std::endl;
            ++dumpCount;
        }
    }
}

Move RankedMovePicker::next() {
    while (m_stage != MovePickerStage::End) {
        switch (m_stage) {
            case MovePickerStage::TT: {
                Move move = emitTTMove();
                if (move != NO_MOVE) {
                    return move;
                }
                m_stage = m_inCheck ? MovePickerStage::GenerateQuiets
                                     : MovePickerStage::GenerateGoodCaptures;
                break;
            }
            case MovePickerStage::GenerateGoodCaptures:
                m_stage = MovePickerStage::EmitGoodCaptures;
                break;
            case MovePickerStage::EmitGoodCaptures: {
                if (!m_inCheck) {
                    Move move = emitShortlistMove();
                    if (move != NO_MOVE) {
                        return move;
                    }
                }
                m_stage = MovePickerStage::GenerateKillers;
                break;
            }
            case MovePickerStage::GenerateKillers:
                m_stage = MovePickerStage::EmitKillers;
                break;
            case MovePickerStage::EmitKillers:
                // Legacy picker handles killer/counter moves within the remainder walk.
                m_stage = MovePickerStage::GenerateQuiets;
                break;
            case MovePickerStage::GenerateQuiets:
                m_stage = MovePickerStage::EmitQuiets;
                break;
            case MovePickerStage::EmitQuiets: {
                Move move = emitRemainderMove();
                if (move != NO_MOVE) {
                    return move;
                }
                m_stage = MovePickerStage::GenerateBadCaptures;
                break;
            }
            case MovePickerStage::GenerateBadCaptures:
                m_stage = MovePickerStage::EmitBadCaptures;
                break;
            case MovePickerStage::EmitBadCaptures:
            {
                Move move = emitBadCaptureMove();
                if (move != NO_MOVE) {
                    return move;
                }
                m_stage = MovePickerStage::End;
                break;
            }
            case MovePickerStage::End:
                break;
        }
    }

#ifdef DEBUG
    assert(m_yieldedCount == m_generatedCount && "Coverage mismatch: not all moves yielded");
#endif
    return NO_MOVE;
}

Move RankedMovePicker::emitTTMove() {
    if (m_ttMove == NO_MOVE || m_ttMoveYielded) {
        return NO_MOVE;
    }

    m_ttMoveYielded = true;

    const bool ttMoveInList = std::find(m_moves.begin(), m_moves.end(), m_ttMove) != m_moves.end();

#ifdef DEBUG
    if (m_inCheck && ttMoveInList) {
        assert(std::find(m_moves.begin(), m_moves.end(), m_ttMove) != m_moves.end()
               && "TT move must be in evasion list when in check");
    }
#endif

    if (!ttMoveInList) {
        return NO_MOVE;
    }

    m_yieldIndex++;

#ifdef SEARCH_STATS
    if (m_limits && m_limits->showMovePickerStats && m_searchData) {
        m_searchData->movePickerStats.ttFirstYield++;
    }
#endif

    recordLegacyYield(m_ttMove, classifyLegacyYield(m_ttMove, LegacyYieldStage::TT), LegacyYieldStage::TT);

#ifdef DEBUG
    m_yieldedCount++;
#endif
    return m_ttMove;
}

Move RankedMovePicker::emitShortlistMove() {
    if (m_inCheck) {
        return NO_MOVE;
    }

    while (m_shortlistIndex < m_shortlistSize) {
#ifdef DEBUG
        assert(m_shortlistIndex >= 0 && m_shortlistIndex < m_shortlistSize);
        assert(m_shortlistSize <= MAX_SHORTLIST_SIZE);
        assert(!m_inCheck && "Shortlist should not be used when in check");
#endif

        Move move = m_shortlist[m_shortlistIndex++];

        const bool isCaptureMove = isCapture(move) || isEnPassant(move);
        const bool isPromo = isPromotion(move);

        if (isCaptureMove && !isPromo) {
            int seeScore = ::seajay::seeSign(m_board, move);
            if (seeScore < 0) {
                if (m_badCaptureCount < static_cast<int>(::seajay::MAX_MOVES)) {
                    m_badCaptures[m_badCaptureCount++] = move;
                }
                continue;
            }
        }

        m_yieldIndex++;

        recordLegacyYield(move, classifyLegacyYield(move, LegacyYieldStage::Shortlist), LegacyYieldStage::Shortlist);

#ifdef DEBUG
        m_yieldedCount++;
#endif
        return move;
    }

    // After K shortlist entries, continue scanning legacy-ordered list for remaining
    // profitable captures/promotions so telemetry tags them as shortlist-stage yields.
    while (m_captureScanIndex < m_moves.size()) {
        const size_t index = m_captureScanIndex++;
        const Move move = m_moves[index];

        if (move == m_ttMove) {
            continue;
        }

        if (index < ::seajay::MAX_MOVES && m_inShortlistMap[index]) {
            continue;
        }

        const bool isPromo = isPromotion(move) && !isCapture(move) && !isEnPassant(move);
        const bool isCaptureMove = isCapture(move) || isEnPassant(move);

        if (!isPromo && !isCaptureMove) {
            continue;
        }

        if (index < ::seajay::MAX_MOVES) {
            m_inShortlistMap[index] = true;
        }

        bool yieldCapture = isPromo;
        if (isCaptureMove && !yieldCapture) {
            int seeScore = ::seajay::seeSign(m_board, move);
            yieldCapture = (seeScore >= 0);
        }

        if (!yieldCapture) {
            if (isCaptureMove && m_badCaptureCount < static_cast<int>(::seajay::MAX_MOVES)) {
                m_badCaptures[m_badCaptureCount++] = move;
            }
            continue;
        }

        m_yieldIndex++;

        recordLegacyYield(move, classifyLegacyYield(move, LegacyYieldStage::Shortlist), LegacyYieldStage::Shortlist);

#ifdef DEBUG
        m_yieldedCount++;
#endif
        return move;
    }

    return NO_MOVE;
}

Move RankedMovePicker::emitRemainderMove() {
    while (m_moveIndex < m_moves.size()) {
#ifdef DEBUG
        assert(m_moveIndex <= m_moves.size() && "Move index out of bounds");
#endif
        size_t currentIndex = m_moveIndex;
        Move move = m_moves[m_moveIndex++];

        if (move == m_ttMove) {
            continue;
        }

        if (!m_inCheck && currentIndex < ::seajay::MAX_MOVES && m_inShortlistMap[currentIndex]) {
            continue;
        }

        m_yieldIndex++;

#ifdef SEARCH_STATS
        if (m_limits && m_limits->showMovePickerStats && m_searchData) {
            m_searchData->movePickerStats.remainderYields++;
        }
#endif

        recordLegacyYield(move, classifyLegacyYield(move, LegacyYieldStage::Remainder), LegacyYieldStage::Remainder);

#ifdef DEBUG
        m_yieldedCount++;
#endif
        return move;
    }

    return NO_MOVE;
}

Move RankedMovePicker::emitBadCaptureMove() {
    if (m_badCaptureIndex >= m_badCaptureCount) {
        return NO_MOVE;
    }

    Move move = m_badCaptures[m_badCaptureIndex++];

    if (move == NO_MOVE) {
        return NO_MOVE;
    }

    m_yieldIndex++;

    recordLegacyYield(move, classifyLegacyYield(move, LegacyYieldStage::BadCapture), LegacyYieldStage::BadCapture);

#ifdef DEBUG
    m_yieldedCount++;
#endif
    return move;
}

MovePickerBucket RankedMovePicker::classifyLegacyYield(Move move, LegacyYieldStage stage) const {
    if (stage == LegacyYieldStage::TT) {
        return MovePickerBucket::TT;
    }

    if (stage == LegacyYieldStage::BadCapture) {
        return MovePickerBucket::BadCapture;
    }

    const bool isCaptureMove = isCapture(move) || isEnPassant(move);
    const bool isPromo = isPromotion(move) && !isCapture(move) && !isEnPassant(move);

    if (isPromo) {
        return MovePickerBucket::Promotion;
    }

    if (isCaptureMove) {
        if (stage == LegacyYieldStage::Shortlist) {
            return MovePickerBucket::GoodCapture;
        }
        auto seeScore = ::seajay::seeSign(m_board, move);
        if (seeScore >= 0) {
            return MovePickerBucket::GoodCapture;
        }
        return MovePickerBucket::BadCapture;
    }

    if (m_killers && m_killers->isKiller(m_ply, move)) {
        return MovePickerBucket::Killer;
    }

    if (m_counterMoves && m_prevMove != NO_MOVE &&
        m_counterMoves->getCounterMove(m_prevMove) == move) {
        return MovePickerBucket::CounterMove;
    }

    if (m_counterMoveHistory && m_prevMove != NO_MOVE) {
        int cmhScore = m_counterMoveHistory->getScore(m_prevMove, move);
        if (cmhScore > 0) {
            return MovePickerBucket::QuietCounterHistory;
        }
    }

    if (m_history) {
        int histScore = m_history->getScore(m_board.sideToMove(), moveFrom(move), moveTo(move));
        if (histScore > 0) {
            return MovePickerBucket::QuietHistory;
        }
    }

    if (!isCapture(move) && !isEnPassant(move)) {
        return MovePickerBucket::QuietFallback;
    }

    return MovePickerBucket::Other;
}

void RankedMovePicker::recordLegacyYield(Move move,
                                        MovePickerBucket bucket,
                                        LegacyYieldStage stage) const {
#ifdef SEARCH_STATS
    if (m_limits && m_limits->showMovePickerStats && m_searchData) {
        const auto index = static_cast<size_t>(bucket);
        if (index < m_searchData->movePickerStats.legacyYields.size()) {
            m_searchData->movePickerStats.legacyYields[index]++;
        }
    }
#endif

    if (!stageLogEnabled()) {
        return;
    }

    const StageLogConfig& cfg = stageLogConfig();
    if (m_ply > cfg.plyLimit) {
        return;
    }

    static int emitted = 0;
    if (cfg.countLimit > 0 && emitted >= cfg.countLimit) {
        return;
    }

    const char* stageLabel = "Unknown";
    switch (stage) {
        case LegacyYieldStage::TT:
            stageLabel = "TT";
            break;
        case LegacyYieldStage::Shortlist:
            stageLabel = "Shortlist";
            break;
        case LegacyYieldStage::Remainder:
            stageLabel = "Remainder";
            break;
        case LegacyYieldStage::BadCapture:
            stageLabel = "BadCaptures";
            break;
    }

    std::ostringstream oss;
    oss << "info string PickerStage ply=" << m_ply
        << " index=" << m_yieldIndex
        << " stage=" << stageLabel
        << " bucket=" << movePickerBucketName(bucket)
        << " move=" << SafeMoveExecutor::moveToString(move);
    std::cout << oss.str() << std::endl;
    ++emitted;
}

/**
 * Phase 2a.0: QS stub implementation
 * Not actually used in Phase 2a (QS uses legacy path)
 */
RankedMovePickerQS::RankedMovePickerQS(const Board& board, Move ttMove)
    : m_board(board)
    , m_ttMove(ttMove)
{
    // Phase 2a.7: Verify this is never constructed in Phase 2a
#ifdef DEBUG
    assert(false && "RankedMovePickerQS should not be constructed in Phase 2a (QS uses legacy path)");
#endif
    // Phase 2a.0: No initialization logic yet
}

Move RankedMovePickerQS::next() {
    // Phase 2a.7: This should never be called in Phase 2a
#ifdef DEBUG
    assert(false && "RankedMovePickerQS::next() should not be called in Phase 2a");
#endif
    // Phase 2a.0: Stub - always return NO_MOVE
    return NO_MOVE;
}

#ifdef SEARCH_STATS
/**
 * Check if a move was in the shortlist
 * Phase 2a.6: Telemetry support
 */
bool RankedMovePicker::wasInShortlist(Move m) const {
    // Check if move is in our shortlist
    for (int i = 0; i < m_shortlistSize; i++) {
        if (m_shortlist[i] == m) {
            return true;
        }
    }
    return false;
}
#endif

} // namespace search
} // namespace seajay
