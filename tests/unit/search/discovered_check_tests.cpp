#include "../../../src/core/board.h"
#include "../../../src/core/types.h"
#include "../../../src/core/move_generation.h"
#include "../../../src/search/discovered_check.h"

#include <cassert>
#include <iostream>

using namespace seajay;
using namespace seajay::search;

void testWAC237DiscoveredDoubleCheck() {
    Board board;
    const bool parsed = board.fromFEN("r5k1/pQp2qpp/8/4pbN1/3P4/6P1/PPr4P/1K1R3R b - - 0 1");
    assert(parsed && "Failed to parse WAC.237 FEN");

    const Square fromSq = stringToSquare("c2");
    const Square toSq = stringToSquare("c1");
    const Move rc1 = makeMove(fromSq, toSq);

    assert(isDiscoveredCheck(board, rc1) && "Rc1+ should register as discovered check");
    assert(isDoubleCheckAfterMove(board, rc1) && "Rc1+ should be a discovered double check");

    std::cout << "✓ WAC.237 discovered double check detected" << std::endl;
}

void testQuietDiscoveredCheckWithoutDouble() {
    Board board;
    board.clear();
    board.setSideToMove(BLACK);

    const Square kingSq = stringToSquare("e1");
    const Square queenSq = stringToSquare("e5");
    const Square bishopSq = stringToSquare("e4");
    const Square bishopTarget = stringToSquare("g2");

    board.setPiece(kingSq, WHITE_KING);
    board.setPiece(queenSq, BLACK_QUEEN);
    board.setPiece(bishopSq, BLACK_BISHOP);

    const Move bishopMove = makeMove(bishopSq, bishopTarget);

    assert(isDiscoveredCheck(board, bishopMove) &&
           "Moving the e4 bishop should uncover the queen's check on e1");
    assert(!isDoubleCheckAfterMove(board, bishopMove) &&
           "Bishop move should not be flagged as a double check");

    std::cout << "✓ Quiet discovered check recognised without double check" << std::endl;
}

void testNonDiscoveredMove() {
    Board board;
    board.clear();
    board.setSideToMove(WHITE);

    const Square kingSq = stringToSquare("e8");
    const Square rookSq = stringToSquare("a8");
    const Square knightSq = stringToSquare("b1");
    const Square knightTarget = stringToSquare("c3");

    board.setPiece(kingSq, BLACK_KING);
    board.setPiece(rookSq, BLACK_ROOK);
    board.setPiece(knightSq, WHITE_KNIGHT);

    const Move knightMove = makeMove(knightSq, knightTarget);

    assert(!isDiscoveredCheck(board, knightMove));
    assert(!isDoubleCheckAfterMove(board, knightMove));

    std::cout << "✓ Non-discovered move correctly ignored" << std::endl;
}

int main() {
    std::cout << "\n=== Discovered Check Detection Tests ===\n" << std::endl;

    MoveGenerator::initializeAttackTables();

    testWAC237DiscoveredDoubleCheck();
    testQuietDiscoveredCheckWithoutDouble();
    testNonDiscoveredMove();

    std::cout << "\n✅ All discovered check tests passed\n" << std::endl;
    return 0;
}
