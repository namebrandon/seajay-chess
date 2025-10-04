#pragma once

#include "../core/types.h"
#include "../core/transposition_table.h"
#include "../core/move_list.h"
#include "../evaluation/types.h"
#include "killer_moves.h"
#include "history_heuristic.h"
#include "countermoves.h"
#include "principal_variation.h"
#include "node_context.h"
#include <chrono>
#include <cstdint>
#include <array>
#include <algorithm>
#include <vector>
#include <atomic>
#include <string>

// Stage 13, Deliverable 5.2b: Performance optimizations
#ifdef NDEBUG
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
#else
    #define ALWAYS_INLINE inline
#endif
#include <cmath>

namespace seajay::search {

struct SingularDebugEvent {
    std::string fen;
    Move candidate = NO_MOVE;
    int depth = 0;
    int ply = 0;
    int ttDepth = -1;
    eval::Score ttScore = eval::Score::zero();
    Bound ttBound = Bound::NONE;
    eval::Score beta = eval::Score::zero();
    eval::Score margin = eval::Score::zero();
    eval::Score singularBeta = eval::Score::zero();
    eval::Score verificationScore = eval::Score::zero();
    int verificationReduction = 0;
    bool verificationRan = false;
    bool failLow = false;
    bool failHigh = false;
    bool extensionScheduled = false;
    int extensionAmount = 0;
    bool extensionApplied = false;
    bool stackedExtension = false;
    uint64_t nodesBefore = 0;
    uint64_t nodesAfter = 0;
};

// Forward declarations
class CounterMoveHistory;

// Stage 15: SEE pruning modes (moved from quiescence.h to avoid circular dependency)
enum class SEEPruningMode {
    OFF = 0,
    CONSERVATIVE = 1,
    MODERATE = 2,
    AGGRESSIVE = 3
};

// Scratch buffer helpers (defined in search_scratch.cpp)
MoveList& getMoveScratch(int ply);
TriangularPV* getPvScratch(int ply);
TriangularPV& getRootPvScratch();
void resetScratchBuffers();

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
    bool useAggressiveNullMove = false; // Phase 4.2: Extra null-move reduction toggle (default off)
    int aggressiveNullMinEval = 600;    // Minimum static eval (cp) required to consider aggressive null move
    int aggressiveNullMaxApplications = 64; // Global cap on aggressive applications per search (0 = unlimited)
    bool aggressiveNullRequirePositiveBeta = true; // Require beta > 0 before attempting aggressive null
    
    // Futility Pruning parameters
    bool useFutilityPruning = true;     // Enable/disable futility pruning
    int futilityMargin1 = 240;          // Futility margin for depth 1 (SPSA: FutilityBase)
    int futilityMargin2 = 313;          // Futility margin for depth 2 (SPSA: Base+Scale*1)
    int futilityMargin3 = 386;          // Futility margin for depth 3 (SPSA: Base+Scale*2)
    int futilityMargin4 = 459;          // Futility margin for depth 4 (SPSA: Base+Scale*3)
    
    // Stage 15: SEE pruning mode (read-only during search)
    std::string seePruningMode = "off";  // off, conservative, aggressive
    // Quiescence SEE pruning mode (separate control for qsearch only)
    std::string seePruningModeQ = "conservative";  // off, conservative, aggressive
    
    // Quiescence capture budget per node (0 = unlimited, default 32)
    int qsearchMaxCaptures = 32;
    
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
    int moveCountMaxDepth = 8;          // Maximum depth to apply move-count pruning (LMP)
    int moveCountHistoryThreshold = 0;    // History score threshold (SPSA-tuned: disabled)
    int moveCountHistoryBonus = 6;      // Extra moves for good history
    int moveCountImprovingRatio = 75;   // Percentage of moves when not improving (75 = 3/4)
    
    // Phase R1: Razoring parameters (conservative implementation)
    bool useRazoring = false;            // Enable/disable razoring (default false for safety)
    int razorMargin1 = 274;              // Razoring margin for depth 1 (SPSA-tuned)
    int razorMargin2 = 468;              // Razoring margin for depth 2 (SPSA-tuned)
    
    // Node explosion diagnostics (temporary for debugging)
    bool nodeExplosionDiagnostics = false; // Enable detailed node explosion tracking
    
    // Phase 2a: Ranked MovePicker
    bool useRankedMovePicker = false;    // Enable ranked move picker (Phase 2a)
    bool useUnorderedMovePicker = false; // Diagnostic: bypass all move ordering for baseline comparisons
    bool useTTPriorityRepair = false;    // Prototype: reinsert TT move when shortlist displaces it
    bool trackTTDiagnostics = false;     // Enable expensive TT legality diagnostics

    // Phase 2a.6: Telemetry for move picker analysis (UCI toggle)
    bool showMovePickerStats = false;     // Show move picker statistics at end of search

