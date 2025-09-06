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

// Forward declarations
class CounterMoveHistory;

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
    
    // External stop flag (for UCI stop command)
    // Pointer to atomic bool that can be set from UCI thread
    // This allows clean shutdown of search threads (LazySMP compatible)
    std::atomic<bool>* stopFlag = nullptr;
    
    // Stage 14, Deliverable 1.8: UCI option for quiescence search
    bool useQuiescence = true;  // Enable/disable quiescence search
    
    // Stage 13 Remediation: Aspiration window parameters (SPSA-tuned 2025-09-04)
    int aspirationWindow = 13;        // SPSA-tuned with 250k games (2025-09-04)
    int aspirationMaxAttempts = 5;    // SPSA-tuned with 250k games (2025-09-04)
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
    int maxCheckPly = 6;               // Maximum check extension depth in quiescence search
    
    // Phase 2.2: Root quiet re-ranking
    int rootKingPenalty = 0;          // Penalty for non-capturing, non-castling king moves at root (0 = no penalty)
    
    // Stage 18: Late Move Reductions (LMR) parameters
    bool lmrEnabled = true;           // Enable/disable LMR via UCI
    int lmrMinDepth = 2;              // Minimum depth to apply LMR (SPSA-tuned)
    int lmrMinMoveNumber = 2;         // Start reducing after this many moves (SPSA-tuned)
    int lmrBaseReduction = 1;         // Base reduction amount
    int lmrDepthFactor = 100;         // For formula: reduction = base + (depth-minDepth)/depthFactor
    int lmrHistoryThreshold = 50;     // History score threshold percentage (0-100)
    int lmrPvReduction = 1;           // Reduction adjustment for PV nodes
    int lmrNonImprovingBonus = 1;     // Extra reduction when not improving
    
    // Stage 21: Null Move Pruning parameters
    bool useNullMove = true;          // Enable/disable null move pruning (enabled for Phase A2)
    int nullMoveStaticMargin = 87;   // Margin for static null move pruning (SPSA-tuned)
    int nullMoveMinDepth = 2;         // Minimum depth for null move pruning (SPSA-tuned)
    int nullMoveReductionBase = 4;    // Base null move reduction (SPSA-tuned)
    int nullMoveReductionDepth6 = 4;  // Reduction at depth >= 6 (SPSA-tuned)
    int nullMoveReductionDepth12 = 5; // Reduction at depth >= 12 (SPSA-tuned)
    int nullMoveVerifyDepth = 10;     // Depth threshold for verification search
    int nullMoveEvalMargin = 198;     // Extra reduction when eval >> beta (SPSA-tuned)
    
    // Futility Pruning parameters
    bool useFutilityPruning = true;     // Enable/disable futility pruning
    int futilityMargin1 = 240;          // Futility margin for depth 1 (SPSA: FutilityBase)
    int futilityMargin2 = 313;          // Futility margin for depth 2 (SPSA: Base+Scale*1)
    int futilityMargin3 = 386;          // Futility margin for depth 3 (SPSA: Base+Scale*2)
    int futilityMargin4 = 459;          // Futility margin for depth 4 (SPSA: Base+Scale*3)
    
    // Stage 15: SEE pruning mode (read-only during search)
    std::string seePruningMode = "off";  // off, conservative, aggressive
    
    // Stage 22 Phase P3.5: PVS statistics output control
    bool showPVSStats = false;  // Show PVS statistics after each depth
    // B0: One-shot search summary at end of search
    bool showSearchStats = false;  // Print summary stats at end
    
    // Stage 23 CM3.3: Countermove heuristic bonus
    int countermoveBonus = 0;  // Bonus score for countermoves (0 = disabled)
    
    // Phase 4.3.a: Counter-move history weight
    float counterMoveHistoryWeight = 0.0f;  // Weight for CMH in move ordering (0 = disabled, see deferred_items_tracker.md)
    
    // Phase 3: Move Count Pruning parameters (conservative implementation)
    bool useMoveCountPruning = true;    // Enable/disable move count pruning
    int moveCountLimit3 = 7;            // Move limit for depth 3 (SPSA-tuned)
    int moveCountLimit4 = 15;           // Move limit for depth 4 (SPSA-tuned)
    int moveCountLimit5 = 20;           // Move limit for depth 5 (SPSA-tuned)
    int moveCountLimit6 = 25;           // Move limit for depth 6 (SPSA-tuned)
    int moveCountLimit7 = 36;           // Move limit for depth 7
    int moveCountLimit8 = 42;           // Move limit for depth 8
    int moveCountHistoryThreshold = 0;    // History score threshold (SPSA-tuned: disabled)
    int moveCountHistoryBonus = 6;      // Extra moves for good history
    int moveCountImprovingRatio = 75;   // Percentage of moves when not improving (75 = 3/4)
    
    // Phase R1: Razoring parameters (conservative implementation)
    bool useRazoring = false;            // Enable/disable razoring (default false for safety)
    int razorMargin1 = 274;              // Razoring margin for depth 1 (SPSA-tuned)
    int razorMargin2 = 468;              // Razoring margin for depth 2 (SPSA-tuned)
    
    // Node explosion diagnostics (temporary for debugging)
    bool nodeExplosionDiagnostics = false; // Enable detailed node explosion tracking
    
    // Default constructor
    SearchLimits() = default;
};

