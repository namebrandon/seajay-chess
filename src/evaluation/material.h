#pragma once

#include "types.h"
#include "../core/types.h"
#include "../core/bitboard.h"
#include <array>
#include <cassert>

namespace seajay::eval {

// Default middlegame piece values (SPSA tuned 2025-01-04 with 150k games)
inline std::array<Score, 6> PIECE_VALUES_MG = {
    Score(71),    // PAWN (lower value = more dynamic play)
    Score(325),   // KNIGHT  
    Score(344),   // BISHOP (stronger than traditional)
    Score(487),   // ROOK
    Score(895),   // QUEEN
    Score(0)      // KING (not counted in material)
};

// Default endgame piece values (SPSA tuned 2025-01-04 with 150k games)
inline std::array<Score, 6> PIECE_VALUES_EG = {
    Score(92),    // PAWN
    Score(311),   // KNIGHT
    Score(327),   // BISHOP
    Score(510),   // ROOK (strong in endgame)
    Score(932),   // QUEEN
    Score(0)      // KING (not counted in material)
};

// Keep backward compatibility - PIECE_VALUES now points to MG values
inline std::array<Score, 6>& PIECE_VALUES = PIECE_VALUES_MG;

// UCI interface to update middlegame piece values (backward compatible)
inline void setPieceValue(PieceType pt, int value) {
    if (pt >= PAWN && pt <= QUEEN) {
        PIECE_VALUES_MG[pt] = Score(value);
    }
}

// UCI interface to update middlegame piece values (explicit)
inline void setPieceValueMg(PieceType pt, int value) {
    if (pt >= PAWN && pt <= QUEEN) {
        PIECE_VALUES_MG[pt] = Score(value);
    }
}

// UCI interface to update endgame piece values
inline void setPieceValueEg(PieceType pt, int value) {
    if (pt >= PAWN && pt <= QUEEN) {
        PIECE_VALUES_EG[pt] = Score(value);
    }
}

class alignas(64) Material {
public:
    Material() noexcept = default;
    
    constexpr void update(Piece p, bool add) noexcept {
        // Add bounds checking to prevent crashes with invalid pieces
        if (p < WHITE_PAWN || p > BLACK_KING) return;
        
        const int delta = add ? 1 : -1;
        m_counts[p] = static_cast<int8_t>(m_counts[p] + delta);
        
        const Color c = colorOf(p);
        const PieceType pt = typeOf(p);
        if (pt != KING) {  // Don't count king value
            if (add) {
                m_values_mg[c] += PIECE_VALUES_MG[pt];
                m_values_eg[c] += PIECE_VALUES_EG[pt];
            } else {
                m_values_mg[c] -= PIECE_VALUES_MG[pt];
                m_values_eg[c] -= PIECE_VALUES_EG[pt];
            }
        }
        
        assert(m_counts[p] >= 0);
        assert(m_counts[p] <= 10);  // Sanity check (8 pawns + 2 promoted)
    }
    
    constexpr void add(Piece p) noexcept {
        if (p < WHITE_PAWN || p > BLACK_KING) return;  // Bounds check
        update(p, true);
    }
    
    constexpr void remove(Piece p) noexcept {
        if (p < WHITE_PAWN || p > BLACK_KING) return;  // Bounds check
        update(p, false);
    }
    
    [[nodiscard]] constexpr Score balance(Color stm) const noexcept {
        // For backward compatibility, return middlegame values
        const Score white_val = m_values_mg[WHITE];
        const Score black_val = m_values_mg[BLACK];
        return stm == WHITE ? white_val - black_val : black_val - white_val;
    }
    
    [[nodiscard]] constexpr Score balanceMg(Color stm) const noexcept {
        const Score white_val = m_values_mg[WHITE];
        const Score black_val = m_values_mg[BLACK];
        return stm == WHITE ? white_val - black_val : black_val - white_val;
    }
    