    // Phase 2a.8a: In-check class ordering
    bool useInCheckClassOrdering = false; // Enable class-based ordering for check evasions

    // Phase 2b: Rank-aware pruning gates
    bool useRankAwareGates = true;        // Enable rank-aware pruning/reduction gates (default ON for integration)

    // Phase 6: Search Node API refactor toggle (default OFF)
    bool useSearchNodeAPIRefactor = true;
    bool enableExcludedMoveParam = false; // Phase 6c: Thread excluded move via NodeContext (default OFF)

    // Stage SE0.2a: Singular extension master toggle (default OFF until feature ships)
    bool useSingularExtensions = false;

    // Stage SE3.1b: Optional stacking of multiple extension sources (default OFF)
    bool allowStackedExtensions = false;

    // Investigation: allow bypassing TT exact cutoffs during singular verification probes
    bool bypassSingularTTExact = false;

    // Stage SE3.1c: Optionally suppress check extensions during singular verification nodes
    bool disableCheckDuringSingular = false;

    // Stage SE4.1a: Tunable singular extension parameters (UCI exposed)
    int singularDepthMin = 8;
    int singularMarginBase = 64;
    int singularVerificationReduction = 3;
    // Stage SE3.2a: Singular extension depth increment (default 1 ply)
    int singularExtensionDepth = 1;

    // Debug instrumentation: track specific moves through the search pipeline
    std::vector<std::string> debugTrackedMoves;  // UCI move strings (e.g., h3h7) to trace during search
    bool logRootTTStores = false;                // Emit root TT probe/store diagnostics

    // Singular debug logging (off by default; heavy instrumentation)
    bool singularDebugLog = false;
    size_t singularDebugMaxEvents = 64;
    std::vector<SingularDebugEvent>* singularDebugSink = nullptr;

    // Benchmark/diagnostics
    bool suppressDebugOutput = false;     // Suppress debug stderr logging during bench

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
    
    static constexpr int SCRATCH_PLY = KillerMoves::MAX_PLY;

    ALWAYS_INLINE MoveList& acquireMoveList(int ply) {
        const int index = std::clamp(ply, 0, SCRATCH_PLY - 1);
        auto& list = getMoveScratch(index);
        list.clear();
        return list;
    }
    
    ALWAYS_INLINE TriangularPV* acquireChildPV(int ply) {
        const int index = std::clamp(ply + 1, 0, SCRATCH_PLY);
        TriangularPV* pv = getPvScratch(index);
        if (pv) {
            pv->clear();
        }
        return pv;
    }
    
    ALWAYS_INLINE TriangularPV& rootPV() {
        return getRootPvScratch();
    }
    
    ALWAYS_INLINE const TriangularPV& rootPV() const {
        return getRootPvScratch();
    }
    
    void clearScratch() {
        resetScratchBuffers();
    }
    
    // Time management
    std::chrono::steady_clock::time_point startTime;
    std::chrono::milliseconds timeLimit{0};
    bool stopped = false;          // Search has been stopped

    bool singularTelemetryEnabled = false; // Stage SE0 scaffolding: enable/disable singular telemetry aggregation

    // Stage 14, Deliverable 1.8: Runtime quiescence control
    bool useQuiescence = true;     // Enable/disable quiescence search
    
    // Stage 14 Remediation: Pre-parsed SEE mode to avoid string parsing in hot path
    SEEPruningMode seePruningModeEnum = SEEPruningMode::OFF;
    // Separate SEE pruning mode for quiescence
    SEEPruningMode seePruningModeEnumQ = SEEPruningMode::CONSERVATIVE;
    
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
    
