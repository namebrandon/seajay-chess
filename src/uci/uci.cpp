#include "uci.h"
#include "../version.h"                // For SEAJAY_VERSION
#include "../benchmark/benchmark.h"
#include "../search/search.h"
#include "../search/negamax.h"
#include "../search/types.h"
#include "../search/game_phase.h"  // Phase A2: coarse phase detection
#include "../core/bitboard.h"      // Phase A2: popCount for phase(0-256)
#include "../search/move_ordering.h"  // Stage 15: For SEE integration
#include "../search/quiescence.h"      // Stage 15 Day 6: For SEE pruning mode
#include "../evaluation/king_safety.h"  // For king safety parameter tuning
#include "../search/lmr.h"             // For LMR table initialization
#include "../core/engine_config.h"    // Stage 10 Remediation: Runtime configuration
#include <cmath>                       // For std::round in SPSA float parsing
#include "../core/magic_bitboards.h"  // Phase 3.3.a: For initialization
#include "../evaluation/pawn_structure.h"  // Phase PP2: For initialization
#include "../evaluation/evaluate.h"   // For evaluation functions
#include "../evaluation/eval_trace.h"  // For evaluation tracing
#include "../evaluation/pst.h"       // For SPSA PST tuning
#include <iostream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <thread>
#include <cmath>

using namespace seajay;

UCIEngine::UCIEngine() : m_quit(false), m_tt() {
    // Phase 3.3.a: Initialize magic bitboards at startup (removed from hot path)
    seajay::magic_v2::initMagics();
    
    // Initialize passed pawn masks (Phase PP2)
    PawnStructure::initPassedPawnMasks();
    
    // Initialize LMR reduction table (improved logarithmic version)
    search::initLMRTable();
    
    // Initialize piece values to defaults (can be overridden via UCI)
    eval::setPieceValueMg(PAWN, m_pawnValueMg);
    eval::setPieceValueMg(KNIGHT, m_knightValueMg);
    eval::setPieceValueMg(BISHOP, m_bishopValueMg);
    eval::setPieceValueMg(ROOK, m_rookValueMg);
    eval::setPieceValueMg(QUEEN, m_queenValueMg);
    
    // Initialize endgame piece values
    eval::setPieceValueEg(PAWN, m_pawnValueEg);
    eval::setPieceValueEg(KNIGHT, m_knightValueEg);
    eval::setPieceValueEg(BISHOP, m_bishopValueEg);
    eval::setPieceValueEg(ROOK, m_rookValueEg);
    eval::setPieceValueEg(QUEEN, m_queenValueEg);
    
    // Initialize board to starting position
    m_board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    m_board.clearGameHistory();
    // No need to push - board tracks its own history
}

UCIEngine::~UCIEngine() {
    // Clean up search thread on destruction
    stopSearch();
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
        else if (command == "dumpPST") {
            handleDumpPST();  // SPSA debug: dump current PST values
        }
        else if (command == "d" || command == "debug") {
            handleDebug(tokens);  // Debug command handler
        }
        // Ignore unknown commands (UCI protocol requirement)
    }
}

