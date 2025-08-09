#include <iostream>
#include <vector>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Creating board and using setStartingPosition..." << std::endl;
    Board board;
    
    // Let's try the legacy fromFEN instead of the new parseFEN
    std::cout << "Testing legacy fromFEN..." << std::endl;
    bool result = board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    if (!result) {
        std::cout << "Legacy fromFEN failed!" << std::endl;
        return 1;
    } else {
        std::cout << "Legacy fromFEN succeeded!" << std::endl;
    }
    
    std::cout << "Testing individual validation functions..." << std::endl;
    
    std::cout << "validatePieceCounts: " << std::flush;
    bool pieceCounts = board.validatePieceCounts();
    std::cout << (pieceCounts ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "validateKings: " << std::flush;
    bool kings = board.validateKings();
    std::cout << (kings ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "validateEnPassant: " << std::flush;
    bool enPassant = board.validateEnPassant();
    std::cout << (enPassant ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "validateCastlingRights: " << std::flush;
    bool castling = board.validateCastlingRights();
    std::cout << (castling ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "validateNotInCheck: " << std::flush;
    bool notInCheck = board.validateNotInCheck();
    std::cout << (notInCheck ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "validateBitboardSync: " << std::flush;
    bool bitboardSync = board.validateBitboardSync();
    std::cout << (bitboardSync ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "validateZobrist: " << std::flush;
    bool zobrist = board.validateZobrist();
    std::cout << (zobrist ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "All validation tests completed!" << std::endl;
    
    return 0;
}