    // Stage SE0.1a: Singular extension telemetry (thread-local)
    struct alignas(64) SingularStats {
        static constexpr int kSlackBucketWidth = 4;   // Bucket granularity in centipawns
        static constexpr int kSlackBucketCount = 64;  // Buckets cover 0..255cp with overflow bucket
        static constexpr int kSlackBucketCap = kSlackBucketWidth * kSlackBucketCount; // Saturation cap for telemetry sums
        uint64_t candidatesExamined = 0;        // Candidate moves evaluated for singularity
        uint64_t candidatesQualified = 0;       // Candidates that passed legality/quiet filters
        uint64_t candidatesRejectedIllegal = 0; // TT moves rejected for illegality
        uint64_t candidatesRejectedTactical = 0; // TT moves rejected for tactical properties
        uint64_t verificationsStarted = 0;      // Verification searches launched
        uint64_t verificationFailLow = 0;       // Verification result below singular beta
        uint64_t verificationFailHigh = 0;      // Verification result met/exceeded singular beta
        uint64_t verificationNodesEntered = 0;  // Verification nodes entered (context carries excluded move)
        uint64_t verificationNodesTTExact = 0;  // Verification nodes that returned via TT exact hit
        uint64_t verificationNodesExpanded = 0; // Verification nodes that reached move expansion
        uint64_t extensionsApplied = 0;         // Singular extensions successfully applied
        uint32_t maxExtensionDepth = 0;         // Maximum depth reached with stacked extensions
        uint32_t verificationCacheHits = 0;     // Cache hits in verification helpers
        int64_t verificationFailLowSlackSum = 0;  // Sum(beta - score) for fail-low cases
        int64_t verificationFailHighSlackSum = 0; // Sum(score - beta) for fail-high cases
        uint64_t stackingCandidates = 0;        // Times recapture stacking was considered
        uint64_t stackingApplied = 0;           // Times recapture stacking passed all guardrails
        uint64_t stackingRejectedDepth = 0;     // Rejections due to depth guard
        uint64_t stackingRejectedEval = 0;      // Rejections due to evaluation guard
        uint64_t stackingRejectedTT = 0;        // Rejections due to TT-depth guard
        uint64_t stackingBudgetClamped = 0;     // Times stacking failed due to budget clamp
        uint64_t stackingExtraDepth = 0;        // Additional depth contributed by stacking
        std::array<uint64_t, kSlackBucketCount> failLowSlackBuckets{};  // Histogram of fail-low slack values
        std::array<uint64_t, kSlackBucketCount> failHighSlackBuckets{}; // Histogram of fail-high slack values
        uint64_t checkExtensionsSuppressed = 0;  // Check extensions skipped on singular verification nodes
        uint64_t checkExtensionsApplied = 0;     // Check extensions applied on singular verification nodes

        void reset() noexcept {
            candidatesExamined = 0;
            candidatesQualified = 0;
            candidatesRejectedIllegal = 0;
            candidatesRejectedTactical = 0;
            verificationsStarted = 0;
            verificationFailLow = 0;
            verificationFailHigh = 0;
            verificationNodesEntered = 0;
            verificationNodesTTExact = 0;
            verificationNodesExpanded = 0;
            extensionsApplied = 0;
            maxExtensionDepth = 0;
            verificationCacheHits = 0;
            verificationFailLowSlackSum = 0;
            verificationFailHighSlackSum = 0;
            stackingCandidates = 0;
            stackingApplied = 0;
            stackingRejectedDepth = 0;
            stackingRejectedEval = 0;
            stackingRejectedTT = 0;
            stackingBudgetClamped = 0;
            stackingExtraDepth = 0;
            failLowSlackBuckets.fill(0);
            failHighSlackBuckets.fill(0);
            checkExtensionsSuppressed = 0;
            checkExtensionsApplied = 0;
        }

        bool empty() const noexcept {
            return candidatesExamined == 0 && candidatesQualified == 0 &&
                   candidatesRejectedIllegal == 0 && candidatesRejectedTactical == 0 &&
                   verificationsStarted == 0 && verificationFailLow == 0 &&
                   verificationFailHigh == 0 && verificationNodesEntered == 0 &&
                   verificationNodesTTExact == 0 && verificationNodesExpanded == 0 &&
                   extensionsApplied == 0 &&
                   maxExtensionDepth == 0 && verificationCacheHits == 0 &&
                   verificationFailLowSlackSum == 0 &&
                   verificationFailHighSlackSum == 0 &&
                   stackingCandidates == 0 && stackingApplied == 0 &&
                   stackingRejectedDepth == 0 && stackingRejectedEval == 0 &&
                   stackingRejectedTT == 0 && stackingBudgetClamped == 0 &&
                   stackingExtraDepth == 0 &&
                   std::all_of(failLowSlackBuckets.begin(), failLowSlackBuckets.end(), [](uint64_t v) { return v == 0; }) &&
                   std::all_of(failHighSlackBuckets.begin(), failHighSlackBuckets.end(), [](uint64_t v) { return v == 0; }) &&
                   checkExtensionsSuppressed == 0 && checkExtensionsApplied == 0;
        }
    };

    static_assert(alignof(SingularStats) == 64, "SingularStats must remain cache aligned");

    // Prefer [[no_unique_address]] when available to eliminate padding in builds where
    // telemetry is compiled out entirely.
#if defined(__has_cpp_attribute)
#  if __has_cpp_attribute(no_unique_address)
#    define CJ_NO_UNIQUE_ADDR [[no_unique_address]]
#  else
#    define CJ_NO_UNIQUE_ADDR
#  endif
#else
#  define CJ_NO_UNIQUE_ADDR
#endif

    CJ_NO_UNIQUE_ADDR SingularStats singularStats{}; // Thread-local stats (zero-overhead when unused)

#undef CJ_NO_UNIQUE_ADDR

    std::vector<SingularDebugEvent> singularDebugEvents;