void UCIEngine::handleUCI() {
    std::cout << "id name SeaJay v" << SEAJAY_VERSION << std::endl;
    std::cout << "id author Brandon Harris" << std::endl;
    // Stage 15: Static Exchange Evaluation (SEE) - Day 3 X-Ray Support
    
    // Stage 10 Remediation: UCI option for magic bitboards (79x speedup!)
    std::cout << "option name UseMagicBitboards type check default true" << std::endl;
    
    // Stage 14, Deliverable 1.8: UCI option for quiescence search
    std::cout << "option name UseQuiescence type check default true" << std::endl;
    
    // Stage 14 Remediation: Runtime node limit for quiescence search
    std::cout << "option name QSearchNodeLimit type spin default 0 min 0 max 10000000" << std::endl;
    
    // Maximum check extension depth in quiescence search
    std::cout << "option name MaxCheckPly type spin default 6 min 0 max 10" << std::endl;
    
    // Stage 15 Day 5: SEE integration mode option
    std::cout << "option name SEEMode type combo default off var off var testing var shadow var production" << std::endl;
    
    // Stage 15 Day 6: SEE-based pruning in quiescence
    std::cout << "option name SEEPruning type combo default conservative var off var conservative var aggressive" << std::endl;
    
    // Stage 18: Late Move Reductions (LMR) options
    std::cout << "option name LMREnabled type check default true" << std::endl;
    std::cout << "option name LMRMinDepth type spin default 2 min 0 max 10" << std::endl;
    std::cout << "option name LMRMinMoveNumber type spin default 2 min 0 max 20" << std::endl;
    std::cout << "option name LMRBaseReduction type spin default 50 min 0 max 200" << std::endl;
    std::cout << "option name LMRDepthFactor type spin default 225 min 100 max 400" << std::endl;
    std::cout << "option name LMRHistoryThreshold type spin default 50 min 10 max 90" << std::endl;
    std::cout << "option name LMRPvReduction type spin default 1 min 0 max 2" << std::endl;
    std::cout << "option name LMRNonImprovingBonus type spin default 1 min 0 max 3" << std::endl;
    
    // Stage 21: Null Move Pruning options
    std::cout << "option name UseNullMove type check default true" << std::endl;  // Enabled for Phase A2
    std::cout << "option name NullMoveStaticMargin type spin default 87 min 50 max 300" << std::endl;  // SPSA-tuned
    std::cout << "option name NullMoveMinDepth type spin default 2 min 2 max 5" << std::endl;
    std::cout << "option name NullMoveReductionBase type spin default 4 min 1 max 6" << std::endl;
    std::cout << "option name NullMoveReductionDepth6 type spin default 4 min 2 max 6" << std::endl;
    std::cout << "option name NullMoveReductionDepth12 type spin default 5 min 3 max 7" << std::endl;
    std::cout << "option name NullMoveVerifyDepth type spin default 10 min 6 max 14" << std::endl;
    std::cout << "option name NullMoveEvalMargin type spin default 198 min 100 max 400" << std::endl;
    
    // PST Phase Interpolation option
    std::cout << "option name UsePSTInterpolation type check default true" << std::endl;
    
    // Phase A2: Phase visibility toggle for eval
    std::cout << "option name ShowPhaseInfo type check default true" << std::endl;
    // B0: One-shot search summary at end of go
    std::cout << "option name SearchStats type check default false" << std::endl;
    
    // Node explosion diagnostics option
    std::cout << "option name NodeExplosionDiagnostics type check default false" << std::endl;
    
    // Evaluation detail option
    std::cout << "option name EvalExtended type check default false" << std::endl;
    
    // Middlegame piece values (SPSA tuned 2025-01-04 with 150k games)
    std::cout << "option name PawnValueMg type spin default 71 min 50 max 130" << std::endl;
    std::cout << "option name KnightValueMg type spin default 325 min 280 max 360" << std::endl;
    std::cout << "option name BishopValueMg type spin default 344 min 290 max 370" << std::endl;
    std::cout << "option name RookValueMg type spin default 487 min 450 max 570" << std::endl;
    std::cout << "option name QueenValueMg type spin default 895 min 850 max 1050" << std::endl;
    
    // Endgame piece values (SPSA tuned 2025-01-04 with 150k games)
    std::cout << "option name PawnValueEg type spin default 92 min 60 max 140" << std::endl;
    std::cout << "option name KnightValueEg type spin default 311 min 270 max 340" << std::endl;
    std::cout << "option name BishopValueEg type spin default 327 min 300 max 380" << std::endl;
    std::cout << "option name RookValueEg type spin default 510 min 480 max 600" << std::endl;
    std::cout << "option name QueenValueEg type spin default 932 min 830 max 1030" << std::endl;
    
    // Phase R1/R2: Razoring options
    std::cout << "option name UseRazoring type check default true" << std::endl;
    std::cout << "option name RazorMargin1 type spin default 274 min 100 max 800" << std::endl;
    std::cout << "option name RazorMargin2 type spin default 468 min 200 max 1200" << std::endl;
    
    // Futility Pruning options
    std::cout << "option name UseFutilityPruning type check default true" << std::endl;
    std::cout << "option name FutilityMargin1 type spin default 240 min 50 max 400" << std::endl;
    std::cout << "option name FutilityMargin2 type spin default 313 min 100 max 500" << std::endl;
    std::cout << "option name FutilityMargin3 type spin default 386 min 150 max 600" << std::endl;
    std::cout << "option name FutilityMargin4 type spin default 459 min 200 max 700" << std::endl;
    
    // SPSA PST Tuning Options - Simplified approach with zones
    // Pawn endgame values
    std::cout << "option name pawn_eg_r3_d type spin default 8 min 0 max 30" << std::endl;
    std::cout << "option name pawn_eg_r3_e type spin default 7 min 0 max 30" << std::endl;
    std::cout << "option name pawn_eg_r4_d type spin default 18 min 10 max 50" << std::endl;
    std::cout << "option name pawn_eg_r4_e type spin default 16 min 10 max 50" << std::endl;
    std::cout << "option name pawn_eg_r5_d type spin default 29 min 20 max 70" << std::endl;
    std::cout << "option name pawn_eg_r5_e type spin default 27 min 20 max 70" << std::endl;
    std::cout << "option name pawn_eg_r6_d type spin default 51 min 30 max 100" << std::endl;
    std::cout << "option name pawn_eg_r6_e type spin default 48 min 30 max 100" << std::endl;
    std::cout << "option name pawn_eg_r7_center type spin default 75 min 50 max 150" << std::endl;
    
    // Knight endgame values
    std::cout << "option name knight_eg_center type spin default 15 min 5 max 25" << std::endl;
    std::cout << "option name knight_eg_extended type spin default 10 min 0 max 20" << std::endl;
    std::cout << "option name knight_eg_edge type spin default -25 min -40 max -10" << std::endl;
    std::cout << "option name knight_eg_corner type spin default -40 min -50 max -20" << std::endl;
    
    // Bishop endgame values
    std::cout << "option name bishop_eg_long_diag type spin default 19 min 10 max 35" << std::endl;
    std::cout << "option name bishop_eg_center type spin default 14 min 5 max 25" << std::endl;
    std::cout << "option name bishop_eg_edge type spin default -5 min -15 max 5" << std::endl;
    
    // Rook endgame values
    std::cout << "option name rook_eg_7th type spin default 20 min 15 max 40" << std::endl;
    std::cout << "option name rook_eg_active type spin default 12 min 5 max 20" << std::endl;
    std::cout << "option name rook_eg_passive type spin default 5 min 0 max 15" << std::endl;
    
    // Queen endgame values
    std::cout << "option name queen_eg_center type spin default 9 min 5 max 20" << std::endl;
    std::cout << "option name queen_eg_active type spin default 7 min 0 max 20" << std::endl;
    std::cout << "option name queen_eg_back type spin default -5 min -10 max 5" << std::endl;
    
    // King safety parameters
    std::cout << "option name KingSafetyDirectShieldMg type spin default 19 min 0 max 50" << std::endl;
    std::cout << "option name KingSafetyAdvancedShieldMg type spin default 6 min 0 max 40" << std::endl;
    std::cout << "option name KingSafetyEnableScoring type spin default 1 min 0 max 1" << std::endl;
    
    // King PST middlegame values (key squares for castling incentives)
    std::cout << "option name king_mg_e1 type spin default 21 min -50 max 50" << std::endl;  // Starting square
    std::cout << "option name king_mg_b1 type spin default -5 min -50 max 50" << std::endl;   // Queenside castled
    std::cout << "option name king_mg_g1 type spin default 16 min -50 max 50" << std::endl;   // Kingside castled
    std::cout << "option name king_mg_a1 type spin default 25 min -50 max 50" << std::endl;   // Corner (SPSA: +25)
    std::cout << "option name king_mg_h1 type spin default 12 min -50 max 50" << std::endl;   // Corner (SPSA: +12)
    std::cout << "option name king_mg_c1 type spin default 8 min -50 max 50" << std::endl;    // Near queenside (SPSA: +8)
    std::cout << "option name king_mg_d1 type spin default -27 min -50 max 50" << std::endl;  // Center (SPSA: -27)
    std::cout << "option name king_mg_f1 type spin default -28 min -50 max 50" << std::endl;  // Center (SPSA: -28)
    
    // Stage 12: Transposition Table options
    std::cout << "option name Hash type spin default 16 min 1 max 16384" << std::endl;  // TT size in MB
    std::cout << "option name UseTranspositionTable type check default true" << std::endl;  // Enable/disable TT
    
    // Multi-threading option (stub for OpenBench compatibility)
    std::cout << "option name Threads type spin default 1 min 1 max 1024" << std::endl;
    
    // Stage 13 Remediation: Aspiration window and time management options
    std::cout << "option name AspirationWindow type spin default 13 min 5 max 50" << std::endl;
    
    // Stage 22 Phase P3.5: PVS statistics output option
    std::cout << "option name ShowPVSStats type check default false" << std::endl;
    
    // Stage 23 CM3.1: Countermove heuristic bonus (micro-phase testing)
    std::cout << "option name CountermoveBonus type spin default 7960 min 0 max 20000" << std::endl;
    
    // Phase 3: Move Count Pruning parameters (conservative implementation)
    std::cout << "option name MoveCountPruning type check default true" << std::endl;
    std::cout << "option name MoveCountLimit3 type spin default 7 min 3 max 50" << std::endl;
    std::cout << "option name MoveCountLimit4 type spin default 15 min 5 max 60" << std::endl;
    std::cout << "option name MoveCountLimit5 type spin default 20 min 8 max 70" << std::endl;
    std::cout << "option name MoveCountLimit6 type spin default 25 min 10 max 80" << std::endl;
    std::cout << "option name MoveCountLimit7 type spin default 36 min 12 max 90" << std::endl;
    std::cout << "option name MoveCountLimit8 type spin default 42 min 15 max 100" << std::endl;
    std::cout << "option name MoveCountHistoryThreshold type spin default 0 min 0 max 5000" << std::endl;
    std::cout << "option name MoveCountHistoryBonus type spin default 6 min 0 max 20" << std::endl;
    std::cout << "option name MoveCountImprovingRatio type spin default 75 min 50 max 100" << std::endl;
    
    std::cout << "option name AspirationMaxAttempts type spin default 5 min 3 max 10" << std::endl;
    std::cout << "option name StabilityThreshold type spin default 6 min 3 max 12" << std::endl;
    std::cout << "option name UseAspirationWindows type check default true" << std::endl;
    
    // Stage 13 Remediation Phase 4: Advanced features
    std::cout << "option name AspirationGrowth type combo default exponential var linear var moderate var exponential var adaptive" << std::endl;
    std::cout << "option name UsePhaseStability type check default true" << std::endl;
    std::cout << "option name OpeningStability type spin default 4 min 2 max 8" << std::endl;
    std::cout << "option name MiddlegameStability type spin default 6 min 3 max 10" << std::endl;
    std::cout << "option name EndgameStability type spin default 8 min 4 max 12" << std::endl;
    
    // Futility Pruning options (Phase 4 investigation)
    std::cout << "option name FutilityPruning type check default true" << std::endl;
    std::cout << "option name FutilityMaxDepth type spin default 7 min 0 max 10" << std::endl;
    std::cout << "option name FutilityBase type spin default 240 min 50 max 500" << std::endl;
    std::cout << "option name FutilityScale type spin default 73 min 20 max 200" << std::endl;
    
    // Important notice about evaluation scoring
    std::cout << "info string NOTE: SeaJay uses negamax scoring - all evaluations are from the side-to-move perspective. Positive scores mean the current player to move is winning." << std::endl;
    
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
    
    // Recalculate material values with current piece values AFTER all moves
    // This ensures UCI-configured piece values are applied to the final position
    m_board.recalculateMaterial();
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

void UCIEngine::stopSearch() {
    // Signal stop to any running search
    m_stopRequested.store(true, std::memory_order_relaxed);
    
    // Wait for search thread to finish if it's running
    if (m_searchThread.joinable()) {
        m_searchThread.join();
    }
    
    m_searching.store(false, std::memory_order_relaxed);
}

void UCIEngine::search(const SearchParams& params) {
    // Stop any previous search
    stopSearch();
    
    // Reset stop flag for new search
    m_stopRequested.store(false, std::memory_order_relaxed);
    
    // Start search in separate thread for proper stop handling
    m_searching.store(true, std::memory_order_relaxed);
    m_searchThread = std::thread(&UCIEngine::searchThreadFunc, this, params);
    
    // For non-infinite searches, wait for completion
    // Infinite searches will continue running until stop command
    if (!params.infinite) {
        if (m_searchThread.joinable()) {
            m_searchThread.join();
        }
        m_searching.store(false, std::memory_order_relaxed);
    }
}

void UCIEngine::searchThreadFunc(const SearchParams& params) {
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
            m_searching.store(false, std::memory_order_relaxed);
            return;
        } else {
            // No legal moves (checkmate or stalemate)
            std::cout << "info depth 1 score mate 0 nodes 1" << std::endl;
            std::cout << "bestmove 0000" << std::endl;
            m_searching.store(false, std::memory_order_relaxed);
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
    
    // Pass stop flag for external stop handling (LazySMP compatible)
    limits.stopFlag = &m_stopRequested;
    
    // Stage 14, Deliverable 1.8: Pass quiescence option to search
    limits.useQuiescence = m_useQuiescence;
    
    // Stage 14 Remediation: Pass runtime node limit
    limits.qsearchNodeLimit = m_qsearchNodeLimit;
    limits.maxCheckPly = m_maxCheckPly;  // Pass maximum check extension depth
    
    // Stage 18: Pass LMR parameters  
    limits.lmrEnabled = m_lmrEnabled;
    limits.lmrMinDepth = m_lmrMinDepth;
    limits.lmrMinMoveNumber = m_lmrMinMoveNumber;
    limits.lmrBaseReduction = m_lmrBaseReduction;
    limits.lmrDepthFactor = m_lmrDepthFactor;
    limits.lmrHistoryThreshold = m_lmrHistoryThreshold;
    limits.lmrPvReduction = m_lmrPvReduction;
    limits.lmrNonImprovingBonus = m_lmrNonImprovingBonus;
    
    // Stage 21: Pass null move pruning options
    limits.useNullMove = m_useNullMove;
    limits.nullMoveStaticMargin = m_nullMoveStaticMargin;
    limits.nullMoveMinDepth = m_nullMoveMinDepth;
    limits.nullMoveReductionBase = m_nullMoveReductionBase;
    limits.nullMoveReductionDepth6 = m_nullMoveReductionDepth6;
    limits.nullMoveReductionDepth12 = m_nullMoveReductionDepth12;
    limits.nullMoveVerifyDepth = m_nullMoveVerifyDepth;
    limits.nullMoveEvalMargin = m_nullMoveEvalMargin;
    
    // Futility pruning parameters
    limits.useFutilityPruning = m_useFutilityPruning;
    limits.futilityMargin1 = m_futilityMargin1;
    limits.futilityMargin2 = m_futilityMargin2;
    limits.futilityMargin3 = m_futilityMargin3;
    limits.futilityMargin4 = m_futilityMargin4;
    
    // Phase R1: Pass razoring options
    limits.useRazoring = m_useRazoring;
    limits.razorMargin1 = m_razorMargin1;
    limits.razorMargin2 = m_razorMargin2;
    
    // Stage 13 Remediation: Pass aspiration window parameters
    limits.aspirationWindow = m_aspirationWindow;
    limits.aspirationMaxAttempts = m_aspirationMaxAttempts;
    limits.stabilityThreshold = m_stabilityThreshold;
    limits.useAspirationWindows = m_useAspirationWindows;
    
    // Stage 13 Remediation Phase 4: Pass advanced features
    limits.aspirationGrowth = m_aspirationGrowth;
    limits.usePhaseStability = m_usePhaseStability;
    limits.openingStability = m_openingStability;
    limits.middlegameStability = m_middlegameStability;
    limits.endgameStability = m_endgameStability;
    
    // Stage 15: Pass SEE pruning mode
    limits.seePruningMode = m_seePruning;
    
    // Stage 22 Phase P3.5: Pass PVS statistics output flag
    limits.showPVSStats = m_showPVSStats;
    // B0: One-shot search summary toggle
    limits.showSearchStats = m_showSearchStats;
    // Node explosion diagnostics toggle
    limits.nodeExplosionDiagnostics = m_nodeExplosionDiagnostics;
    
    // Stage 23 CM3.3: Pass countermove bonus to search
    limits.countermoveBonus = m_countermoveBonus;
    
    // Phase 3: Pass move count pruning parameters to search
    limits.useMoveCountPruning = m_useMoveCountPruning;
    limits.moveCountLimit3 = m_moveCountLimit3;
    limits.moveCountLimit4 = m_moveCountLimit4;
    limits.moveCountLimit5 = m_moveCountLimit5;
    limits.moveCountLimit6 = m_moveCountLimit6;
    limits.moveCountLimit7 = m_moveCountLimit7;
    limits.moveCountLimit8 = m_moveCountLimit8;
    limits.moveCountHistoryThreshold = m_moveCountHistoryThreshold;
    limits.moveCountHistoryBonus = m_moveCountHistoryBonus;
    limits.moveCountImprovingRatio = m_moveCountImprovingRatio;
    
    // Stage 13, Deliverable 5.1a: Use iterative test wrapper for enhanced UCI output
    Move bestMove = search::searchIterativeTest(m_board, limits, &m_tt);
    
    // Note: search::search already outputs UCI info during search
    
    // ALWAYS send bestmove, even if stopped (UCI protocol requirement)
    // GUIs expect a bestmove response after every go command
    if (bestMove != Move()) {
        sendBestMove(bestMove);
    } else {
        // No legal moves or no best move found - send null move
        // This ensures GUI always gets a response
        sendBestMove(Move());
    }
    
    // Mark search as complete
    m_searching.store(false, std::memory_order_relaxed);
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
    // Stop any ongoing search
    stopSearch();
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
    // Stage 14 Remediation: Handle QSearchNodeLimit option
    else if (optionName == "QSearchNodeLimit") {
        try {
            uint64_t limit = std::stoull(value);
            m_qsearchNodeLimit = limit;
            if (limit == 0) {
                std::cerr << "info string Quiescence node limit: unlimited" << std::endl;
            } else {
                std::cerr << "info string Quiescence node limit: " << limit << " nodes per position" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid QSearchNodeLimit value: " << value << std::endl;
        }
    }
    // Handle MaxCheckPly option
    else if (optionName == "MaxCheckPly") {
        try {
            int checkPly = std::stoi(value);
            if (checkPly >= 0 && checkPly <= 10) {
                m_maxCheckPly = checkPly;
                std::cerr << "info string Maximum check extension depth set to: " << checkPly << std::endl;
            } else {
                std::cerr << "info string Invalid MaxCheckPly value: " << value << " (must be 0-10)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid MaxCheckPly value: " << value << std::endl;
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
    // Handle Threads option (multi-threading stub for OpenBench compatibility)
    else if (optionName == "Threads") {
        try {
            int threads = std::stoi(value);
            if (threads >= 1 && threads <= 1024) {
                m_threads = threads;
                if (threads == 1) {
                    std::cerr << "info string Threads set to 1" << std::endl;
                } else {
                    std::cerr << "info string Threads set to " << threads 
                              << " (multi-threading not yet implemented, using 1 thread)" << std::endl;
                }
            } else {
                std::cerr << "info string Invalid Threads value: " << value 
                          << " (must be between 1 and 1024)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid Threads value: " << value << std::endl;
        }
    }
    // Handle King Safety parameters (with proper SPSA rounding)
    else if (optionName == "KingSafetyDirectShieldMg") {
        try {
            // SPSA-compatible rounding for float values
            int value_int = 0;
            try {
                double dv = std::stod(value);
                value_int = static_cast<int>(std::round(dv));
            } catch (...) {
                value_int = std::stoi(value);  // Fallback for integer strings
            }
            
            if (value_int >= 0 && value_int <= 50) {
                auto params = eval::KingSafety::getParams();
                params.directShieldMg = value_int;
                eval::KingSafety::setParams(params);
                std::cerr << "info string KingSafetyDirectShieldMg set to " << value_int << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid KingSafetyDirectShieldMg value: " << value << std::endl;
        }
    }
    else if (optionName == "KingSafetyAdvancedShieldMg") {
        try {
            // SPSA-compatible rounding for float values
            int value_int = 0;
            try {
                double dv = std::stod(value);
                value_int = static_cast<int>(std::round(dv));
            } catch (...) {
                value_int = std::stoi(value);  // Fallback for integer strings
            }
            
            if (value_int >= 0 && value_int <= 40) {
                auto params = eval::KingSafety::getParams();
                params.advancedShieldMg = value_int;
                eval::KingSafety::setParams(params);
                std::cerr << "info string KingSafetyAdvancedShieldMg set to " << value_int << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid KingSafetyAdvancedShieldMg value: " << value << std::endl;
        }
    }
    else if (optionName == "KingSafetyEnableScoring") {
        try {
            // SPSA-compatible rounding for float values
            int value_int = 0;
            try {
                double dv = std::stod(value);
                value_int = static_cast<int>(std::round(dv));
            } catch (...) {
                value_int = std::stoi(value);  // Fallback for integer strings
            }
            
            if (value_int >= 0 && value_int <= 1) {
                auto params = eval::KingSafety::getParams();
                params.enableScoring = value_int;
                eval::KingSafety::setParams(params);
                std::cerr << "info string KingSafetyEnableScoring set to " << value_int << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid KingSafetyEnableScoring value: " << value << std::endl;
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
            
            std::cerr << "info string SEE pruning mode set to: " << value << std::endl;
            
            // Additional info for each mode
            if (value == "conservative") {
                std::cerr << "info string Conservative SEE Pruning: Prune captures with SEE < -100" << std::endl;
            } else if (value == "aggressive") {
                std::cerr << "info string Aggressive SEE Pruning: Prune captures with SEE < -50 to -75" << std::endl;
            } else {
                std::cerr << "info string SEE Pruning disabled" << std::endl;
            }
        } else {
            std::cerr << "info string Invalid SEEPruning value: " << value << std::endl;
            std::cerr << "info string Valid values: off, conservative, aggressive" << std::endl;
        }
    }
    // Stage 18: Handle LMR options
    else if (optionName == "LMREnabled") {
        // Make boolean parsing case-insensitive and accept common variations
        std::string lowerValue = value;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
        
        if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on") {
            m_lmrEnabled = true;
            std::cerr << "info string LMR enabled" << std::endl;
        } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "off") {
            m_lmrEnabled = false;
            std::cerr << "info string LMR disabled" << std::endl;
        } else {
            std::cerr << "info string Invalid LMREnabled value: " << value << std::endl;
            std::cerr << "info string Valid values: true, false, 1, 0, yes, no, on, off (case-insensitive)" << std::endl;
        }
    }
    else if (optionName == "LMRMinDepth") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int depth = 0;
            try {
                double dv = std::stod(value);
                depth = static_cast<int>(std::round(dv));
            } catch (...) {
                depth = std::stoi(value);
            }
            if (depth >= 0 && depth <= 10) {
                m_lmrMinDepth = depth;
                std::cerr << "info string LMR min depth set to: " << depth << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid LMRMinDepth value: " << value << std::endl;
        }
    }
    else if (optionName == "LMRMinMoveNumber") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int moveNum = 0;
            try {
                double dv = std::stod(value);
                moveNum = static_cast<int>(std::round(dv));
            } catch (...) {
                moveNum = std::stoi(value);
            }
            if (moveNum >= 0 && moveNum <= 20) {
                m_lmrMinMoveNumber = moveNum;
                std::cerr << "info string LMR min move number set to: " << moveNum << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid LMRMinMoveNumber value: " << value << std::endl;
        }
    }
    else if (optionName == "LMRBaseReduction") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int reduction = 0;
            try {
                double dv = std::stod(value);
                reduction = static_cast<int>(std::round(dv));
            } catch (...) {
                reduction = std::stoi(value);
            }
            if (reduction >= 0 && reduction <= 200) {
                m_lmrBaseReduction = reduction;
                std::cerr << "info string LMR base reduction set to: " << reduction << " (" << (reduction/100.0) << " in formula)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid LMRBaseReduction value: " << value << std::endl;
        }
    }
    else if (optionName == "LMRDepthFactor") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int factor = 0;
            try {
                double dv = std::stod(value);
                factor = static_cast<int>(std::round(dv));
            } catch (...) {
                factor = std::stoi(value);
            }
            if (factor >= 100 && factor <= 400) {
                m_lmrDepthFactor = factor;
                std::cerr << "info string LMR depth factor set to: " << factor << " (" << (factor/100.0) << " divisor in formula)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid LMRDepthFactor value: " << value << std::endl;
        }
    }
    else if (optionName == "LMRHistoryThreshold") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int threshold = 0;
            try {
                double dv = std::stod(value);
                threshold = static_cast<int>(std::round(dv));
            } catch (...) {
                threshold = std::stoi(value);
            }
            if (threshold >= 10 && threshold <= 90) {
                m_lmrHistoryThreshold = threshold;
                std::cerr << "info string LMR history threshold set to: " << threshold << "%" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid LMRHistoryThreshold value: " << value << std::endl;
        }
    }
    else if (optionName == "LMRPvReduction") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int reduction = 0;
            try {
                double dv = std::stod(value);
                reduction = static_cast<int>(std::round(dv));
            } catch (...) {
                reduction = std::stoi(value);
            }
            if (reduction >= 0 && reduction <= 2) {
                m_lmrPvReduction = reduction;
                std::cerr << "info string LMR PV reduction set to: " << reduction << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid LMRPvReduction value: " << value << std::endl;
        }
    }
    else if (optionName == "LMRNonImprovingBonus") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int bonus = 0;
            try {
                double dv = std::stod(value);
                bonus = static_cast<int>(std::round(dv));
            } catch (...) {
                bonus = std::stoi(value);
            }
            if (bonus >= 0 && bonus <= 3) {
                m_lmrNonImprovingBonus = bonus;
                std::cerr << "info string LMR non-improving bonus set to: " << bonus << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid LMRNonImprovingBonus value: " << value << std::endl;
        }
    }
    // Stage 21: Handle UseNullMove option
    else if (optionName == "UseNullMove") {
        // Make boolean parsing case-insensitive and accept common variations
        std::string lowerValue = value;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
        
        if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on") {
            m_useNullMove = true;
            std::cerr << "info string Null move pruning enabled" << std::endl;
        } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "off") {
            m_useNullMove = false;
            std::cerr << "info string Null move pruning disabled" << std::endl;
        } else {
            std::cerr << "info string Invalid UseNullMove value: " << value << std::endl;
            std::cerr << "info string Valid values: true, false, 1, 0, yes, no, on, off (case-insensitive)" << std::endl;
        }
    }
    else if (optionName == "NullMoveStaticMargin") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int margin = 0;
            try {
                double dv = std::stod(value);
                margin = static_cast<int>(std::round(dv));
            } catch (...) {
                margin = std::stoi(value);
            }
            if (margin >= 50 && margin <= 300) {
                m_nullMoveStaticMargin = margin;
                std::cerr << "info string Null move static margin set to: " << margin << " cp" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid NullMoveStaticMargin value: " << value << std::endl;
        }
    }
    else if (optionName == "NullMoveMinDepth") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int depth = 0;
            try {
                double dv = std::stod(value);
                depth = static_cast<int>(std::round(dv));
            } catch (...) {
                depth = std::stoi(value);
            }
            if (depth >= 2 && depth <= 5) {
                m_nullMoveMinDepth = depth;
                std::cerr << "info string Null move min depth set to: " << depth << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid NullMoveMinDepth value: " << value << std::endl;
        }
    }
    else if (optionName == "NullMoveReductionBase") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int reduction = 0;
            try {
                double dv = std::stod(value);
                reduction = static_cast<int>(std::round(dv));
            } catch (...) {
                reduction = std::stoi(value);
            }
            if (reduction >= 1 && reduction <= 4) {
                m_nullMoveReductionBase = reduction;
                std::cerr << "info string Null move base reduction set to: " << reduction << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid NullMoveReductionBase value: " << value << std::endl;
        }
    }
    else if (optionName == "NullMoveReductionDepth6") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int reduction = 0;
            try {
                double dv = std::stod(value);
                reduction = static_cast<int>(std::round(dv));
            } catch (...) {
                reduction = std::stoi(value);
            }
            if (reduction >= 2 && reduction <= 5) {
                m_nullMoveReductionDepth6 = reduction;
                std::cerr << "info string Null move depth 6 reduction set to: " << reduction << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid NullMoveReductionDepth6 value: " << value << std::endl;
        }
    }
    else if (optionName == "NullMoveReductionDepth12") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int reduction = 0;
            try {
                double dv = std::stod(value);
                reduction = static_cast<int>(std::round(dv));
            } catch (...) {
                reduction = std::stoi(value);
            }
            if (reduction >= 3 && reduction <= 6) {
                m_nullMoveReductionDepth12 = reduction;
                std::cerr << "info string Null move depth 12 reduction set to: " << reduction << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid NullMoveReductionDepth12 value: " << value << std::endl;
        }
    }
    else if (optionName == "NullMoveVerifyDepth") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int depth = 0;
            try {
                double dv = std::stod(value);
                depth = static_cast<int>(std::round(dv));
            } catch (...) {
                depth = std::stoi(value);
            }
            if (depth >= 6 && depth <= 14) {
                m_nullMoveVerifyDepth = depth;
                std::cerr << "info string Null move verification depth set to: " << depth << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid NullMoveVerifyDepth value: " << value << std::endl;
        }
    }
    else if (optionName == "NullMoveEvalMargin") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int margin = 0;
            try {
                double dv = std::stod(value);
                margin = static_cast<int>(std::round(dv));
            } catch (...) {
                margin = std::stoi(value);
            }
            if (margin >= 100 && margin <= 400) {
                m_nullMoveEvalMargin = margin;
                std::cerr << "info string Null move eval margin set to: " << margin << " cp" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid NullMoveEvalMargin value: " << value << std::endl;
        }
    }
    // PST Phase Interpolation: Handle UsePSTInterpolation option
    else if (optionName == "UsePSTInterpolation") {
        // Make boolean parsing case-insensitive and accept common variations
        std::string lowerValue = value;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
        
        if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on") {
            m_usePSTInterpolation = true;
            seajay::getConfig().usePSTInterpolation = true;
            std::cerr << "info string PST phase interpolation enabled" << std::endl;
        } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "off") {
            m_usePSTInterpolation = false;
            seajay::getConfig().usePSTInterpolation = false;
            std::cerr << "info string PST phase interpolation disabled" << std::endl;
        } else {
            std::cerr << "info string Invalid UsePSTInterpolation value: " << value << std::endl;
            std::cerr << "info string Valid values: true, false, 1, 0, yes, no, on, off (case-insensitive)" << std::endl;
        }
    }
    // Futility Pruning options
    else if (optionName == "UseFutilityPruning") {
        // Boolean parsing with case-insensitive support
        std::string lowerValue = value;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
        
        if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on") {
            m_useFutilityPruning = true;
            std::cerr << "info string Futility pruning enabled" << std::endl;
        } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "off") {
            m_useFutilityPruning = false;
            std::cerr << "info string Futility pruning disabled" << std::endl;
        } else {
            std::cerr << "info string Invalid UseFutilityPruning value: " << value << std::endl;
        }
    }
    else if (optionName == "FutilityMargin1") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int margin = 0;
            try {
                double dv = std::stod(value);
                margin = static_cast<int>(std::round(dv));
            } catch (...) {
                margin = std::stoi(value);
            }
            if (margin >= 50 && margin <= 200) {
                m_futilityMargin1 = margin;
                std::cerr << "info string Futility margin 1 set to: " << margin << " cp" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid FutilityMargin1 value: " << value << std::endl;
        }
    }
    else if (optionName == "FutilityMargin2") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int margin = 0;
            try {
                double dv = std::stod(value);
                margin = static_cast<int>(std::round(dv));
            } catch (...) {
                margin = std::stoi(value);
            }
            if (margin >= 100 && margin <= 300) {
                m_futilityMargin2 = margin;
                std::cerr << "info string Futility margin 2 set to: " << margin << " cp" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid FutilityMargin2 value: " << value << std::endl;
        }
    }
    else if (optionName == "FutilityMargin3") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int margin = 0;
            try {
                double dv = std::stod(value);
                margin = static_cast<int>(std::round(dv));
            } catch (...) {
                margin = std::stoi(value);
            }
            if (margin >= 150 && margin <= 400) {
                m_futilityMargin3 = margin;
                std::cerr << "info string Futility margin 3 set to: " << margin << " cp" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid FutilityMargin3 value: " << value << std::endl;
        }
    }
    else if (optionName == "FutilityMargin4") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int margin = 0;
            try {
                double dv = std::stod(value);
                margin = static_cast<int>(std::round(dv));
            } catch (...) {
                margin = std::stoi(value);
            }
            if (margin >= 200 && margin <= 500) {
                m_futilityMargin4 = margin;
                std::cerr << "info string Futility margin 4 set to: " << margin << " cp" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid FutilityMargin4 value: " << value << std::endl;
        }
    }
    // Legacy futility options (kept for backwards compatibility)
    else if (optionName == "FutilityPruning") {
        // Boolean parsing with case-insensitive support  
        std::string lowerValue = value;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
        
        if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on") {
            seajay::getConfig().useFutilityPruning = true;
            m_useFutilityPruning = true;
            std::cerr << "info string Futility pruning enabled" << std::endl;
        } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "off") {
            seajay::getConfig().useFutilityPruning = false;
            m_useFutilityPruning = false;
            std::cerr << "info string Futility pruning disabled" << std::endl;
        } else {
            std::cerr << "info string Invalid FutilityPruning value: " << value << std::endl;
        }
    }
    else if (optionName == "FutilityMaxDepth") {
        try {
            // SPSA-compatible: handle float values from OpenBench
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                // Fallback for integer strings
                paramValue = std::stoi(value);
            }
            
            if (paramValue >= 0 && paramValue <= 10) {
                seajay::getConfig().futilityMaxDepth = paramValue;
                std::cerr << "info string FutilityMaxDepth set to " << paramValue << std::endl;
            } else {
                std::cerr << "info string Invalid FutilityMaxDepth value: " << paramValue << " (must be 0-10)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid FutilityMaxDepth value: " << value << std::endl;
        }
    }
    else if (optionName == "FutilityBase") {
        try {
            // SPSA-compatible: handle float values from OpenBench
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                // Fallback for integer strings
                paramValue = std::stoi(value);
            }
            
            if (paramValue >= 50 && paramValue <= 500) {
                seajay::getConfig().futilityBase = paramValue;
                std::cerr << "info string FutilityBase set to " << paramValue << std::endl;
            } else {
                std::cerr << "info string Invalid FutilityBase value: " << paramValue << " (must be 50-500)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid FutilityBase value: " << value << std::endl;
        }
    }
    else if (optionName == "FutilityScale") {
        try {
            // SPSA-compatible: handle float values from OpenBench
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                // Fallback for integer strings
                paramValue = std::stoi(value);
            }
            
            if (paramValue >= 20 && paramValue <= 200) {
                seajay::getConfig().futilityScale = paramValue;
                std::cerr << "info string FutilityScale set to " << paramValue << std::endl;
            } else {
                std::cerr << "info string Invalid FutilityScale value: " << paramValue << " (must be 20-200)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid FutilityScale value: " << value << std::endl;
        }
    }
    // Stage 22 Phase P3.5: Handle ShowPVSStats option
    else if (optionName == "ShowPVSStats") {
        // Make boolean parsing case-insensitive and accept common variations
        std::string lowerValue = value;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
        
        if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on") {
            m_showPVSStats = true;
            std::cerr << "info string PVS statistics output enabled" << std::endl;
        } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "off") {
            m_showPVSStats = false;
            std::cerr << "info string PVS statistics output disabled" << std::endl;
        } else {
            std::cerr << "info string Invalid ShowPVSStats value: " << value << std::endl;
            std::cerr << "info string Valid values: true, false, 1, 0, yes, no, on, off (case-insensitive)" << std::endl;
        }
    }
    // Stage 23 CM3.1: Handle CountermoveBonus option (micro-phase testing)
    else if (optionName == "CountermoveBonus") {
        try {
            int bonus = std::stoi(value);
            if (bonus >= 0 && bonus <= 20000) {
                m_countermoveBonus = bonus;
                std::cerr << "info string CountermoveBonus set to " << bonus << std::endl;
            } else {
                std::cerr << "info string CountermoveBonus value out of range: " << value << std::endl;
            }
        } catch (const std::exception&) {
            std::cerr << "info string Invalid CountermoveBonus value: " << value << std::endl;
        }
    }
    // SPSA PST Tuning Parameters
    else if (optionName.find("pawn_eg_") == 0 || 
             optionName.find("knight_eg_") == 0 ||
             optionName.find("bishop_eg_") == 0 ||
             optionName.find("rook_eg_") == 0 ||
             optionName.find("queen_eg_") == 0 ||
             optionName.find("king_mg_") == 0) {
        try {
            // OpenBench may send floats for integer parameters (e.g., 90.6).
            // Round to nearest int instead of truncating to avoid downward bias.
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                // Fallback to integer parsing when value has no decimal point
                paramValue = std::stoi(value);
            }
            eval::PST::updateFromUCIParam(optionName, paramValue);
            // CRITICAL: Recalculate PST score for current board position
            // This ensures evaluation reflects new PST values immediately
            m_board.recalculatePSTScore();
            std::cerr << "info string PST parameter " << optionName << " set to " << paramValue << std::endl;
        } catch (...) {
            std::cerr << "info string Invalid value for " << optionName << ": " << value << std::endl;
        }
    }
    // Phase 3: Move Count Pruning options
    else if (optionName == "MoveCountPruning") {
        m_useMoveCountPruning = (value == "true");
        std::cerr << "info string MoveCountPruning set to " << value << std::endl;
    }
    else if (optionName == "MoveCountLimit3") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int limit = 0;
            try {
                double dv = std::stod(value);
                limit = static_cast<int>(std::round(dv));
            } catch (...) {
                limit = std::stoi(value);
            }
            m_moveCountLimit3 = limit;
            std::cerr << "info string MoveCountLimit3 set to " << limit << std::endl;
        } catch (...) {
            std::cerr << "info string Invalid MoveCountLimit3 value: " << value << std::endl;
        }
    }
    else if (optionName == "MoveCountLimit4") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int limit = 0;
            try {
                double dv = std::stod(value);
                limit = static_cast<int>(std::round(dv));
            } catch (...) {
                limit = std::stoi(value);
            }
            m_moveCountLimit4 = limit;
            std::cerr << "info string MoveCountLimit4 set to " << limit << std::endl;
        } catch (...) {
            std::cerr << "info string Invalid MoveCountLimit4 value: " << value << std::endl;
        }
    }
    else if (optionName == "MoveCountLimit5") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int limit = 0;
            try {
                double dv = std::stod(value);
                limit = static_cast<int>(std::round(dv));
            } catch (...) {
                limit = std::stoi(value);
            }
            m_moveCountLimit5 = limit;
            std::cerr << "info string MoveCountLimit5 set to " << limit << std::endl;
        } catch (...) {
            std::cerr << "info string Invalid MoveCountLimit5 value: " << value << std::endl;
        }
    }
    else if (optionName == "MoveCountLimit6") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int limit = 0;
            try {
                double dv = std::stod(value);
                limit = static_cast<int>(std::round(dv));
            } catch (...) {
                limit = std::stoi(value);
            }
            m_moveCountLimit6 = limit;
            std::cerr << "info string MoveCountLimit6 set to " << limit << std::endl;
        } catch (...) {
            std::cerr << "info string Invalid MoveCountLimit6 value: " << value << std::endl;
        }
    }
    else if (optionName == "MoveCountLimit7") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int limit = 0;
            try {
                double dv = std::stod(value);
                limit = static_cast<int>(std::round(dv));
            } catch (...) {
                limit = std::stoi(value);
            }
            m_moveCountLimit7 = limit;
            std::cerr << "info string MoveCountLimit7 set to " << limit << std::endl;
        } catch (...) {
            std::cerr << "info string Invalid MoveCountLimit7 value: " << value << std::endl;
        }
    }
    else if (optionName == "MoveCountLimit8") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int limit = 0;
            try {
                double dv = std::stod(value);
                limit = static_cast<int>(std::round(dv));
            } catch (...) {
                limit = std::stoi(value);
            }
            m_moveCountLimit8 = limit;
            std::cerr << "info string MoveCountLimit8 set to " << limit << std::endl;
        } catch (...) {
            std::cerr << "info string Invalid MoveCountLimit8 value: " << value << std::endl;
        }
    }
    else if (optionName == "MoveCountHistoryThreshold") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int threshold = 0;
            try {
                double dv = std::stod(value);
                threshold = static_cast<int>(std::round(dv));
            } catch (...) {
                threshold = std::stoi(value);
            }
            m_moveCountHistoryThreshold = threshold;
            std::cerr << "info string MoveCountHistoryThreshold set to " << threshold << std::endl;
        } catch (...) {
            std::cerr << "info string Invalid MoveCountHistoryThreshold value: " << value << std::endl;
        }
    }
    else if (optionName == "MoveCountHistoryBonus") {
        m_moveCountHistoryBonus = std::stoi(value);
        std::cerr << "info string MoveCountHistoryBonus set to " << value << std::endl;
    }
    else if (optionName == "MoveCountImprovingRatio") {
        m_moveCountImprovingRatio = std::stoi(value);
        std::cerr << "info string MoveCountImprovingRatio set to " << value << std::endl;
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
    // Stage 13 Remediation Phase 4: Advanced features
    else if (optionName == "AspirationGrowth") {
        if (value == "linear" || value == "moderate" || value == "exponential" || value == "adaptive") {
            m_aspirationGrowth = value;
            std::cerr << "info string Aspiration growth mode set to: " << value << std::endl;
        } else {
            std::cerr << "info string Invalid AspirationGrowth value: " << value << std::endl;
        }
    }
    else if (optionName == "UsePhaseStability") {
        if (value == "true") {
            m_usePhaseStability = true;
            std::cerr << "info string Game phase stability adjustment enabled" << std::endl;
        } else if (value == "false") {
            m_usePhaseStability = false;
            std::cerr << "info string Game phase stability adjustment disabled" << std::endl;
        }
    }
    else if (optionName == "OpeningStability") {
        try {
            int threshold = std::stoi(value);
            if (threshold >= 2 && threshold <= 8) {
                m_openingStability = threshold;
                std::cerr << "info string Opening stability threshold set to: " << threshold << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid OpeningStability value: " << value << std::endl;
        }
    }
    else if (optionName == "MiddlegameStability") {
        try {
            int threshold = std::stoi(value);
            if (threshold >= 3 && threshold <= 10) {
                m_middlegameStability = threshold;
                std::cerr << "info string Middlegame stability threshold set to: " << threshold << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid MiddlegameStability value: " << value << std::endl;
        }
    }
    else if (optionName == "EndgameStability") {
        try {
            int threshold = std::stoi(value);
            if (threshold >= 4 && threshold <= 12) {
                m_endgameStability = threshold;
                std::cerr << "info string Endgame stability threshold set to: " << threshold << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid EndgameStability value: " << value << std::endl;
        }
    }
    else if (optionName == "ShowPhaseInfo") {
        if (value == "true") {
            m_showPhaseInfo = true;
            std::cerr << "info string ShowPhaseInfo enabled" << std::endl;
        } else if (value == "false") {
            m_showPhaseInfo = false;
            std::cerr << "info string ShowPhaseInfo disabled" << std::endl;
        }
    }
    else if (optionName == "SearchStats") {
        if (value == "true") {
            m_showSearchStats = true;
            std::cerr << "info string SearchStats enabled" << std::endl;
        } else if (value == "false") {
            m_showSearchStats = false;
            std::cerr << "info string SearchStats disabled" << std::endl;
        }
    }
    else if (optionName == "NodeExplosionDiagnostics") {
        if (value == "true") {
            m_nodeExplosionDiagnostics = true;
            std::cerr << "info string NodeExplosionDiagnostics enabled" << std::endl;
        } else if (value == "false") {
            m_nodeExplosionDiagnostics = false;
            std::cerr << "info string NodeExplosionDiagnostics disabled" << std::endl;
        }
    }
    else if (optionName == "EvalExtended") {
        if (value == "true") {
            m_evalExtended = true;
            std::cerr << "info string EvalExtended enabled - detailed evaluation breakdown available" << std::endl;
        } else if (value == "false") {
            m_evalExtended = false;
            std::cerr << "info string EvalExtended disabled" << std::endl;
        }
    }
    // Middlegame piece values (with SPSA float rounding)
    else if (optionName == "PawnValueMg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 70 && paramValue <= 130) {
                m_pawnValueMg = paramValue;
                eval::setPieceValueMg(PAWN, paramValue);
                std::cerr << "info string PawnValueMg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid PawnValueMg value: " << value << std::endl;
        }
    }
    else if (optionName == "KnightValueMg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 280 && paramValue <= 360) {
                m_knightValueMg = paramValue;
                eval::setPieceValueMg(KNIGHT, paramValue);
                std::cerr << "info string KnightValueMg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid KnightValueMg value: " << value << std::endl;
        }
    }
    else if (optionName == "BishopValueMg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 290 && paramValue <= 370) {
                m_bishopValueMg = paramValue;
                eval::setPieceValueMg(BISHOP, paramValue);
                std::cerr << "info string BishopValueMg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid BishopValueMg value: " << value << std::endl;
        }
    }
    else if (optionName == "RookValueMg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 450 && paramValue <= 570) {
                m_rookValueMg = paramValue;
                eval::setPieceValueMg(ROOK, paramValue);
                std::cerr << "info string RookValueMg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid RookValueMg value: " << value << std::endl;
        }
    }
    else if (optionName == "QueenValueMg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 850 && paramValue <= 1050) {
                m_queenValueMg = paramValue;
                eval::setPieceValueMg(QUEEN, paramValue);
                std::cerr << "info string QueenValueMg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid QueenValueMg value: " << value << std::endl;
        }
    }
    // Endgame piece values (SPSA tunable)
    else if (optionName == "PawnValueEg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 80 && paramValue <= 140) {
                m_pawnValueEg = paramValue;
                eval::setPieceValueEg(PAWN, paramValue);
                std::cerr << "info string PawnValueEg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid PawnValueEg value: " << value << std::endl;
        }
    }
    else if (optionName == "KnightValueEg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 270 && paramValue <= 340) {
                m_knightValueEg = paramValue;
                eval::setPieceValueEg(KNIGHT, paramValue);
                std::cerr << "info string KnightValueEg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid KnightValueEg value: " << value << std::endl;
        }
    }
    else if (optionName == "BishopValueEg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 300 && paramValue <= 380) {
                m_bishopValueEg = paramValue;
                eval::setPieceValueEg(BISHOP, paramValue);
                std::cerr << "info string BishopValueEg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid BishopValueEg value: " << value << std::endl;
        }
    }
    else if (optionName == "RookValueEg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 480 && paramValue <= 600) {
                m_rookValueEg = paramValue;
                eval::setPieceValueEg(ROOK, paramValue);
                std::cerr << "info string RookValueEg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid RookValueEg value: " << value << std::endl;
        }
    }
    else if (optionName == "QueenValueEg") {
        try {
            int paramValue = 0;
            try {
                double dv = std::stod(value);
                paramValue = static_cast<int>(std::round(dv));
            } catch (...) {
                paramValue = std::stoi(value);
            }
            if (paramValue >= 830 && paramValue <= 1030) {
                m_queenValueEg = paramValue;
                eval::setPieceValueEg(QUEEN, paramValue);
                std::cerr << "info string QueenValueEg set to " << paramValue << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid QueenValueEg value: " << value << std::endl;
        }
    }
    // Phase R1: Razoring options
    else if (optionName == "UseRazoring") {
        if (value == "true") {
            m_useRazoring = true;
            std::cerr << "info string Razoring enabled" << std::endl;
        } else if (value == "false") {
            m_useRazoring = false;
            std::cerr << "info string Razoring disabled" << std::endl;
        }
    }
    else if (optionName == "RazorMargin1") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int margin = 0;
            try {
                double dv = std::stod(value);
                margin = static_cast<int>(std::round(dv));
            } catch (...) {
                margin = std::stoi(value);
            }
            if (margin >= 100 && margin <= 800) {
                m_razorMargin1 = margin;
                std::cerr << "info string RazorMargin1 set to " << margin << " cp" << std::endl;
            } else {
                std::cerr << "info string RazorMargin1 out of range: " << margin << " (must be 100-800)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid RazorMargin1 value: " << value << std::endl;
        }
    }
    else if (optionName == "RazorMargin2") {
        try {
            // SPSA-safe parsing: handle float values from OpenBench
            int margin = 0;
            try {
                double dv = std::stod(value);
                margin = static_cast<int>(std::round(dv));
            } catch (...) {
                margin = std::stoi(value);
            }
            if (margin >= 200 && margin <= 1200) {
                m_razorMargin2 = margin;
                std::cerr << "info string RazorMargin2 set to " << margin << " cp" << std::endl;
            } else {
                std::cerr << "info string RazorMargin2 out of range: " << margin << " (must be 200-1200)" << std::endl;
            }
        } catch (...) {
            std::cerr << "info string Invalid RazorMargin2 value: " << value << std::endl;
        }
    }
    // Ignore unknown options (UCI requirement)
}


