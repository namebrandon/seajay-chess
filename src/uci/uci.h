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

private:
    // Core engine state
    Board m_board;
    bool m_quit;
    TranspositionTable m_tt;  // Transposition table for search
    
    // UCI options (Stage 14, Deliverable 1.8)
    bool m_useQuiescence = true;  // Enable/disable quiescence search
    
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