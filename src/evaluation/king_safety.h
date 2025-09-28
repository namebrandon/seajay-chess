#pragma once

#include "../core/bitboard.h"
#include "../core/types.h"
#include "types.h"  // For Score type

namespace seajay::eval {

// King safety evaluation class
// Phase KS1: Infrastructure only - provides structure but no scoring yet
class KingSafety {
public:
    // Phase-dependent scoring values (using 4ku reference)
    // S(mg, eg) format where mg = middlegame, eg = endgame
    struct KingSafetyParams {
        // Direct shield pawns (one square in front)
        int directShieldMg = 28;   // Middlegame value
        int directShieldEg = -8;   // Endgame value (negative = less important)

        // Advanced shield pawns (two squares in front)
        int advancedShieldMg = 12;  // Middlegame value
        int advancedShieldEg = -3;  // Endgame value

        // Missing shield penalties
        int missingDirectPenaltyMg = 26;
        int missingDirectPenaltyEg = 6;
        int missingAdvancedPenaltyMg = 10;
        int missingAdvancedPenaltyEg = 3;

        // Prophylaxis bonus for king with air squares (luft)
        int airSquareBonusMg = 4;
        int airSquareBonusEg = 1;

        // File exposure penalties
        int semiOpenFilePenaltyMg = 18;
        int semiOpenFilePenaltyEg = 4;
        int openFilePenaltyMg = 28;
        int openFilePenaltyEg = 6;
        int rookOnOpenFilePenaltyMg = 38;
        int rookOnOpenFilePenaltyEg = 10;

        // Attacked king ring squares
        int attackedRingPenaltyMg = 8;
        int attackedRingPenaltyEg = 3;

        // Piece proximity penalties
        int minorProximityPenaltyMg = 11;
        int minorProximityPenaltyEg = 4;
        int majorProximityPenaltyMg = 16;
        int majorProximityPenaltyEg = 7;
        int queenContactPenaltyMg = 20;
        int queenContactPenaltyEg = 8;

        // Enable scoring toggle (kept for UCI compatibility)
        int enableScoring = 1;
    };
    
    // Evaluate king safety for a given side
    // Returns score from white's perspective
    static Score evaluate(const Board& board, Color side);
    
    // Helper functions for shield detection
    static Bitboard getShieldPawns(const Board& board, Color side, Square kingSquare);
    static Bitboard getAdvancedShieldPawns(const Board& board, Color side, Square kingSquare);
    
    // Phase A4: Check for air squares (luft) created by pawn moves
    static bool hasAirSquares(const Board& board, Color side, Square kingSquare);
    
    // Check if king is in a reasonable position (castled or near-castled)
    static bool isReasonableKingPosition(Square kingSquare, Color side);
    
    // Get the current parameters (for UCI tuning later)
    static const KingSafetyParams& getParams() { return s_params; }
    static void setParams(const KingSafetyParams& params) { s_params = params; }
    
private:
    // Reasonable king squares mask (from 4ku: 0xC3D7)
    // These are typical castled positions
    // White: a1, b1, c1, g1, h1, a2, b2, g2
    // Black: a8, b8, c8, g8, h8, a7, b7, g7
    static constexpr Bitboard REASONABLE_KING_SQUARES_WHITE = 0xC7;    // Rank 1: a1,b1,c1,g1,h1 + Rank 2: a2,b2,g2
    static constexpr Bitboard REASONABLE_KING_SQUARES_BLACK = 0xC700000000000000ULL; // Same pattern on rank 7-8
    
    // Shield zones for different king positions
    static Bitboard getShieldZone(Square kingSquare, Color side);
    
    // Parameters (will be tunable via UCI in later phases)
    static KingSafetyParams s_params;
};

} // namespace seajay::eval
