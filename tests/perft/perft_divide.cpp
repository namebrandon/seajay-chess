#include <iostream>
#include <string>
#include <iomanip>
#include <map>
#include "board.h"
#include "move_generation.h"
#include "move_list.h"
#include "types.h"

int main() {
    std::string fen = "rnbqkb1r/pp1p1ppp/5n2/4p3/2PP4/8/PP2PPPP/RNBQKBNR w KQkq e6 0 4";
    
    seajay::Board board;
    board.fromFEN(fen);
    
    seajay::MoveList moves;
    seajay::MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Perft(2) divide for Position 6:\n";
    std::cout << "=================================\n\n";
    
    std::map<std::string, int> expectedCounts = {
        {"d4e5", 28},
        {"a2a3", 30},
        {"a2a4", 30},
        {"b2b3", 30},
        {"b2b4", 29},
        {"e2e3", 30},
        {"e2e4", 29},
        {"f2f3", 30},
        {"f2f4", 31},
        {"g2g3", 30},
        {"g2g4", 30},
        {"h2h3", 30},
        {"h2h4", 30},
        {"c4c5", 28},
        {"d4d5", 28},
        {"b1d2", 30},
        {"b1a3", 30},
        {"b1c3", 30},
        {"g1f3", 30},
        {"g1h3", 30},
        {"c1d2", 30},
        {"c1e3", 30},
        {"c1f4", 31},
        {"c1g5", 29},
        {"c1h6", 29},
        {"d1c2", 30},
        {"d1d2", 30},
        {"d1b3", 30},
        {"d1d3", 30},
        {"d1a4", 28},
        {"e1d2", 30}
    };
    
    int totalNodes = 0;
    int totalExpected = 0;
    
    for (size_t i = 0; i < moves.size(); ++i) {
        seajay::Move move = moves[i];
        
        // Make the move
        seajay::Board newBoard = board;
        seajay::Board::UndoInfo undo;
        newBoard.makeMove(move, undo);
        
        // Generate responses
        seajay::MoveList responses;
        seajay::MoveGenerator::generateLegalMoves(newBoard, responses);
        
        std::string moveStr = seajay::squareToString(seajay::from(move)) +
                              seajay::squareToString(seajay::to(move));
        
        int count = responses.size();
        totalNodes += count;
        
        auto it = expectedCounts.find(moveStr);
        int expected = (it != expectedCounts.end()) ? it->second : -1;
        totalExpected += (expected > 0) ? expected : 0;
        
        std::cout << std::left << std::setw(8) << moveStr << ": " 
                  << std::setw(3) << count;
        
        if (expected >= 0) {
            if (count != expected) {
                std::cout << " ❌ (expected " << expected << ", diff: " 
                          << (count - expected) << ")";
            } else {
                std::cout << " ✓";
            }
        } else {
            std::cout << " [not in expected list]";
        }
        
        std::cout << "\n";
    }
    
    std::cout << "\n----------------------------\n";
    std::cout << "Total: " << totalNodes << " (expected: " << totalExpected << ")\n";
    
    if (totalNodes != 824) {
        std::cout << "\n⚠️ WRONG TOTAL! Expected 824, got " << totalNodes << "\n";
        std::cout << "Difference: " << (totalNodes - 824) << "\n";
    }
    
    return 0;
}