// Search statistics and state information
struct SearchData {
    // Virtual destructor to make class polymorphic for dynamic_cast
    virtual ~SearchData() = default;
    
    // Virtual method to check if this is an IterativeSearchData
    // This replaces expensive dynamic_cast in hot path
    virtual bool isIterativeSearch() const { return false; }
    
    // Node statistics
    uint64_t nodes = 0;           // Total nodes searched
    uint64_t betaCutoffs = 0;     // Total beta cutoffs
    uint64_t betaCutoffsFirst = 0; // Beta cutoffs on first move (move ordering efficiency)
    
    // Diagnostic: Cutoff position distribution  
    uint64_t cutoffsByPosition[5] = {0, 0, 0, 0, 0};  // [move1, move2, move3, moves4-10, moves11+]
    uint64_t totalMoves = 0;       // Total moves examined
    
    // Transposition table statistics
    uint64_t ttProbes = 0;         // Total TT probes
    uint64_t ttHits = 0;           // Total TT hits
    uint64_t ttCutoffs = 0;        // TT cutoffs (immediate returns)
    uint64_t ttMoveHits = 0;       // TT move found for ordering
    uint64_t ttStores = 0;         // Total TT stores
    uint64_t ttCollisions = 0;     // TT corrupted moves detected (Bug #013)
    
    // TT replacement tracking (for diagnostics)
    uint64_t ttReplaceEmpty = 0;      // Replaced empty entries
    uint64_t ttReplaceOldGen = 0;     // Replaced old generation entries
    uint64_t ttReplaceDepth = 0;      // Replaced shallower entries (same gen)
    uint64_t ttReplaceSkipped = 0;    // Skipped replacement (entry was deeper)
    
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
    
    // Phase 2: Currmove tracking for UCI info at root
    Move currentRootMove;          // Current move being searched at root
    int currentRootMoveNumber = 0; // Move number (1-based) at root
    
    // UCI Score Conversion FIX: Store root side-to-move for correct UCI output
    Color rootSideToMove = WHITE;  // Side to move at root (for UCI perspective conversion)
    
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
        int historyThreshold = 50;     // History score threshold percentage (0-100)
        int pvReduction = 1;           // Reduction adjustment for PV nodes
        int nonImprovingBonus = 1;     // Extra reduction when not improving
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
        
        // TT remediation Phase 1.2: Track missing TT stores
        uint64_t nullMoveNoStore = 0;     // Null-move cutoffs without TT store
        uint64_t staticNullNoStore = 0;   // Static null returns without TT store
        
        void reset() {
            attempts = 0;
            cutoffs = 0;
            zugzwangAvoids = 0;
            verificationFails = 0;
            staticCutoffs = 0;
            nullMoveNoStore = 0;
            staticNullNoStore = 0;
        }
        
        double cutoffRate() const {
            return attempts > 0 ? (100.0 * cutoffs / attempts) : 0.0;
        }
        
        double nullMoveNoStoreRate() const {
            return cutoffs > 0 ? (100.0 * nullMoveNoStore / cutoffs) : 0.0;
        }
        
