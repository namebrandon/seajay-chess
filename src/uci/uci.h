#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>
#include <memory>
#include <atomic>
#include <mutex>
#include "../core/board.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../core/types.h"  // For Hash type
#include "../core/transposition_table.h"  // For TT integration

namespace seajay {

/**
 * UCI (Universal Chess Interface) Protocol Handler for SeaJay Chess Engine
 * 
 * Implements the minimal UCI command set required for Stage 3:
 * - uci: Engine identification
 * - isready: Readiness confirmation
 * - position: Board setup (startpos, fen, moves)
 * - go: Search initiation with time controls
 * - quit/stop: Engine control
 */
class UCIEngine {
public:
    UCIEngine();
    ~UCIEngine();  // Destructor to clean up search thread
    
    /**
     * Main UCI loop - processes commands until quit
     */
    void run();
    
    /**
     * Run benchmark suite directly (for OpenBench compatibility)
     * @param depth Optional fixed depth (0 = use default depths)
     */
    void runBenchmark(int depth = 0);

private:
    // Core engine state
    Board m_board;
    bool m_quit;
    TranspositionTable m_tt;  // Transposition table for search
    
    // Thread management for search
    // Note: Designed for forward compatibility with LazySMP
    // Currently single search thread, but atomic stop flag will work with multiple threads
    std::unique_ptr<std::thread> m_searchThread;
    std::atomic<bool> m_searching{false};
    std::atomic<bool> m_stopRequested{false};  // Global stop flag for all search threads (LazySMP ready)
    std::mutex m_searchMutex;  // Protects search state changes
    
    // UCI options (Stage 14, Deliverable 1.8)
    bool m_useQuiescence = true;  // Enable/disable quiescence search
    bool m_useMagicBitboards = true;  // Stage 10: Enable/disable magic bitboards (79x speedup!)
    uint64_t m_qsearchNodeLimit = 0;  // Stage 14 Remediation: Runtime node limit (0 = unlimited)
    int m_maxCheckPly = 6;  // Maximum check extension depth in quiescence search
    int m_qsearchMaxCaptures = 32;    // Max captures per qsearch node (0 = unlimited)
    
    // Stage 15 Day 5: SEE integration mode
    std::string m_seeMode = "off";  // SEE mode: off, testing, shadow, production
    
    // Stage 15 Day 6: SEE-based pruning in quiescence
    std::string m_seePruning = "conservative";  // SEE pruning: off, conservative, aggressive
    // Quiescence-only SEE pruning mode (overrides SEEPruning in qsearch)
    std::string m_seePruningQ = "conservative";
    
    // Phase 2.2: Root quiet re-ranking
    int m_rootKingPenalty = 0;  // Penalty for non-capturing, non-castling king moves at root (0 = no penalty)
    
    // Stage 18: Late Move Reductions (LMR) parameters
    bool m_lmrEnabled = true;           // Enable/disable LMR via UCI (default on - +42 ELO)
    int m_lmrMinDepth = 2;              // Minimum depth to apply LMR (SPSA-tuned)
    int m_lmrMinMoveNumber = 2;         // Start reducing after this many moves (SPSA-tuned)  
    int m_lmrBaseReduction = 50;        // Base reduction in formula (50 = 0.5)
    int m_lmrDepthFactor = 225;         // Divisor in formula (225 = 2.25)
    int m_lmrHistoryThreshold = 50;     // History score threshold percentage (0-100)
    int m_lmrPvReduction = 1;           // Reduction adjustment for PV nodes
    int m_lmrNonImprovingBonus = 1;     // Extra reduction when not improving
    
    // Stage 21: Null Move Pruning parameters  
    bool m_useNullMove = true;          // Enable/disable null move pruning (enabled for Phase A2)
    int m_nullMoveStaticMargin = 87;   // Margin for static null move pruning (SPSA-tuned)
    int m_nullMoveMinDepth = 2;         // Minimum depth for null move pruning (SPSA-tuned)
    int m_nullMoveReductionBase = 4;    // Base null move reduction (SPSA-tuned)
    int m_nullMoveReductionDepth6 = 4;  // Reduction at depth >= 6 (SPSA-tuned)
    int m_nullMoveReductionDepth12 = 5; // Reduction at depth >= 12 (SPSA-tuned)
    int m_nullMoveVerifyDepth = 10;     // Depth threshold for verification search
    int m_nullMoveEvalMargin = 198;     // Extra reduction when eval >> beta (SPSA-tuned)
    bool m_useAggressiveNullMove = false; // Phase 4.2 toggle (default off)
    int m_aggressiveNullMinEval = 600;    // Default min eval for aggressive null
    int m_aggressiveNullMaxApplications = 64; // Default cap on applications
    bool m_aggressiveNullRequirePositiveBeta = true; // Require beta > 0 by default
    
