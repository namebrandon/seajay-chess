#include <iostream>
#include <string>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"
#include "src/evaluation/pawn_structure.h"

using namespace seajay;

void testPosition(const std::string& fen, const std::string& description) {
    Board board;
    board.setFromFen(fen);
    
    // Get pawn bitboards
    Bitboard whitePawns = board.pieces(WHITE, PAWN);
    Bitboard blackPawns = board.pieces(BLACK, PAWN);
    
    // Get doubled pawns
    Bitboard whiteDoubled = g_pawnStructure.getDoubledPawns(WHITE, whitePawns);
    Bitboard blackDoubled = g_pawnStructure.getDoubledPawns(BLACK, blackPawns);
    
    int whiteDoubledCount = popCount(whiteDoubled);
    int blackDoubledCount = popCount(blackDoubled);
    
    // Evaluate position
    eval::Score score = eval::evaluate(board);
    
    std::cout << "\n" << description << std::endl;
    std::cout << "FEN: " << fen << std::endl;
    std::cout << "White doubled pawns: " << whiteDoubledCount << std::endl;
    std::cout << "Black doubled pawns: " << blackDoubledCount << std::endl;
    std::cout << "Evaluation: " << score.value() << " cp (from white's perspective)" << std::endl;
}

int main() {
    // Initialize
    initBitboards();
    initZobrist();
    PawnStructure::initPassedPawnMasks();
    
    std::cout << "Testing Doubled Pawn Evaluation\n";
    std::cout << "================================\n";
    
    // Test 1: Starting position (no doubled pawns)
    testPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                 "Starting position - no doubled pawns");
    
    // Test 2: White has doubled pawns on d-file
    testPosition("rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1",
                 "White has doubled d-pawns (d2, d4)");
    
    // Test 3: Black has doubled pawns on d-file  
    testPosition("rnbqkbnr/ppp1pppp/8/3p4/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                 "Black has doubled d-pawns (d7, d5)");
    
    // Test 4: Both sides have doubled pawns
    testPosition("rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1",
                 "Both have doubled d-pawns");
    
    // Test 5: White has tripled pawns
    testPosition("rnbqkbnr/pppppppp/8/3P4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1",
                 "White has tripled d-pawns (d2, d4, d5)");
    
    // Test 6: Typical French Defense structure
    testPosition("rnbqkb1r/ppp2ppp/4pn2/3p4/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 1",
                 "French Defense - no doubled pawns yet");
    
    return 0;
}