    struct alignas(64) GlobalSingularStats {
        uint64_t totalExamined = 0;
        uint64_t totalQualified = 0;
        uint64_t totalIllegalRejects = 0;
        uint64_t totalTacticalRejects = 0;
        uint64_t totalVerified = 0;
        uint64_t totalFailLow = 0;
        uint64_t totalFailHigh = 0;
        uint64_t totalVerificationNodesEntered = 0;
        uint64_t totalVerificationNodesTTExact = 0;
        uint64_t totalVerificationNodesExpanded = 0;
        uint64_t totalExtended = 0;
        uint32_t maxExtensionDepth = 0;
        uint64_t totalCacheHits = 0;
        int64_t totalFailLowSlackSum = 0;
        int64_t totalFailHighSlackSum = 0;
        uint64_t totalStackingCandidates = 0;
        uint64_t totalStackingApplied = 0;
        uint64_t totalStackingRejectedDepth = 0;
        uint64_t totalStackingRejectedEval = 0;
        uint64_t totalStackingRejectedTT = 0;
        uint64_t totalStackingBudgetClamped = 0;
        uint64_t totalStackingExtraDepth = 0;
        std::array<uint64_t, SingularStats::kSlackBucketCount> totalFailLowSlackBuckets{};
        std::array<uint64_t, SingularStats::kSlackBucketCount> totalFailHighSlackBuckets{};
        uint64_t totalCheckExtensionsSuppressed = 0;
        uint64_t totalCheckExtensionsApplied = 0;

        void reset() noexcept {
            totalExamined = 0;
            totalQualified = 0;
            totalIllegalRejects = 0;
            totalTacticalRejects = 0;
            totalVerified = 0;
            totalFailLow = 0;
            totalFailHigh = 0;
            totalVerificationNodesEntered = 0;
            totalVerificationNodesTTExact = 0;
            totalVerificationNodesExpanded = 0;
            totalExtended = 0;
            maxExtensionDepth = 0;
            totalCacheHits = 0;
            totalFailLowSlackSum = 0;
            totalFailHighSlackSum = 0;
            totalStackingCandidates = 0;
            totalStackingApplied = 0;
            totalStackingRejectedDepth = 0;
            totalStackingRejectedEval = 0;
            totalStackingRejectedTT = 0;
            totalStackingBudgetClamped = 0;
            totalStackingExtraDepth = 0;
            totalFailLowSlackBuckets.fill(0);
            totalFailHighSlackBuckets.fill(0);
            totalCheckExtensionsSuppressed = 0;
            totalCheckExtensionsApplied = 0;
        }