    [[nodiscard]] constexpr Score balanceEg(Color stm) const noexcept {
        const Score white_val = m_values_eg[WHITE];
        const Score black_val = m_values_eg[BLACK];
        return stm == WHITE ? white_val - black_val : black_val - white_val;
    }
    
    [[nodiscard]] constexpr Score value(Color c) const noexcept {
        // For backward compatibility, return middlegame values
        return m_values_mg[c];
    }
    
    [[nodiscard]] constexpr Score valueMg(Color c) const noexcept {
        return m_values_mg[c];
    }
    
    [[nodiscard]] constexpr Score valueEg(Color c) const noexcept {
        return m_values_eg[c];
    }
    
    [[nodiscard]] constexpr int count(Piece p) const noexcept {
        assert(p >= WHITE_PAWN && p <= BLACK_KING);
        return m_counts[p];
    }
    
    [[nodiscard]] constexpr int count(Color c, PieceType pt) const noexcept {
        return m_counts[makePiece(c, pt)];
    }
    
    [[nodiscard]] constexpr Score nonPawnMaterial(Color c) const noexcept {
        Score total = Score::zero();
        // Manually iterate through piece types (using MG values for backward compat)
        total += PIECE_VALUES_MG[KNIGHT] * count(c, KNIGHT);
        total += PIECE_VALUES_MG[BISHOP] * count(c, BISHOP);
        total += PIECE_VALUES_MG[ROOK] * count(c, ROOK);
        total += PIECE_VALUES_MG[QUEEN] * count(c, QUEEN);
        return total;
    }
    
    [[nodiscard]] constexpr Score nonPawnMaterialMg(Color c) const noexcept {
        Score total = Score::zero();
        total += PIECE_VALUES_MG[KNIGHT] * count(c, KNIGHT);
        total += PIECE_VALUES_MG[BISHOP] * count(c, BISHOP);
        total += PIECE_VALUES_MG[ROOK] * count(c, ROOK);
        total += PIECE_VALUES_MG[QUEEN] * count(c, QUEEN);
        return total;
    }
    
    [[nodiscard]] constexpr Score nonPawnMaterialEg(Color c) const noexcept {
        Score total = Score::zero();
        total += PIECE_VALUES_EG[KNIGHT] * count(c, KNIGHT);
        total += PIECE_VALUES_EG[BISHOP] * count(c, BISHOP);
        total += PIECE_VALUES_EG[ROOK] * count(c, ROOK);
        total += PIECE_VALUES_EG[QUEEN] * count(c, QUEEN);
        return total;
    }
    
    [[nodiscard]] constexpr bool isInsufficientMaterial() const noexcept {
        // K vs K
        int totalPieces = 0;
        int totalNonKings = 0;
        for (int i = 0; i < 12; ++i) {
            totalPieces += m_counts[i];
            if (i != WHITE_KING && i != BLACK_KING) {
                totalNonKings += m_counts[i];
            }
        }
        
        if (totalNonKings == 0) return true;  // K vs K
        
        // K+minor vs K
        if (totalNonKings == 1) {
            if (m_counts[WHITE_KNIGHT] == 1 || m_counts[BLACK_KNIGHT] == 1) return true;
            if (m_counts[WHITE_BISHOP] == 1 || m_counts[BLACK_BISHOP] == 1) return true;
        }
        
        // K+NN vs K (usually draw)
        if (totalNonKings == 2) {
            if (m_counts[WHITE_KNIGHT] == 2 || m_counts[BLACK_KNIGHT] == 2) return true;
        }
        
        // KB vs KB with same colored bishops
        if (totalNonKings == 2 && 
            m_counts[WHITE_BISHOP] == 1 && 
            m_counts[BLACK_BISHOP] == 1) {
            return isSameColoredBishops();
        }
        
        return false;
    }
    
    [[nodiscard]] bool isSameColoredBishops() const noexcept {
        // This requires board position info, will be checked in Board class
        // For now, return false and let Board handle it
        return false;
    }
    
