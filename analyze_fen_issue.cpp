#include <iostream>
#include <string>
#include <vector>

void analyzeFEN(const std::string& fen, const std::string& label) {
    std::cout << "\n=== " << label << " ===\n";
    std::cout << "FEN: " << fen << "\n\n";
    
    // Parse just the board part
    size_t space_pos = fen.find(' ');
    std::string board_part = fen.substr(0, space_pos);
    
    // Count pieces rank by rank
    std::cout << "Board layout (rank by rank):\n";
    int rank = 8;
    size_t pos = 0;
    
    for (char c : board_part) {
        if (c == '/') {
            std::cout << "\n";
            rank--;
            continue;
        }
        
        if (isdigit(c)) {
            int empty = c - '0';
            for (int i = 0; i < empty; i++) {
                std::cout << '.';
            }
        } else {
            std::cout << c;
        }
    }
    std::cout << "\n\n";
    
    // Count pieces
    int white_pawns = 0, black_pawns = 0;
    int white_rooks = 0, black_rooks = 0;
    int white_knights = 0, black_knights = 0;
    int white_bishops = 0, black_bishops = 0;
    int white_queens = 0, black_queens = 0;
    int white_kings = 0, black_kings = 0;
    
    for (char c : board_part) {
        switch(c) {
            case 'P': white_pawns++; break;
            case 'p': black_pawns++; break;
            case 'R': white_rooks++; break;
            case 'r': black_rooks++; break;
            case 'N': white_knights++; break;
            case 'n': black_knights++; break;
            case 'B': white_bishops++; break;
            case 'b': black_bishops++; break;
            case 'Q': white_queens++; break;
            case 'q': black_queens++; break;
            case 'K': white_kings++; break;
            case 'k': black_kings++; break;
        }
    }
    
    std::cout << "Piece count:\n";
    std::cout << "White: P=" << white_pawns << " N=" << white_knights 
              << " B=" << white_bishops << " R=" << white_rooks 
              << " Q=" << white_queens << " K=" << white_kings << "\n";
    std::cout << "Black: P=" << black_pawns << " N=" << black_knights
              << " B=" << black_bishops << " R=" << black_rooks
              << " Q=" << black_queens << " K=" << black_kings << "\n";
    
    // Check second rank for white pawns
    std::cout << "\nSecond rank analysis: ";
    // Extract rank 2 (7th line in FEN before the first /)
    std::vector<std::string> ranks;
    std::string current_rank;
    for (char c : board_part) {
        if (c == '/') {
            ranks.push_back(current_rank);
            current_rank = "";
        } else {
            current_rank += c;
        }
    }
    ranks.push_back(current_rank); // Last rank
    
    if (ranks.size() >= 7) {
        std::cout << ranks[6] << " (should be rank 2)\n";
        
        // Decode rank 2
        std::cout << "Decoded rank 2: ";
        for (char c : ranks[6]) {
            if (isdigit(c)) {
                int empty = c - '0';
                for (int i = 0; i < empty; i++) {
                    std::cout << '.';
                }
            } else {
                std::cout << c;
            }
        }
        std::cout << "\n";
    }
}

int main() {
    // The position after Nxa1 according to the problem description
    analyzeFEN("r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/n4RK1 b kq - 0 12", 
               "Position AFTER Nxa1 (from problem)");
    
    // What the position should be if a knight on c2 captured the rook on a1
    // The c2 pawn shouldn't exist - it should still show the knight's former square as empty
    analyzeFEN("r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1P3PPP/n4RK1 b kq - 0 12", 
               "Expected position AFTER Nxa1 (corrected - no c2 pawn)");
    
    // Position before (with knight on c2 and rook on a1) 
    analyzeFEN("r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1Pn2PPP/R4RK1 w kq - 0 12",
               "Position BEFORE Nxa1 (knight on c2)");
    
    std::cout << "\n=== ANALYSIS ===\n";
    std::cout << "The problem FEN shows 'PP' on rank 2 (pawns on b2 and c2)\n";
    std::cout << "But if a knight just moved from c2 to capture on a1,\n";
    std::cout << "there shouldn't be a pawn on c2!\n";
    std::cout << "\nThis explains the inflated material count.\n";
    std::cout << "The FEN is incorrect - it has an extra white pawn.\n";
    
    return 0;
}