    // PST Phase Interpolation parameters
    bool m_usePSTInterpolation = true;  // Enable/disable PST phase interpolation
    
    // Phase A2: Debug visibility for phase info in UCI eval
    bool m_showPhaseInfo = true;        // Print phase (0-256) and coarse GamePhase in eval
    // B0: One-shot search summary toggle
    bool m_showSearchStats = false;     // Print search summary after go
    // Node explosion diagnostics toggle
    bool m_nodeExplosionDiagnostics = false;  // Enable diagnostic statistics collection
    
    // Evaluation detail option
    bool m_evalExtended = false;        // Show detailed evaluation breakdown
    
    // Middlegame piece values (SPSA tuned 2025-01-04 with 150k games)
    int m_pawnValueMg = 71;             // Pawn middlegame value
    int m_knightValueMg = 325;          // Knight middlegame value  
    int m_bishopValueMg = 344;          // Bishop middlegame value
    int m_rookValueMg = 487;            // Rook middlegame value
    int m_queenValueMg = 895;           // Queen middlegame value
    
    // Endgame piece values (SPSA tuned 2025-01-04 with 150k games)
    int m_pawnValueEg = 92;             // Pawn endgame value
    int m_knightValueEg = 311;          // Knight endgame value
    int m_bishopValueEg = 327;          // Bishop endgame value
    int m_rookValueEg = 510;            // Rook endgame value
    int m_queenValueEg = 932;           // Queen endgame value
    
    // Phase R1: Razoring parameters
    bool m_useRazoring = true;          // Enable/disable razoring (default true - SPRT proven +5.89 ELO)
    int m_razorMargin1 = 274;           // Razoring margin for depth 1 (SPSA-tuned)
    int m_razorMargin2 = 468;           // Razoring margin for depth 2 (SPSA-tuned)
    
    // Stage 22 Phase P3.5: PVS statistics output control
    bool m_showPVSStats = false;        // Show PVS statistics after each depth
    
    // Stage 23 CM3.1: Countermove heuristic bonus (SPSA-tuned 2025-09-04)
    int m_countermoveBonus = 7960;      // SPSA-tuned with 250k games (2025-09-04)
    
    // Futility Pruning parameters
    bool m_useFutilityPruning = true;     // Enable/disable futility pruning
    int m_futilityMargin1 = 240;          // Futility margin for depth 1 (SPSA: FutilityBase)
    int m_futilityMargin2 = 313;          // Futility margin for depth 2 (SPSA: Base+Scale*1)
    int m_futilityMargin3 = 386;          // Futility margin for depth 3 (SPSA: Base+Scale*2)
    int m_futilityMargin4 = 459;          // Futility margin for depth 4 (SPSA: Base+Scale*3)
    
    // Phase 3: Move Count Pruning parameters (conservative implementation)
    bool m_useMoveCountPruning = true;    // Enable/disable move count pruning
    int m_moveCountLimit3 = 7;            // Move limit for depth 3 (SPSA-tuned)
    int m_moveCountLimit4 = 15;           // Move limit for depth 4 (SPSA-tuned)
    int m_moveCountLimit5 = 20;           // Move limit for depth 5 (SPSA-tuned)
    int m_moveCountLimit6 = 25;           // Move limit for depth 6 (SPSA-tuned)
    int m_moveCountLimit7 = 36;           // Move limit for depth 7
    int m_moveCountLimit8 = 42;           // Move limit for depth 8
    int m_moveCountMaxDepth = 8;          // Maximum depth to apply move-count pruning
    int m_moveCountHistoryThreshold = 0;    // History score threshold (SPSA-tuned: disabled)
    int m_moveCountHistoryBonus = 6;      // Extra moves for good history
    int m_moveCountImprovingRatio = 75;   // Percentage of moves when not improving (75 = 3/4)
    
    // Multi-threading support (stub for OpenBench compatibility)
    int m_threads = 1;                  // Number of threads requested (currently always uses 1)

