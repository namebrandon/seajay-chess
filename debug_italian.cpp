#include <iostream>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"
#include "src/evaluation/material.h"

using namespace seajay;

void analyzePosition(const std::string& fen, const std::string& name) {
    Board board;
    board.fromFEN(fen);
    
    std::cout << "\n" << name << ":\n";
    std::cout << "FEN: " << fen << "\n";
    
    // Material
    const Material& mat = board.material();
    eval::Score matWhite = mat.value(WHITE);
    eval::Score matBlack = mat.value(BLACK);
    std::cout << "Material: White=" << matWhite.value() << " Black=" << matBlack.value() 
              << " Diff=" << (matWhite - matBlack).value() << "\n";
    
    // PST
    const eval::MgEgScore& pst = board.pstScore();
    std::cout << "PST Score: " << pst.mg.value() << "\n";
    
    // Total eval
    eval::Score eval = eval::evaluate(board);
    std::cout << "Total Eval: " << eval.value() << " cp\n";
    
    // Check castling rights
    std::cout << "Castling: ";
    if (board.canCastleKingside(WHITE)) std::cout << "K";
    if (board.canCastleQueenside(WHITE)) std::cout << "Q";
    if (board.canCastleKingside(BLACK)) std::cout << "k";
    if (board.canCastleQueenside(BLACK)) std::cout << "q";
    if (!board.canCastleKingside(WHITE) && !board.canCastleQueenside(WHITE) &&
        !board.canCastleKingside(BLACK) && !board.canCastleQueenside(BLACK)) std::cout << "-";
    std::cout << "\n";
}

int main() {
    std::cout << "Debugging Italian Game Asymmetry\n";
    std::cout << "=================================\n";
    
    // Original position
    analyzePosition("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1", 
                   "Original Italian");
    
    // Color-flipped version  
    analyzePosition("RNBQK2R/PPPP1PPP/5N2/2B1P3/4p3/2n2n2/pppp1ppp/r1bqkb1r w - - 0 1",
                   "Color-flipped Italian");
    
    // Try without castling rights to see if that's the issue
    analyzePosition("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w - - 0 1", 
                   "Original Italian (no castling)");
    
    analyzePosition("RNBQK2R/PPPP1PPP/5N2/2B1P3/4p3/2n2n2/pppp1ppp/r1bqkb1r w - - 0 1",
                   "Color-flipped (no castling)");
    
    return 0;
}