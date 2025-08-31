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
    [[nodiscard]] static MgEgScore value(PieceType pt, Square sq, Color c) noexcept {
        // For black pieces, mirror the square vertically (rank mirroring)
        // Use XOR 56 for efficient rank mirroring as per plan
        Square lookupSq = (c == WHITE) ? sq : (sq ^ 56);
        MgEgScore val = s_pstTables[pt][lookupSq];
        // Return raw value - board.cpp handles the perspective
        return val;
    }
    
    // Get the PST difference for moving a piece from one square to another
    [[nodiscard]] static MgEgScore diff(PieceType pt, Square from, Square to, Color c) noexcept {
        return value(pt, to, c) - value(pt, from, c);
    }
    
    // Get raw PST value without color adjustment (for testing)
    [[nodiscard]] static MgEgScore rawValue(PieceType pt, Square sq) noexcept {
        return s_pstTables[pt][sq];
    }
    
    // SPSA tuning interface - update endgame value for a specific square
    static void updateEndgameValue(PieceType pt, Square sq, int value) noexcept {
        s_pstTables[pt][sq].eg = Score(value);
    }
    
    // Update with symmetry - updates both left and right sides
    static void updateEndgameSymmetric(PieceType pt, Square sq, int value) noexcept {
        // Update the square itself
        s_pstTables[pt][sq].eg = Score(value);
        
        // Update symmetric square (left-right mirror)
        File f = fileOf(sq);
        Rank r = rankOf(sq);
        File symFile = File(7 - f);
        Square symSq = makeSquare(symFile, r);
        s_pstTables[pt][symSq].eg = Score(value);
    }
    
    // SPSA parameter update from UCI
    static void updateFromUCIParam(const std::string& param, int value) noexcept;
    
    // Debug: dump current PST values
    static void dumpTables() noexcept;
    
    // Validate PST tables at compile time (public for static_assert)
    static consteval bool validateTables() noexcept {
        // Ensure no pawn values on 1st/8th ranks
        for (int i = A1; i <= H1; ++i) {
            Square sq = static_cast<Square>(i);
            if (s_defaultPSTTables[PAWN][sq].mg.value() != 0) return false;
        }
        for (int i = A8; i <= H8; ++i) {
            Square sq = static_cast<Square>(i);
            if (s_defaultPSTTables[PAWN][sq].mg.value() != 0) return false;
        }
        return true;
    }
    
    // Reset to default values
    static void resetToDefaults() noexcept {
        s_pstTables = s_defaultPSTTables;
    }
    
