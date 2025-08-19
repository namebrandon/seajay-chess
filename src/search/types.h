#pragma once

#include "../core/types.h"
#include "../core/transposition_table.h"
#include "../evaluation/types.h"
#include <chrono>
#include <cstdint>

// Stage 13, Deliverable 5.2b: Performance optimizations
#ifdef NDEBUG
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
#else
    #define ALWAYS_INLINE inline
#endif
#include <cmath>

namespace seajay::search {

// Stage 15: SEE pruning modes (moved from quiescence.h to avoid circular dependency)
enum class SEEPruningMode {
    OFF = 0,
    CONSERVATIVE = 1,
    AGGRESSIVE = 2
};

// Search time limits and constraints
struct SearchLimits {
    // Time controls (in milliseconds)
    std::chrono::milliseconds time[NUM_COLORS] = {std::chrono::milliseconds(0), std::chrono::milliseconds(0)};
    std::chrono::milliseconds inc[NUM_COLORS] = {std::chrono::milliseconds(0), std::chrono::milliseconds(0)};
    std::chrono::milliseconds movetime{0};
    
    // Search constraints
    int maxDepth = 64;        // Maximum search depth
    int movestogo = 0;        // Moves until time control (0 = sudden death)
    uint64_t nodes = 0;       // Node limit (0 = no limit)
    
    // Search modes
    bool infinite = false;    // Infinite analysis mode
    bool ponder = false;      // Pondering mode (thinking on opponent's time)
    
    // Stage 14, Deliverable 1.8: UCI option for quiescence search
    bool useQuiescence = true;  // Enable/disable quiescence search
    
    // Stage 13 Remediation: Configurable aspiration window parameters
    int aspirationWindow = 16;        // Initial aspiration window size in centipawns
    int aspirationMaxAttempts = 5;    // Max re-search attempts
    int stabilityThreshold = 6;       // Iterations needed for move stability
    bool useAspirationWindows = true; // Enable/disable aspiration windows
    
    // Stage 13 Remediation Phase 4: Advanced features
    std::string aspirationGrowth = "exponential";  // Window growth mode
    bool usePhaseStability = true;    // Use game phase-based stability
    int openingStability = 4;         // Stability threshold for opening
    int middlegameStability = 6;      // Stability threshold for middlegame
    int endgameStability = 8;         // Stability threshold for endgame
    
    // Stage 14 Remediation: Runtime quiescence node limit
    uint64_t qsearchNodeLimit = 0;    // Per-position node limit (0 = unlimited)
    
    // Stage 18: Late Move Reductions (LMR) parameters
    bool lmrEnabled = true;           // Enable/disable LMR via UCI
    int lmrMinDepth = 3;              // Minimum depth to apply LMR (0 to disable)
    int lmrMinMoveNumber = 4;         // Start reducing after this many moves
    int lmrBaseReduction = 1;         // Base reduction amount
    int lmrDepthFactor = 100;         // For formula: reduction = base + (depth-minDepth)/depthFactor
    
    // Stage 15: SEE pruning mode (read-only during search)
    std::string seePruningMode = "off";  // off, conservative, aggressive
    
    // Default constructor
    SearchLimits() = default;
};

// Search statistics and state information
struct SearchData {
    // Node statistics
    uint64_t nodes = 0;           // Total nodes searched
    uint64_t betaCutoffs = 0;     // Total beta cutoffs
    uint64_t betaCutoffsFirst = 0; // Beta cutoffs on first move (move ordering efficiency)
    uint64_t totalMoves = 0;       // Total moves examined
    
    // Transposition table statistics
    uint64_t ttProbes = 0;         // Total TT probes
    uint64_t ttHits = 0;           // Total TT hits
    uint64_t ttCutoffs = 0;        // TT cutoffs (immediate returns)
    uint64_t ttMoveHits = 0;       // TT move found for ordering
    uint64_t ttStores = 0;         // Total TT stores
    uint64_t ttCollisions = 0;     // TT corrupted moves detected (Bug #013)
    
    // Quiescence search statistics
    uint64_t qsearchNodes = 0;     // Nodes searched in quiescence
    uint64_t qsearchCutoffs = 0;   // Beta cutoffs in quiescence
    uint64_t standPatCutoffs = 0;  // Stand-pat cutoffs
    uint64_t deltasPruned = 0;     // Positions pruned by delta pruning
    uint64_t qsearchNodesLimited = 0; // Times we hit per-position node limit
    uint64_t qsearchTTHits = 0;    // TT hits in quiescence search
    
    // Depth tracking
    int depth = 0;                 // Current iterative deepening depth
    int seldepth = 0;              // Maximum depth reached (selective depth)
    
    // Best move information
    Move bestMove;                 // Best move found so far
    eval::Score bestScore = eval::Score::zero(); // Score of best move
    
    // Time management
    std::chrono::steady_clock::time_point startTime;
    std::chrono::milliseconds timeLimit{0};
    bool stopped = false;          // Search has been stopped
    
    // Stage 14, Deliverable 1.8: Runtime quiescence control
    bool useQuiescence = true;     // Enable/disable quiescence search
    
    // Stage 14 Remediation: Pre-parsed SEE mode to avoid string parsing in hot path
    SEEPruningMode seePruningModeEnum = SEEPruningMode::OFF;
    
