#include "uci.h"
#include "../benchmark/benchmark.h"
#include "../search/search.h"
#include "../search/negamax.h"
#include "../search/types.h"
#include "../search/move_ordering.h"  // Stage 15: For SEE integration
#include "../search/quiescence.h"      // Stage 15 Day 6: For SEE pruning mode
#include "../core/engine_config.h"    // Stage 10 Remediation: Runtime configuration
#include <iostream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <thread>

using namespace seajay;

UCIEngine::UCIEngine() : m_quit(false), m_tt() {
    // Initialize board to starting position
    m_board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    m_board.clearGameHistory();
    // No need to push - board tracks its own history
}

void UCIEngine::run() {
    std::string line;
    
    while (!m_quit && std::getline(std::cin, line)) {
        auto tokens = tokenize(line);
        if (tokens.empty()) continue;
        
        const std::string& command = tokens[0];
        
        if (command == "uci") {
            handleUCI();
        }
        else if (command == "isready") {
            handleIsReady();
        }
        else if (command == "ucinewgame") {
            handleUCINewGame();
        }
        else if (command == "position") {
            handlePosition(tokens);
        }
        else if (command == "go") {
            handleGo(tokens);
        }
        else if (command == "stop") {
            handleStop();
        }
        else if (command == "quit") {
            handleQuit();
        }
        else if (command == "bench") {
            handleBench(tokens);
        }
        else if (command == "setoption") {
            handleSetOption(tokens);  // Stage 14, Deliverable 1.8
        }
        // Ignore unknown commands (UCI protocol requirement)
    }
}

void UCIEngine::handleUCI() {
    // Build mode indicator for Stage 14 Quiescence Search
    std::string buildMode;
#ifdef QSEARCH_TESTING
    buildMode = " (Quiescence: TESTING MODE - 10K limit)";
#elif defined(QSEARCH_TUNING)
    buildMode = " (Quiescence: TUNING MODE - 100K limit)";
#else
    buildMode = " (Quiescence: PRODUCTION MODE)";
#endif
    
    std::cout << "id name SeaJay Stage12-TT-Improved" << buildMode << std::endl;
    std::cout << "id author Brandon Harris" << std::endl;
    // Stage 15: Static Exchange Evaluation (SEE) - Day 3 X-Ray Support
    
    // Stage 10 Remediation: UCI option for magic bitboards (79x speedup!)
    std::cout << "option name UseMagicBitboards type check default true" << std::endl;
    
    // Stage 14, Deliverable 1.8: UCI option for quiescence search
    std::cout << "option name UseQuiescence type check default true" << std::endl;
    
    // Stage 15 Day 5: SEE integration mode option
    std::cout << "option name SEEMode type combo default off var off var testing var shadow var production" << std::endl;
    
    // Stage 15 Day 6: SEE-based pruning in quiescence
    std::cout << "option name SEEPruning type combo default off var off var conservative var aggressive" << std::endl;
    
    // Stage 12: Transposition Table options
    std::cout << "option name Hash type spin default 16 min 1 max 16384" << std::endl;  // TT size in MB
    std::cout << "option name UseTranspositionTable type check default true" << std::endl;  // Enable/disable TT
    
    // Stage 13 Remediation: Aspiration window and time management options
    std::cout << "option name AspirationWindow type spin default 16 min 5 max 50" << std::endl;
    std::cout << "option name AspirationMaxAttempts type spin default 5 min 3 max 10" << std::endl;
    std::cout << "option name StabilityThreshold type spin default 6 min 3 max 12" << std::endl;
    std::cout << "option name UseAspirationWindows type check default true" << std::endl;
    
    std::cout << "uciok" << std::endl;
}

void UCIEngine::handleIsReady() {
    std::cout << "readyok" << std::endl;
}

void UCIEngine::handlePosition(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return;
    
    size_t index = 1;
    const std::string& type = tokens[index];
    
    // Setup initial position
    if (!setupPosition(type, tokens, index)) {
        // Invalid position, keep current position
        return;
    }
    
    // Apply moves if present
    if (index < tokens.size() && tokens[index] == "moves") {
        index++; // Skip "moves" keyword
        std::vector<std::string> moveStrings(tokens.begin() + index, tokens.end());
        applyMoves(moveStrings);
    }
}