private:
    // Mutable PST tables for SPSA tuning
    static inline std::array<std::array<MgEgScore, 64>, 6> s_pstTables = {
        // Pawn table - stronger advancement bonus in endgame (passed pawns critical)
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
            MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(10, 15), MgEgScore(20, 25),
            MgEgScore(20, 25), MgEgScore(10, 15), MgEgScore(5, 5), MgEgScore(5, 5),
            // Rank 5
            MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(20, 30), MgEgScore(30, 40),
            MgEgScore(30, 40), MgEgScore(20, 30), MgEgScore(10, 15), MgEgScore(10, 15),
            // Rank 6 - passed pawns more valuable in endgame
            MgEgScore(20, 35), MgEgScore(20, 35), MgEgScore(30, 50), MgEgScore(40, 60),
            MgEgScore(40, 60), MgEgScore(30, 50), MgEgScore(20, 35), MgEgScore(20, 35),
            // Rank 7 - near promotion critical in endgame
            MgEgScore(50, 90), MgEgScore(50, 90), MgEgScore(50, 90), MgEgScore(50, 90),
            MgEgScore(50, 90), MgEgScore(50, 90), MgEgScore(50, 90), MgEgScore(50, 90),
            // Rank 8 - pawns should never be here (promotion)
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0)
        },
        
        // Knight table - slightly weaker in endgames but edge penalties reduced
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(-50, -40), MgEgScore(-40, -30), MgEgScore(-30, -25), MgEgScore(-30, -25),
            MgEgScore(-30, -25), MgEgScore(-30, -25), MgEgScore(-40, -30), MgEgScore(-50, -40),
            // Rank 2
            MgEgScore(-40, -30), MgEgScore(-20, -15), MgEgScore(0, 0), MgEgScore(5, 5),
            MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(-20, -15), MgEgScore(-40, -30),
            // Rank 3
            MgEgScore(-30, -25), MgEgScore(5, 5), MgEgScore(10, 10), MgEgScore(15, 12),
            MgEgScore(15, 12), MgEgScore(10, 10), MgEgScore(5, 5), MgEgScore(-30, -25),
            // Rank 4
            MgEgScore(-30, -25), MgEgScore(0, 0), MgEgScore(15, 12), MgEgScore(20, 15),
            MgEgScore(20, 15), MgEgScore(15, 12), MgEgScore(0, 0), MgEgScore(-30, -25),
            // Rank 5
            MgEgScore(-30, -25), MgEgScore(5, 5), MgEgScore(15, 12), MgEgScore(20, 15),
            MgEgScore(20, 15), MgEgScore(15, 12), MgEgScore(5, 5), MgEgScore(-30, -25),
            // Rank 6
            MgEgScore(-30, -25), MgEgScore(0, 0), MgEgScore(10, 10), MgEgScore(15, 12),
            MgEgScore(15, 12), MgEgScore(10, 10), MgEgScore(0, 0), MgEgScore(-30, -25),
            // Rank 7
            MgEgScore(-40, -30), MgEgScore(-20, -15), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-20, -15), MgEgScore(-40, -30),
            // Rank 8
            MgEgScore(-50, -40), MgEgScore(-40, -30), MgEgScore(-30, -25), MgEgScore(-30, -25),
            MgEgScore(-30, -25), MgEgScore(-30, -25), MgEgScore(-40, -30), MgEgScore(-50, -40)
        },
        
        // Bishop table - bishops stronger in open endgame positions
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(-20, -10), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-10, -5),
            MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-20, -10),
            // Rank 2
            MgEgScore(-10, -5), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(-10, -5),
            // Rank 3
            MgEgScore(-10, -5), MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(-10, -5),
            // Rank 4
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(10, 15), MgEgScore(15, 20),
            MgEgScore(15, 20), MgEgScore(10, 15), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 5
            MgEgScore(-10, -5), MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(15, 20),
            MgEgScore(15, 20), MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(-10, -5),
            // Rank 6
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 7
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 8
            MgEgScore(-20, -10), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-10, -5),
            MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-20, -10)
        },
        
        // Rook table - rooks dominate endgames, especially on 7th rank
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 10), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(5, 10), MgEgScore(0, 0), MgEgScore(0, 0),
            // Rank 2
            MgEgScore(-5, 0), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-5, 0),
            // Rank 3
            MgEgScore(-5, 0), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-5, 0),
            // Rank 4
            MgEgScore(-5, 5), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10),
            MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(-5, 5),
            // Rank 5
            MgEgScore(-5, 5), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10),
            MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(-5, 5),
            // Rank 6
            MgEgScore(-5, 5), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10),
            MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(-5, 5),
            // Rank 7 - rooks love the 7th rank, especially in endgames
            MgEgScore(10, 25), MgEgScore(10, 25), MgEgScore(10, 25), MgEgScore(10, 25),
            MgEgScore(10, 25), MgEgScore(10, 25), MgEgScore(10, 25), MgEgScore(10, 25),
            // Rank 8
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0)
        },
        
        // Queen table - queens need more activity in endgames
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(-20, -5), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-5, 0),
            MgEgScore(-5, 0), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-20, -5),
            // Rank 2
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 3
            MgEgScore(-10, -5), MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 4
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-5, 0),
            // Rank 5
            MgEgScore(-5, 0), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-5, 0),
            // Rank 6
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 7
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 8
            MgEgScore(-20, -5), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-5, 0),
            MgEgScore(-5, 0), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-20, -5)
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
    
    // Default PST tables for resetting after SPSA tuning (same values as initial)
    static constexpr std::array<std::array<MgEgScore, 64>, 6> s_defaultPSTTables = {
        // Pawn table - stronger advancement bonus in endgame (passed pawns critical)
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
            MgEgScore(5, 5), MgEgScore(5, 5), MgEgScore(10, 15), MgEgScore(20, 25),
            MgEgScore(20, 25), MgEgScore(10, 15), MgEgScore(5, 5), MgEgScore(5, 5),
            // Rank 5
            MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(20, 30), MgEgScore(30, 40),
            MgEgScore(30, 40), MgEgScore(20, 30), MgEgScore(10, 15), MgEgScore(10, 15),
            // Rank 6 - passed pawns more valuable in endgame
            MgEgScore(20, 35), MgEgScore(20, 35), MgEgScore(30, 50), MgEgScore(40, 60),
            MgEgScore(40, 60), MgEgScore(30, 50), MgEgScore(20, 35), MgEgScore(20, 35),
            // Rank 7 - near promotion critical in endgame
            MgEgScore(50, 90), MgEgScore(50, 90), MgEgScore(50, 90), MgEgScore(50, 90),
            MgEgScore(50, 90), MgEgScore(50, 90), MgEgScore(50, 90), MgEgScore(50, 90),
            // Rank 8 - pawns should never be here (promotion)
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0)
        },
        
        // Knight table - slightly weaker in endgames but edge penalties reduced
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(-50, -40), MgEgScore(-40, -30), MgEgScore(-30, -25), MgEgScore(-30, -25),
            MgEgScore(-30, -25), MgEgScore(-30, -25), MgEgScore(-40, -30), MgEgScore(-50, -40),
            // Rank 2
            MgEgScore(-40, -30), MgEgScore(-20, -15), MgEgScore(0, 0), MgEgScore(5, 5),
            MgEgScore(5, 5), MgEgScore(0, 0), MgEgScore(-20, -15), MgEgScore(-40, -30),
            // Rank 3
            MgEgScore(-30, -25), MgEgScore(5, 5), MgEgScore(10, 10), MgEgScore(15, 12),
            MgEgScore(15, 12), MgEgScore(10, 10), MgEgScore(5, 5), MgEgScore(-30, -25),
            // Rank 4
            MgEgScore(-30, -25), MgEgScore(0, 0), MgEgScore(15, 12), MgEgScore(20, 15),
            MgEgScore(20, 15), MgEgScore(15, 12), MgEgScore(0, 0), MgEgScore(-30, -25),
            // Rank 5
            MgEgScore(-30, -25), MgEgScore(5, 5), MgEgScore(15, 12), MgEgScore(20, 15),
            MgEgScore(20, 15), MgEgScore(15, 12), MgEgScore(5, 5), MgEgScore(-30, -25),
            // Rank 6
            MgEgScore(-30, -25), MgEgScore(0, 0), MgEgScore(10, 10), MgEgScore(15, 12),
            MgEgScore(15, 12), MgEgScore(10, 10), MgEgScore(0, 0), MgEgScore(-30, -25),
            // Rank 7
            MgEgScore(-40, -30), MgEgScore(-20, -15), MgEgScore(0, 0), MgEgScore(0, 0),
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(-20, -15), MgEgScore(-40, -30),
            // Rank 8
            MgEgScore(-50, -40), MgEgScore(-40, -30), MgEgScore(-30, -25), MgEgScore(-30, -25),
            MgEgScore(-30, -25), MgEgScore(-30, -25), MgEgScore(-40, -30), MgEgScore(-50, -40)
        },
        
        // Bishop table - bishops stronger in open endgame positions
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(-20, -10), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-10, -5),
            MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-20, -10),
            // Rank 2
            MgEgScore(-10, -5), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(-10, -5),
            // Rank 3
            MgEgScore(-10, -5), MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(10, 15), MgEgScore(-10, -5),
            // Rank 4
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(10, 15), MgEgScore(15, 20),
            MgEgScore(15, 20), MgEgScore(10, 15), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 5
            MgEgScore(-10, -5), MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(15, 20),
            MgEgScore(15, 20), MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(-10, -5),
            // Rank 6
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 7
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 8
            MgEgScore(-20, -10), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-10, -5),
            MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-20, -10)
        },
        
        // Rook table - rooks dominate endgames, especially on 7th rank
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 10), MgEgScore(10, 15),
            MgEgScore(10, 15), MgEgScore(5, 10), MgEgScore(0, 0), MgEgScore(0, 0),
            // Rank 2
            MgEgScore(-5, 0), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-5, 0),
            // Rank 3
            MgEgScore(-5, 0), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-5, 0),
            // Rank 4
            MgEgScore(-5, 5), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10),
            MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(-5, 5),
            // Rank 5
            MgEgScore(-5, 5), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10),
            MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(-5, 5),
            // Rank 6
            MgEgScore(-5, 5), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10),
            MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(0, 10), MgEgScore(-5, 5),
            // Rank 7 - rooks love the 7th rank, especially in endgames
            MgEgScore(10, 25), MgEgScore(10, 25), MgEgScore(10, 25), MgEgScore(10, 25),
            MgEgScore(10, 25), MgEgScore(10, 25), MgEgScore(10, 25), MgEgScore(10, 25),
            // Rank 8
            MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(0, 0), MgEgScore(0, 0), MgEgScore(0, 0)
        },
        
        // Queen table - queens need more activity in endgames
        std::array<MgEgScore, 64>{
            // Rank 1
            MgEgScore(-20, -5), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-5, 0),
            MgEgScore(-5, 0), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-20, -5),
            // Rank 2
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 3
            MgEgScore(-10, -5), MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 4
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-5, 0),
            // Rank 5
            MgEgScore(-5, 0), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-5, 0),
            // Rank 6
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(5, 10), MgEgScore(5, 10),
            MgEgScore(5, 10), MgEgScore(5, 10), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 7
            MgEgScore(-10, -5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5),
            MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(0, 5), MgEgScore(-10, -5),
            // Rank 8
            MgEgScore(-20, -5), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-5, 0),
            MgEgScore(-5, 0), MgEgScore(-10, -5), MgEgScore(-10, -5), MgEgScore(-20, -5)
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