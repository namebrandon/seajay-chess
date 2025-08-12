/**
 * Magic Bitboards Game Playing Validation
 * Stage 10 - Phase 4C: Game Playing Validation
 * 
 * This test plays self-play games to validate:
 * 1. No illegal moves are generated
 * 2. No crashes during gameplay
 * 3. Game outcomes are reasonable
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <random>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/search/search.h"
#include "../src/core/magic_bitboards.h"

using namespace seajay;

struct GameResult {
    enum Outcome { WHITE_WIN, BLACK_WIN, DRAW, ERROR };
    Outcome outcome;
    int moves;
    std::string finalPosition;
    std::string error;
};

/**
 * Play a single game with random move selection for quick validation
 */
GameResult playRandomGame(int maxMoves = 200) {
    Board board;
    board.initialize();
    
    GameResult result;
    result.moves = 0;
    
    // Play up to maxMoves
    for (int moveNum = 0; moveNum < maxMoves; ++moveNum) {
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        // Check for game end
        if (moves.empty()) {
            if (board.isInCheck()) {
                // Checkmate
                result.outcome = (board.getSideToMove() == Color::WHITE) ? 
                    GameResult::BLACK_WIN : GameResult::WHITE_WIN;
            } else {
                // Stalemate
                result.outcome = GameResult::DRAW;
            }
            result.finalPosition = board.toFEN();
            result.moves = moveNum;
            return result;
        }
        
        // Check for draw conditions
        if (board.isDrawByRepetition() || board.isDrawByFiftyMoves() || 
            board.isInsufficientMaterial()) {
            result.outcome = GameResult::DRAW;
            result.finalPosition = board.toFEN();
            result.moves = moveNum;
            return result;
        }
        
        // Select a random legal move
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
        Move selectedMove = moves[dist(rng)];
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(selectedMove, undo);
        
        // Basic sanity check - king should not be capturable
        MoveList responses;
        MoveGenerator::generateLegalMoves(board, responses);
        bool kingCapturable = false;
        for (Move response : responses) {
            if (board.getPieceAt(getTo(response)) == Piece::KING ||
                board.getPieceAt(getTo(response)) == Piece::BLACK_KING) {
                kingCapturable = true;
                break;
            }
        }
        
        if (kingCapturable) {
            result.outcome = GameResult::ERROR;
            result.error = "King can be captured after move!";
            result.finalPosition = board.toFEN();
            result.moves = moveNum;
            return result;
        }
    }
    
    // Game didn't end within move limit - call it a draw
    result.outcome = GameResult::DRAW;
    result.finalPosition = board.toFEN();
    result.moves = maxMoves;
    return result;
}

/**
 * Play a game using the engine's search
 */