bool UCIEngine::setupPosition(const std::string& type, const std::vector<std::string>& tokens, size_t& index) {
    if (type == "startpos") {
        index++; // Move past "startpos"
        bool result = m_board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        if (result) {
            m_board.clearGameHistory();  // New game, clear history
            // No need to push - board tracks its own history
        }
        return result;
    }
    else if (type == "fen") {
        index++; // Move past "fen"
        
        // Collect FEN string (may have spaces)
        std::string fen;
        int fenParts = 0;
        while (index < tokens.size() && fenParts < 6) {
            if (!fen.empty()) fen += " ";
            fen += tokens[index];
            index++;
            fenParts++;
            
            // Stop if we hit "moves" keyword
            if (index < tokens.size() && tokens[index] == "moves") {
                break;
            }
        }
        
        bool result = m_board.fromFEN(fen);
        if (result) {
            m_board.clearGameHistory();  // New position, clear history
            // No need to push - board tracks its own history
        }
        return result;
    }
    
    return false; // Unknown position type
}

bool UCIEngine::applyMoves(const std::vector<std::string>& moveStrings) {
    for (const auto& moveStr : moveStrings) {
        Move move = parseUCIMove(moveStr);
        if (move == Move()) {
            // Invalid move, stop processing
            return false;
        }
        
        // Verify move is legal
        MoveList legalMoves;
        MoveGenerator::generateLegalMoves(m_board, legalMoves);
        
        bool isLegal = false;
        for (size_t i = 0; i < legalMoves.size(); ++i) {
            if (legalMoves[i] == move) {
                isLegal = true;
                break;
            }
        }
        
        if (!isLegal) {
            // Illegal move, stop processing
            return false;
        }
        
        // Apply the move
        Board::UndoInfo undo;
        m_board.makeMove(move, undo);
        // makeMove automatically pushes to game history
    }
    
    return true;
}

Move UCIEngine::parseUCIMove(const std::string& uciMove) const {
    if (uciMove.length() < 4 || uciMove.length() > 5) {
        return Move(); // Invalid move format
    }
    
    // Parse from and to squares
    std::string fromStr = uciMove.substr(0, 2);
    std::string toStr = uciMove.substr(2, 2);
    
    Square from = stringToSquare(fromStr);
    Square to = stringToSquare(toStr);
    
    if (from == NO_SQUARE || to == NO_SQUARE) {
        return Move(); // Invalid squares
    }
    
    // Check for promotion
    PieceType promotion = PAWN;
    if (uciMove.length() == 5) {
        char promoChar = uciMove[4];
        switch (promoChar) {
            case 'q': promotion = QUEEN; break;
            case 'r': promotion = ROOK; break;
            case 'b': promotion = BISHOP; break;
            case 'n': promotion = KNIGHT; break;
            default: return Move(); // Invalid promotion
        }
    }
    
    // Generate all legal moves and find matching move
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(m_board, legalMoves);
    
    for (size_t i = 0; i < legalMoves.size(); ++i) {
        Move move = legalMoves[i];
        if (moveFrom(move) == from && moveTo(move) == to) {
            // Check promotion type if applicable
            if (uciMove.length() == 5) {
                if (isPromotion(move) && promotionType(move) == promotion) {
                    return move;
                }
            } else {
                // Normal move or non-promotion
                return move;
            }
        }
    }
    
    return Move(); // Move not found in legal moves
}

std::string UCIEngine::moveToUCI(Move move) const {
    if (move == Move()) {
        return "0000"; // Null move
    }
    
    std::string result = squareToString(moveFrom(move)) + squareToString(moveTo(move));
    
    // Add promotion piece if applicable
    if (isPromotion(move)) {
        PieceType promo = promotionType(move);
        switch (promo) {
            case QUEEN: result += "q"; break;
            case ROOK: result += "r"; break;
            case BISHOP: result += "b"; break;
            case KNIGHT: result += "n"; break;
            default: break; // Shouldn't happen
        }
    }
    
    return result;
}

void UCIEngine::handleGo(const std::vector<std::string>& tokens) {
    SearchParams params = parseGoCommand(tokens);
    search(params);
}

UCIEngine::SearchParams UCIEngine::parseGoCommand(const std::vector<std::string>& tokens) {
    SearchParams params;
    
    for (size_t i = 1; i < tokens.size(); i++) {
        const std::string& param = tokens[i];
        
        if (param == "infinite") {
            params.infinite = true;
        }
        else if (i + 1 < tokens.size()) {
            // Parameters that need a value
            int value = std::stoi(tokens[i + 1]);
            
            if (param == "movetime") {
                params.movetime = value;
                i++; // Skip the value
            }
            else if (param == "wtime") {
                params.wtime = value;
                i++;
            }
            else if (param == "btime") {
                params.btime = value;
                i++;
            }
            else if (param == "winc") {
                params.winc = value;
                i++;
            }
            else if (param == "binc") {
                params.binc = value;
                i++;
            }
            else if (param == "depth") {
                params.depth = value;
                i++;
            }
        }
    }
    
    return params;
}

