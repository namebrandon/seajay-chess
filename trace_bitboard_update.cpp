#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/core/bitboard.h"

using namespace seajay;

// Need to access private members for debugging
class BoardDebugger : public Board {
public:
    void debugParseBoardPosition(const std::string& boardStr) {
        std::cout << "=== Parsing board position: " << boardStr << " ===" << std::endl;
        
        // Clear the board first
        clear();
        
        // Now parse the board position manually to trace what happens
        Square sq = SQ_A8;  // Start from a8
        int rank = 7;
        int file = 0;
        
        for (char c : boardStr) {
            if (c == '/') {
                rank--;
                file = 0;
                sq = static_cast<Square>(rank * 8);
            } else if (c >= '1' && c <= '8') {
                int skip = c - '0';
                file += skip;
                sq = static_cast<Square>(sq + skip);
            } else if (Board::PIECE_CHAR_LUT[static_cast<uint8_t>(c)] != NO_PIECE) {
                Piece p = Board::PIECE_CHAR_LUT[static_cast<uint8_t>(c)];
                
                std::cout << "  Placing " << PIECE_CHARS[p] << " (piece=" << (int)p << ") at " 
                          << squareToString(sq) << " (square=" << (int)sq << ")" << std::endl;
                
                // Set in mailbox
                m_mailbox[sq] = p;
                
                file++;
                sq++;
            }
        }
        
        std::cout << "\n=== Mailbox after parsing ===" << std::endl;
        printMailbox();
        
        std::cout << "\n=== Rebuilding bitboards ===" << std::endl;
        
        // Clear bitboards first
        m_pieceBB.fill(0);
        m_pieceTypeBB.fill(0);
        m_colorBB.fill(0);
        m_occupied = 0;
        
        // Rebuild from mailbox
        for (Square s = SQ_A1; s <= SQ_H8; ++s) {
            Piece p = m_mailbox[s];
            if (p != NO_PIECE) {
                std::cout << "  Square " << squareToString(s) << " has " << PIECE_CHARS[p] 
                          << " (piece=" << (int)p << ")" << std::endl;
                updateBitboards(s, p, true);
                
                // After each update, show the king bitboards if it was a king
                if (p == WHITE_KING || p == BLACK_KING) {
                    std::cout << "    After adding this king:" << std::endl;
                    std::cout << "      WHITE_KING BB: 0x" << std::hex << m_pieceBB[WHITE_KING] << std::dec << std::endl;
                    std::cout << "      BLACK_KING BB: 0x" << std::hex << m_pieceBB[BLACK_KING] << std::dec << std::endl;
                }
            }
        }
        
        std::cout << "\n=== Final bitboard state ===" << std::endl;
        std::cout << "WHITE_KING (piece " << (int)WHITE_KING << ") BB: 0x" 
                  << std::hex << m_pieceBB[WHITE_KING] << std::dec;
        if (m_pieceBB[WHITE_KING] != 0) {
            std::cout << " = square " << squareToString(lsb(m_pieceBB[WHITE_KING]));
        }
        std::cout << std::endl;
        
        std::cout << "BLACK_KING (piece " << (int)BLACK_KING << ") BB: 0x" 
                  << std::hex << m_pieceBB[BLACK_KING] << std::dec;
        if (m_pieceBB[BLACK_KING] != 0) {
            std::cout << " = square " << squareToString(lsb(m_pieceBB[BLACK_KING]));
        }
        std::cout << std::endl;
        
        // Now test validateKings
        std::cout << "\n=== Testing validateKings ===" << std::endl;
        bool valid = validateKings();
        std::cout << "Result: " << (valid ? "PASS" : "FAIL") << std::endl;
        
        if (!valid) {
            // Debug why it failed
            int whiteKings = popCount(m_pieceBB[WHITE_KING]);
            int blackKings = popCount(m_pieceBB[BLACK_KING]);
            std::cout << "  White kings count: " << whiteKings << std::endl;
            std::cout << "  Black kings count: " << blackKings << std::endl;
            
            if (whiteKings == 1 && blackKings == 1) {
                Square wk = lsb(m_pieceBB[WHITE_KING]);
                Square bk = lsb(m_pieceBB[BLACK_KING]);
                int fileDiff = abs(fileOf(wk) - fileOf(bk));
                int rankDiff = abs(rankOf(wk) - rankOf(bk));
                std::cout << "  Kings adjacent check: file diff=" << fileDiff 
                          << ", rank diff=" << rankDiff << std::endl;
                if (fileDiff <= 1 && rankDiff <= 1) {
                    std::cout << "  FAILED: Kings are adjacent!" << std::endl;
                }
            }
        }
    }
    
    void printMailbox() {
        for (int r = 7; r >= 0; --r) {
            std::cout << (r+1) << " ";
            for (int f = 0; f < 8; ++f) {
                Square s = makeSquare(static_cast<File>(f), static_cast<Rank>(r));
                Piece p = m_mailbox[s];
                std::cout << PIECE_CHARS[p] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "  a b c d e f g h" << std::endl;
    }
};

int main() {
    // Initialize the piece character lookup table
    if (!Board::s_lutInitialized) {
        initPieceCharLookup();
    }
    
    BoardDebugger board;
    
    // Test the problematic position
    board.debugParseBoardPosition("rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR");
    
    return 0;
}