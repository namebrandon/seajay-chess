#pragma once

#include "../core/types.h"
#include "types.h"
#include <array>
#include <cstdint>

namespace seajay::eval {

// Piece-Square Table infrastructure for positional evaluation
// Stage 9 implementation following modern C++20 best practices

// Score for both game phases (middlegame and endgame)
// For Stage 9, we only use middlegame values but structure supports future tapering
struct MgEgScore {
    Score mg;  // middlegame score
    Score eg;  // endgame score
    
    constexpr MgEgScore() noexcept : mg(Score::zero()), eg(Score::zero()) {}
    constexpr MgEgScore(Score middlegame, Score endgame) noexcept 
        : mg(middlegame), eg(endgame) {}
    
    // Single value constructor (same for both phases)
    constexpr explicit MgEgScore(int value) noexcept 
        : mg(Score(value)), eg(Score(value)) {}
    
    // Two-value constructor for different phases
    constexpr MgEgScore(int mgValue, int egValue) noexcept 
        : mg(Score(mgValue)), eg(Score(egValue)) {}
    
    constexpr MgEgScore operator+(MgEgScore rhs) const noexcept {
        return MgEgScore(mg + rhs.mg, eg + rhs.eg);
    }
    
    constexpr MgEgScore operator-(MgEgScore rhs) const noexcept {
        return MgEgScore(mg - rhs.mg, eg - rhs.eg);
    }
    
    constexpr MgEgScore operator-() const noexcept {
        return MgEgScore(-mg, -eg);
    }
    
    constexpr MgEgScore& operator+=(MgEgScore rhs) noexcept {
        mg += rhs.mg;
        eg += rhs.eg;
        return *this;
    }
    
    constexpr MgEgScore& operator-=(MgEgScore rhs) noexcept {
        mg -= rhs.mg;
        eg -= rhs.eg;
        return *this;
    }
    
    constexpr bool operator==(const MgEgScore&) const noexcept = default;
};

// PST manager class - provides static access to piece-square tables
class PST {
public:
    // Get PST value for a piece on a square
    // Automatically handles color perspective (white positive, black negative)
    [[nodiscard]] static constexpr MgEgScore value(PieceType pt, Square sq, Color c) noexcept {
        // For black pieces, mirror the square vertically (rank mirroring)
        // Use XOR 56 for efficient rank mirroring as per plan
        Square lookupSq = (c == WHITE) ? sq : (sq ^ 56);
        MgEgScore val = s_pstTables[pt][lookupSq];
        // CRITICAL FIX: Negate PST values for Black pieces
        // Black pieces on good squares should decrease White's evaluation
        return (c == WHITE) ? val : -val;
    }
    
    // Get the PST difference for moving a piece from one square to another
    [[nodiscard]] static constexpr MgEgScore diff(PieceType pt, Square from, Square to, Color c) noexcept {
        return value(pt, to, c) - value(pt, from, c);
    }
    
    // Get raw PST value without color adjustment (for testing)
    [[nodiscard]] static constexpr MgEgScore rawValue(PieceType pt, Square sq) noexcept {
        return s_pstTables[pt][sq];
    }
    
