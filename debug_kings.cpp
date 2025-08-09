#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/core/bitboard.h"

using namespace seajay;

int main() {
    Board board;
    
    std::cout << "=== Debugging King Validation Issue ===" << std::endl;
    std::cout << "\nTesting FEN: rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1" << std::endl;
    
    // Create a board with known king positions
    Board testBoard;
    testBoard.clear();
    
    // Set up a simple position with just kings to test
    testBoard.setPiece(E1, WHITE_KING);
    testBoard.setPiece(E8, BLACK_KING);
    
    std::cout << "\n1. Test simple kings-only position:" << std::endl;
    bool kingsValid = testBoard.validateKings();
    std::cout << "Kings valid (simple): " << (kingsValid ? "YES" : "NO") << std::endl;
    
    // Now test the actual problematic FEN
    std::cout << "\n2. Testing problematic FEN..." << std::endl;
    std::string fen = "rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1";
    
    // Use a temporary board to track what happens during parsing
    Board tempBoard;
    
    // First, try parsing with parseFEN to see the error
    auto result = tempBoard.parseFEN(fen);
    if (!result) {
        std::cout << "parseFEN failed: " << result.error().message << std::endl;
    } else {
        std::cout << "parseFEN succeeded!" << std::endl;
    }
    
    // Print mailbox state AFTER attempted parse
    std::cout << "\n3. Mailbox state after parse attempt:" << std::endl;
    for (int r = 7; r >= 0; --r) {
        std::cout << (r+1) << " ";
        for (int f = 0; f < 8; ++f) {
            Square s = makeSquare(static_cast<File>(f), static_cast<Rank>(r));
            Piece p = tempBoard.pieceAt(s);
            std::cout << PIECE_CHARS[p] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "  a b c d e f g h" << std::endl;
    
    // Check what pieces() returns
    std::cout << "\n4. Bitboard state check:" << std::endl;
    std::cout << "White king bitboard: 0x" << std::hex << tempBoard.pieces(WHITE_KING) << std::dec << std::endl;
    std::cout << "Black king bitboard: 0x" << std::hex << tempBoard.pieces(BLACK_KING) << std::dec << std::endl;
    
    // Find kings in mailbox directly
    std::cout << "\n5. Looking for kings in mailbox:" << std::endl;
    int whiteKingCount = 0, blackKingCount = 0;
    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
        Piece p = tempBoard.pieceAt(sq);
        if (p == WHITE_KING) {
            std::cout << "  White king at " << squareToString(sq) << std::endl;
            whiteKingCount++;
        } else if (p == BLACK_KING) {
            std::cout << "  Black king at " << squareToString(sq) << std::endl;
            blackKingCount++;
        }
    }
    std::cout << "Total white kings in mailbox: " << whiteKingCount << std::endl;
    std::cout << "Total black kings in mailbox: " << blackKingCount << std::endl;
    
    // Check individual validation functions
    std::cout << "\n6. Individual validation checks:" << std::endl;
    std::cout << "  Piece counts valid: " << (tempBoard.validatePieceCounts() ? "YES" : "NO") << std::endl;
    std::cout << "  Kings valid: " << (tempBoard.validateKings() ? "YES" : "NO") << std::endl;
    std::cout << "  En passant valid: " << (tempBoard.validateEnPassant() ? "YES" : "NO") << std::endl;
    std::cout << "  Castling rights valid: " << (tempBoard.validateCastlingRights() ? "YES" : "NO") << std::endl;
    std::cout << "  Bitboard sync valid: " << (tempBoard.validateBitboardSync() ? "YES" : "NO") << std::endl;
    
    return 0;
}