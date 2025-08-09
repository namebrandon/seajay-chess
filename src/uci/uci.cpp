#include "uci.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <thread>

using namespace seajay;

UCIEngine::UCIEngine() : m_quit(false) {
    // Initialize board to starting position
    m_board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
        // Ignore unknown commands (UCI protocol requirement)
    }
}

void UCIEngine::handleUCI() {
    std::cout << "id name SeaJay 1.0" << std::endl;
    std::cout << "id author Brandon Harris" << std::endl;
    // No options to report for Stage 3
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
        return m_board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
        
        return m_board.fromFEN(fen);
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
    
    for (size_t i = 1; i < tokens.size(); i += 2) {
        if (i + 1 >= tokens.size()) break; // Need parameter value
        
        const std::string& param = tokens[i];
        int value = std::stoi(tokens[i + 1]);
        
        if (param == "movetime") {
            params.movetime = value;
        }
        else if (param == "wtime") {
            params.wtime = value;
        }
        else if (param == "btime") {
            params.btime = value;
        }
        else if (param == "winc") {
            params.winc = value;
        }
        else if (param == "binc") {
            params.binc = value;
        }
        else if (param == "depth") {
            params.depth = value;
        }
        else if (param == "infinite") {
            params.infinite = true;
            i--; // infinite has no parameter
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
    auto startTime = std::chrono::steady_clock::now();
    
    // For Stage 3: simple random move selection
    Move bestMove = selectRandomMove();
    
    // Calculate actual search time
    auto endTime = std::chrono::steady_clock::now();
    int64_t searchTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    
    // Generate search info
    SearchInfo info;
    updateSearchInfo(info, bestMove, searchTimeMs);
    
    // Send info and best move
    if (bestMove != Move()) {
        std::cout << "info depth " << info.depth 
                  << " nodes " << info.nodes
                  << " time " << info.timeMs
                  << " pv " << info.pv << std::endl;
        
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
    
    // Simple random selection
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, legalMoves.size() - 1);
    
    return legalMoves[dist(gen)];
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