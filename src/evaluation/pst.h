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
    // Returns the raw PST value - board.cpp handles perspective
    [[nodiscard]] static constexpr MgEgScore value(PieceType pt, Square sq, Color c) noexcept {
        // For black pieces, mirror the square vertically (rank mirroring)
        // Use XOR 56 for efficient rank mirroring as per plan
        Square lookupSq = (c == WHITE) ? sq : (sq ^ 56);
        MgEgScore val = s_pstTables[pt][lookupSq];
        // Return raw value - board.cpp handles the perspective
        return val;
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
        // Pawn table - Phase 2a: Enhanced endgame advancement bonuses for passed pawns
        std::array<MgEgScore, 64>{
            // Rank 1 - pawns should never be here
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            // Rank 2 - slightly favor central pawns
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-5, -5),
            MgEgScore(-5, -5), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            // Rank 3 - modest central preference
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(0, 5),
            // Rank 4 - increased endgame advancement bonus
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(10, 20), MgEgScore(20, 30),
            MgEgScore(20, 30), MgEgScore(10, 20), MgEgScore(5, 10), MgEgScore(5, 10),
            // Rank 5 - stronger endgame push incentive
            MgEgScore(10, 20), MgEgScore(10, 20), MgEgScore(20, 35), MgEgScore(30, 45),
            MgEgScore(30, 45), MgEgScore(20, 35), MgEgScore(10, 20), MgEgScore(10, 20),
            // Rank 6 - passed pawns much more valuable in endgame
            MgEgScore(20, 40), MgEgScore(20, 40), MgEgScore(30, 55), MgEgScore(40, 70),
            MgEgScore(40, 70), MgEgScore(30, 55), MgEgScore(20, 40), MgEgScore(20, 40),
            // Rank 7 - near promotion critical in endgame
            MgEgScore(50, 100), MgEgScore(50, 100), MgEgScore(50, 100), MgEgScore(50, 100),
            MgEgScore(50, 100), MgEgScore(50, 100), MgEgScore(50, 100), MgEgScore(50, 100),
            // Rank 8 - pawns should never be here (promotion)
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0)
        },
        
        // Knight table - Phase 2b: Conservative endgame adjustments (slightly weaker in EG)
        std::array<MgEgScore, 64>{
            // Rank 1 - knights struggle on the rim, less so in endgame
            MgEgScore(-50, -35), MgEgScore(-40, -25), MgEgScore(-30, -20), MgEgScore(-30, -20),
            MgEgScore(-30, -20), MgEgScore(-30, -20), MgEgScore(-40, -25), MgEgScore(-50, -35),
            // Rank 2
            MgEgScore(-40, -25), MgEgScore(-20, -10), MgEgScore(0, -5), MgEgScore(5, 0),
            MgEgScore(5, 0), MgEgScore(0, -5), MgEgScore(-20, -10), MgEgScore(-40, -25),
            // Rank 3
            MgEgScore(-30, -20), MgEgScore(5, 0), MgEgScore(10, 5), MgEgScore(15, 10),
            MgEgScore(15, 10), MgEgScore(10, 5), MgEgScore(5, 0), MgEgScore(-30, -20),
            // Rank 4 - central squares still valuable but less in endgame
            MgEgScore(-30, -20), MgEgScore(0, -5), MgEgScore(15, 10), MgEgScore(20, 12),
            MgEgScore(20, 12), MgEgScore(15, 10), MgEgScore(0, -5), MgEgScore(-30, -20),
            // Rank 5
            MgEgScore(-30, -20), MgEgScore(5, 0), MgEgScore(15, 10), MgEgScore(20, 12),
            MgEgScore(20, 12), MgEgScore(15, 10), MgEgScore(5, 0), MgEgScore(-30, -20),
            // Rank 6
            MgEgScore(-30, -20), MgEgScore(0, -5), MgEgScore(10, 5), MgEgScore(15, 10),
            MgEgScore(15, 10), MgEgScore(10, 5), MgEgScore(0, -5), MgEgScore(-30, -20),
            // Rank 7
            MgEgScore(-40, -25), MgEgScore(-20, -10), MgEgScore(0, -5), MgEgScore(0, -5),
            MgEgScore(0, -5), MgEgScore(0, -5), MgEgScore(-20, -10), MgEgScore(-40, -25),
            // Rank 8
            MgEgScore(-50, -35), MgEgScore(-40, -25), MgEgScore(-30, -20), MgEgScore(-30, -20),
            MgEgScore(-30, -20), MgEgScore(-30, -20), MgEgScore(-40, -25), MgEgScore(-50, -35)
        },
        
        // Bishop table - Phase 2b: Conservative endgame adjustments (slightly stronger in open EG)
        std::array<MgEgScore, 64>{
            // Rank 1 - corners less penalized in endgame
            MgEgScore(-20, -5), MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-10, 0),
            MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-20, -5),
            // Rank 2
            MgEgScore(-10, 0), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(-10, 0),
            // Rank 3
            MgEgScore(-10, 0), MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(-10, 0),
            // Rank 4 - long diagonals more valuable in endgame
            MgEgScore(-10, 0), MgEgScore(0, 5), MgEgScore(10, 15), MgEgScore(15, 22),
            MgEgScore(15, 22), MgEgScore(10, 15), MgEgScore(0, 5), MgEgScore(-10, 0),
            // Rank 5
            MgEgScore(-10, 0), MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(15, 22),
            MgEgScore(15, 22), MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(-10, 0),
            // Rank 6
            MgEgScore(-10, 0), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-10, 0),
            // Rank 7
            MgEgScore(-10, 0), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-10, 0),
            // Rank 8
            MgEgScore(-20, -5), MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-10, 0),
            MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-20, -5)
        },
        
        // Rook table - Phase 2a: Enhanced endgame activity, 7th rank dominance
        std::array<MgEgScore, 64>{
            // Rank 1 - prefer central files for castling flexibility
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 10), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(5, 10), MgEgScore(0, 0), MgEgScore(0, 0),
            // Rank 2 - slight activity bonus in endgame
            MgEgScore(-5, 5), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10),
            MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(-5, 5),
            // Rank 3 - encourage activity
            MgEgScore(-5, 5), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10),
            MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(-5, 5),
            // Rank 4 - more activity in endgame
            MgEgScore(-5, 10), MgEgScore(0, 15), MgEgScore(0, 15), MgEgScore(0, 15),
            MgEgScore(0, 15), MgEgScore(0, 15), MgEgScore(0, 15), MgEgScore(-5, 10),
            // Rank 5 - aggressive placement rewarded
            MgEgScore(-5, 10), MgEgScore(0, 15), MgEgScore(0, 15), MgEgScore(0, 15),
            MgEgScore(0, 15), MgEgScore(0, 15), MgEgScore(0, 15), MgEgScore(-5, 10),
            // Rank 6 - penetration into enemy territory
            MgEgScore(-5, 15), MgEgScore(0, 20), MgEgScore(0, 20), MgEgScore(0, 20),
            MgEgScore(0, 20), MgEgScore(0, 20), MgEgScore(0, 20), MgEgScore(-5, 15),
            // Rank 7 - rooks dominate on 7th rank in endgames
            MgEgScore(15, 35), MgEgScore(15, 35), MgEgScore(15, 35), MgEgScore(15, 35),
            MgEgScore(15, 35), MgEgScore(15, 35), MgEgScore(15, 35), MgEgScore(15, 35),
            // Rank 8 - back rank, prefer central files
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(5, 15),
            MgEgScore(5, 15), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5)
        },
        
        // Queen table - Phase 2b: Conservative endgame adjustments (slightly more active in EG)
        std::array<MgEgScore, 64>{
            // Rank 1 - back rank less penalized in endgame
            MgEgScore(-20, 0), MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-5, 5),
            MgEgScore(-5, 5), MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-20, 0),
            // Rank 2
            MgEgScore(-10, 0), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-10, 0),
            // Rank 3
            MgEgScore(-10, 0), MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-10, 0),
            // Rank 4 - central control slightly more important in endgame
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(5, 12), MgEgScore(5, 12),
            MgEgScore(5, 12), MgEgScore(5, 12), MgEgScore(0, 5), MgEgScore(-5, 2),
            // Rank 5
            MgEgScore(-5, 2), MgEgScore(0, 5), MgEgScore(5, 12), MgEgScore(5, 12),
            MgEgScore(5, 12), MgEgScore(5, 12), MgEgScore(0, 5), MgEgScore(-5, 2),
            // Rank 6 - activity more important in endgame
            MgEgScore(-10, 0), MgEgScore(0, 8), MgEgScore(5, 12), MgEgScore(5, 12),
            MgEgScore(5, 12), MgEgScore(5, 12), MgEgScore(0, 8), MgEgScore(-10, 0),
            // Rank 7
            MgEgScore(-10, 0), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-10, 0),
            // Rank 8
            MgEgScore(-20, 0), MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-5, 5),
            MgEgScore(-5, 5), MgEgScore(-10, 0), MgEgScore(-10, 0), MgEgScore(-20, 0)
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