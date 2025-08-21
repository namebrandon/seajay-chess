#pragma once

#include "../core/types.h"
#include "../core/board.h"
#include "../core/move_list.h"
#include "../core/see.h"  // Stage 15: SEE integration
#include "killer_moves.h"  // Stage 19: Killer moves integration
#include "history_heuristic.h"  // Stage 20: History heuristic integration
#include "countermoves.h"  // Stage 23: Countermove heuristic integration
#include <algorithm>
#include <cassert>
#include <array>
#include <iostream>
#include <atomic>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>

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
    
    // Order moves with killer move integration (Stage 19, Phase A2)
    void orderMovesWithKillers(const Board& board, MoveList& moves, 
                              const KillerMoves& killers, int ply) const;
    
    // Order moves with both killers and history (Stage 20, Phase B2)
    void orderMovesWithHistory(const Board& board, MoveList& moves,
                              const KillerMoves& killers, 
                              const HistoryHeuristic& history, int ply) const;
    
    // Order moves with killers, history, and countermoves (Stage 23, CM3.2)
    void orderMovesWithHistory(const Board& board, MoveList& moves,
                              const KillerMoves& killers, 
                              const HistoryHeuristic& history,
                              const CounterMoves& counterMoves,
                              Move prevMove, int ply, int countermoveBonus) const;
    
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

// ================================================================================
// Stage 15 Day 5: SEE Integration Infrastructure
// ================================================================================

// SEE integration modes
enum class SEEMode {
    OFF,         // SEE disabled, use MVV-LVA only
    TESTING,     // Use SEE for captures, log all values
    SHADOW,      // Calculate both SEE and MVV-LVA, use MVV-LVA, log differences
    PRODUCTION   // Use SEE for all captures in production
};

// Parallel scoring result for validation
struct ParallelScore {
    Move move;
    int mvvLvaScore;
    SEEValue seeValue;
    bool agree;  // Do they agree on move ordering?
};

// Statistics for SEE vs MVV-LVA comparison
struct SEEComparisonStats {
    std::atomic<uint64_t> totalComparisons{0};
    std::atomic<uint64_t> agreements{0};          // Same ordering decision
    std::atomic<uint64_t> seePreferred{0};        // SEE would order differently (better)
    std::atomic<uint64_t> mvvLvaPreferred{0};     // MVV-LVA was actually better
    std::atomic<uint64_t> equalScores{0};         // Both methods gave same score
    std::atomic<uint64_t> seeCalculations{0};     // Number of SEE calculations
    std::atomic<uint64_t> seeCacheHits{0};        // SEE cache hits
    std::atomic<uint64_t> capturesProcessed{0};   // Total captures evaluated
    std::atomic<uint64_t> promotionsProcessed{0}; // Total promotions evaluated
    
    double agreementRate() const {
        uint64_t total = totalComparisons.load();
        return total > 0 ? (100.0 * agreements.load() / total) : 0.0;
    }
    
    void reset() {
        totalComparisons = 0;
        agreements = 0;
        seePreferred = 0;
        mvvLvaPreferred = 0;
        equalScores = 0;
        seeCalculations = 0;
        seeCacheHits = 0;
        capturesProcessed = 0;
        promotionsProcessed = 0;
    }
    
    void print(std::ostream& out) const {
        out << "SEE vs MVV-LVA Comparison Statistics:\n";
        out << "  Total comparisons: " << totalComparisons << "\n";
        out << "  Agreements: " << agreements << " (" 
            << std::fixed << std::setprecision(1) << agreementRate() << "%)\n";
        out << "  SEE preferred: " << seePreferred << "\n";
        out << "  MVV-LVA preferred: " << mvvLvaPreferred << "\n";
        out << "  Equal scores: " << equalScores << "\n";
        out << "  SEE calculations: " << seeCalculations << "\n";
        out << "  SEE cache hits: " << seeCacheHits << "\n";
        out << "  Captures processed: " << capturesProcessed << "\n";
        out << "  Promotions processed: " << promotionsProcessed << "\n";
    }
};

// Enhanced move ordering with SEE integration
class SEEMoveOrdering final : public IMoveOrderingPolicy {
public:
    SEEMoveOrdering();
    
    // Order moves using current mode
    void orderMoves(const Board& board, MoveList& moves) const override;
    
    // Set the SEE mode
    void setMode(SEEMode mode) { m_mode = mode; }
    SEEMode getMode() const { return m_mode; }
    
    // Get comparison statistics
    static SEEComparisonStats& getStats() {
        static SEEComparisonStats stats;
        return stats;
    }
    
    // Parallel scoring for validation (Day 5.1)
    std::vector<ParallelScore> scoreMovesParallel(const Board& board, const MoveList& moves) const;
    
    // Score a move with SEE
    SEEValue scoreMoveWithSEE(const Board& board, Move move) const;
    
    // Compare SEE and MVV-LVA ordering for a pair of moves
    bool compareOrdering(const Board& board, Move a, Move b) const;
    
    // Log discrepancies to file
    void logDiscrepancy(const Board& board, Move move, int mvvScore, SEEValue seeValue) const;
    
private:
    mutable SEECalculator m_see;  // SEE calculator instance
    SEEMode m_mode = SEEMode::OFF;  // Current operating mode
    mutable std::ofstream m_logFile;  // Log file for discrepancies
    
    // Helper to determine if a move should use SEE
    bool shouldUseSEE(Move move) const;
    
    // Order moves using SEE in production mode
    void orderMovesWithSEE(const Board& board, MoveList& moves) const;
    
    // Order moves in testing mode (log everything)
    void orderMovesTestingMode(const Board& board, MoveList& moves) const;
    
    // Order moves in shadow mode (calculate both, use MVV-LVA)
    void orderMovesShadowMode(const Board& board, MoveList& moves) const;
};

// Global SEE move ordering instance
extern SEEMoveOrdering g_seeMoveOrdering;

// Helper function to get current SEE mode from string
SEEMode parseSEEMode(const std::string& mode);

// Helper function to convert SEE mode to string
std::string seeModeToString(SEEMode mode);

} // namespace seajay::search