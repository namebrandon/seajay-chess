#include <iostream>
#include <iomanip>
#include <vector>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/move_list.h"
#include "src/evaluation/pawn_structure.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;

void simulateGame() {
    Board board;
    board.setStartingPosition();
    
    std::cout << "Simulating Game with Pawn Hash Caching\n";
    std::cout << "=======================================\n\n";
    
    // Clear cache and counters
    g_pawnStructure.clear();
#ifdef DEBUG
    g_pawnStructure.m_cacheHits = 0;
    g_pawnStructure.m_cacheMisses = 0;
#endif
    
    // Simulate a game with many evaluations
    std::vector<Move> gameMoves;
    int evalCount = 0;
    
    // Play 20 half-moves
    for (int ply = 0; ply < 20; ++ply) {
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        if (moves.size() == 0) break;
        
        // Evaluate each possible move (simulating search)
        for (size_t i = 0; i < moves.size(); ++i) {
            Board::UndoInfo undo;
            board.makeMove(moves[i], undo);
            
            eval::Score score = eval::evaluate(board);
            evalCount++;
            
            board.unmakeMove(moves[i], undo);
        }
        
        // Make the first legal move
        Board::UndoInfo undo;
        board.makeMove(moves[0], undo);
        gameMoves.push_back(moves[0]);
        
        // Evaluate after the move
        eval::Score score = eval::evaluate(board);
        evalCount++;
    }
    
    std::cout << "Game simulation complete:\n";
    std::cout << "  Moves played: " << gameMoves.size() << "\n";
    std::cout << "  Evaluations:  " << evalCount << "\n\n";
    
#ifdef DEBUG
    size_t totalProbes = g_pawnStructure.m_cacheHits + g_pawnStructure.m_cacheMisses;
    double hitRate = totalProbes > 0 ? (100.0 * g_pawnStructure.m_cacheHits / totalProbes) : 0.0;
    
    std::cout << "Pawn Hash Statistics:\n";
    std::cout << "  Cache hits:   " << std::setw(8) << g_pawnStructure.m_cacheHits << "\n";
    std::cout << "  Cache misses: " << std::setw(8) << g_pawnStructure.m_cacheMisses << "\n";
    std::cout << "  Total probes: " << std::setw(8) << totalProbes << "\n";
    std::cout << "  Hit rate:     " << std::fixed << std::setprecision(2) << hitRate << "%\n\n";
    
    if (hitRate > 85.0) {
        std::cout << "✓ EXCELLENT: Cache hit rate > 85% indicates pawn hash is working efficiently!\n";
        std::cout << "  Most non-pawn moves are reusing cached pawn structure evaluation.\n";
    } else if (hitRate > 70.0) {
        std::cout << "✓ GOOD: Cache hit rate > 70% shows pawn hash is helping significantly.\n";
    } else if (hitRate > 50.0) {
        std::cout << "⚠ WARNING: Cache hit rate " << hitRate << "% is lower than expected.\n";
        std::cout << "  This might indicate the pawn hash is not being used effectively.\n";
    } else {
        std::cout << "✗ ERROR: Cache hit rate " << hitRate << "% suggests pawn hash is not working!\n";
        std::cout << "  With proper pawn-only hashing, we should see >85% hit rate.\n";
    }
    
    // Calculate average hits per position
    double avgHitsPerPosition = gameMoves.size() > 0 ? 
        (double)g_pawnStructure.m_cacheHits / gameMoves.size() : 0.0;
    std::cout << "\nAverage cache hits per position: " << std::fixed << std::setprecision(1) 
              << avgHitsPerPosition << "\n";
    
    if (avgHitsPerPosition > 20.0) {
        std::cout << "✓ Each position benefits from ~" << (int)avgHitsPerPosition 
                  << " cache hits (excellent reuse)\n";
    }
#else
    std::cout << "Build in Debug mode to see cache statistics.\n";
    std::cout << "Use: cmake -DCMAKE_BUILD_TYPE=Debug ..\n";
#endif
}

int main() {
    simulateGame();
    return 0;
}