#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;
using namespace seajay::eval;

std::string moveToString(Move move) {
    Square from = moveFrom(move);
    Square to = moveTo(move);
    
    std::string result;
    result += char('a' + fileOf(from));
    result += char('1' + rankOf(from));
    result += char('a' + fileOf(to));
    result += char('1' + rankOf(to));
    
    if (isPromotion(move)) {
        switch (promotionType(move)) {
            case QUEEN: result += 'q'; break;
            case ROOK: result += 'r'; break;
            case BISHOP: result += 'b'; break;
            case KNIGHT: result += 'n'; break;
            default: break;
        }
    }
    
    return result;
}

int main() {
    // Test from position after 1.d4
    Board board;
    board.setFromFEN("rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1");
    
    std::cout << "Position after 1.d4:\n";
    std::cout << "Black to move\n\n";
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Legal moves for Black: " << moves.size() << std::endl;
    std::cout << "\nFirst 20 moves (in generation order):\n";
    for (int i = 0; i < std::min(20, (int)moves.size()); i++) {
        Move move = moves[i];
        std::cout << std::setw(2) << i+1 << ". " << moveToString(move) << std::endl;
    }
    
    // Now test evaluation for each move
    std::cout << "\nEvaluation after each move (from Black's perspective):\n";
    std::cout << "Move     Eval   Material  PST\n";
    std::cout << "-----    ----   --------  ---\n";
    
    for (int i = 0; i < std::min(20, (int)moves.size()); i++) {
        Move move = moves[i];
        Board testBoard = board;  // Make a copy
        testBoard.makeMove(move);
        
        Score eval = evaluate(testBoard);
        
        // Get material difference
        const Material& mat = testBoard.material();
        Score matDiff = mat.value(WHITE) - mat.value(BLACK);
        
        // PST is the difference
        Score pstValue = eval - (-matDiff);  // Negate because eval is from Black's perspective
        
        std::cout << moveToString(move) << "  " 
                  << std::setw(6) << eval 
                  << std::setw(9) << matDiff
                  << std::setw(5) << pstValue
                  << std::endl;
    }
    
    return 0;
}