        void aggregate(const SingularStats& local, bool threadSafe) noexcept {
            if (local.empty()) {
                return;
            }

            if (threadSafe) {
                std::atomic_ref<uint64_t> examined(totalExamined);
                std::atomic_ref<uint64_t> qualified(totalQualified);
                std::atomic_ref<uint64_t> illegal(totalIllegalRejects);
                std::atomic_ref<uint64_t> tactical(totalTacticalRejects);
                std::atomic_ref<uint64_t> verified(totalVerified);
                std::atomic_ref<uint64_t> failLow(totalFailLow);
                std::atomic_ref<uint64_t> failHigh(totalFailHigh);
                std::atomic_ref<uint64_t> nodesEntered(totalVerificationNodesEntered);
                std::atomic_ref<uint64_t> nodesTTExact(totalVerificationNodesTTExact);
                std::atomic_ref<uint64_t> nodesExpanded(totalVerificationNodesExpanded);
                std::atomic_ref<uint64_t> extended(totalExtended);
                std::atomic_ref<uint64_t> cacheHits(totalCacheHits);
                std::atomic_ref<int64_t> failLowSlack(totalFailLowSlackSum);
                std::atomic_ref<int64_t> failHighSlack(totalFailHighSlackSum);
                std::atomic_ref<uint64_t> stackCand(totalStackingCandidates);
                std::atomic_ref<uint64_t> stackApplied(totalStackingApplied);
                std::atomic_ref<uint64_t> stackRejectDepth(totalStackingRejectedDepth);
                std::atomic_ref<uint64_t> stackRejectEval(totalStackingRejectedEval);
                std::atomic_ref<uint64_t> stackRejectTT(totalStackingRejectedTT);
                std::atomic_ref<uint64_t> stackClamp(totalStackingBudgetClamped);
                std::atomic_ref<uint64_t> stackExtra(totalStackingExtraDepth);
                std::atomic_ref<uint64_t> checkSupp(totalCheckExtensionsSuppressed);
                std::atomic_ref<uint64_t> checkApplied(totalCheckExtensionsApplied);
                examined.fetch_add(local.candidatesExamined, std::memory_order_relaxed);
                qualified.fetch_add(local.candidatesQualified, std::memory_order_relaxed);
                illegal.fetch_add(local.candidatesRejectedIllegal, std::memory_order_relaxed);
                tactical.fetch_add(local.candidatesRejectedTactical, std::memory_order_relaxed);
                verified.fetch_add(local.verificationsStarted, std::memory_order_relaxed);
                failLow.fetch_add(local.verificationFailLow, std::memory_order_relaxed);
                failHigh.fetch_add(local.verificationFailHigh, std::memory_order_relaxed);
                nodesEntered.fetch_add(local.verificationNodesEntered, std::memory_order_relaxed);
                nodesTTExact.fetch_add(local.verificationNodesTTExact, std::memory_order_relaxed);
                nodesExpanded.fetch_add(local.verificationNodesExpanded, std::memory_order_relaxed);
                extended.fetch_add(local.extensionsApplied, std::memory_order_relaxed);
                cacheHits.fetch_add(local.verificationCacheHits, std::memory_order_relaxed);
                failLowSlack.fetch_add(local.verificationFailLowSlackSum, std::memory_order_relaxed);
                failHighSlack.fetch_add(local.verificationFailHighSlackSum, std::memory_order_relaxed);
                stackCand.fetch_add(local.stackingCandidates, std::memory_order_relaxed);
                stackApplied.fetch_add(local.stackingApplied, std::memory_order_relaxed);
                stackRejectDepth.fetch_add(local.stackingRejectedDepth, std::memory_order_relaxed);
                stackRejectEval.fetch_add(local.stackingRejectedEval, std::memory_order_relaxed);
                stackRejectTT.fetch_add(local.stackingRejectedTT, std::memory_order_relaxed);
                stackClamp.fetch_add(local.stackingBudgetClamped, std::memory_order_relaxed);
                stackExtra.fetch_add(local.stackingExtraDepth, std::memory_order_relaxed);
                checkSupp.fetch_add(local.checkExtensionsSuppressed, std::memory_order_relaxed);
                checkApplied.fetch_add(local.checkExtensionsApplied, std::memory_order_relaxed);
                for (std::size_t i = 0; i < local.failLowSlackBuckets.size(); ++i) {
                    const uint64_t lowCount = local.failLowSlackBuckets[i];
                    const uint64_t highCount = local.failHighSlackBuckets[i];
                    if (lowCount != 0) {
                        std::atomic_ref<uint64_t> bucket(totalFailLowSlackBuckets[i]);
                        bucket.fetch_add(lowCount, std::memory_order_relaxed);
                    }
                    if (highCount != 0) {
                        std::atomic_ref<uint64_t> bucket(totalFailHighSlackBuckets[i]);
                        bucket.fetch_add(highCount, std::memory_order_relaxed);
                    }
                }

                if (local.maxExtensionDepth > 0) {
                    std::atomic_ref<uint32_t> maxDepth(maxExtensionDepth);
                    uint32_t expected = maxDepth.load(std::memory_order_relaxed);
                    while (expected < local.maxExtensionDepth &&
                           !maxDepth.compare_exchange_weak(expected, local.maxExtensionDepth,
                                                           std::memory_order_relaxed,
                                                           std::memory_order_relaxed)) {
                    }
                }
                return;
            }

            totalExamined += local.candidatesExamined;
            totalQualified += local.candidatesQualified;
            totalIllegalRejects += local.candidatesRejectedIllegal;
            totalTacticalRejects += local.candidatesRejectedTactical;
            totalVerified += local.verificationsStarted;
            totalFailLow += local.verificationFailLow;
            totalFailHigh += local.verificationFailHigh;
            totalVerificationNodesEntered += local.verificationNodesEntered;
            totalVerificationNodesTTExact += local.verificationNodesTTExact;
            totalVerificationNodesExpanded += local.verificationNodesExpanded;
            totalExtended += local.extensionsApplied;
            totalCacheHits += local.verificationCacheHits;
            totalFailLowSlackSum += local.verificationFailLowSlackSum;
            totalFailHighSlackSum += local.verificationFailHighSlackSum;
            totalStackingCandidates += local.stackingCandidates;
            totalStackingApplied += local.stackingApplied;
            totalStackingRejectedDepth += local.stackingRejectedDepth;
            totalStackingRejectedEval += local.stackingRejectedEval;
            totalStackingRejectedTT += local.stackingRejectedTT;
            totalStackingBudgetClamped += local.stackingBudgetClamped;
            totalStackingExtraDepth += local.stackingExtraDepth;
            for (std::size_t i = 0; i < local.failLowSlackBuckets.size(); ++i) {
                totalFailLowSlackBuckets[i] += local.failLowSlackBuckets[i];
                totalFailHighSlackBuckets[i] += local.failHighSlackBuckets[i];
            }
            totalCheckExtensionsSuppressed += local.checkExtensionsSuppressed;
            totalCheckExtensionsApplied += local.checkExtensionsApplied;
            if (local.maxExtensionDepth > maxExtensionDepth) {
                maxExtensionDepth = local.maxExtensionDepth;
            }
        }