    void clear() noexcept {
        m_counts.fill(0);
        m_values_mg[WHITE] = Score::zero();
        m_values_mg[BLACK] = Score::zero();
        m_values_eg[WHITE] = Score::zero();
        m_values_eg[BLACK] = Score::zero();
    }
    
    bool operator==(const Material& other) const noexcept {
        return m_counts == other.m_counts && 
               m_values_mg[WHITE] == other.m_values_mg[WHITE] &&
               m_values_mg[BLACK] == other.m_values_mg[BLACK] &&
               m_values_eg[WHITE] == other.m_values_eg[WHITE] &&
               m_values_eg[BLACK] == other.m_values_eg[BLACK];
    }
    
    bool operator!=(const Material& other) const noexcept {
        return !(*this == other);
    }
    
#ifdef DEBUG
    void verify() const noexcept {
        // Verify internal consistency for both MG and EG values
        Score white_calc_mg = Score::zero();
        Score black_calc_mg = Score::zero();
        Score white_calc_eg = Score::zero();
        Score black_calc_eg = Score::zero();
        
        // Verify middlegame values
        white_calc_mg += PIECE_VALUES_MG[PAWN] * count(WHITE, PAWN);
        white_calc_mg += PIECE_VALUES_MG[KNIGHT] * count(WHITE, KNIGHT);
        white_calc_mg += PIECE_VALUES_MG[BISHOP] * count(WHITE, BISHOP);
        white_calc_mg += PIECE_VALUES_MG[ROOK] * count(WHITE, ROOK);
        white_calc_mg += PIECE_VALUES_MG[QUEEN] * count(WHITE, QUEEN);
        
        black_calc_mg += PIECE_VALUES_MG[PAWN] * count(BLACK, PAWN);
        black_calc_mg += PIECE_VALUES_MG[KNIGHT] * count(BLACK, KNIGHT);
        black_calc_mg += PIECE_VALUES_MG[BISHOP] * count(BLACK, BISHOP);
        black_calc_mg += PIECE_VALUES_MG[ROOK] * count(BLACK, ROOK);
        black_calc_mg += PIECE_VALUES_MG[QUEEN] * count(BLACK, QUEEN);
        
        // Verify endgame values
        white_calc_eg += PIECE_VALUES_EG[PAWN] * count(WHITE, PAWN);
        white_calc_eg += PIECE_VALUES_EG[KNIGHT] * count(WHITE, KNIGHT);
        white_calc_eg += PIECE_VALUES_EG[BISHOP] * count(WHITE, BISHOP);
        white_calc_eg += PIECE_VALUES_EG[ROOK] * count(WHITE, ROOK);
        white_calc_eg += PIECE_VALUES_EG[QUEEN] * count(WHITE, QUEEN);
        
        black_calc_eg += PIECE_VALUES_EG[PAWN] * count(BLACK, PAWN);
        black_calc_eg += PIECE_VALUES_EG[KNIGHT] * count(BLACK, KNIGHT);
        black_calc_eg += PIECE_VALUES_EG[BISHOP] * count(BLACK, BISHOP);
        black_calc_eg += PIECE_VALUES_EG[ROOK] * count(BLACK, ROOK);
        black_calc_eg += PIECE_VALUES_EG[QUEEN] * count(BLACK, QUEEN);
        
        assert(white_calc_mg == m_values_mg[WHITE]);
        assert(black_calc_mg == m_values_mg[BLACK]);
        assert(white_calc_eg == m_values_eg[WHITE]);
        assert(black_calc_eg == m_values_eg[BLACK]);
        
        // Verify reasonable piece counts
        for (int i = 0; i < 12; ++i) {
            assert(m_counts[i] >= 0);
            assert(m_counts[i] <= 10);
        }
    }
#endif
    
private:
    std::array<int8_t, 12> m_counts{};  // Piece counts indexed by Piece enum
    std::array<Score, 2> m_values_mg{Score::zero(), Score::zero()};  // Middlegame material values per side
    std::array<Score, 2> m_values_eg{Score::zero(), Score::zero()};  // Endgame material values per side
};

} // namespace seajay::eval