int UCIEngine::SearchParams::calculateSearchTime(Color sideToMove) const {
    // Fixed movetime takes precedence
    if (movetime > 0) {
        return movetime;
    }
    
    // Calculate from time controls
    int timeRemaining = (sideToMove == WHITE) ? wtime : btime;
    int increment = (sideToMove == WHITE) ? winc : binc;
    
    if (timeRemaining > 0) {
        // Use 1/30th of remaining time plus increment
        // Minimum 100ms, maximum 10 seconds for Stage 3
        int calculatedTime = timeRemaining / 30 + increment;
        return std::max(100, std::min(calculatedTime, 10000));
    }
    
    // Default for Stage 3: very quick move selection
    return 100;
}

void UCIEngine::search(const SearchParams& params) {
    // Stage 9b: Check for immediate draw before searching
    if (m_board.isDraw()) {
        reportDrawIfDetected();
        
        // Get any legal move to return
        MoveList legalMoves;
        MoveGenerator::generateLegalMoves(m_board, legalMoves);
        if (legalMoves.size() > 0) {
            Move anyMove = legalMoves[0];
            
            // Report draw score and return a move
            std::cout << "info depth 1 score cp 0 nodes 1 pv " 
                     << moveToUCI(anyMove) << std::endl;
            std::cout << "bestmove " << moveToUCI(anyMove) << std::endl;
            return;
        } else {
            // No legal moves (checkmate or stalemate)
            std::cout << "info depth 1 score mate 0 nodes 1" << std::endl;
            std::cout << "bestmove 0000" << std::endl;
            return;
        }
    }
    
    // Convert UCI parameters to search limits
    search::SearchLimits limits;
    
    // Set depth limit
    if (params.depth > 0) {
        limits.maxDepth = params.depth;
    } else {
        limits.maxDepth = 64;  // Default max depth
    }
    
    // Set time controls
    if (params.movetime > 0) {
        limits.movetime = std::chrono::milliseconds(params.movetime);
    } else if (params.wtime > 0 || params.btime > 0) {
        limits.time[WHITE] = std::chrono::milliseconds(params.wtime);
        limits.time[BLACK] = std::chrono::milliseconds(params.btime);
        limits.inc[WHITE] = std::chrono::milliseconds(params.winc);
        limits.inc[BLACK] = std::chrono::milliseconds(params.binc);
    }
    
    limits.infinite = params.infinite;
    
    // Stage 14, Deliverable 1.8: Pass quiescence option to search
    limits.useQuiescence = m_useQuiescence;
    
    // Stage 13 Remediation: Pass aspiration window parameters
    limits.aspirationWindow = m_aspirationWindow;
    limits.aspirationMaxAttempts = m_aspirationMaxAttempts;
    limits.stabilityThreshold = m_stabilityThreshold;
    limits.useAspirationWindows = m_useAspirationWindows;
    
    // Stage 13, Deliverable 5.1a: Use iterative test wrapper for enhanced UCI output
    Move bestMove = search::searchIterativeTest(m_board, limits, &m_tt);
    
    // Note: search::search already outputs UCI info during search
    
    // Send best move
    if (bestMove != Move()) {
        sendBestMove(bestMove);
    } else {
        // No legal moves (checkmate or stalemate)
        sendBestMove(Move());
    }
}

Move UCIEngine::selectRandomMove() {
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(m_board, legalMoves);
    
    if (legalMoves.size() == 0) {
        return Move(); // No legal moves
    }
    
    // Simple random selection using lazy initialization to avoid static init hang
    static auto getRandomGenerator = []() -> std::mt19937& {
        static std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
        return gen;
    };
    
    std::uniform_int_distribution<size_t> dist(0, legalMoves.size() - 1);
    
    return legalMoves[dist(getRandomGenerator())];
}

void UCIEngine::updateSearchInfo(SearchInfo& info, Move bestMove, int64_t searchTimeMs) {
    info.depth = 1; // Stage 3: no actual search depth
    info.nodes = 1; // Stage 3: minimal node count
    info.timeMs = searchTimeMs;
    info.pv = moveToUCI(bestMove); // Principal variation is just the move
}

void UCIEngine::handleStop() {
    // For Stage 3: move selection is instantaneous, so just acknowledge
    // In future phases, this would interrupt search
}

void UCIEngine::handleQuit() {
    m_quit = true;
}