        double staticNullNoStoreRate() const {
            return staticCutoffs > 0 ? (100.0 * staticNullNoStore / staticCutoffs) : 0.0;
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
        
        // MO2a: Killer validation statistics
        uint64_t killerValidationAttempts = 0;  // Times we checked killer legality
        uint64_t killerValidationFailures = 0;  // Times killer was illegal (pollution)
        
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
            killerValidationAttempts = killerValidationFailures = 0;  // MO2a
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

    // B0: Pruning breakdown by depth buckets (1-3, 4-6, 7-9, 10+)
    struct PruneBreakdown {
        uint64_t futility[4] = {0,0,0,0};      // Nominal-depth futility (legacy pre-legal block)
        uint64_t futilityEff[4] = {0,0,0,0};   // Effective-depth futility (post-legal LMR-aware)
        uint64_t moveCount[4] = {0,0,0,0};
        void reset() {
            for (int i = 0; i < 4; ++i) { futility[i] = 0; futilityEff[i] = 0; moveCount[i] = 0; }
        }
        static int bucketForDepth(int depth) {
            if (depth <= 3) return 0; if (depth <= 6) return 1; if (depth <= 9) return 2; return 3;
        }
    } pruneBreakdown;

    // B0: Aspiration window telemetry across iterations
    struct AspirationStats {
        uint64_t attempts = 0;  // Total re-search attempts (sum of window.attempts)
        uint64_t failLow = 0;   // Iterations that failed low at least once
        uint64_t failHigh = 0;  // Iterations that failed high at least once
        void reset() { attempts = failLow = failHigh = 0; }
    } aspiration;
    
    // Phase R1/R2/R2.1: Razoring statistics
    struct RazoringStats {
        uint64_t attempts = 0;           // Total razoring attempts
        uint64_t cutoffs = 0;            // Successful razoring cutoffs
        std::array<uint64_t, 2> depthBuckets = {0, 0};  // Cutoffs by depth [d1, d2]
        uint64_t tacticalSkips = 0;      // Phase R2: Times tactical guard triggered
        uint64_t ttContextSkips = 0;     // Phase R2.1: TT LOWER bound near alpha
        uint64_t endgameSkips = 0;       // Phase R2.1: Low material positions
        void reset() { 
            attempts = cutoffs = tacticalSkips = ttContextSkips = endgameSkips = 0; 
            depthBuckets = {0, 0}; 
        }
    } razoring;
    uint64_t razoringCutoffs = 0;       // Legacy counter (kept for compatibility)
    
    // Stage 19: Killer moves for move ordering
    // PERFORMANCE FIX: Changed from embedded to pointer to reduce SearchData size from 42KB to 1KB
    // This fixes cache thrashing that caused 30-40 ELO loss
    KillerMoves* killers = nullptr;
    
    // Stage 20: History heuristic for move ordering
    // PERFORMANCE FIX: Changed from embedded to pointer (was 32KB embedded)
    HistoryHeuristic* history = nullptr;
    
    // Stage 23: Countermove heuristic for move ordering
    // PERFORMANCE FIX: Changed from embedded to pointer (was 16KB embedded)
    CounterMoves* counterMoves = nullptr;
    int countermoveBonus = 0;  // CM3.3: Bonus value from UCI
    
    // Phase 4.3.a: Counter-move history heuristic
    // Tracks history scores for move pairs (previous move -> current move)
    // PERFORMANCE: Pointer to thread-local storage (32MB per thread)
    CounterMoveHistory* counterMoveHistory = nullptr;

    // B0: Legality telemetry (lazy legality path)
    uint64_t illegalPseudoBeforeFirst = 0;  // Pseudo-legal moves rejected before first legal
    uint64_t illegalPseudoTotal = 0;        // Total pseudo-legal moves rejected
    
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
        pruneBreakdown.reset();  // B0: Reset prune breakdown
        aspiration.reset();      // B0: Reset aspiration stats
        razoring.reset();        // Phase R1: Reset razoring stats
        razoringCutoffs = 0;     // Phase 4: Reset razoring counter (legacy)
        if (killers) killers->clear();  // Stage 19: Clear killer moves
        // Stage 20 Fix: DON'T clear history here - let it accumulate
        // history.clear();  // REMOVED to preserve history across iterations
        depth = 0;
        seldepth = 0;
        bestMove = Move();
        bestScore = eval::Score::zero();
        currentRootMove = Move();
        currentRootMoveNumber = 0;
        startTime = std::chrono::steady_clock::now();
        stopped = false;
        m_timeCheckCounter = 0;
        m_cachedElapsed = std::chrono::milliseconds(0);
    }
};

} // namespace seajay::search
