#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <chrono>
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
    
    // Stage 21: Null Move Pruning parameters  
    bool m_useNullMove = true;          // Enable/disable null move pruning (enabled for Phase A2)
    int m_nullMoveStaticMargin = 90;   // Margin for static null move pruning - reduced from 120
    
    // PST Phase Interpolation parameters
    bool m_usePSTInterpolation = true;  // Enable/disable PST phase interpolation
    
    // Stage 22 Phase P3.5: PVS statistics output control
    bool m_showPVSStats = false;        // Show PVS statistics after each depth
    
    // Stage 23 CM3.1: Countermove heuristic bonus (micro-phase testing)
    int m_countermoveBonus = 8000;      // Optimal bonus score for countermoves (CM3.5 testing)
    
    // Phase 3: Move Count Pruning parameters (conservative implementation)
    bool m_useMoveCountPruning = true;    // Enable/disable move count pruning
    int m_moveCountLimit3 = 12;           // Move limit for depth 3
    int m_moveCountLimit4 = 18;           // Move limit for depth 4
    int m_moveCountLimit5 = 24;           // Move limit for depth 5
    int m_moveCountLimit6 = 30;           // Move limit for depth 6
    int m_moveCountLimit7 = 36;           // Move limit for depth 7
    int m_moveCountLimit8 = 42;           // Move limit for depth 8
    int m_moveCountHistoryThreshold = 1500; // History score threshold for bonus moves
    int m_moveCountHistoryBonus = 6;      // Extra moves for good history
    int m_moveCountImprovingRatio = 75;   // Percentage of moves when not improving (75 = 3/4)
    
    // Multi-threading support (stub for OpenBench compatibility)
    int m_threads = 1;                  // Number of threads requested (currently always uses 1)
    
    // Stage 13 Remediation: Configurable aspiration window parameters
    int m_aspirationWindow = 16;        // Initial aspiration window size in centipawns
    int m_aspirationMaxAttempts = 5;    // Max re-search attempts before infinite window
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
    void handleEval();  // Phase 3: UCI eval command for position analysis
    
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