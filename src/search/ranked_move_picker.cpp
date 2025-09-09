#include "ranked_move_picker.h"
#include "../core/move_generation.h"
#include <cstring>

// Coverage verification - only enabled in debug builds
#ifdef DEBUG_RANKED_PICKER
#include <unordered_set>
#include <iostream>
static uint64_t g_coverageChecks = 0;
static uint64_t g_coverageMisses = 0;
static uint64_t g_coverageDuplicates = 0;
#endif

namespace seajay::search {

// MoveGenerator is already in seajay namespace

#ifdef SEARCH_STATS
RankedMovePickerStats g_rankedMovePickerStats;
#endif

// ========================================================================
// RankedMovePicker Implementation
// ========================================================================

RankedMovePicker::RankedMovePicker(const Board& board,
                                   Move ttMove,
                                   const KillerMoves* killers,
                                   const HistoryHeuristic* history,
                                   const CounterMoves* counterMoves,
                                   const CounterMoveHistory* counterMoveHistory,
                                   Move prevMove,
                                   int ply,
                                   int depth)
    : m_board(board)
    , m_ttMove(ttMove)
    , m_killers(killers)
    , m_history(history)
    , m_counterMoves(counterMoves)
    , m_counterMoveHistory(counterMoveHistory)
    , m_prevMove(prevMove)
    , m_ply(ply)
    , m_depth(depth)
    , m_phase(Phase::TT_MOVE)
    , m_seeCalculator()
{
    // Generate pseudo-legal moves (MUCH faster than legal moves)
    MoveGenerator::generatePseudoLegalMoves(board, m_allMoves);
    
    // Validate TT move is in the move list (pseudo-legal check)
    // We'll validate legality when we actually try it
    if (m_ttMove != NO_MOVE) {
        bool found = false;
        for (size_t i = 0; i < m_allMoves.size(); ++i) {
            if (m_allMoves[i] == m_ttMove) {
                found = true;
                break;
            }
        }
        if (!found) {
            #ifdef SEARCH_STATS
            g_rankedMovePickerStats.illegalTTMoves++;
            #endif
            m_ttMove = NO_MOVE;
        }
    }
    
    // Score all moves and build shortlist in single pass
    int16_t minShortlistScore = INT16_MIN;
    int minShortlistIdx = 0;  // Track the index of minimum score
    
    // PHASE 2A SIMPLIFIED: Only rank captures in shortlist for now
    // Quiets will use legacy ordering
    constexpr bool CAPTURES_ONLY_SHORTLIST = true;
    
    for (size_t i = 0; i < m_allMoves.size(); ++i) {
        Move move = m_allMoves[i];
        
        // Skip TT move (will be yielded first)
        if (move == m_ttMove) continue;
        
        // Score the move
        int16_t score;
        if (isCapture(move)) {
            score = scoreCapture(board, move);
            #ifdef SEARCH_STATS
            g_rankedMovePickerStats.capturesTotal++;
            #endif
        } else {
            if (CAPTURES_ONLY_SHORTLIST) {
                // Skip quiets for shortlist in Phase 2a
                continue;
            }
            score = scoreQuiet(move);
        }
        
        // Try to add to shortlist
        if (m_shortlistSize < RankedMovePickerConfig::SHORTLIST_SIZE) {
            // Shortlist not full, just add
            m_shortlist[m_shortlistSize] = {move, score};
            
            // Track minimum score position
            if (m_shortlistSize == 0 || score < minShortlistScore) {
                minShortlistScore = score;
                minShortlistIdx = m_shortlistSize;
            }
            
            m_shortlistSize++;
        } else if (score > minShortlistScore) {
            // Replace minimum scored move
            m_shortlist[minShortlistIdx] = {move, score};
            
            // Find new minimum in one pass
            minShortlistScore = m_shortlist[0].score;
            minShortlistIdx = 0;
            for (int j = 1; j < m_shortlistSize; ++j) {
                if (m_shortlist[j].score < minShortlistScore) {
                    minShortlistScore = m_shortlist[j].score;
                    minShortlistIdx = j;
                }
            }
        }
    }
    
    // Sort the shortlist
    std::sort(m_shortlist, m_shortlist + m_shortlistSize);
}

Move RankedMovePicker::next() {
#ifdef DEBUG_RANKED_PICKER
    Move result = nextInternal();
    
    // Track yielded moves for coverage verification
    if (result != NO_MOVE) {
        if (m_yieldedMoves.find(result) != m_yieldedMoves.end()) {
            g_coverageDuplicates++;
            std::cerr << "DUPLICATE MOVE YIELDED: " << std::hex << result 
                      << " at ply " << std::dec << m_ply << std::endl;
        }
        m_yieldedMoves.insert(result);
    } else {
        // Check coverage when done (1/1000 sampling)
        if (++g_coverageChecks % 1000 == 0) {
            verifyCoverage();
        }
    }
    
    return result;
#else
    return nextInternal();
#endif
}

Move RankedMovePicker::nextInternal() {
    switch (m_phase) {
        case Phase::TT_MOVE:
            m_phase = Phase::SHORTLIST;
            if (m_ttMove != NO_MOVE && !m_ttMoveUsed) {
                m_ttMoveUsed = true;
                #ifdef SEARCH_STATS
                g_rankedMovePickerStats.ttMoveUsed++;
                #endif
                return m_ttMove;
            }
            // Fall through to shortlist
            [[fallthrough]];
            
        case Phase::SHORTLIST:
            while (m_shortlistIndex < m_shortlistSize) {
                Move move = m_shortlist[m_shortlistIndex++].move;
                if (move != m_ttMove) {  // Skip if already used as TT move
                    #ifdef SEARCH_STATS
                    g_rankedMovePickerStats.shortlistHits++;
                    #endif
                    return move;
                }
            }
            m_phase = Phase::REMAINING_GOOD_CAPTURES;
            m_moveIndex = 0;
            [[fallthrough]];
            
        case Phase::REMAINING_GOOD_CAPTURES:
            // Phase 2a SIMPLIFIED: No SEE, just MVV-LVA ordering
            // All captures not in shortlist, ordered by MVV-LVA
            while (m_moveIndex < static_cast<int>(m_allMoves.size())) {
                Move move = m_allMoves[m_moveIndex++];
                if (move == m_ttMove || isInShortlist(move)) continue;
                
                if (isCapture(move)) {
                    return move;
                }
            }
            m_phase = Phase::REMAINING_QUIETS;
            m_moveIndex = 0;
            [[fallthrough]];
            
        case Phase::REMAINING_BAD_CAPTURES:
            // Skip this phase in 2a - no SEE filtering
            m_phase = Phase::REMAINING_QUIETS;
            [[fallthrough]];
            
        case Phase::REMAINING_QUIETS:
            while (m_moveIndex < static_cast<int>(m_allMoves.size())) {
                Move move = m_allMoves[m_moveIndex++];
                if (move == m_ttMove || isInShortlist(move)) continue;
                
                if (!isCapture(move)) {
                    return move;
                }
            }
            m_phase = Phase::DONE;
            [[fallthrough]];
            
        case Phase::DONE:
            return NO_MOVE;
    }
    
    return NO_MOVE;
}

#ifdef DEBUG_RANKED_PICKER
void RankedMovePicker::verifyCoverage() {
    // Compare yielded moves with generated moves
    std::unordered_set<Move> generatedSet;
    for (size_t i = 0; i < m_allMoves.size(); ++i) {
        generatedSet.insert(m_allMoves[i]);
    }
    
    // Check for missing moves
    for (Move m : generatedSet) {
        if (m_yieldedMoves.find(m) == m_yieldedMoves.end()) {
            g_coverageMisses++;
            std::cerr << "MISSING MOVE: " << std::hex << m 
                      << " at ply " << std::dec << m_ply 
                      << " (board hash: " << std::hex << m_board.zobristKey() << ")" << std::endl;
        }
    }
    
    // Report if counts don't match
    if (m_yieldedMoves.size() != generatedSet.size()) {
        std::cerr << "COVERAGE MISMATCH: generated=" << generatedSet.size() 
                  << " yielded=" << m_yieldedMoves.size() 
                  << " at ply " << m_ply << std::endl;
    }
}
#endif

int16_t RankedMovePicker::scoreCapture(const Board& board, Move move) {
    int16_t score = getMVVLVAScore(board, move);
    
    // Add promotion bonus
    if (isPromotion(move)) {
        score += getPromotionBonus(move);
    }
    
    // Small check bonus
    // Note: Checking if move gives check would require making the move
    // For now, we'll skip this optimization in Phase 2a
    
    return score;
}

int16_t RankedMovePicker::scoreQuiet(Move move) {
    int16_t score = 0;
    
    // Killer move bonus
    if (m_killers && m_killers->isKiller(m_ply, move)) {
        score += RankedMovePickerConfig::KILLER_BONUS;
    }
    
    // Countermove bonus
    if (m_counterMoves && m_prevMove != NO_MOVE && 
        m_counterMoves->getCounterMove(m_prevMove) == move) {
        score += RankedMovePickerConfig::COUNTERMOVE_BONUS;
    }
    
    // History heuristic score
    if (m_history) {
        Color side = m_board.sideToMove();
        score += m_history->getScore(side, moveFrom(move), moveTo(move)) * RankedMovePickerConfig::HISTORY_WEIGHT;
    }
    
    // Counter-move history score
    if (m_counterMoveHistory && m_prevMove != NO_MOVE) {
        score += m_counterMoveHistory->getScore(m_prevMove, move) * RankedMovePickerConfig::REFUTATION_BONUS / 100;
    }
    
    return score;
}

int16_t RankedMovePicker::getMVVLVAScore(const Board& board, Move move) {
    // Basic MVV-LVA: victim value - attacker value
    Square toSq = moveTo(move);
    Square fromSq = moveFrom(move);
    
    Piece victim = board.pieceAt(toSq);
    Piece attacker = board.pieceAt(fromSq);
    
    if (victim == NO_PIECE) {
        // En passant special case
        if (isEnPassant(move)) {
            return 100 - 1;  // Pawn takes pawn
        }
        return 0;
    }
    
    // Use the MVV-LVA tables from move_ordering.h
    static constexpr int VICTIM_VALUES[7] = {100, 325, 325, 500, 900, 10000, 0};
    static constexpr int ATTACKER_VALUES[7] = {1, 3, 3, 5, 9, 100, 0};
    
    PieceType victimType = typeOf(victim);
    PieceType attackerType = typeOf(attacker);
    
    return VICTIM_VALUES[victimType] - ATTACKER_VALUES[attackerType];
}

int16_t RankedMovePicker::getPromotionBonus(Move move) {
    if (!isPromotion(move)) return 0;
    
    PieceType promoType = promotionType(move);
    switch (promoType) {
        case QUEEN:  return RankedMovePickerConfig::PROMOTION_QUEEN_BONUS;
        case ROOK:   return RankedMovePickerConfig::PROMOTION_ROOK_BONUS;
        case BISHOP: return RankedMovePickerConfig::PROMOTION_BISHOP_BONUS;
        case KNIGHT: return RankedMovePickerConfig::PROMOTION_KNIGHT_BONUS;
        default:     return 0;
    }
}

bool RankedMovePicker::shouldComputeSEE(const Board& board, Move move, int16_t mvvlva) {
    // Phase 2a SIMPLIFIED: Never compute SEE
    // This function is kept for future phases but always returns false for now
    (void)board;
    (void)move;
    (void)mvvlva;
    return false;
}

bool RankedMovePicker::isInShortlist(Move move) const {
    for (int i = 0; i < m_shortlistSize; ++i) {
        if (m_shortlist[i].move == move) {
            return true;
        }
    }
    return false;
}

// ========================================================================
// RankedMovePickerQS Implementation
// ========================================================================

RankedMovePickerQS::RankedMovePickerQS(const Board& board, Move ttMove)
    : m_board(board)
    , m_ttMove(ttMove)
    , m_phase(PhaseQS::TT_MOVE)
    , m_seeCalculator()
{
    // Generate only captures (and promotions which are included in captures)
    // This is MUCH more efficient than generating all moves and filtering
    MoveGenerator::generateCaptures(board, m_captures);
    
    // Validate TT move (must be capture or promotion)
    if (m_ttMove != NO_MOVE) {
        if (!isCapture(m_ttMove) && !isPromotion(m_ttMove)) {
            m_ttMove = NO_MOVE;
        } else {
            bool isLegal = false;
            for (size_t i = 0; i < m_captures.size(); ++i) {
                if (m_captures[i] == m_ttMove) {
                    isLegal = true;
                    break;
                }
            }
            if (!isLegal) {
                m_ttMove = NO_MOVE;
            }
        }
    }
    
    // Score all captures
    m_scoredCapturesCount = 0;
    for (size_t i = 0; i < m_captures.size() && m_scoredCapturesCount < MAX_CAPTURES; ++i) {
        Move move = m_captures[i];
        if (move == m_ttMove) continue;  // Skip TT move
        
        int16_t score = scoreCaptureQS(board, move);
        m_scoredCaptures[m_scoredCapturesCount++] = {move, score};
    }
    
    // Sort by score
    std::sort(m_scoredCaptures, m_scoredCaptures + m_scoredCapturesCount);
}

Move RankedMovePickerQS::next() {
    switch (m_phase) {
        case PhaseQS::TT_MOVE:
            m_phase = PhaseQS::GOOD_CAPTURES;
            if (m_ttMove != NO_MOVE && !m_ttMoveUsed) {
                m_ttMoveUsed = true;
                return m_ttMove;
            }
            [[fallthrough]];
            
        case PhaseQS::GOOD_CAPTURES:
            while (m_captureIndex < static_cast<size_t>(m_scoredCapturesCount)) {
                const auto& sm = m_scoredCaptures[m_captureIndex++];
                if (sm.score >= 0) {  // Good capture (positive MVV-LVA or SEE)
                    return sm.move;
                } else {
                    // We've reached bad captures
                    m_phase = PhaseQS::PROMOTIONS;
                    m_captureIndex--;  // Back up one
                    break;
                }
            }
            if (m_captureIndex >= static_cast<size_t>(m_scoredCapturesCount)) {
                m_phase = PhaseQS::PROMOTIONS;
                m_captureIndex = 0;
            }
            [[fallthrough]];
            
        case PhaseQS::PROMOTIONS:
            // Check for non-capture promotions
            for (size_t i = 0; i < m_captures.size(); ++i) {
                Move move = m_captures[i];
                if (move != m_ttMove && isPromotion(move) && !isCapture(move)) {
                    // Note: This is simplified - in practice, promotions are usually sorted
                    m_phase = PhaseQS::BAD_CAPTURES;
                    return move;
                }
            }
            m_phase = PhaseQS::BAD_CAPTURES;
            [[fallthrough]];
            
        case PhaseQS::BAD_CAPTURES:
            while (m_captureIndex < static_cast<size_t>(m_scoredCapturesCount)) {
                const auto& sm = m_scoredCaptures[m_captureIndex++];
                if (sm.score < 0) {  // Bad capture
                    return sm.move;
                }
            }
            m_phase = PhaseQS::DONE;
            [[fallthrough]];
            
        case PhaseQS::DONE:
            return NO_MOVE;
    }
    
    return NO_MOVE;
}

int16_t RankedMovePickerQS::scoreCaptureQS(const Board& board, Move move) {
    // Simple MVV-LVA scoring for quiescence
    Square toSq = moveTo(move);
    Square fromSq = moveFrom(move);
    
    Piece victim = board.pieceAt(toSq);
    Piece attacker = board.pieceAt(fromSq);
    
    if (victim == NO_PIECE) {
        if (isEnPassant(move)) {
            return 100 - 1;  // Pawn takes pawn
        }
        if (isPromotion(move)) {
            // Non-capture promotion
            PieceType promoType = promotionType(move);
            switch (promoType) {
                case QUEEN:  return 2000;
                case ROOK:   return 750;
                case BISHOP: return 500;
                case KNIGHT: return 1000;
                default:     return 0;
            }
        }
        return 0;
    }
    
    static constexpr int VICTIM_VALUES[7] = {100, 325, 325, 500, 900, 10000, 0};
    static constexpr int ATTACKER_VALUES[7] = {1, 3, 3, 5, 9, 100, 0};
    
    PieceType victimType = typeOf(victim);
    PieceType attackerType = typeOf(attacker);
    
    int16_t score = VICTIM_VALUES[victimType] - ATTACKER_VALUES[attackerType];
    
    // Add promotion bonus for capture-promotions
    if (isPromotion(move)) {
        PieceType promoType = promotionType(move);
        switch (promoType) {
            case QUEEN:  score += 2000; break;
            case ROOK:   score += 750; break;
            case BISHOP: score += 500; break;
            case KNIGHT: score += 1000; break;
            default: break;
        }
    }
    
    return score;
}

} // namespace seajay::search