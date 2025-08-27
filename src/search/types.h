#pragma once

#include "../core/types.h"
#include "../core/transposition_table.h"
#include "../evaluation/types.h"
#include "killer_moves.h"
#include "history_heuristic.h"
#include "countermoves.h"
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
    
    // Stage 21: Null Move Pruning parameters
    bool useNullMove = true;          // Enable/disable null move pruning (enabled for Phase A2)
    int nullMoveStaticMargin = 90;   // Margin for static null move pruning - reduced from 120
    
    // Stage 15: SEE pruning mode (read-only during search)
    std::string seePruningMode = "off";  // off, conservative, aggressive
    
    // Stage 22 Phase P3.5: PVS statistics output control
    bool showPVSStats = false;  // Show PVS statistics after each depth
    
    // Stage 23 CM3.3: Countermove heuristic bonus
    int countermoveBonus = 0;  // Bonus score for countermoves (0 = disabled)
    
    // Phase 3: Move Count Pruning parameters (conservative implementation)
    bool useMoveCountPruning = true;    // Enable/disable move count pruning
    int moveCountLimit3 = 12;           // Move limit for depth 3
    int moveCountLimit4 = 18;           // Move limit for depth 4
    int moveCountLimit5 = 24;           // Move limit for depth 5
    int moveCountLimit6 = 30;           // Move limit for depth 6
    int moveCountLimit7 = 36;           // Move limit for depth 7
    int moveCountLimit8 = 42;           // Move limit for depth 8
    int moveCountHistoryThreshold = 1500; // History score threshold for bonus moves
    int moveCountHistoryBonus = 6;      // Extra moves for good history
    int moveCountImprovingRatio = 75;   // Percentage of moves when not improving (75 = 3/4)
    
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
        bool enabled = true;           // Enable/disable LMR via UCI (default on - +42 ELO tested)
        int minDepth = 3;              // Minimum depth to apply LMR
        int minMoveNumber = 6;         // Start reducing after this many moves (tuned for +42 ELO)
        int baseReduction = 1;         // Base reduction amount
        int depthFactor = 3;           // For formula: reduction = base + (depth-minDepth)/depthFactor
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
    
    // Stage 21: Null Move Pruning statistics
    struct NullMoveStats {
        uint64_t attempts = 0;            // Total null move attempts
        uint64_t cutoffs = 0;             // Successful null move cutoffs
        uint64_t zugzwangAvoids = 0;      // Times avoided due to zugzwang detection
        uint64_t verificationFails = 0;   // Verification search failures (for Phase A3)
        uint64_t staticCutoffs = 0;       // Static null move cutoffs (for Phase A4)
        
        void reset() {
            attempts = 0;
            cutoffs = 0;
            zugzwangAvoids = 0;
            verificationFails = 0;
            staticCutoffs = 0;
        }
        
        double cutoffRate() const {
            return attempts > 0 ? (100.0 * cutoffs / attempts) : 0.0;
        }
    } nullMoveStats;
    
    // Stage 22 Phase P3: Principal Variation Search (PVS) statistics
    struct PVSStats {
        uint64_t scoutSearches = 0;     // Total scout searches
        uint64_t reSearches = 0;         // Times we had to re-search
        uint64_t scoutCutoffs = 0;       // Scout searches that caused cutoff
        
        void reset() {
            scoutSearches = 0;
            reSearches = 0;
            scoutCutoffs = 0;
        }
        
        double reSearchRate() const {
            return scoutSearches > 0 ? (100.0 * reSearches / scoutSearches) : 0.0;
        }
    } pvsStats;
    
    // Singular extensions counter
    uint64_t singularExtensions = 0;    // Total singular extensions applied
    
    // Stage 23, Phase CM2: Countermove statistics (shadow mode)
    struct CounterMoveStats {
        uint64_t updates = 0;           // Total countermove updates
        uint64_t hits = 0;               // Times countermove was found (Phase CM3+)
        uint64_t cutoffs = 0;            // Times countermove caused cutoff (Phase CM3+)
        
        void reset() {
            updates = 0;
            hits = 0;
            cutoffs = 0;
        }
        
        double hitRate() const {
            return updates > 0 ? (100.0 * hits / updates) : 0.0;
        }
    } counterMoveStats;
    
    // Comprehensive move ordering statistics
    struct MoveOrderingStats {
        // Which move caused the cutoff
        uint64_t ttMoveCutoffs = 0;         // Cutoffs from TT move
        uint64_t firstCaptureCutoffs = 0;   // Cutoffs from first capture
        uint64_t killerCutoffs = 0;         // Cutoffs from killer moves
        uint64_t counterMoveCutoffs = 0;    // Cutoffs from countermove
        uint64_t quietCutoffs = 0;          // Cutoffs from quiet moves
        uint64_t badCaptureCutoffs = 0;     // Cutoffs from bad captures (QxP, RxP)
        
        // Move index statistics
        uint64_t cutoffsAtMove[10] = {0};   // Track cutoffs by move index (0-9)
        uint64_t cutoffsAfter10 = 0;        // Cutoffs after move 10
        
        // Special cases
        uint64_t qxpAttempts = 0;           // Times QxP was tried
        uint64_t rxpAttempts = 0;           // Times RxP was tried
        uint64_t qxpCutoffs = 0;            // Times QxP caused cutoff
        uint64_t rxpCutoffs = 0;            // Times RxP caused cutoff
        
        // Opening vs middlegame/endgame
        uint64_t openingNodes = 0;          // Nodes in opening (piece count > 28)
        uint64_t middlegameNodes = 0;       // Nodes in middlegame (16-28 pieces)
        uint64_t endgameNodes = 0;          // Nodes in endgame (< 16 pieces)
        
        void reset() {
            ttMoveCutoffs = 0;
            firstCaptureCutoffs = 0;
            killerCutoffs = 0;
            counterMoveCutoffs = 0;
            quietCutoffs = 0;
            badCaptureCutoffs = 0;
            for (int i = 0; i < 10; i++) cutoffsAtMove[i] = 0;
            cutoffsAfter10 = 0;
            qxpAttempts = rxpAttempts = 0;
            qxpCutoffs = rxpCutoffs = 0;
            openingNodes = middlegameNodes = endgameNodes = 0;
        }
        
        double cutoffDistribution(int moveIndex) const {
            uint64_t total = 0;
            for (int i = 0; i < 10; i++) total += cutoffsAtMove[i];
            total += cutoffsAfter10;
            if (total == 0) return 0.0;
            if (moveIndex < 10) {
                return (100.0 * cutoffsAtMove[moveIndex]) / total;
            }
            return (100.0 * cutoffsAfter10) / total;
        }
    } moveOrderingStats;
    
    // Phase 2.1: Futility pruning statistics
    uint64_t futilityPruned = 0;        // Positions pruned by futility pruning
    
    // Phase 3: Move count pruning statistics (conservative version)
    uint64_t moveCountPruned = 0;       // Moves pruned by move count pruning
    
    // Phase 4: Razoring statistics
    uint64_t razoringCutoffs = 0;       // Positions cut off by razoring
    
    // Stage 19: Killer moves for move ordering
    KillerMoves killers;
    
    // Stage 20: History heuristic for move ordering
    HistoryHeuristic history;
    
    // Stage 23: Countermove heuristic for move ordering
    CounterMoves counterMoves;
    int countermoveBonus = 0;  // CM3.3: Bonus value from UCI
    
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
        lmrStats.reset();  // Stage 18: Reset LMR statistics
        nullMoveStats.reset();  // Stage 21: Reset null move statistics
        futilityPruned = 0;  // Phase 2.1: Reset futility pruning counter
        moveCountPruned = 0;  // Phase 3: Reset move count pruning counter
        razoringCutoffs = 0;  // Phase 4: Reset razoring counter
        killers.clear();  // Stage 19: Clear killer moves
        // Stage 20 Fix: DON'T clear history here - let it accumulate
        // history.clear();  // REMOVED to preserve history across iterations
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