void UCIEngine::handleDumpPST() {
    // Dump current PST values for debugging SPSA tuning
    eval::PST::dumpTables();
}

void UCIEngine::handleDebug(const std::vector<std::string>& tokens) {
    // Debug command handler - provides detailed evaluation and other debug info
    // Usage: "d" or "debug" [eval|tt]
    
    if (tokens.size() > 1 && tokens[1] == "eval") {
        // Show detailed evaluation if EvalExtended is enabled
        if (m_evalExtended) {
            eval::EvalTrace trace;
            eval::Score score = eval::evaluateWithTrace(m_board, trace);
            
            // Print the detailed breakdown
            trace.print(m_board.sideToMove());
        } else {
            // Just show the basic score
            eval::Score score = eval::evaluate(m_board);
            std::cout << "Evaluation: " << score.value() << " cp" << std::endl;
            std::cout << "(Enable EvalExtended option for detailed breakdown)" << std::endl;
        }
    } else if (tokens.size() > 1 && tokens[1] == "tt") {
        // Show TT collision statistics
        const auto& stats = m_tt.stats();
        std::cout << "=== TT Collision Diagnostics ===" << std::endl;
        std::cout << "Probes: " << stats.probes.load() << std::endl;
        std::cout << "Hits: " << stats.hits.load() << " (" << stats.hitRate() << "%)" << std::endl;
        std::cout << "Stores: " << stats.stores.load() << std::endl;
        std::cout << "Store-side collisions: " << stats.collisions.load() << std::endl;
        std::cout << "Probe empties: " << stats.probeEmpties.load() << std::endl;
        std::cout << "Probe mismatches (real collisions): " << stats.probeMismatches.load() 
                  << " (" << stats.collisionRate() << "%)" << std::endl;
        std::cout << "Hashfull: " << m_tt.hashfull() << "/1000" << std::endl;
    } else {
        // Default debug output - show board state
        std::cout << "\n" << m_board.toString() << std::endl;
        std::cout << "FEN: " << m_board.toFEN() << std::endl;
        std::cout << "Side to move: " << (m_board.sideToMove() == WHITE ? "White" : "Black") << std::endl;
        
        // If EvalExtended is enabled, also show evaluation
        if (m_evalExtended) {
            eval::EvalTrace trace;
            eval::Score score = eval::evaluateWithTrace(m_board, trace);
            trace.print(m_board.sideToMove());
        }
    }
}
