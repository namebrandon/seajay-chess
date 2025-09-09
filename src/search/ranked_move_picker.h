#pragma once

#include "../core/types.h"
#include "../core/board.h"
#include "../core/move_list.h"
#include "../core/see.h"
#include "killer_moves.h"
#include "history_heuristic.h"
#include "countermoves.h"
#include "countermove_history.h"
#include <cstdint>
#include <algorithm>

namespace seajay::search {

// Configuration for RankedMovePicker
struct RankedMovePickerConfig {
    static constexpr int SHORTLIST_SIZE = 10;
    static constexpr int16_t KILLER_BONUS = 1000;
    static constexpr int16_t COUNTERMOVE_BONUS = 500;
    static constexpr int16_t REFUTATION_BONUS = 300;
    static constexpr int16_t CHECK_BONUS = 50;
    static constexpr int16_t HISTORY_WEIGHT = 2;
    static constexpr int16_t PROMOTION_QUEEN_BONUS = 2000;
    static constexpr int16_t PROMOTION_ROOK_BONUS = 750;
    static constexpr int16_t PROMOTION_BISHOP_BONUS = 500;
    static constexpr int16_t PROMOTION_KNIGHT_BONUS = 1000;
};

// Telemetry for debugging and tuning
#ifdef SEARCH_STATS
struct RankedMovePickerStats {
    uint64_t bestMoveRank[4] = {0};  // [1], [2-5], [6-10], [11+]
    uint64_t shortlistHits = 0;
    uint64_t seeCallsLazy = 0;
    uint64_t capturesTotal = 0;
    uint64_t ttMoveUsed = 0;
    uint64_t illegalTTMoves = 0;
    
    void reset() {
        std::fill(bestMoveRank, bestMoveRank + 4, 0);
        shortlistHits = 0;
        seeCallsLazy = 0;
        capturesTotal = 0;
        ttMoveUsed = 0;
        illegalTTMoves = 0;
    }
};

// Global stats instance
extern RankedMovePickerStats g_rankedMovePickerStats;
#endif

// Main RankedMovePicker class for negamax
class RankedMovePicker {
public:
    // Constructor - pre-ranks all moves in single pass
    RankedMovePicker(const Board& board,
                     Move ttMove,
                     const KillerMoves* killers,
                     const HistoryHeuristic* history,
                     const CounterMoves* counterMoves,
                     const CounterMoveHistory* counterMoveHistory,
                     Move prevMove,
                     int ply,
                     int depth);
    
    // Iterator interface - returns next move or NO_MOVE when done
    Move next();
    
    // Check if picker has more moves
    bool hasNext() const { return m_phase != Phase::DONE; }
    
    // Get number of moves generated (for debugging)
    int moveCount() const { return m_allMoves.size(); }
    
private:
    // Phases for move yielding
    enum class Phase : uint8_t {
        TT_MOVE,
        SHORTLIST,
        REMAINING_GOOD_CAPTURES,
        REMAINING_BAD_CAPTURES,
        REMAINING_QUIETS,
        DONE
    };
    
    // Move with its score
    struct ScoredMove {
        Move move;
        int16_t score;
        
        bool operator<(const ScoredMove& other) const {
            // Primary: score
            if (score != other.score) return score > other.score;
            // Secondary: MVV-LVA approximation (to square)
            Square to1 = moveTo(move);
            Square to2 = moveTo(other.move);
            if (to1 != to2) return to1 < to2;
            // Tertiary: raw move encoding
            return move < other.move;
        }
    };
    
    // Scoring functions
    int16_t scoreCapture(const Board& board, Move move);
    int16_t scoreQuiet(Move move);
    int16_t getMVVLVAScore(const Board& board, Move move);
    int16_t getPromotionBonus(Move move);
    bool shouldComputeSEE(const Board& board, Move move, int16_t mvvlva);
    
    // Helper to check if move is in shortlist
    bool isInShortlist(Move move) const;
    
    // Member variables
    const Board& m_board;
    Move m_ttMove;
    const KillerMoves* m_killers;
    const HistoryHeuristic* m_history;
    const CounterMoves* m_counterMoves;
    const CounterMoveHistory* m_counterMoveHistory;
    Move m_prevMove;
    int m_ply;
    int m_depth;
    
    // Move storage
    MoveList m_allMoves;
    ScoredMove m_shortlist[RankedMovePickerConfig::SHORTLIST_SIZE];
    int m_shortlistSize = 0;
    
    // Iteration state
    Phase m_phase;
    int m_shortlistIndex = 0;
    int m_moveIndex = 0;
    bool m_ttMoveUsed = false;
    
    // SEE calculator (mutable for lazy evaluation)
    mutable SEECalculator m_seeCalculator;
};

// Simplified variant for quiescence search
class RankedMovePickerQS {
public:
    // Constructor - only considers captures and promotions
    RankedMovePickerQS(const Board& board, Move ttMove);
    
    // Iterator interface
    Move next();
    bool hasNext() const { return m_phase != PhaseQS::DONE; }
    
private:
    enum class PhaseQS : uint8_t {
        TT_MOVE,
        GOOD_CAPTURES,
        PROMOTIONS,
        BAD_CAPTURES,
        DONE
    };
    
    struct ScoredMoveQS {
        Move move;
        int16_t score;
        
        bool operator<(const ScoredMoveQS& other) const {
            return score > other.score;
        }
    };
    
    int16_t scoreCaptureQS(const Board& board, Move move);
    
    const Board& m_board;
    Move m_ttMove;
    MoveList m_captures;
    // Fixed-size array instead of vector to avoid allocation
    static constexpr int MAX_CAPTURES = 32;  // More than enough for captures
    ScoredMoveQS m_scoredCaptures[MAX_CAPTURES];
    int m_scoredCapturesCount = 0;
    
    PhaseQS m_phase;
    size_t m_captureIndex = 0;
    bool m_ttMoveUsed = false;
    
    mutable SEECalculator m_seeCalculator;
};

} // namespace seajay::search