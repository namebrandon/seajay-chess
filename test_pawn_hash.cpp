#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/move_list.h"
#include "src/evaluation/pawn_structure.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;

void testPawnHashCaching() {
    Board board;
    board.setStartingPosition();
    
    std::cout << "Testing Pawn Hash Caching\n";
    std::cout << "=========================\n\n";
    
    // Test 1: Pawn moves should change pawn hash
    uint64_t initialPawnHash = board.pawnZobristKey();
    std::cout << "Initial pawn hash: 0x" << std::hex << initialPawnHash << std::dec << "\n";
    
    // Make a pawn move
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    Move e2e4 = NO_MOVE;
    for (size_t i = 0; i < moves.size(); ++i) {
        if (moveFrom(moves[i]) == E2 && moveTo(moves[i]) == E4) {
            e2e4 = moves[i];
            break;
        }
    }
    
    if (e2e4 != NO_MOVE) {
        Board::UndoInfo undo;
        board.makeMove(e2e4, undo);
        uint64_t afterPawnMove = board.pawnZobristKey();
        std::cout << "After e2-e4: 0x" << std::hex << afterPawnMove << std::dec << "\n";
        std::cout << "Changed: " << (afterPawnMove != initialPawnHash ? "YES (correct)" : "NO (BUG!)") << "\n\n";
        board.unmakeMove(e2e4, undo);
    }
    
    // Test 2: Non-pawn moves should NOT change pawn hash
    Move g1f3 = NO_MOVE;
    MoveGenerator::generateLegalMoves(board, moves);
    for (size_t i = 0; i < moves.size(); ++i) {
        if (moveFrom(moves[i]) == G1 && moveTo(moves[i]) == F3) {
            g1f3 = moves[i];
            break;
        }
    }
    
    if (g1f3 != NO_MOVE) {
        Board::UndoInfo undo;
        board.makeMove(g1f3, undo);
        uint64_t afterKnightMove = board.pawnZobristKey();
        std::cout << "After Ng1-f3: 0x" << std::hex << afterKnightMove << std::dec << "\n";
        std::cout << "Changed: " << (afterKnightMove != initialPawnHash ? "NO (BUG!)" : "YES (correct)") << "\n\n";
        board.unmakeMove(g1f3, undo);
    }
    
    // Test 3: Cache hit rate test
    std::cout << "Testing cache hit rate:\n";
    std::cout << "-----------------------\n";
    
    // Clear cache
    g_pawnStructure.clear();
    
    // First evaluation - should be cache miss
    eval::Score eval1 = eval::evaluate(board);
    
    // Second evaluation - should be cache hit
    eval::Score eval2 = eval::evaluate(board);
    
    // Make non-pawn move
    if (g1f3 != NO_MOVE) {
        Board::UndoInfo undo;
        board.makeMove(g1f3, undo);
        
        // This should still be a cache hit (pawn structure unchanged)
        eval::Score eval3 = eval::evaluate(board);
        
        // Make pawn move
        MoveGenerator::generateLegalMoves(board, moves);
        Move e7e5 = NO_MOVE;
        for (size_t i = 0; i < moves.size(); ++i) {
            if (moveFrom(moves[i]) == E7 && moveTo(moves[i]) == E5) {
                e7e5 = moves[i];
                break;
            }
        }
        
        if (e7e5 != NO_MOVE) {
            Board::UndoInfo undo2;
            board.makeMove(e7e5, undo2);
            
            // This should be a cache miss (pawn structure changed)
            eval::Score eval4 = eval::evaluate(board);
            
            std::cout << "Eval 1 (initial): " << eval1.value() << " (expected: cache miss)\n";
            std::cout << "Eval 2 (repeat):  " << eval2.value() << " (expected: cache hit)\n";
            std::cout << "Eval 3 (after Nf3): " << eval3.value() << " (expected: cache hit)\n";
            std::cout << "Eval 4 (after e7e5): " << eval4.value() << " (expected: cache miss)\n";
            
            board.unmakeMove(e7e5, undo2);
        }
        
        board.unmakeMove(g1f3, undo);
    }
    
    std::cout << "\nNote: Cannot directly measure cache hits without instrumentation.\n";
    std::cout << "But if pawn hash is working, non-pawn moves should reuse cached values.\n";
}

int main() {
    testPawnHashCaching();
    return 0;
}