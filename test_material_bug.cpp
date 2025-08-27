#include <iostream>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"
#include "src/uci/uci.h"

using namespace seajay;

void testPosition(const std::string& fen, const std::string& description) {
    std::cout << "\n=== " << description << " ===" << std::endl;
    std::cout << "FEN: " << fen << std::endl;
    
    Board board;
    board.fromFEN(fen);
    
    // Get material count
    const eval::Material& mat = board.material();
    
    // Count pieces manually
    int whitePawns = 0, whiteKnights = 0, whiteBishops = 0, whiteRooks = 0, whiteQueens = 0;
    int blackPawns = 0, blackKnights = 0, blackBishops = 0, blackRooks = 0, blackQueens = 0;
    
    for (int i = 0; i < 64; i++) {
        Square sq = static_cast<Square>(i);
        Piece p = board.pieceAt(sq);
        
        if (p != NO_PIECE) {
            Color c = colorOf(p);
            PieceType pt = typeOf(p);
            
            if (c == WHITE) {
                switch(pt) {
                    case PAWN: whitePawns++; break;
                    case KNIGHT: whiteKnights++; break;
                    case BISHOP: whiteBishops++; break;
                    case ROOK: whiteRooks++; break;
                    case QUEEN: whiteQueens++; break;
                    default: break;
                }
            } else {
                switch(pt) {
                    case PAWN: blackPawns++; break;
                    case KNIGHT: blackKnights++; break;
                    case BISHOP: blackBishops++; break;
                    case ROOK: blackRooks++; break;
                    case QUEEN: blackQueens++; break;
                    default: break;
                }
            }
        }
    }
    
    std::cout << "\nManual piece count:" << std::endl;
    std::cout << "White: P=" << whitePawns << " N=" << whiteKnights 
              << " B=" << whiteBishops << " R=" << whiteRooks << " Q=" << whiteQueens << std::endl;
    std::cout << "Black: P=" << blackPawns << " N=" << blackKnights 
              << " B=" << blackBishops << " R=" << blackRooks << " Q=" << blackQueens << std::endl;
              
    std::cout << "\nMaterial class piece count:" << std::endl;
    std::cout << "White: P=" << mat.count(WHITE, PAWN) << " N=" << mat.count(WHITE, KNIGHT) 
              << " B=" << mat.count(WHITE, BISHOP) << " R=" << mat.count(WHITE, ROOK) 
              << " Q=" << mat.count(WHITE, QUEEN) << std::endl;
    std::cout << "Black: P=" << mat.count(BLACK, PAWN) << " N=" << mat.count(BLACK, KNIGHT) 
              << " B=" << mat.count(BLACK, BISHOP) << " R=" << mat.count(BLACK, ROOK) 
              << " Q=" << mat.count(BLACK, QUEEN) << std::endl;
              
    // Calculate material values
    eval::Score whiteMatValue = mat.value(WHITE);
    eval::Score blackMatValue = mat.value(BLACK);
    
    std::cout << "\nMaterial values:" << std::endl;
    std::cout << "White: " << whiteMatValue.value() << " cp" << std::endl;
    std::cout << "Black: " << blackMatValue.value() << " cp" << std::endl;
    std::cout << "Difference (White perspective): " << (whiteMatValue - blackMatValue).value() << " cp" << std::endl;
    
    // Get full evaluation
    eval::Score evalScore = eval::evaluate(board);
    std::cout << "\nFull evaluation (from " << (board.sideToMove() == WHITE ? "White" : "Black") 
              << "'s perspective): " << evalScore.value() << " cp (" << (evalScore.value() / 100.0) << " pawns)" << std::endl;
}

int main() {
    std::cout << "Testing Material Counting Bug" << std::endl;
    std::cout << "=============================" << std::endl;
    
    // Test the fork position
    testPosition("r2qk2r/ppp1bppp/3p1n2/4p3/2B1P3/2NP2QP/PPn2PP1/R1B2RK1 w kq - 0 11",
                 "Position with Nc2 forking Queen and Rook");
    
    // Test after rook capture - original FEN from bug report  
    testPosition("r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/n4RK1 b kq - 0 12",
                 "After Nxa1 (White missing Rook) - ORIGINAL FEN");
                 
    // Test corrected FEN (without c2 pawn)
    testPosition("r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1P3PPP/n4RK1 b kq - 0 12",
                 "After Nxa1 (White missing Rook) - CORRECTED FEN");
                 
    // Test a clear position where White is down a rook
    testPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/1NBQKBNR w Kkq - 0 1",
                 "Starting position with White missing Ra1");
    
    return 0;
}