void UCIEngine::handleBench(const std::vector<std::string>& tokens) {
    // Parse optional depth parameter
    int depth = 0;  // 0 means use default depths
    
    if (tokens.size() > 1) {
        try {
            depth = std::stoi(tokens[1]);
            if (depth < 1 || depth > 10) {
                sendInfo("Invalid bench depth. Using default depths.");
                depth = 0;
            }
        } catch (...) {
            sendInfo("Invalid bench parameter. Usage: bench [depth]");
            depth = 0;
        }
    }
    
    // Run the benchmark suite
    auto result = BenchmarkSuite::runBenchmark(depth, true);
    
    // Send final summary as info string for GUI compatibility
    std::ostringstream oss;
    oss << "Benchmark complete: " << result.totalNodes << " nodes, "
        << std::fixed << std::setprecision(0) << result.averageNps() << " nps";
    sendInfo(oss.str());
}

void UCIEngine::runBenchmark(int depth) {
    // Run benchmark directly without UCI loop (for OpenBench)
    // Use verbose=false to avoid console output conflicts
    auto result = BenchmarkSuite::runBenchmark(depth, false);
    
    // Output final result as info string (OpenBench format)
    std::cout << "info string Benchmark complete: " << result.totalNodes 
              << " nodes, " << std::fixed << std::setprecision(0) 
              << result.averageNps() << " nps" << std::endl;
}

void UCIEngine::sendInfo(const std::string& message) {
    std::cout << "info string " << message << std::endl;
}

void UCIEngine::sendBestMove(Move move) {
    std::cout << "bestmove " << moveToUCI(move) << std::endl;
}

std::vector<std::string> UCIEngine::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// Stage 9b: Draw detection helper methods
void UCIEngine::clearGameHistory() {
    m_board.clearGameHistory();  // Clear board's game history
}

void UCIEngine::updateGameHistory() {
    // Push current position to board's game history
    m_board.pushGameHistory();
}

int UCIEngine::countRepetitionsInGame(Hash key) const {
    // This method is not needed - Board class handles repetition counting
    return 0;
}

void UCIEngine::reportDrawIfDetected() {
    // Check for draws and report via info string
    if (m_board.isRepetitionDraw()) {
        std::cout << "info string Draw by threefold repetition detected" << std::endl;
    } else if (m_board.isFiftyMoveRule()) {
        std::cout << "info string Draw by fifty-move rule detected" << std::endl;
    } else if (m_board.isInsufficientMaterial()) {
        std::cout << "info string Draw by insufficient material detected" << std::endl;
    }
}

void UCIEngine::handleUCINewGame() {
    // Clear all game state for a new game
    m_board.clear();
    m_board.setStartingPosition();
    m_board.clearGameHistory();
    m_tt.clear();  // Clear TT for new game
    // No need to push - board tracks its own history
}

