// UCI Draw Integration Implementation for SeaJay
// This code shows exactly how to integrate draw detection with UCI protocol

#include "uci.h"
#include "../core/board.h"
#include "../core/attack_cache.h"  // Phase 5.2: For cache clearing
#include "../search/search.h"
#include <sstream>
#include <iomanip>

// Example modifications for UCIEngine class

// Add these member variables to UCIEngine class:
// std::vector<uint64_t> m_gameHistory;  // Track game-level repetitions

// Modified handlePosition to track game history properly
void UCIEngine::handlePositionWithDrawTracking(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return;
    
    size_t index = 1;
    const std::string& type = tokens[index];
    
    // Clear game history when setting new position
    m_gameHistory.clear();
    
    // Setup initial position
    if (!setupPosition(type, tokens, index)) {
        return;
    }
    
    // Add initial position to game history
    m_gameHistory.push_back(m_board.getZobristHash());
    
    // Apply moves if present
    if (index < tokens.size() && tokens[index] == "moves") {
        index++;
        std::vector<std::string> moveStrings(tokens.begin() + index, tokens.end());
        applyMovesWithHistory(moveStrings);
    }
}

// Modified applyMoves to track history
bool UCIEngine::applyMovesWithHistory(const std::vector<std::string>& moveStrings) {
    for (const auto& moveStr : moveStrings) {
        Move move = parseUCIMove(moveStr);
        if (move == Move()) {
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
            return false;
        }
        
        // Apply the move
        Board::UndoInfo undo;
        m_board.makeMove(move, undo);
        
        // Add new position to game history
        m_gameHistory.push_back(m_board.getZobristHash());
    }
    
    return true;
}

// Enhanced handleGo with draw detection
void UCIEngine::handleGoWithDrawDetection(const std::vector<std::string>& tokens) {
    // Check for immediate draw at root position
    DrawInfo drawInfo = checkDrawAtRoot();
    
    if (drawInfo.isDraw) {
        // Report the draw
        reportDraw(drawInfo);
        
        // Still need to return a move for GUI compatibility
        MoveList legalMoves;
        MoveGenerator::generateLegalMoves(m_board, legalMoves);
        
        if (legalMoves.size() > 0) {
            // Send minimal search info
            std::cout << "info depth 1 score cp 0 nodes 1 pv " 
                     << moveToUCI(legalMoves[0]) << std::endl;
            sendBestMove(legalMoves[0]);
        } else {
            // No legal moves (shouldn't happen if draw by repetition/50-move)
            sendBestMove(Move());
        }
        return;
    }
    
    // Normal search
    SearchParams params = parseGoCommand(tokens);
    searchWithDrawAwareness(params);
}

// Draw detection at root
UCIEngine::DrawInfo UCIEngine::checkDrawAtRoot() {
    DrawInfo info;
    info.isDraw = false;
    
    // Check stalemate first (most specific)
    if (isStalemate()) {
        info.isDraw = true;
        info.type = DRAW_STALEMATE;
        return info;
    }
    
    // Check insufficient material
    if (m_board.isInsufficientMaterial()) {
        info.isDraw = true;
        info.type = DRAW_INSUFFICIENT_MATERIAL;
        return info;
    }
    
    // Check fifty-move rule
    if (m_board.getHalfmoveClock() >= 100) {
        info.isDraw = true;
        info.type = DRAW_FIFTY_MOVE;
        return info;
    }
    
    // Check threefold repetition in game history
    uint64_t currentHash = m_board.getZobristHash();
    int repetitions = 0;
    for (const auto& hash : m_gameHistory) {
        if (hash == currentHash) {
            repetitions++;
            if (repetitions >= 3) {
                info.isDraw = true;
                info.type = DRAW_REPETITION;
                return info;
            }
        }
    }
    
    return info;
}

// Report draw to GUI
void UCIEngine::reportDraw(const DrawInfo& drawInfo) {
    switch (drawInfo.type) {
        case DRAW_REPETITION:
            std::cout << "info string Draw by threefold repetition detected" << std::endl;
            break;
        case DRAW_FIFTY_MOVE:
            std::cout << "info string Draw by fifty-move rule (halfmove clock: " 
                     << m_board.getHalfmoveClock() << ")" << std::endl;
            break;
        case DRAW_INSUFFICIENT_MATERIAL:
            std::cout << "info string Draw by insufficient material" << std::endl;
            break;
        case DRAW_STALEMATE:
            std::cout << "info string Draw by stalemate" << std::endl;
            break;
    }
}

