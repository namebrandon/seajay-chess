#pragma once

#include "types.h"
#include "../core/types.h"
#include "../core/bitboard.h"
#include <array>
#include <cassert>

namespace seajay::eval {

// Default piece values (can be overridden via UCI)
inline std::array<Score, 6> PIECE_VALUES = {
    Score(100),   // PAWN
    Score(320),   // KNIGHT  
    Score(330),   // BISHOP (slightly > knight)
    Score(510),   // ROOK (updated based on expert feedback)
    Score(950),   // QUEEN (updated based on expert feedback)
    Score(0)      // KING (not counted in material)
};

// UCI interface to update piece values
inline void setPieceValue(PieceType pt, int value) {
    if (pt >= PAWN && pt <= QUEEN) {
        PIECE_VALUES[pt] = Score(value);
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
                m_values[c] += PIECE_VALUES[pt];
            } else {
                m_values[c] -= PIECE_VALUES[pt];
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
        const Score white_val = m_values[WHITE];
        const Score black_val = m_values[BLACK];
        return stm == WHITE ? white_val - black_val : black_val - white_val;
    }
    
    [[nodiscard]] constexpr Score value(Color c) const noexcept {
        return m_values[c];
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
        // Manually iterate through piece types
        total += PIECE_VALUES[KNIGHT] * count(c, KNIGHT);
        total += PIECE_VALUES[BISHOP] * count(c, BISHOP);
        total += PIECE_VALUES[ROOK] * count(c, ROOK);
        total += PIECE_VALUES[QUEEN] * count(c, QUEEN);
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
        m_values[WHITE] = Score::zero();
        m_values[BLACK] = Score::zero();
    }
    
    bool operator==(const Material& other) const noexcept {
        return m_counts == other.m_counts && 
               m_values[WHITE] == other.m_values[WHITE] &&
               m_values[BLACK] == other.m_values[BLACK];
    }
    
    bool operator!=(const Material& other) const noexcept {
        return !(*this == other);
    }
    
#ifdef DEBUG
    void verify() const noexcept {
        // Verify internal consistency
        Score white_calc = Score::zero();
        Score black_calc = Score::zero();
        
        // Manually iterate through piece types for verification
        white_calc += PIECE_VALUES[PAWN] * count(WHITE, PAWN);
        white_calc += PIECE_VALUES[KNIGHT] * count(WHITE, KNIGHT);
        white_calc += PIECE_VALUES[BISHOP] * count(WHITE, BISHOP);
        white_calc += PIECE_VALUES[ROOK] * count(WHITE, ROOK);
        white_calc += PIECE_VALUES[QUEEN] * count(WHITE, QUEEN);
        
        black_calc += PIECE_VALUES[PAWN] * count(BLACK, PAWN);
        black_calc += PIECE_VALUES[KNIGHT] * count(BLACK, KNIGHT);
        black_calc += PIECE_VALUES[BISHOP] * count(BLACK, BISHOP);
        black_calc += PIECE_VALUES[ROOK] * count(BLACK, ROOK);
        black_calc += PIECE_VALUES[QUEEN] * count(BLACK, QUEEN);
        
        assert(white_calc == m_values[WHITE]);
        assert(black_calc == m_values[BLACK]);
        
        // Verify reasonable piece counts
        for (int i = 0; i < 12; ++i) {
            assert(m_counts[i] >= 0);
            assert(m_counts[i] <= 10);
        }
    }
#endif
    
private:
    std::array<int8_t, 12> m_counts{};  // Piece counts indexed by Piece enum
    std::array<Score, 2> m_values{Score::zero(), Score::zero()};  // Material values per side
};

} // namespace seajay::eval