// Stage 14, Deliverable 1.8: Handle UCI setoption command
void UCIEngine::handleSetOption(const std::vector<std::string>& tokens) {
    // Format: setoption name <name> value <value>
    if (tokens.size() < 5) return;
    
    // Find the option name
    size_t nameIdx = 0;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "name" && i + 1 < tokens.size()) {
            nameIdx = i + 1;
            break;
        }
    }
    
    if (nameIdx == 0) return;
    
    // Find the value
    size_t valueIdx = 0;
    for (size_t i = nameIdx + 1; i < tokens.size(); i++) {
        if (tokens[i] == "value" && i + 1 < tokens.size()) {
            valueIdx = i + 1;
            break;
        }
    }
    
    if (valueIdx == 0) return;
    
    const std::string& optionName = tokens[nameIdx];
    const std::string& value = tokens[valueIdx];
    
    // Handle UseQuiescence option
    if (optionName == "UseQuiescence") {
        if (value == "true") {
            m_useQuiescence = true;
            std::cerr << "info string Quiescence search enabled" << std::endl;
        } else if (value == "false") {
            m_useQuiescence = false;
            std::cerr << "info string Quiescence search disabled" << std::endl;
        }
    }
    // Stage 10 Remediation: Handle UseMagicBitboards option
    else if (optionName == "UseMagicBitboards") {
        if (value == "true") {
            m_useMagicBitboards = true;
            seajay::getConfig().useMagicBitboards = true;
            std::cerr << "info string Magic bitboards enabled (79x speedup!)" << std::endl;
        } else if (value == "false") {
            m_useMagicBitboards = false;
            seajay::getConfig().useMagicBitboards = false;
            std::cerr << "info string Magic bitboards disabled (using ray-based)" << std::endl;
        }
    }
    // Stage 12: Handle Hash option (TT size in MB)
    else if (optionName == "Hash") {
        try {
            int sizeInMB = std::stoi(value);
            if (sizeInMB >= 1 && sizeInMB <= 16384) {
                m_tt.resize(sizeInMB);
                std::cerr << "info string Hash table resized to " << sizeInMB << " MB" << std::endl;
            } else {
                std::cerr << "info string Invalid hash size (must be 1-16384 MB)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid hash size value" << std::endl;
        }
    }
    // Stage 12: Handle UseTranspositionTable option
    else if (optionName == "UseTranspositionTable") {
        if (value == "true") {
            m_tt.setEnabled(true);
            std::cerr << "info string Transposition table enabled" << std::endl;
        } else if (value == "false") {
            m_tt.setEnabled(false);
            std::cerr << "info string Transposition table disabled" << std::endl;
        }
    }
    // Stage 15 Day 5: Handle SEEMode option
    else if (optionName == "SEEMode") {
        if (value == "off" || value == "testing" || value == "shadow" || value == "production") {
            m_seeMode = value;
            
            // Update the global SEE move ordering instance
            search::SEEMode mode = search::parseSEEMode(value);
            search::g_seeMoveOrdering.setMode(mode);
            
            // Report the mode change
            std::cerr << "info string SEE mode set to: " << value << std::endl;
            
            // Reset statistics when changing modes
            search::SEEMoveOrdering::getStats().reset();
            
            // Additional info for each mode
            if (value == "testing") {
                std::cerr << "info string SEE Testing Mode: Using SEE for captures, logging all values" << std::endl;
            } else if (value == "shadow") {
                std::cerr << "info string SEE Shadow Mode: Calculating both SEE and MVV-LVA, using MVV-LVA" << std::endl;
            } else if (value == "production") {
                std::cerr << "info string SEE Production Mode: Using SEE for all captures" << std::endl;
            } else {
                std::cerr << "info string SEE Off: Using MVV-LVA only" << std::endl;
            }
        } else {
            std::cerr << "info string Invalid SEEMode value: " << value << std::endl;
            std::cerr << "info string Valid values: off, testing, shadow, production" << std::endl;
        }
    }
    // Stage 15 Day 6: Handle SEEPruning option
    else if (optionName == "SEEPruning") {
        if (value == "off" || value == "conservative" || value == "aggressive") {
            m_seePruning = value;
            
            // Update the global quiescence search SEE pruning mode
            search::g_seePruningMode = search::parseSEEPruningMode(value);
            search::g_seePruningStats.reset();  // Reset statistics when changing mode
            
            std::cerr << "info string SEE pruning mode set to: " << value << std::endl;
            
            // Additional info for each mode
            if (value == "conservative") {
                std::cerr << "info string Conservative SEE Pruning: Prune captures with SEE < -100" << std::endl;
            } else if (value == "aggressive") {
                std::cerr << "info string Aggressive SEE Pruning: Prune captures with SEE < -50" << std::endl;
            } else {
                std::cerr << "info string SEE Pruning disabled" << std::endl;
            }
        } else {
            std::cerr << "info string Invalid SEEPruning value: " << value << std::endl;
            std::cerr << "info string Valid values: off, conservative, aggressive" << std::endl;
        }
    }
    // Stage 13 Remediation: Handle aspiration window options
    else if (optionName == "AspirationWindow") {
        try {
            int windowSize = std::stoi(value);
            if (windowSize >= 5 && windowSize <= 50) {
                m_aspirationWindow = windowSize;
                std::cerr << "info string Aspiration window set to: " << windowSize << " cp" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid AspirationWindow value: " << value << std::endl;
        }
    }
    else if (optionName == "AspirationMaxAttempts") {
        try {
            int attempts = std::stoi(value);
            if (attempts >= 3 && attempts <= 10) {
                m_aspirationMaxAttempts = attempts;
                std::cerr << "info string Aspiration max attempts set to: " << attempts << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid AspirationMaxAttempts value: " << value << std::endl;
        }
    }
    else if (optionName == "StabilityThreshold") {
        try {
            int threshold = std::stoi(value);
            if (threshold >= 3 && threshold <= 12) {
                m_stabilityThreshold = threshold;
                std::cerr << "info string Stability threshold set to: " << threshold << " iterations" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid StabilityThreshold value: " << value << std::endl;
        }
    }
    else if (optionName == "UseAspirationWindows") {
        if (value == "true") {
            m_useAspirationWindows = true;
            std::cerr << "info string Aspiration windows enabled" << std::endl;
        } else if (value == "false") {
            m_useAspirationWindows = false;
            std::cerr << "info string Aspiration windows disabled" << std::endl;
        }
    }
    // Ignore unknown options (UCI requirement)
}