GameResult playEngineGame(int depthLimit = 4, int maxMoves = 200) {
    Board board;
    board.initialize();
    
    Search search;
    GameResult result;
    result.moves = 0;
    
    for (int moveNum = 0; moveNum < maxMoves; ++moveNum) {
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        // Check for game end
        if (moves.empty()) {
            if (board.isInCheck()) {
                result.outcome = (board.getSideToMove() == Color::WHITE) ? 
                    GameResult::BLACK_WIN : GameResult::WHITE_WIN;
            } else {
                result.outcome = GameResult::DRAW;
            }
            result.finalPosition = board.toFEN();
            result.moves = moveNum;
            return result;
        }
        
        // Check for draw conditions
        if (board.isDrawByRepetition() || board.isDrawByFiftyMoves() || 
            board.isInsufficientMaterial()) {
            result.outcome = GameResult::DRAW;
            result.finalPosition = board.toFEN();
            result.moves = moveNum;
            return result;
        }
        
        // Use engine to find best move
        SearchInfo info;
        info.depth = depthLimit;
        Move bestMove = search.findBestMove(board, info);
        
        if (bestMove == MOVE_NONE) {
            result.outcome = GameResult::ERROR;
            result.error = "Engine returned MOVE_NONE";
            result.finalPosition = board.toFEN();
            result.moves = moveNum;
            return result;
        }
        
        // Validate the move is legal
        bool isLegal = false;
        for (Move m : moves) {
            if (m == bestMove) {
                isLegal = true;
                break;
            }
        }
        
        if (!isLegal) {
            result.outcome = GameResult::ERROR;
            result.error = "Engine returned illegal move";
            result.finalPosition = board.toFEN();
            result.moves = moveNum;
            return result;
        }
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(bestMove, undo);
    }
    
    // Game didn't end within move limit
    result.outcome = GameResult::DRAW;
    result.finalPosition = board.toFEN();
    result.moves = maxMoves;
    return result;
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "   Magic Bitboards Game Playing Test     " << std::endl;
    std::cout << "        Stage 10 - Phase 4C              " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;
    
    // Initialize magic bitboards
    magic::initMagics();
    
    // Statistics
    int totalGames = 0;
    int whiteWins = 0;
    int blackWins = 0;
    int draws = 0;
    int errors = 0;
    int totalMoves = 0;
    
    // Play random games for quick validation
    std::cout << "Playing 20 random games for quick validation..." << std::endl;
    for (int i = 0; i < 20; ++i) {
        GameResult result = playRandomGame();
        totalGames++;
        totalMoves += result.moves;
        
        switch (result.outcome) {
            case GameResult::WHITE_WIN:
                whiteWins++;
                std::cout << "Game " << (i+1) << ": White wins in " << result.moves << " moves" << std::endl;
                break;
            case GameResult::BLACK_WIN:
                blackWins++;
                std::cout << "Game " << (i+1) << ": Black wins in " << result.moves << " moves" << std::endl;
                break;
            case GameResult::DRAW:
                draws++;
                std::cout << "Game " << (i+1) << ": Draw after " << result.moves << " moves" << std::endl;
                break;
            case GameResult::ERROR:
                errors++;
                std::cout << "Game " << (i+1) << ": ERROR - " << result.error << std::endl;
                std::cout << "  Final position: " << result.finalPosition << std::endl;
                break;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Random Games Summary:" << std::endl;
    std::cout << "  White wins: " << whiteWins << std::endl;
    std::cout << "  Black wins: " << blackWins << std::endl;
    std::cout << "  Draws:      " << draws << std::endl;
    std::cout << "  Errors:     " << errors << std::endl;
    std::cout << "  Avg moves:  " << (totalMoves / 20.0) << std::endl;
    std::cout << std::endl;
    
    // Play engine vs engine games
    std::cout << "Playing 10 engine self-play games (depth 3)..." << std::endl;
    
    int engineWhiteWins = 0;
    int engineBlackWins = 0;
    int engineDraws = 0;
    int engineErrors = 0;
    int engineTotalMoves = 0;
    
    auto startTime = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        std::cout << "Game " << (i+1) << ": ";
        std::cout.flush();
        
        GameResult result = playEngineGame(3, 150);  // Depth 3, max 150 moves
        engineTotalMoves += result.moves;
        
        switch (result.outcome) {
            case GameResult::WHITE_WIN:
                engineWhiteWins++;
                std::cout << "White wins in " << result.moves << " moves" << std::endl;
                break;
            case GameResult::BLACK_WIN:
                engineBlackWins++;
                std::cout << "Black wins in " << result.moves << " moves" << std::endl;
                break;
            case GameResult::DRAW:
                engineDraws++;
                std::cout << "Draw after " << result.moves << " moves" << std::endl;
                break;
            case GameResult::ERROR:
                engineErrors++;
                std::cout << "ERROR - " << result.error << std::endl;
                break;
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
    
    std::cout << std::endl;
    std::cout << "Engine Self-Play Summary:" << std::endl;
    std::cout << "  White wins: " << engineWhiteWins << std::endl;
    std::cout << "  Black wins: " << engineBlackWins << std::endl;
    std::cout << "  Draws:      " << engineDraws << std::endl;
    std::cout << "  Errors:     " << engineErrors << std::endl;
    std::cout << "  Avg moves:  " << (engineTotalMoves / 10.0) << std::endl;
    std::cout << "  Total time: " << elapsed.count() << " seconds" << std::endl;
    std::cout << std::endl;
    
    // Final validation
    std::cout << "==========================================" << std::endl;
    if (errors == 0 && engineErrors == 0) {
        std::cout << "✅ GAME PLAYING VALIDATION PASSED" << std::endl;
        std::cout << "   No illegal moves or crashes detected" << std::endl;
    } else {
        std::cout << "❌ VALIDATION FAILED" << std::endl;
        std::cout << "   Errors detected during gameplay" << std::endl;
    }
    std::cout << "==========================================" << std::endl;
    
    return (errors == 0 && engineErrors == 0) ? 0 : 1;
}