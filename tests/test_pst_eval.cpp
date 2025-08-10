#include <iostream>
#include "../src/core/board.h"
#include "../src/evaluation/evaluate.h"

using namespace seajay;
using namespace seajay::eval;

void testPosition(const std::string& fen, const std::string& description) {
    Board board;
    board.fromFEN(fen);
    
    Score eval = evaluate(board);
    MgEgScore pst = board.pstScore();
    
    std::cout << description << ":\n";
    std::cout << "  FEN: " << fen << "\n";
    std::cout << "  Evaluation: " << eval.value() << " cp\n";
    std::cout << "  PST Score (mg): " << pst.mg.value() << " cp\n";
    std::cout << "  Material: " << board.material().balance(board.sideToMove()).value() << " cp\n\n";
}

int main() {
    std::cout << "Testing PST evaluation integration...\n\n";
    
    // Test 1: Starting position should be equal
    testPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                 "Starting position");
    
    // Test 2: Position with central knight should be better
    testPosition("r1bqkb1r/pppp1ppp/2n2n2/4N3/4P3/8/PPPP1PPP/RNBQKB1R w KQkq -",
                 "White knight on e5 (good), black knights less centralized");
    
    // Test 3: Advanced pawn structure
    testPosition("8/8/4P3/8/8/4p3/8/8 w - -",
                 "White pawn on e6 vs black pawn on e3");
    
    // Test 4: After castling
    testPosition("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R4RK1 w kq -",
                 "White castled kingside, black king in center");
    
    // Test 5: Make a move and verify PST updates
    std::cout << "Testing incremental PST updates:\n";
    Board board;
    board.setStartingPosition();
    
    Score evalBefore = evaluate(board);
    MgEgScore pstBefore = board.pstScore();
    
    // Make e2-e4
    Board::UndoInfo undo;
    Move e2e4 = makeMove(E2, E4, DOUBLE_PAWN);
    board.makeMove(e2e4, undo);
    
    Score evalAfter = evaluate(board);
    MgEgScore pstAfter = board.pstScore();
    
    std::cout << "After e2-e4:\n";
    std::cout << "  Eval change: " << evalBefore.value() << " -> " << evalAfter.value() 
              << " (diff: " << (evalAfter.value() - evalBefore.value()) << ")\n";
    std::cout << "  PST change: " << pstBefore.mg.value() << " -> " << pstAfter.mg.value()
              << " (diff: " << (pstAfter.mg.value() - pstBefore.mg.value()) << ")\n";
    
    // The pawn advanced from e2 to e4, gaining positional value
    int expectedPSTChange = PST::rawValue(PAWN, E4).mg.value() - PST::rawValue(PAWN, E2).mg.value();
    std::cout << "  Expected PST change: " << expectedPSTChange << "\n";
    
    if ((pstAfter.mg.value() - pstBefore.mg.value()) != expectedPSTChange) {
        std::cerr << "ERROR: PST incremental update incorrect!\n";
        return 1;
    }
    
    // Unmake and verify
    board.unmakeMove(e2e4, undo);
    MgEgScore pstUnmade = board.pstScore();
    if (pstUnmade != pstBefore) {
        std::cerr << "ERROR: PST not restored after unmake!\n";
        return 1;
    }
    
    std::cout << "\nAll PST evaluation tests passed!\n";
    return 0;
}