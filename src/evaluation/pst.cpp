#include "pst.h"
#include "../core/types.h"
#include <iostream>
#include <iomanip>

namespace seajay::eval {

// Helper constants for file and rank
constexpr int FILE_A = 0, FILE_B = 1, FILE_C = 2, FILE_D = 3;
constexpr int FILE_E = 4, FILE_F = 5, FILE_G = 6, FILE_H = 7;
constexpr int RANK_1 = 0, RANK_2 = 1, RANK_3 = 2, RANK_4 = 3;
constexpr int RANK_5 = 4, RANK_6 = 5, RANK_7 = 6, RANK_8 = 7;

// Implementation of SPSA parameter update from UCI
void PST::updateFromUCIParam(const std::string& param, int value) noexcept {
    // Clamp value as safety measure (belt-and-suspenders)
    // Even though UCI should enforce bounds, some GUIs might not
    value = std::clamp(value, -200, 200);
    
    // Parse parameter name and update corresponding PST values
    // Using simplified zone-based approach for initial SPSA implementation
    
    // ===== PAWN ENDGAME PST PARAMETERS =====
    if (param == "pawn_eg_r3_d") {
        // Rank 3, D file - use updateEndgameValue to control D and its mirror separately
        updateEndgameValue(PAWN, makeSquare(File(FILE_D), Rank(RANK_3)), value);
        updateEndgameValue(PAWN, makeSquare(File(FILE_E), Rank(RANK_3)), value);
    }
    else if (param == "pawn_eg_r3_e") {
        // Rank 3, E file - Actually controls C and F (the outer center files)
        updateEndgameValue(PAWN, makeSquare(File(FILE_C), Rank(RANK_3)), value);
        updateEndgameValue(PAWN, makeSquare(File(FILE_F), Rank(RANK_3)), value);
    }
    else if (param == "pawn_eg_r4_d") {
        updateEndgameValue(PAWN, makeSquare(File(FILE_D), Rank(RANK_4)), value);
        updateEndgameValue(PAWN, makeSquare(File(FILE_E), Rank(RANK_4)), value);
    }
    else if (param == "pawn_eg_r4_e") {
        // Actually controls C and F files
        updateEndgameValue(PAWN, makeSquare(File(FILE_C), Rank(RANK_4)), value);
        updateEndgameValue(PAWN, makeSquare(File(FILE_F), Rank(RANK_4)), value);
    }
    else if (param == "pawn_eg_r5_d") {
        updateEndgameValue(PAWN, makeSquare(File(FILE_D), Rank(RANK_5)), value);
        updateEndgameValue(PAWN, makeSquare(File(FILE_E), Rank(RANK_5)), value);
    }
    else if (param == "pawn_eg_r5_e") {
        // Actually controls C and F files
        updateEndgameValue(PAWN, makeSquare(File(FILE_C), Rank(RANK_5)), value);
        updateEndgameValue(PAWN, makeSquare(File(FILE_F), Rank(RANK_5)), value);
    }
    else if (param == "pawn_eg_r6_d") {
        updateEndgameValue(PAWN, makeSquare(File(FILE_D), Rank(RANK_6)), value);
        updateEndgameValue(PAWN, makeSquare(File(FILE_E), Rank(RANK_6)), value);
    }
    else if (param == "pawn_eg_r6_e") {
        // Actually controls C and F files
        updateEndgameValue(PAWN, makeSquare(File(FILE_C), Rank(RANK_6)), value);
        updateEndgameValue(PAWN, makeSquare(File(FILE_F), Rank(RANK_6)), value);
    }
    else if (param == "pawn_eg_r7_center") {
        // Rank 7 - all central files get same value
        for (int f = FILE_C; f <= FILE_F; ++f) {
            updateEndgameValue(PAWN, makeSquare(File(f), Rank(RANK_7)), value);
        }
    }
    
    // ===== KNIGHT ENDGAME PST PARAMETERS =====
    else if (param == "knight_eg_center") {
        // Central 4 squares
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_D), Rank(RANK_4)), value);
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_E), Rank(RANK_4)), value);
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_D), Rank(RANK_5)), value);
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_E), Rank(RANK_5)), value);
    }
    else if (param == "knight_eg_extended") {
        // Extended center (16 squares around center)
        // Rank 3
        for (int f = FILE_C; f <= FILE_F; ++f) {
            updateEndgameValue(KNIGHT, makeSquare(File(f), Rank(RANK_3)), value);
        }
        // Rank 4 (skip D4, E4 which are center)
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_C), Rank(RANK_4)), value);
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_F), Rank(RANK_4)), value);
        // Rank 5 (skip D5, E5 which are center)
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_C), Rank(RANK_5)), value);
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_F), Rank(RANK_5)), value);
        // Rank 6
        for (int f = FILE_C; f <= FILE_F; ++f) {
            updateEndgameValue(KNIGHT, makeSquare(File(f), Rank(RANK_6)), value);
        }
    }
    else if (param == "knight_eg_edge") {
        // Edge squares (not corners)
        // A and H files (except corners)
        for (int r = RANK_2; r <= RANK_7; ++r) {
            updateEndgameValue(KNIGHT, makeSquare(FILE_A, Rank(r)), value);
            updateEndgameValue(KNIGHT, makeSquare(FILE_H, Rank(r)), value);
        }
        // Ranks 1 and 8 (except corners)
        for (int f = FILE_B; f <= FILE_G; ++f) {
            updateEndgameValue(KNIGHT, makeSquare(File(f), RANK_1), value);
            updateEndgameValue(KNIGHT, makeSquare(File(f), RANK_8), value);
        }
    }
    else if (param == "knight_eg_corner") {
        // Corner squares
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_A), Rank(RANK_1)), value);
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_H), Rank(RANK_1)), value);
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_A), Rank(RANK_8)), value);
        updateEndgameValue(KNIGHT, makeSquare(File(FILE_H), Rank(RANK_8)), value);
    }
    
    // ===== BISHOP ENDGAME PST PARAMETERS =====
    else if (param == "bishop_eg_long_diag") {
        // Long diagonals - A1-H8
        for (int i = 0; i < 8; ++i) {
            updateEndgameValue(BISHOP, makeSquare(File(i), Rank(i)), value);
        }
        // Long diagonal - A8-H1
        for (int i = 0; i < 8; ++i) {
            updateEndgameValue(BISHOP, makeSquare(File(i), Rank(7-i)), value);
        }
    }
    else if (param == "bishop_eg_center") {
        // Central squares
        updateEndgameValue(BISHOP, makeSquare(File(FILE_D), Rank(RANK_4)), value);
        updateEndgameValue(BISHOP, makeSquare(File(FILE_E), Rank(RANK_4)), value);
        updateEndgameValue(BISHOP, makeSquare(File(FILE_D), Rank(RANK_5)), value);
        updateEndgameValue(BISHOP, makeSquare(File(FILE_E), Rank(RANK_5)), value);
        updateEndgameValue(BISHOP, makeSquare(File(FILE_C), Rank(RANK_3)), value);
        updateEndgameValue(BISHOP, makeSquare(File(FILE_F), Rank(RANK_3)), value);
        updateEndgameValue(BISHOP, makeSquare(File(FILE_C), Rank(RANK_6)), value);
        updateEndgameValue(BISHOP, makeSquare(File(FILE_F), Rank(RANK_6)), value);
    }
    else if (param == "bishop_eg_edge") {
        // Edge squares
        for (int r = RANK_1; r <= RANK_8; ++r) {
            updateEndgameValue(BISHOP, makeSquare(FILE_A, Rank(r)), value);
            updateEndgameValue(BISHOP, makeSquare(FILE_H, Rank(r)), value);
        }
        for (int f = FILE_B; f <= FILE_G; ++f) {
            updateEndgameValue(BISHOP, makeSquare(File(f), RANK_1), value);
            updateEndgameValue(BISHOP, makeSquare(File(f), RANK_8), value);
        }
    }
    
    // ===== ROOK ENDGAME PST PARAMETERS =====
    else if (param == "rook_eg_7th") {
        // 7th rank only - rank mirroring handles Black's 2nd rank automatically
        for (int f = FILE_A; f <= FILE_H; ++f) {
            updateEndgameValue(ROOK, makeSquare(File(f), RANK_7), value);
        }
    }
    else if (param == "rook_eg_active") {
        // Active squares (ranks 4-6)
        for (int r = RANK_4; r <= RANK_6; ++r) {
            for (int f = FILE_A; f <= FILE_H; ++f) {
                updateEndgameValue(ROOK, makeSquare(File(f), Rank(r)), value);
            }
        }
    }
    else if (param == "rook_eg_passive") {
        // Passive squares - back rank and rank 3 only
        // Rank mirroring handles Black's equivalent ranks
        for (int f = FILE_A; f <= FILE_H; ++f) {
            updateEndgameValue(ROOK, makeSquare(File(f), RANK_1), value);
            updateEndgameValue(ROOK, makeSquare(File(f), RANK_3), value);
        }
    }
    
    // ===== QUEEN ENDGAME PST PARAMETERS =====
    else if (param == "queen_eg_center") {
        // Central squares
        for (int r = RANK_3; r <= RANK_6; ++r) {
            for (int f = FILE_C; f <= FILE_F; ++f) {
                updateEndgameValue(QUEEN, makeSquare(File(f), Rank(r)), value);
            }
        }
    }
    else if (param == "queen_eg_active") {
        // Active squares (non-central, non-back rank)
        for (int r = RANK_2; r <= RANK_7; ++r) {
            for (int f = FILE_A; f <= FILE_H; ++f) {
                // Skip if already covered by center
                if (r >= RANK_3 && r <= RANK_6 && f >= FILE_C && f <= FILE_F) continue;
                updateEndgameValue(QUEEN, makeSquare(File(f), Rank(r)), value);
            }
        }
    }
    else if (param == "queen_eg_back") {
        // Back rank only - rank mirroring handles Black's back rank
        for (int f = FILE_A; f <= FILE_H; ++f) {
            updateEndgameValue(QUEEN, makeSquare(File(f), RANK_1), value);
        }
    }
    // ===== KING MIDDLEGAME PST PARAMETERS =====
    // These control castling incentives - critical for opening play
    else if (param == "king_mg_e1") {
        // E1 - starting square for white king
        updateMiddlegameValue(KING, makeSquare(FILE_E, RANK_1), value);
    }
    else if (param == "king_mg_b1") {
        // B1 - queenside castled position
        updateMiddlegameValue(KING, makeSquare(FILE_B, RANK_1), value);
    }
    else if (param == "king_mg_g1") {
        // G1 - kingside castled position  
        updateMiddlegameValue(KING, makeSquare(FILE_G, RANK_1), value);
    }
    else if (param == "king_mg_a1") {
        // A1 - corner square
        updateMiddlegameValue(KING, makeSquare(FILE_A, RANK_1), value);
    }
    else if (param == "king_mg_h1") {
        // H1 - corner square
        updateMiddlegameValue(KING, makeSquare(FILE_H, RANK_1), value);
    }
    else if (param == "king_mg_c1") {
        // C1 - near queenside castle
        updateMiddlegameValue(KING, makeSquare(FILE_C, RANK_1), value);
    }
    else if (param == "king_mg_d1") {
        // D1 - center-ish
        updateMiddlegameValue(KING, makeSquare(FILE_D, RANK_1), value);
    }
    else if (param == "king_mg_f1") {
        // F1 - center-ish
        updateMiddlegameValue(KING, makeSquare(FILE_F, RANK_1), value);
    }
}

// Debug: dump current PST values
void PST::dumpTables() noexcept {
    const char* pieceNames[] = {"Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};
    
    std::cout << "\n===== Current PST Values =====\n\n";
    
    for (int pt = PAWN; pt <= KING; ++pt) {
        std::cout << "=== " << pieceNames[pt] << " ===\n";
        std::cout << "Square format: mg/eg\n\n";
        
        // Print from rank 8 to rank 1 (standard chess board view)
        for (int r = RANK_8; r >= RANK_1; --r) {
            std::cout << (r + 1) << " |";
            for (int f = FILE_A; f <= FILE_H; ++f) {
                Square sq = makeSquare(File(f), Rank(r));
                int mg = s_pstTables[pt][sq].mg.value();
                int eg = s_pstTables[pt][sq].eg.value();
                
                // Format: mg/eg with padding
                std::cout << std::setw(4) << mg << "/" << std::setw(3) << eg << " ";
            }
            std::cout << "\n";
        }
        std::cout << "  +----------------------------------------\n";
        std::cout << "    a      b      c      d      e      f      g      h\n\n";
    }
    
    std::cout << "===== End PST Values =====\n";
}

} // namespace seajay::eval