        SingularStats snapshot() const noexcept {
            SingularStats totals;
            totals.candidatesExamined = totalExamined;
            totals.candidatesQualified = totalQualified;
            totals.candidatesRejectedIllegal = totalIllegalRejects;
            totals.candidatesRejectedTactical = totalTacticalRejects;
            totals.verificationsStarted = totalVerified;
            totals.verificationFailLow = totalFailLow;
            totals.verificationFailHigh = totalFailHigh;
            totals.verificationNodesEntered = totalVerificationNodesEntered;
            totals.verificationNodesTTExact = totalVerificationNodesTTExact;
            totals.verificationNodesExpanded = totalVerificationNodesExpanded;
            totals.extensionsApplied = totalExtended;
            totals.maxExtensionDepth = maxExtensionDepth;
            totals.verificationCacheHits = totalCacheHits;
            totals.verificationFailLowSlackSum = totalFailLowSlackSum;
            totals.verificationFailHighSlackSum = totalFailHighSlackSum;
            totals.stackingCandidates = totalStackingCandidates;
            totals.stackingApplied = totalStackingApplied;
            totals.stackingRejectedDepth = totalStackingRejectedDepth;
            totals.stackingRejectedEval = totalStackingRejectedEval;
            totals.stackingRejectedTT = totalStackingRejectedTT;
            totals.stackingBudgetClamped = totalStackingBudgetClamped;
            totals.stackingExtraDepth = totalStackingExtraDepth;
            totals.failLowSlackBuckets = totalFailLowSlackBuckets;
            totals.failHighSlackBuckets = totalFailHighSlackBuckets;
            totals.checkExtensionsSuppressed = totalCheckExtensionsSuppressed;
            totals.checkExtensionsApplied = totalCheckExtensionsApplied;
            return totals;
        }
    };

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

        // Phase 4.2 instrumentation for aggressive reduction
        uint64_t aggressiveCandidates = 0;     // Positions eligible for aggressive reduction
        uint64_t aggressiveApplied = 0;        // Times we actually applied the extra reduction
        uint64_t aggressiveSuppressed = 0;     // Eligible but rejected (gate, cap, etc.)
        uint64_t aggressiveBlockedByTT = 0;    // Blocked because TT data suggested caution
        uint64_t aggressiveCutoffs = 0;        // Successful cutoffs following aggressive reduction
        uint64_t aggressiveVerifyPasses = 0;   // Verification searches that succeeded after aggressive reduction
        uint64_t aggressiveVerifyFails = 0;    // Verification searches that failed after aggressive reduction
        uint64_t aggressiveCapHits = 0;        // Rejections due to hitting the global application cap
        