    // Validate PST tables at compile time (public for static_assert)
    static consteval bool validateTables() noexcept {
        // Ensure no pawn values on 1st/8th ranks
        for (int i = A1; i <= H1; ++i) {
            Square sq = static_cast<Square>(i);
            if (s_pstTables[PAWN][sq].mg.value() != 0) return false;
        }
        for (int i = A8; i <= H8; ++i) {
            Square sq = static_cast<Square>(i);
            if (s_pstTables[PAWN][sq].mg.value() != 0) return false;
        }
        return true;
    }
    
private:
    // Piece-square tables indexed by [PieceType][Square]
    // Values are from white's perspective on squares A1-H8
    // These will be defined in pst.cpp with actual values
    static constexpr std::array<std::array<MgEgScore, 64>, 6> s_pstTables = {
        // Pawn table
        std::array<MgEgScore, 64>{
            // Rank 1 - pawns should never be here
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            // Rank 2
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-5, -5),
            MgEgScore(-5, -5), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            // Rank 3
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 5), MgEgScore(10, 10),
            MgEgScore(10, 10), MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(0, 0),
            // Rank 4
            MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(10, 10), MgEgScore(20, 20),
            MgEgScore(20, 20), MgEgScore(10, 10), MgEgScore(5, 5), MgEgScore(5, 5),
            // Rank 5
            MgEgScore(10, 10), MgEgScore(10, 10), MgEgScore(20, 20), MgEgScore(30, 30),
            MgEgScore(30, 30), MgEgScore(20, 20), MgEgScore(10, 10), MgEgScore(10, 10),
            // Rank 6
            MgEgScore(20, 20), MgEgScore(20, 20), MgEgScore(30, 30), MgEgScore(40, 40),
            MgEgScore(40, 40), MgEgScore(30, 30), MgEgScore(20, 20), MgEgScore(20, 20),
            // Rank 7
            MgEgScore(50, 50), MgEgScore(50, 50), MgEgScore(50, 50), MgEgScore(50, 50),
            MgEgScore(50, 50), MgEgScore(50, 50), MgEgScore(50, 50), MgEgScore(50, 50),
            // Rank 8 - pawns should never be here (promotion)
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0)
        },
        
        // Knight table - knights love the center
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(-50, -50), MgEgScore(-40, -40), MgEgScore(-30, -30), MgEgScore(-30, -30),
            MgEgScore(-30, -30), MgEgScore(-30, -30), MgEgScore(-40, -40), MgEgScore(-50, -50),
            // Rank 2
            MgEgScore(-40, -40), MgEgScore(-20, -20), MgEgScore(0, 0), MgEgScore(5, 5),
            MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(-20, -20), MgEgScore(-40, -40),
            // Rank 3
            MgEgScore(-30, -30), MgEgScore(5, 5), MgEgScore(10, 10), MgEgScore(15, 15),
            MgEgScore(15, 15), MgEgScore(10, 10), MgEgScore(5, 5), MgEgScore(-30, -30),
            // Rank 4
            MgEgScore(-30, -30), MgEgScore(0, 0), MgEgScore(15, 15), MgEgScore(20, 20),
            MgEgScore(20, 20), MgEgScore(15, 15), MgEgScore(0, 0), MgEgScore(-30, -30),
            // Rank 5
            MgEgScore(-30, -30), MgEgScore(5, 5), MgEgScore(15, 15), MgEgScore(20, 20),
            MgEgScore(20, 20), MgEgScore(15, 15), MgEgScore(5, 5), MgEgScore(-30, -30),
            // Rank 6
            MgEgScore(-30, -30), MgEgScore(0, 0), MgEgScore(10, 10), MgEgScore(15, 15),
            MgEgScore(15, 15), MgEgScore(10, 10), MgEgScore(0, 0), MgEgScore(-30, -30),
            // Rank 7
            MgEgScore(-40, -40), MgEgScore(-20, -20), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-20, -20), MgEgScore(-40, -40),
            // Rank 8
            MgEgScore(-50, -50), MgEgScore(-40, -40), MgEgScore(-30, -30), MgEgScore(-30, -30),
            MgEgScore(-30, -30), MgEgScore(-30, -30), MgEgScore(-40, -40), MgEgScore(-50, -50)
        },
        
        // Bishop table - bishops like long diagonals
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(-20, -20), MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-10, -10),
            MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-20, -20),
            // Rank 2
            MgEgScore(-10, -10), MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 5), MgEgScore(-10, -10),
            // Rank 3
            MgEgScore(-10, -10), MgEgScore(10, 10), MgEgScore(10, 10), MgEgScore(10, 10),
            MgEgScore(10, 10), MgEgScore(10, 10), MgEgScore(10, 10), MgEgScore(-10, -10),
            // Rank 4
            MgEgScore(-10, -10), MgEgScore(0, 0), MgEgScore(10, 10), MgEgScore(15, 15),
            MgEgScore(15, 15), MgEgScore(10, 10), MgEgScore(0, 0), MgEgScore(-10, -10),
            // Rank 5
            MgEgScore(-10, -10), MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(15, 15),
            MgEgScore(15, 15), MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(-10, -10),
            // Rank 6
            MgEgScore(-10, -10), MgEgScore(0, 0), MgEgScore(5, 5), MgEgScore(10, 10),
            MgEgScore(10, 10), MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(-10, -10),
            // Rank 7
            MgEgScore(-10, -10), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-10, -10),
            // Rank 8
            MgEgScore(-20, -20), MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-10, -10),
            MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-20, -20)
        },
        
        // Rook table - rooks like 7th rank and open files
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 5), MgEgScore(10, 10),
            MgEgScore(10, 10), MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(0, 0),
            // Rank 2
            MgEgScore(-5, -5), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-5, -5),
            // Rank 3
            MgEgScore(-5, -5), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-5, -5),
            // Rank 4
            MgEgScore(-5, -5), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-5, -5),
            // Rank 5
            MgEgScore(-5, -5), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-5, -5),
            // Rank 6
            MgEgScore(-5, -5), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-5, -5),
            // Rank 7 - rooks love the 7th rank
            MgEgScore(10, 10), MgEgScore(10, 10), MgEgScore(10, 10), MgEgScore(10, 10),
            MgEgScore(10, 10), MgEgScore(10, 10), MgEgScore(10, 10), MgEgScore(10, 10),
            // Rank 8
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 5),
            MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0)
        },
        
        // Queen table - queens like center control
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(-20, -20), MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-5, -5),
            MgEgScore(-5, -5), MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-20, -20),
            // Rank 2
            MgEgScore(-10, -10), MgEgScore(0, 0), MgEgScore(5, 5), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-10, -10),
            // Rank 3
            MgEgScore(-10, -10), MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(5, 5),
            MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(-10, -10),
            // Rank 4
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 5), MgEgScore(5, 5),
            MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(-5, -5),
            // Rank 5
            MgEgScore(-5, -5), MgEgScore(0, 0), MgEgScore(5, 5), MgEgScore(5, 5),
            MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(-5, -5),
            // Rank 6
            MgEgScore(-10, -10), MgEgScore(0, 0), MgEgScore(5, 5), MgEgScore(5, 5),
            MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(-10, -10),
            // Rank 7
            MgEgScore(-10, -10), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-10, -10),
            // Rank 8
            MgEgScore(-20, -20), MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-5, -5),
            MgEgScore(-5, -5), MgEgScore(-10, -10), MgEgScore(-10, -10), MgEgScore(-20, -20)
        },
        
        // King table - encourage castling and corner safety in middlegame
        std::array<MgEgScore, 64>{
            // Rank 1 - encourage castling positions
            MgEgScore(20, -30), MgEgScore(30, -10), MgEgScore(10, -10), MgEgScore(-20, -20),
            MgEgScore(-30, -30), MgEgScore(-30, -20), MgEgScore(20, -10), MgEgScore(20, -30),
            // Rank 2
            MgEgScore(20, -20), MgEgScore(20, 0), MgEgScore(0, 0), MgEgScore(-20, -10),
            MgEgScore(-20, -10), MgEgScore(0, 0), MgEgScore(20, 0), MgEgScore(20, -20),
            // Rank 3
            MgEgScore(-10, -10), MgEgScore(-20, 10), MgEgScore(-20, 20), MgEgScore(-20, 30),
            MgEgScore(-20, 30), MgEgScore(-20, 20), MgEgScore(-20, 10), MgEgScore(-10, -10),
            // Rank 4
            MgEgScore(-20, -10), MgEgScore(-30, 20), MgEgScore(-30, 30), MgEgScore(-40, 40),
            MgEgScore(-40, 40), MgEgScore(-30, 30), MgEgScore(-30, 20), MgEgScore(-20, -10),
            // Rank 5
            MgEgScore(-30, -10), MgEgScore(-40, 20), MgEgScore(-40, 30), MgEgScore(-50, 40),
            MgEgScore(-50, 40), MgEgScore(-40, 30), MgEgScore(-40, 20), MgEgScore(-30, -10),
            // Rank 6
            MgEgScore(-30, -20), MgEgScore(-40, 10), MgEgScore(-40, 20), MgEgScore(-50, 30),
            MgEgScore(-50, 30), MgEgScore(-40, 20), MgEgScore(-40, 10), MgEgScore(-30, -20),
            // Rank 7
            MgEgScore(-30, -30), MgEgScore(-40, -10), MgEgScore(-40, 0), MgEgScore(-50, 10),
            MgEgScore(-50, 10), MgEgScore(-40, 0), MgEgScore(-40, -10), MgEgScore(-30, -30),
            // Rank 8
            MgEgScore(-30, -50), MgEgScore(-30, -30), MgEgScore(-30, -20), MgEgScore(-30, -10),
            MgEgScore(-30, -10), MgEgScore(-30, -20), MgEgScore(-30, -30), MgEgScore(-30, -50)
        }
    };
};

// Compile-time validation
static_assert(PST::validateTables(), "PST tables have invalid values");

// Global PST object removed - causes static initialization issues
// The PST class uses static methods and static data members internally
// alignas(64) inline constexpr auto pstTables = PST();  // REMOVED: Bug #010 fix

} // namespace seajay::eval