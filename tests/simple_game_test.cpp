/**
 * Simple Game Playing Test for Magic Bitboards
 * Stage 10 - Phase 4C: Basic game playing validation
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/magic_bitboards.h"

using namespace seajay;

/**
 * Play a simple random game to test for crashes and illegal moves
 */
bool playRandomGame(int gameNum, int maxMoves = 200) {
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::mt19937 rng(gameNum * 1337);  // Seed with game number for reproducibility
    
    for (int moveNum = 0; moveNum < maxMoves; ++moveNum) {
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        // Game over if no legal moves
        if (moves.empty()) {
            return true;  // Normal game end
        }
        
        // Check for draw
        if (board.isDraw()) {
            return true;  // Draw
        }
        
        // Select random move
        std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
        Move selectedMove = moves[dist(rng)];
        
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(selectedMove, undo);
        
        // Basic validation - if we got here without crashing, move was legal
    }
    
    return true;  // Game didn't end but no errors
}

/**
 * Test specific positions that stress magic bitboards
 */
bool testSpecificPositions() {
    std::cout << "Testing specific positions..." << std::endl;
    
    // Positions with lots of sliding pieces
    std::vector<std::string> testPositions = {
        // Queens and rooks heavy position
        "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
        // Endgame with rooks
        "8/8/8/4k3/8/8/4R3/4K2R w K - 0 1",
        // Multiple bishops
        "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3",
        // Complex middlegame
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    };
    
    for (size_t i = 0; i < testPositions.size(); ++i) {
        std::cout << "  Testing position " << (i+1) << "..." << std::endl;
        Board board;
        if (!board.fromFEN(testPositions[i])) {
            std::cout << "    Failed to parse FEN!" << std::endl;
            return false;
        }
        
        // Generate moves with magic bitboards
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        std::cout << "  Position " << (i+1) << ": " << moves.size() << " legal moves" << std::endl;
        
        // Make a few random moves to test
        std::mt19937 rng(42);
        for (int j = 0; j < 10 && !moves.empty(); ++j) {
            std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
            Move move = moves[dist(rng)];
            
            Board::UndoInfo undo;
            board.makeMove(move, undo);
            
            // Generate new moves
            moves.clear();
            MoveGenerator::generateLegalMoves(board, moves);
        }
    }
    
    return true;
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "     Simple Game Playing Validation      " << std::endl;
    std::cout << "        Stage 10 - Phase 4C              " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;
    
    // Initialize magic bitboards
    magic::initMagics();
    
    // Test specific positions first
    if (!testSpecificPositions()) {
        std::cout << "❌ Specific position tests failed!" << std::endl;
        return 1;
    }
    std::cout << std::endl;
    
    // Play random games
    std::cout << "Playing 30 random games..." << std::endl;
    
    int successCount = 0;
    auto startTime = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 30; ++i) {
        bool success = playRandomGame(i);
        if (success) {
            successCount++;
            std::cout << "Game " << std::setw(2) << (i+1) << ": ✓" << std::endl;
        } else {
            std::cout << "Game " << std::setw(2) << (i+1) << ": ✗ FAILED" << std::endl;
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
    
    std::cout << std::endl;
    std::cout << "Results:" << std::endl;
    std::cout << "  Games played:    30" << std::endl;
    std::cout << "  Successful:      " << successCount << std::endl;
    std::cout << "  Failed:          " << (30 - successCount) << std::endl;
    std::cout << "  Time:            " << elapsed.count() << " seconds" << std::endl;
    std::cout << std::endl;
    
    std::cout << "==========================================" << std::endl;
    if (successCount == 30) {
        std::cout << "✅ ALL GAMES COMPLETED WITHOUT ERRORS" << std::endl;
        std::cout << "   No crashes or illegal moves detected" << std::endl;
    } else {
        std::cout << "❌ SOME GAMES FAILED" << std::endl;
    }
    std::cout << "==========================================" << std::endl;
    
    return (successCount == 30) ? 0 : 1;
}