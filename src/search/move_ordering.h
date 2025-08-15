#pragma once

#include "../core/types.h"
#include "../core/board.h"
#include "../core/move_list.h"
#include <algorithm>
#include <cassert>
#include <array>
#include <iostream>

namespace seajay::search {

// MVV-LVA is now always enabled via CMakeLists.txt (Phase 2.3, Stage 8)
// No longer using dangerous compile-time feature flags

// Type-safe MoveScore wrapper for sorting
struct MoveScore {
    Move move;
    int score;
    
    // Higher scores come first in sorting
    constexpr bool operator<(const MoveScore& other) const noexcept {
        return score > other.score;
    }
};

// MVV-LVA piece value tables
// Victim values: higher value pieces are worth more points when captured
// Indexed by PieceType: PAWN=0, KNIGHT=1, BISHOP=2, ROOK=3, QUEEN=4, KING=5, NO_PIECE_TYPE=6
constexpr int VICTIM_VALUES[7] = {
    100,    // PAWN (0)
    325,    // KNIGHT (1)
    325,    // BISHOP (2)
    500,    // ROOK (3)
    900,    // QUEEN (4)
    10000,  // KING (5) - should never happen in legal chess
    0       // NO_PIECE_TYPE (6)
};

// Attacker values: lower value attackers get bonus points (subtract from score)
constexpr int ATTACKER_VALUES[7] = {
    1,    // PAWN (0)
    3,    // KNIGHT (1)
    3,    // BISHOP (2)
    5,    // ROOK (3)
    9,    // QUEEN (4)
    100,  // KING (5) - should rarely attack in middlegame
    0     // NO_PIECE_TYPE (6)
};

// Compile-time validation of tables
static_assert(VICTIM_VALUES[PAWN] == 100, "Pawn victim value must be 100");
static_assert(VICTIM_VALUES[QUEEN] == 900, "Queen victim value must be 900");
static_assert(VICTIM_VALUES[KING] == 10000, "King victim value must be 10000");
static_assert(VICTIM_VALUES[NO_PIECE_TYPE] == 0, "No piece type victim value must be 0");
static_assert(ATTACKER_VALUES[PAWN] == 1, "Pawn attacker value must be 1");
static_assert(ATTACKER_VALUES[QUEEN] == 9, "Queen attacker value must be 9");
static_assert(ATTACKER_VALUES[NO_PIECE_TYPE] == 0, "No piece type attacker value must be 0");

// Promotion bonus values (added to base MVV-LVA score)
constexpr int PROMOTION_BASE_SCORE = 100000;
constexpr int PROMOTION_TYPE_BONUS[4] = {
    1000,   // KNIGHT promotion
    500,    // BISHOP promotion  
    750,    // ROOK promotion
    2000    // QUEEN promotion (highest)
};

// Debug infrastructure
#ifdef DEBUG_MOVE_ORDERING
    #define MVV_LVA_ASSERT(cond, msg) \
        do { if (!(cond)) { \
            std::cerr << "MVV-LVA Assert Failed: " << msg << "\n" \
                     << "  File: " << __FILE__ << "\n" \
                     << "  Line: " << __LINE__ << "\n"; \
            std::abort(); \
        }} while(0)
#else
    #define MVV_LVA_ASSERT(cond, msg) ((void)0)
#endif

// Forward declarations
class IMoveOrderingPolicy;
class MvvLvaOrdering;

// Interface for move ordering policies (future extensibility)
class IMoveOrderingPolicy {
public:
    virtual ~IMoveOrderingPolicy() = default;
    virtual void orderMoves(const Board& board, MoveList& moves) const = 0;
};

// MVV-LVA implementation
class MvvLvaOrdering final : public IMoveOrderingPolicy {
public:
    // Order moves using MVV-LVA scoring
    void orderMoves(const Board& board, MoveList& moves) const override;
    
    // Score a single move (exposed for testing)
    static int scoreMove(const Board& board, Move move) noexcept;
    
    // Basic MVV-LVA formula
    static constexpr int mvvLvaScore(PieceType victim, PieceType attacker) noexcept {
        MVV_LVA_ASSERT(victim < 7, "Invalid victim piece type");
        MVV_LVA_ASSERT(attacker < 7, "Invalid attacker piece type");
        return VICTIM_VALUES[victim] - ATTACKER_VALUES[attacker];
    }
    
    // Get statistics (for debugging/tuning)
    struct Statistics {
        uint64_t captures_scored = 0;
        uint64_t promotions_scored = 0;
        uint64_t en_passants_scored = 0;
        uint64_t quiet_moves = 0;
    };
    
    static Statistics& getStatistics() noexcept {
        static thread_local Statistics stats;
        return stats;
    }
    
    static void resetStatistics() noexcept {
        getStatistics() = Statistics{};
    }
    
    static void printStatistics() noexcept {
        const auto& stats = getStatistics();
        std::cout << "MVV-LVA Statistics:\n";
        std::cout << "  Captures scored: " << stats.captures_scored << "\n";
        std::cout << "  Promotions scored: " << stats.promotions_scored << "\n";
        std::cout << "  En passants scored: " << stats.en_passants_scored << "\n";
        std::cout << "  Quiet moves: " << stats.quiet_moves << "\n";
    }
};

// Helper function to order moves with MVV-LVA (integrates with existing code)
template<typename MoveContainer>
void orderMovesWithMvvLva(const Board& board, MoveContainer& moves) noexcept;

// MVV-LVA is always enabled - no conditional compilation
#define MVV_LVA_SCORE_MOVES(board, moves) \
    orderMovesWithMvvLva(board, moves)

} // namespace seajay::search