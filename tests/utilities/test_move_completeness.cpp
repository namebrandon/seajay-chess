#include <iostream>
#include <set>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/search/ranked_move_picker.h"

using namespace seajay;

int main() {
    Board board;
    // Position after: e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6
    board.setFromFEN("r1bqkb1r/1ppp1ppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 5");
    
    // Test 1: Generate pseudo-legal moves directly
    MoveList directMoves;
    MoveGenerator::generatePseudoLegalMoves(board, directMoves);
    std::cout << "Direct pseudo-legal moves: " << directMoves.size() << std::endl;
    
    // Collect all moves from direct generation
    std::set<Move> directSet;
    for (size_t i = 0; i < directMoves.size(); ++i) {
        directSet.insert(directMoves[i]);
    }
    
    // Test 2: Get all moves from RankedMovePicker
    search::RankedMovePicker picker(board, NO_MOVE, nullptr, nullptr, nullptr, nullptr, NO_MOVE, 1, 5);
    std::set<Move> pickerSet;
    Move move;
    int count = 0;
    while ((move = picker.next()) != NO_MOVE) {
        pickerSet.insert(move);
        count++;
        if (count > 100) {
            std::cout << "ERROR: Too many moves from picker!" << std::endl;
            break;
        }
    }
    std::cout << "RankedMovePicker moves: " << count << std::endl;
    
    // Compare the sets
    if (directSet != pickerSet) {
        std::cout << "ERROR: Move sets don't match!" << std::endl;
        
        // Find missing moves
        for (Move m : directSet) {
            if (pickerSet.find(m) == pickerSet.end()) {
                std::cout << "  Missing from picker: " << std::hex << m << std::dec << std::endl;
            }
        }
        
        // Find extra moves
        for (Move m : pickerSet) {
            if (directSet.find(m) == directSet.end()) {
                std::cout << "  Extra in picker: " << std::hex << m << std::dec << std::endl;
            }
        }
    } else {
        std::cout << "SUCCESS: Both methods yield the same moves!" << std::endl;
    }
    
    return 0;
}
