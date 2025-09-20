#include <iostream>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/search/ranked_move_picker.h"

using namespace seajay;

int main() {
    Board board;
    board.setFromFEN("r1bqkb1r/1ppp1ppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 5");
    
    // Test 1: Generate pseudo-legal moves directly
    MoveList moves1;
    MoveGenerator::generatePseudoLegalMoves(board, moves1);
    std::cout << "Direct pseudo-legal moves: " << moves1.size() << std::endl;
    
    // Test 2: Use RankedMovePicker
    search::RankedMovePicker picker(board, NO_MOVE, nullptr, nullptr, nullptr, nullptr, NO_MOVE, 1, 5);
    int count = 0;
    while (picker.next() != NO_MOVE) {
        count++;
    }
    std::cout << "RankedMovePicker moves: " << count << std::endl;
    
    return 0;
}