    // Depth Parity scaffolds: UCI toggles (no behavior change yet)
    bool m_useClusteredTT = true;       // Default ON: clustered TT backend
    bool m_useStagedMovePicker = false; // Toggle for staged MovePicker (scaffold only)
    bool m_useRankedMovePicker = true;  // Default ON: ranked MovePicker (Phase 2a)
    bool m_showMovePickerStats = false; // Toggle for move picker statistics (Phase 2a.6)
    bool m_useInCheckClassOrdering = true; // Default ON: in-check class ordering (Phase 2a.8a)
    bool m_useRankAwareGates = true;    // Phase 2b: rank-aware pruning gates (default ON for integration)
    bool m_useSearchNodeAPIRefactor = true; // Phase 6: NodeContext plumbing (default ON after Stage 6g)
    bool m_enableExcludedMoveParam = false;   // Phase 6c: Excluded move parameter plumbing (default OFF)
    bool m_useSingularExtensions = false;     // Stage SE0.2a: Singular extension toggle (default OFF)
    bool m_allowStackedExtensions = false;    // Stage SE3.1b: Extension stacking toggle (default OFF)
    bool m_bypassSingularTTExact = false;     // SE1 investigation: bypass TT exact in verification (default OFF)
    std::vector<std::string> m_debugTrackedMoves; // UCI move strings to trace during search
    
    // Stage 13 Remediation: Aspiration window parameters (SPSA-tuned 2025-09-04)
    int m_aspirationWindow = 13;        // SPSA-tuned with 250k games (2025-09-04)
    int m_aspirationMaxAttempts = 5;    // SPSA-tuned with 250k games (2025-09-04)
    int m_stabilityThreshold = 6;       // Iterations needed for move stability
    bool m_useAspirationWindows = true; // Enable/disable aspiration windows
    
    // Stage 13 Remediation Phase 4: Advanced features
    std::string m_aspirationGrowth = "exponential";  // Window growth mode
    bool m_usePhaseStability = true;    // Use game phase-based stability
    int m_openingStability = 4;         // Stability threshold for opening
    int m_middlegameStability = 6;      // Stability threshold for middlegame
    int m_endgameStability = 8;         // Stability threshold for endgame
    
    // Helper methods for draw detection
    void updateGameHistory();
    void clearGameHistory();
    void reportDrawIfDetected();
    int countRepetitionsInGame(Hash key) const;
    
    // UCI command handlers
    void handleUCI();
    void handleIsReady();
    void handleUCINewGame();
    void handlePosition(const std::vector<std::string>& tokens);
    void handleGo(const std::vector<std::string>& tokens);
    void handleStop();
    void handleQuit();
    void handleBench(const std::vector<std::string>& tokens);
    void handleSetOption(const std::vector<std::string>& tokens);  // Stage 14, Deliverable 1.8
    void handleDumpPST();  // SPSA debug: dump current PST values
    void handleDebug(const std::vector<std::string>& tokens);  // Debug command handler
    
    // Position setup helpers
    bool setupPosition(const std::string& type, const std::vector<std::string>& tokens, size_t& index);
    bool applyMoves(const std::vector<std::string>& moveStrings);
    
    // Move format conversion
    Move parseUCIMove(const std::string& uciMove) const;
    std::string moveToUCI(Move move) const;
    
    // Time management
    struct SearchParams {
        int movetime = 0;        // Fixed time per move (ms)
        int wtime = 0;           // White time remaining (ms)
        int btime = 0;           // Black time remaining (ms)
        int winc = 0;            // White increment (ms)
        int binc = 0;            // Black increment (ms)
        int depth = 0;           // Fixed search depth
        bool infinite = false;   // Infinite search until stop
        
        int calculateSearchTime(Color sideToMove) const;
    };
    
    // Search and move selection
    void search(const SearchParams& params);
    void searchThreadFunc(const SearchParams& params);
    void stopSearch();
    Move selectRandomMove();
    
    SearchParams parseGoCommand(const std::vector<std::string>& tokens);
    
    // Utility functions
    std::vector<std::string> tokenize(const std::string& line);
    void sendInfo(const std::string& message);
    void sendBestMove(Move move);
    
    // Statistics for info output
    struct SearchInfo {
        int depth = 1;
        uint64_t nodes = 0;
        int64_t timeMs = 0;
        std::string pv;
    };
    
    void updateSearchInfo(SearchInfo& info, Move bestMove, int64_t searchTimeMs);
};

} // namespace seajay