    // Stage 15: SEE pruning statistics (thread-local, no atomics needed)
    struct SEEStats {
        uint64_t totalCaptures = 0;       // Total captures considered
        uint64_t seePruned = 0;           // Captures pruned by SEE
        uint64_t seeEvaluations = 0;      // Number of SEE evaluations
        uint64_t conservativePrunes = 0;  // Prunes with threshold -100
        uint64_t aggressivePrunes = 0;    // Prunes with threshold -50 to -75
        uint64_t endgamePrunes = 0;       // Prunes in endgame positions
        uint64_t equalExchangePrunes = 0; // Prunes of equal exchanges (SEE=0)
        
        void reset() {
            totalCaptures = 0;
            seePruned = 0;
            seeEvaluations = 0;
            conservativePrunes = 0;
            aggressivePrunes = 0;
            endgamePrunes = 0;
            equalExchangePrunes = 0;
        }
        
        double pruneRate() const {
            return totalCaptures > 0 ? (100.0 * seePruned / totalCaptures) : 0.0;
        }
    } seeStats;
    
    // Stage 18: Late Move Reductions (LMR) parameters
    struct LMRParams {
        bool enabled = true;           // Enable/disable LMR via UCI
        int minDepth = 3;              // Minimum depth to apply LMR
        int minMoveNumber = 4;         // Start reducing after this many moves
        int baseReduction = 1;         // Base reduction amount
        int depthFactor = 100;         // For formula: reduction = base + (depth-minDepth)/depthFactor
    } lmrParams;
    
    // Stage 18: LMR statistics
    struct LMRStats {
        uint64_t totalReductions = 0;     // Total moves reduced
        uint64_t reSearches = 0;           // Re-searches after fail-high on reduced search
        uint64_t successfulReductions = 0; // Reductions that held (no re-search needed)
        
        void reset() {
            totalReductions = 0;
            reSearches = 0;
            successfulReductions = 0;
        }
        
        double reSearchRate() const {
            return totalReductions > 0 ? (100.0 * reSearches / totalReductions) : 0.0;
        }
        
        double successRate() const {
            return totalReductions > 0 ? (100.0 * successfulReductions / totalReductions) : 0.0;
        }
    } lmrStats;
    
    // Stage 13, Deliverable 5.2b: Cache for time checks
    mutable uint64_t m_timeCheckCounter = 0;
    mutable std::chrono::milliseconds m_cachedElapsed{0};
    // Time check interval: 2048 nodes
    // - At 1M NPS: ~500 checks/second (0.05% overhead)
    // - Balanced between overhead and time control precision
    // - Prevents time losses while maintaining performance
    static constexpr uint64_t TIME_CHECK_INTERVAL = 2048;  // Check every 2048 nodes
    
    // Constructor
    SearchData() : startTime(std::chrono::steady_clock::now()) {}
    
    // Calculate nodes per second (force accurate time for NPS)
    uint64_t nps() const {
        // Force accurate time update for NPS calculation
        auto now = std::chrono::steady_clock::now();
        auto actual_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        auto elapsed_ms = actual_elapsed.count();
        if (elapsed_ms <= 0) return 0;
        return (nodes * 1000ULL) / static_cast<uint64_t>(elapsed_ms);
    }
    
    // Get elapsed time since search started (optimized with caching)
    ALWAYS_INLINE std::chrono::milliseconds elapsed() const {
        // Stage 13, Deliverable 5.2b: Cache time to avoid frequent system calls
        if ((++m_timeCheckCounter & (TIME_CHECK_INTERVAL - 1)) == 0) {
            auto now = std::chrono::steady_clock::now();
            m_cachedElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        }
        return m_cachedElapsed;
    }
    
    // Check if time limit has been exceeded (optimized for hot path)
    ALWAYS_INLINE bool checkTime() {
        if (timeLimit == std::chrono::milliseconds::max()) {
            return false;  // Infinite search
        }
        return elapsed() >= timeLimit;
    }
    
    // Calculate effective branching factor
    double effectiveBranchingFactor() const {
        if (nodes <= 1 || depth <= 1) return 0.0;
        // Approximate EBF as the nth root of nodes, where n is depth
        // This is a simplified calculation - more accurate would use iterative method
        return std::pow(static_cast<double>(nodes), 1.0 / depth);
    }
    
    // Calculate move ordering efficiency (% of beta cutoffs on first move)
    double moveOrderingEfficiency() const {
        if (betaCutoffs == 0) return 0.0;
        return 100.0 * static_cast<double>(betaCutoffsFirst) / static_cast<double>(betaCutoffs);
    }
    
    // Reset for new search
    void reset() {
        nodes = 0;
        betaCutoffs = 0;
        betaCutoffsFirst = 0;
        totalMoves = 0;
        ttProbes = 0;
        ttHits = 0;
        ttCutoffs = 0;
        ttMoveHits = 0;
        ttStores = 0;
        ttCollisions = 0;
        qsearchNodes = 0;
        qsearchCutoffs = 0;
        standPatCutoffs = 0;
        deltasPruned = 0;
        qsearchNodesLimited = 0;
        qsearchTTHits = 0;
        seeStats.reset();
        depth = 0;
        seldepth = 0;
        bestMove = Move();
        bestScore = eval::Score::zero();
        startTime = std::chrono::steady_clock::now();
        stopped = false;
        m_timeCheckCounter = 0;
        m_cachedElapsed = std::chrono::milliseconds(0);
    }
};

} // namespace seajay::search