#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>
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
    std::thread m_searchThread;
    std::atomic<bool> m_searching{false};
    std::atomic<bool> m_stopRequested{false};  // Global stop flag for all search threads (LazySMP ready)
    std::mutex m_searchMutex;  // Protects search state changes
    
    // UCI options (Stage 14, Deliverable 1.8)
    bool m_useQuiescence = true;  // Enable/disable quiescence search
    bool m_useMagicBitboards = true;  // Stage 10: Enable/disable magic bitboards (79x speedup!)
    uint64_t m_qsearchNodeLimit = 0;  // Stage 14 Remediation: Runtime node limit (0 = unlimited)
    int m_maxCheckPly = 6;  // Maximum check extension depth in quiescence search
    
    // Stage 15 Day 5: SEE integration mode
    std::string m_seeMode = "off";  // SEE mode: off, testing, shadow, production
    
    // Stage 15 Day 6: SEE-based pruning in quiescence
    std::string m_seePruning = "conservative";  // SEE pruning: off, conservative, aggressive
    
    // Stage 18: Late Move Reductions (LMR) parameters
    bool m_lmrEnabled = true;           // Enable/disable LMR via UCI (default on - +42 ELO)
    int m_lmrMinDepth = 3;              // Minimum depth to apply LMR (0 to disable)
    int m_lmrMinMoveNumber = 6;         // Start reducing after this many moves (tuned value)  
    int m_lmrBaseReduction = 1;         // Base reduction amount
    int m_lmrDepthFactor = 3;           // For formula: reduction = base + (depth-minDepth)/depthFactor
    int m_lmrHistoryThreshold = 50;     // History score threshold percentage (0-100)
    int m_lmrPvReduction = 1;           // Reduction adjustment for PV nodes
    int m_lmrNonImprovingBonus = 1;     // Extra reduction when not improving
    
    // Stage 21: Null Move Pruning parameters  
    bool m_useNullMove = true;          // Enable/disable null move pruning (enabled for Phase A2)
    int m_nullMoveStaticMargin = 90;   // Margin for static null move pruning - reduced from 120
    int m_nullMoveMinDepth = 3;         // Minimum depth for null move pruning
    int m_nullMoveReductionBase = 2;    // Base null move reduction
    int m_nullMoveReductionDepth6 = 3;  // Reduction at depth >= 6
    int m_nullMoveReductionDepth12 = 4; // Reduction at depth >= 12
    int m_nullMoveVerifyDepth = 10;     // Depth threshold for verification search
    int m_nullMoveEvalMargin = 200;     // Extra reduction when eval >> beta
    
    // PST Phase Interpolation parameters
    bool m_usePSTInterpolation = true;  // Enable/disable PST phase interpolation
    
    // Phase A2: Debug visibility for phase info in UCI eval
    bool m_showPhaseInfo = true;        // Print phase (0-256) and coarse GamePhase in eval
    // B0: One-shot search summary toggle
    bool m_showSearchStats = false;     // Print search summary after go
    
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
    int m_razorMargin1 = 300;           // Razoring margin for depth 1 (in centipawns)
    int m_razorMargin2 = 500;           // Razoring margin for depth 2 (in centipawns)
    
    // Stage 22 Phase P3.5: PVS statistics output control
    bool m_showPVSStats = false;        // Show PVS statistics after each depth
    
    // Stage 23 CM3.1: Countermove heuristic bonus (SPSA-tuned 2025-09-04)
    int m_countermoveBonus = 7960;      // SPSA-tuned with 250k games (2025-09-04)
    
    // Futility Pruning parameters
    bool m_useFutilityPruning = true;     // Enable/disable futility pruning
    int m_futilityMargin1 = 100;          // Futility margin for depth 1 (in centipawns)
    int m_futilityMargin2 = 175;          // Futility margin for depth 2 (in centipawns)
    int m_futilityMargin3 = 250;          // Futility margin for depth 3 (in centipawns)
    int m_futilityMargin4 = 325;          // Futility margin for depth 4 (in centipawns)
    
    // Phase 3: Move Count Pruning parameters (conservative implementation)
    bool m_useMoveCountPruning = true;    // Enable/disable move count pruning
    int m_moveCountLimit3 = 12;           // Move limit for depth 3
    int m_moveCountLimit4 = 18;           // Move limit for depth 4
    int m_moveCountLimit5 = 24;           // Move limit for depth 5
    int m_moveCountLimit6 = 30;           // Move limit for depth 6
    int m_moveCountLimit7 = 36;           // Move limit for depth 7
    int m_moveCountLimit8 = 42;           // Move limit for depth 8
    int m_moveCountHistoryThreshold = -82; // History score threshold for bonus moves (SPSA-suggested)
    int m_moveCountHistoryBonus = 6;      // Extra moves for good history
    int m_moveCountImprovingRatio = 75;   // Percentage of moves when not improving (75 = 3/4)
    
    // Multi-threading support (stub for OpenBench compatibility)
    int m_threads = 1;                  // Number of threads requested (currently always uses 1)
    
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