        void reset() {
            attempts = 0;
            cutoffs = 0;
            zugzwangAvoids = 0;
            verificationFails = 0;
            staticCutoffs = 0;
            nullMoveNoStore = 0;
            staticNullNoStore = 0;
            aggressiveCandidates = 0;
            aggressiveApplied = 0;
            aggressiveSuppressed = 0;
            aggressiveBlockedByTT = 0;
            aggressiveCutoffs = 0;
            aggressiveVerifyPasses = 0;
            aggressiveVerifyFails = 0;
            aggressiveCapHits = 0;
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

    // Phase 4.1: Track when history / counter-move history ordering is applied
    struct HistoryGatingStats {
        uint64_t basicApplications = 0;          // Times basic history ordering applied
        uint64_t counterApplications = 0;        // Times counter-move history applied
        uint64_t basicFirstMoveHits = 0;         // First-legal hits when basic history active
        uint64_t counterFirstMoveHits = 0;       // First-legal hits when CMH active
        uint64_t basicCutoffs = 0;               // Quiet cutoffs credited to basic history
        uint64_t counterCutoffs = 0;             // Cutoffs credited to counter-move history
        uint64_t basicReSearches = 0;            // PVS re-searches while basic history active
        uint64_t counterReSearches = 0;          // PVS re-searches while CMH active

        void reset() {
            basicApplications = counterApplications = 0;
            basicFirstMoveHits = counterFirstMoveHits = 0;
            basicCutoffs = counterCutoffs = 0;
            basicReSearches = counterReSearches = 0;
        }

        uint64_t totalApplications() const {
            return basicApplications + counterApplications;
        }

        uint64_t totalReSearches() const {
            return basicReSearches + counterReSearches;
        }
    } historyStats;

    enum class HistoryContext : uint8_t {
        None = 0,
        Basic = 1,
        Counter = 2
    };

    std::array<uint8_t, KillerMoves::MAX_PLY> historyContext = {};

    ALWAYS_INLINE void clearHistoryContext(int ply) {
        if (ply >= 0 && ply < static_cast<int>(historyContext.size())) {
            historyContext[ply] = static_cast<uint8_t>(HistoryContext::None);
        }
    }

    ALWAYS_INLINE void registerHistoryApplication(int ply, HistoryContext ctx) {
        if (ply >= 0 && ply < static_cast<int>(historyContext.size())) {
            historyContext[ply] = static_cast<uint8_t>(ctx);
        }
        switch (ctx) {
            case HistoryContext::Basic:
                historyStats.basicApplications++;
                break;
            case HistoryContext::Counter:
                historyStats.counterApplications++;
                break;
            case HistoryContext::None:
            default:
                break;
        }
    }

    ALWAYS_INLINE HistoryContext historyContextAt(int ply) const {
        if (ply < 0 || ply >= static_cast<int>(historyContext.size())) {
            return HistoryContext::None;
        }
        return static_cast<HistoryContext>(historyContext[ply]);
    }

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
    
    // Phase 2a.6: Move picker telemetry (compiled out in Release)
    // This struct tracks effectiveness of the ranked move picker
    // All fields are thread-local, no atomics needed for single-thread OB
#ifdef SEARCH_STATS
enum class MovePickerBucket : uint8_t {
    TT = 0,
    GoodCapture,
    BadCapture,
    Promotion,
    Killer,
    CounterMove,
    QuietCounterHistory,
    QuietHistory,
    QuietFallback,
    Other,
    Count
};

ALWAYS_INLINE constexpr const char* movePickerBucketName(MovePickerBucket bucket) noexcept {
    switch (bucket) {
        case MovePickerBucket::TT: return "TT";
        case MovePickerBucket::GoodCapture: return "GoodCapture";
        case MovePickerBucket::BadCapture: return "BadCapture";
        case MovePickerBucket::Promotion: return "Promotion";
        case MovePickerBucket::Killer: return "Killer";
        case MovePickerBucket::CounterMove: return "Counter";
        case MovePickerBucket::QuietCounterHistory: return "QuietCMH";
        case MovePickerBucket::QuietHistory: return "QuietHist";
        case MovePickerBucket::QuietFallback: return "QuietOther";
        case MovePickerBucket::Other:
        default:
            return "Other";
    }
}

    struct MovePickerStats {
        // Best move rank distribution
        // [0]=rank 1, [1]=ranks 2-5, [2]=ranks 6-10, [3]=ranks 11+
        uint64_t bestMoveRank[4] = {0, 0, 0, 0};
        
        // Shortlist coverage
        uint64_t shortlistHits = 0;      // Cutoff moves that were in shortlist
        
        // SEE call tracking (expected ~0 in 2a since we don't use SEE yet)
        uint64_t seeCallsLazy = 0;       // SEE evaluations performed
        uint64_t capturesTotal = 0;      // Total captures observed
        
        // Optional: Additional tracking
        uint64_t ttFirstYield = 0;       // TT move yielded first
        uint64_t remainderYields = 0;    // Moves from remainder (not TT or shortlist)
        uint64_t ttFallbackRepairs = 0;  // Times TT move was restored via fallback repair
        uint64_t ttFirstYieldFallback = 0; // Times TT move first-yield used fallback path

        std::array<uint64_t, static_cast<size_t>(MovePickerBucket::Count)> firstCutoffBuckets{{0}};
        std::array<uint64_t, static_cast<size_t>(MovePickerBucket::Count)> cutoffBuckets{{0}};
        uint64_t firstCutoffTotal = 0;
        uint64_t cutoffTotal = 0;
        uint64_t firstCutoffTTAvailable = 0;
        uint64_t firstCutoffTTUsed = 0;

        void reset() {
            for (int i = 0; i < 4; i++) bestMoveRank[i] = 0;
            shortlistHits = 0;
            seeCallsLazy = 0;
            capturesTotal = 0;
            ttFirstYield = 0;
            remainderYields = 0;
            ttFallbackRepairs = 0;
            ttFirstYieldFallback = 0;
            firstCutoffBuckets.fill(0);
            cutoffBuckets.fill(0);
            firstCutoffTotal = 0;
            cutoffTotal = 0;
            firstCutoffTTAvailable = 0;
            firstCutoffTTUsed = 0;
        }
    } movePickerStats;
    
    // Phase 2b: Rank-aware gate statistics
    struct RankGateStats {
        uint64_t tried[4] = {0, 0, 0, 0};    // Moves tried per rank bucket
        uint64_t pruned[4] = {0, 0, 0, 0};   // Moves pruned per rank bucket
        uint64_t reduced[4] = {0, 0, 0, 0};  // Moves reduced per rank bucket
        
        void reset() {
            for (int i = 0; i < 4; i++) {
                tried[i] = pruned[i] = reduced[i] = 0;
            }
        }
        
        static ALWAYS_INLINE int bucketForRank(int r) {
            if (r <= 1) return 0;      // Rank 1
            if (r <= 5) return 1;      // Ranks 2-5
            if (r <= 10) return 2;     // Ranks 6-10
            return 3;                  // Ranks 11+
        }
    } rankGates;
#endif  // SEARCH_STATS
    
    // Phase 2b.7: PVS re-search smoothing statistics (thread-local, no locks needed)
    struct PVSReSearchSmoothing {
        static constexpr int DEPTH_BUCKET_COUNT = 3;
        static constexpr int RANK_BUCKET_COUNT = 4;  // Reuse existing buckets: [1], [2-5], [6-10], [11+]
        
        // Per-bucket counters: [depthBucket][rankBucket]
        uint32_t attempts[DEPTH_BUCKET_COUNT][RANK_BUCKET_COUNT] = {};     // Moves searched
        uint32_t reSearches[DEPTH_BUCKET_COUNT][RANK_BUCKET_COUNT] = {};   // Re-searches triggered
        
#ifdef SEARCH_STATS
        uint32_t smoothingApplied[DEPTH_BUCKET_COUNT][RANK_BUCKET_COUNT] = {};  // Times smoothing applied
#endif
        
        // Depth bucket mapping
        static ALWAYS_INLINE int depthBucket(int depth) {
            if (depth <= 6) return 0;   // D1: [4-6]
            if (depth <= 10) return 1;  // D2: [7-10]
            return 2;                    // D3: [11+]
        }
        
        // Rank bucket mapping (reuse RankGateStats buckets)
        static ALWAYS_INLINE int rankBucket(int rank) {
            if (rank <= 1) return 0;    // Rank 1
            if (rank <= 5) return 1;    // Ranks 2-5
            if (rank <= 10) return 2;   // Ranks 6-10
            return 3;                    // Ranks 11+
        }
        
        // Check if smoothing should be applied for this bucket
        ALWAYS_INLINE bool shouldApplySmoothing(int depthBucket, int rankBucket) const {
            uint32_t att = attempts[depthBucket][rankBucket];
            uint32_t res = reSearches[depthBucket][rankBucket];
            
            // Threshold: attempts >= 32 AND reSearches * 100 >= 20 * attempts (20% rate)
            return att >= 32 && res * 100 >= 20 * att;
        }
        
        // Update counters (called from negamax)
        ALWAYS_INLINE void recordMove(int depth, int rank, bool didReSearch) {
            int db = depthBucket(depth);
            int rb = rankBucket(rank);
            
            // Saturating increment to prevent overflow
            if (attempts[db][rb] < 65535) {
                attempts[db][rb]++;
                if (didReSearch && reSearches[db][rb] < 65535) {
                    reSearches[db][rb]++;
                }
            }
            
            // Decay when attempts exceeds 64 to keep data fresh
            if (attempts[db][rb] >= 64) {
                attempts[db][rb] >>= 1;
                reSearches[db][rb] >>= 1;
            }
        }
        
#ifdef SEARCH_STATS
        // Track when smoothing is applied (for telemetry)
        ALWAYS_INLINE void recordSmoothingApplied(int depth, int rank) {
            int db = depthBucket(depth);
            int rb = rankBucket(rank);
            if (smoothingApplied[db][rb] < 65535) {
                smoothingApplied[db][rb]++;
            }
        }
#endif
        
        void reset() {
            for (int d = 0; d < DEPTH_BUCKET_COUNT; d++) {
                for (int r = 0; r < RANK_BUCKET_COUNT; r++) {
                    attempts[d][r] = 0;
                    reSearches[d][r] = 0;
#ifdef SEARCH_STATS
                    smoothingApplied[d][r] = 0;
#endif
                }
            }
        }
    } pvsReSearchSmoothing;
    
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

    ALWAYS_INLINE bool isSingularTelemetryEnabled() const noexcept {
        return singularTelemetryEnabled;
    }

    ALWAYS_INLINE void setSingularTelemetryEnabled(bool enabled) noexcept {
        singularTelemetryEnabled = enabled;
        if (!enabled) {
            singularStats.reset();
        }
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
        clearScratch();
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
        singularStats.reset();
        singularDebugEvents.clear();
        seeStats.reset();
        lmrStats.reset();  // Stage 18: Reset LMR statistics
        nullMoveStats.reset();  // Stage 21: Reset null move statistics
        pvsStats.reset();  // Stage 22: Reset PVS statistics
        pvsReSearchSmoothing.reset();  // Phase 2b.7: Reset PVS re-search smoothing
        futilityPruned = 0;  // Phase 2.1: Reset futility pruning counter
        moveCountPruned = 0;  // Phase 3: Reset move count pruning counter
        pruneBreakdown.reset();  // B0: Reset prune breakdown
        aspiration.reset();      // B0: Reset aspiration stats
        razoring.reset();        // Phase R1: Reset razoring stats
        razoringCutoffs = 0;     // Phase 4: Reset razoring counter (legacy)
        historyStats.reset();    // Phase 4.1: Reset history gating telemetry
        historyContext.fill(static_cast<uint8_t>(HistoryContext::None));
#ifdef SEARCH_STATS
        movePickerStats.reset(); // Phase 2a.6: Reset move picker stats
        rankGates.reset();       // Phase 2b: Reset rank gate stats
#endif
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