// Modified search to handle draws during search
void UCIEngine::searchWithDrawAwareness(const SearchParams& params) {
    search::SearchLimits limits;
    
    // Set up limits as before
    if (params.depth > 0) {
        limits.maxDepth = params.depth;
    } else {
        limits.maxDepth = 64;
    }
    
    if (params.movetime > 0) {
        limits.movetime = std::chrono::milliseconds(params.movetime);
    } else if (params.wtime > 0 || params.btime > 0) {
        limits.time[WHITE] = std::chrono::milliseconds(params.wtime);
        limits.time[BLACK] = std::chrono::milliseconds(params.btime);
        limits.inc[WHITE] = std::chrono::milliseconds(params.winc);
        limits.inc[BLACK] = std::chrono::milliseconds(params.binc);
    }
    
    limits.infinite = params.infinite;
    
    // Pass game history to search for repetition detection
    limits.gameHistory = m_gameHistory;
    
    // Run search with draw awareness
    Move bestMove = search::searchWithDrawDetection(m_board, limits);
    
    // Send best move
    if (bestMove != Move()) {
        sendBestMove(bestMove);
    } else {
        // Check if it's checkmate or stalemate
        if (isCheckmate()) {
            // Score already reported by search
            sendBestMove(Move());
        } else if (isStalemate()) {
            std::cout << "info string Stalemate - no legal moves" << std::endl;
            std::cout << "info depth 1 score cp 0" << std::endl;
            sendBestMove(Move());
        } else {
            sendBestMove(Move());
        }
    }
}

// Handle ucinewgame command
void UCIEngine::handleUCINewGame() {
    // Clear all game history
    m_gameHistory.clear();
    
    // Reset board to starting position
    m_board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    // Add starting position to history
    m_gameHistory.push_back(m_board.getZobristHash());
    
    // Clear any search tables if needed
    // TranspositionTable::clear();  // For future phases

    // Phase 5.2: Clear attack cache on new game
    t_attackCache.clear();
    t_attackCacheHits = 0;
    t_attackCacheMisses = 0;
    t_attackCacheStores = 0;
    
    std::cout << "info string New game started" << std::endl;
}

// Helper function for checking stalemate
bool UCIEngine::isStalemate() {
    // Not in check
    if (m_board.isInCheck()) {
        return false;
    }
    
    // No legal moves
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(m_board, legalMoves);
    return legalMoves.size() == 0;
}

// Helper function for checking checkmate
bool UCIEngine::isCheckmate() {
    // In check
    if (!m_board.isInCheck()) {
        return false;
    }
    
    // No legal moves
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(m_board, legalMoves);
    return legalMoves.size() == 0;
}

// Example of how search should report draws in PV
void reportDrawInSearch(int depth, int nodes, const std::string& pv, DrawType drawType) {
    std::ostringstream oss;
    oss << "info depth " << depth 
        << " score cp 0"  // Draw score is always 0
        << " nodes " << nodes;
    
    if (!pv.empty()) {
        oss << " pv " << pv;
    }
    
    // Add draw type as info string
    switch (drawType) {
        case DRAW_REPETITION:
            oss << " string Draw by repetition in search";
            break;
        case DRAW_FIFTY_MOVE:
            oss << " string Draw by fifty-move rule in search";
            break;
        case DRAW_INSUFFICIENT_MATERIAL:
            oss << " string Draw by insufficient material";
            break;
        default:
            break;
    }
    
    std::cout << oss.str() << std::endl;
}

// GUI Compatibility Notes:
// 
// 1. Arena Chess GUI:
//    - Expects "info string" for draw notifications
//    - Shows these in engine output window
//    - Recognizes score cp 0 as draw
//
// 2. CuteChess:
//    - Very strict UCI compliance
//    - Doesn't require info strings but displays them
//    - Uses score for adjudication
//
// 3. Banksia GUI:
//    - Modern GUI with good draw handling
//    - Shows info strings prominently
//    - Can adjudicate based on draw detection
//
// 4. ChessBase/Fritz:
//    - Commercial GUIs expect standard UCI
//    - May not show all info strings
//    - Rely on score cp 0 for draw indication

// Tournament Adjudication:
// Most tournament managers (cutechess-cli, c-chess-cli) will:
// 1. Adjudicate draw when both engines report score 0 for several moves
// 2. Detect repetition independently for adjudication
// 3. Apply fifty-move rule automatically
// 4. Recognize insufficient material

// Testing commands for manual verification:
/*
position startpos moves e2e4 e7e5 Ng1f3 Ng8f6 Nf3g1 Nf6g8 Ng1f3 Ng8f6 Nf3g1 Nf6g8
go depth 10
// Should detect threefold repetition

position fen "8/8/8/4k3/8/8/3K4/8 w - - 99 50" moves Kd2d3
go depth 10  
// Should detect fifty-move rule

position fen "8/8/8/4k3/8/8/3K4/8 w - - 0 1"
go depth 10
// Should detect insufficient material
*/