#include <iostream>
#include "../src/core/board.h"
#include "../src/core/move_generator.h"
#include "../src/core/see.h"
#include "../src/search/move_ordering.h"

using namespace seajay;

int main() {
    // Italian Game position where Bxf7+ wins a pawn
    std::string fen = "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq -";
    
    Board board;
    board.setFromFEN(fen);
    
    std::cout << "Position: " << fen << "\n\n";
    
    // Generate all moves
    MoveList moves;
    MoveGenerator gen(board);
    gen.generateAllMoves(moves);
    
    std::cout << "Total moves: " << moves.size() << "\n\n";
    
    // Find Bxf7+ move
    Move bxf7 = NULL_MOVE;
    for (Move move : moves) {
        Square from = moveFrom(move);
        Square to = moveTo(move);
        if (from == C4 && to == F7) {
            bxf7 = move;
            break;
        }
    }
    
    if (bxf7 == NULL_MOVE) {
        std::cout << "ERROR: Bxf7+ not found in move list!\n";
        return 1;
    }
    
    std::cout << "Found Bxf7+ move\n";
    
    // Calculate SEE value
    SEECalculator see;
    SEEValue seeValue = see.see(board, bxf7);
    
    std::cout << "SEE value for Bxf7+: " << seeValue << "\n";
    std::cout << "Expected: " << PAWN_VALUE << " (winning a pawn)\n\n";
    
    // Check MVV-LVA score
    int mvvLvaScore = search::MvvLvaOrdering::scoreMove(board, bxf7);
    std::cout << "MVV-LVA score for Bxf7+: " << mvvLvaScore << "\n\n";
    
    // Order moves with SEE
    search::SEEMoveOrdering seeOrdering;
    seeOrdering.setMode(search::SEEMode::PRODUCTION);
    seeOrdering.orderMoves(board, moves);
    
    std::cout << "Top 10 moves after SEE ordering:\n";
    for (int i = 0; i < std::min(10, (int)moves.size()); i++) {
        Move move = moves[i];
        Square from = moveFrom(move);
        Square to = moveTo(move);
        
        std::cout << i+1 << ". " << SQUARE_NAMES[from] << SQUARE_NAMES[to];
        
        if (move == bxf7) {
            std::cout << " <-- Bxf7+ HERE";
        }
        
        if (isCapture(move)) {
            SEEValue sv = see.see(board, move);
            std::cout << " (capture, SEE=" << sv << ")";
        }
        
        std::cout << "\n";
    }
    
    return 0;
}