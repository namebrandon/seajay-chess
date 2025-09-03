#include "king_safety.h"
#include "../core/board.h"
#include "../core/bitboard.h"
#include "../search/game_phase.h"  // Phase KS3: For phase tapering

namespace seajay::eval {

// Initialize static parameters - ALL VALUES CONTROLLED VIA UCI
// Use setoption to modify these values:
// - KingSafetyDirectShieldMg (default 16)
// - KingSafetyAdvancedShieldMg (default 12)  
// - KingSafetyEnableScoring (default 1)
KingSafety::KingSafetyParams KingSafety::s_params = {
    .directShieldMg = 16,     // UCI controlled
    .directShieldEg = -3,     // Endgame values less important
    .advancedShieldMg = 12,   // UCI controlled
    .advancedShieldEg = -2,   // Endgame values less important
    .airSquareBonusMg = 2,    // Air square bonus
    .airSquareBonusEg = 0,    // Not relevant in endgame
    .enableScoring = 1        // UCI controlled (0=disabled, 1=enabled)
};

Score KingSafety::evaluate(const Board& board, Color side) {
    // Phase KS1: Infrastructure phase - compute but don't score
    // This validates the detection logic without affecting evaluation
    
    Square kingSquare = board.kingSquare(side);
    
    // Check if king is in a reasonable position
    if (!isReasonableKingPosition(kingSquare, side)) {
        return Score(0);  // No safety bonus for exposed kings
    }
    
    // Get shield pawns
    Bitboard directShield = getShieldPawns(board, side, kingSquare);
    Bitboard advancedShield = getAdvancedShieldPawns(board, side, kingSquare);
    
    // Count shield pawns
    int directCount = popCount(directShield);
    int advancedCount = popCount(advancedShield);
    
    // Phase KS3: Implement phase-based scoring
    // Detect game phase for proper tapering
    search::GamePhase phase = search::detectGamePhase(board);
    
    int rawScore = 0;
    
    // Phase A4: Check for air squares (prophylaxis)
    bool hasLuft = hasAirSquares(board, side, kingSquare);
    
    // Apply phase-appropriate values
    switch (phase) {
        case search::GamePhase::OPENING:
        case search::GamePhase::MIDDLEGAME:
            // Use middlegame values (king safety more important)
            rawScore = directCount * s_params.directShieldMg + 
                      advancedCount * s_params.advancedShieldMg;
            // Phase A4: Add tiny bonus for air squares in middlegame
            if (hasLuft) {
                rawScore += s_params.airSquareBonusMg;
            }
            break;
            
        case search::GamePhase::ENDGAME:
            // Use endgame values (king safety less important, can be negative)
            rawScore = directCount * s_params.directShieldEg + 
                      advancedCount * s_params.advancedShieldEg;
            // Phase A4: Air squares not relevant in endgame
            break;
    }
    
    // Phase KS3: enableScoring is now 1, so scoring is active
    int finalScore = rawScore * s_params.enableScoring;
    
    // Return positive score for good king safety (caller handles perspective)
    return Score(finalScore);
}

Bitboard KingSafety::getShieldPawns(const Board& board, Color side, Square kingSquare) {
    // Get pawns directly in front of the king (one square ahead)
    Bitboard friendlyPawns = board.pieces(side, PAWN);
    Bitboard shieldZone = getShieldZone(kingSquare, side);
    
    return friendlyPawns & shieldZone;
}

Bitboard KingSafety::getAdvancedShieldPawns(const Board& board, Color side, Square kingSquare) {
    // Get pawns two ranks in front of the king
    // For white: shield is rank 2, advanced is rank 3
    // For black: shield is rank 7, advanced is rank 6
    Bitboard friendlyPawns = board.pieces(side, PAWN);
    
    // Get the shield zone and shift it forward one rank (toward enemy)
    Bitboard shieldZone = getShieldZone(kingSquare, side);
    // "north" in 4ku means toward rank 8, so for both colors it's << 8
    // But for black, "advanced" means toward white (rank 6), so >> 8
    Bitboard advancedZone = (side == WHITE) ? shieldZone << 8 : shieldZone >> 8;
    
    return friendlyPawns & advancedZone;
}

bool KingSafety::isReasonableKingPosition(Square kingSquare, Color side) {
    // Use shared masks from header for castled/near-castled positions
    Bitboard kingBit = 1ULL << kingSquare;
    if (side == WHITE) {
        return (kingBit & REASONABLE_KING_SQUARES_WHITE) != 0;
    } else {
        return (kingBit & REASONABLE_KING_SQUARES_BLACK) != 0;
    }
}

Bitboard KingSafety::getShieldZone(Square kingSquare, Color side) {
    // 4ku's exact approach: const u64 shield = 0x700 << 5 * (file > 2);
    // 0x700 = files a,b,c on rank 2 (for white)
    // Shift by 5 files if king is on kingside (file > 2)
    
    int file = fileOf(kingSquare);
    
    if (side == WHITE) {
        // 4ku logic: shield is ALWAYS on rank 2
        // 0x700 = a2,b2,c2 for queenside
        // 0x700 << 5 = 0xE000 = f2,g2,h2 for kingside
        if (file > 2) {
            // Kingside: f2, g2, h2
            return 0xE000ULL;
        } else {
            // Queenside: a2, b2, c2
            return 0x700ULL;
        }
    } else {  // BLACK
        // Mirror for black: shield is on rank 7
        if (file > 2) {
            // Kingside: f7, g7, h7
            return 0xE0000000000000ULL;
        } else {
            // Queenside: a7, b7, c7
            return 0x7000000000000ULL;
        }
    }
}

bool KingSafety::hasAirSquares(const Board& board, Color side, Square kingSquare) {
    // Phase A4: Check if king has "air" squares (luft) created by pawn moves
    // This detects moves like h2-h3, g2-g3 (for white) that create escape squares
    // We're looking for friendly pawns that have moved forward from the king's zone
    
    // Only check for castled positions
    if (!isReasonableKingPosition(kingSquare, side)) {
        return false;
    }
    
    int file = fileOf(kingSquare);
    Bitboard friendlyPawns = board.pieces(side, PAWN);
    
    if (side == WHITE) {
        // For white, recognize typical luft pawn advances near the king
        if (file > 4) {  // Kingside (f,g,h files)
            // f2-f3, g2-g3, h2-h3
            Bitboard luftPawns = friendlyPawns & ((1ULL << F3) | (1ULL << G3) | (1ULL << H3));
            return luftPawns != 0;
        } else if (file < 3) {  // Queenside (a,b,c files)
            // a2-a3, b2-b3, c2-c3
            Bitboard luftPawns = friendlyPawns & ((1ULL << A3) | (1ULL << B3) | (1ULL << C3));
            return luftPawns != 0;
        }
    } else {  // BLACK
        if (file > 4) {  // Kingside
            // f7-f6, g7-g6, h7-h6
            Bitboard luftPawns = friendlyPawns & ((1ULL << F6) | (1ULL << G6) | (1ULL << H6));
            return luftPawns != 0;
        } else if (file < 3) {  // Queenside
            // a7-a6, b7-b6, c7-c6
            Bitboard luftPawns = friendlyPawns & ((1ULL << A6) | (1ULL << B6) | (1ULL << C6));
            return luftPawns != 0;
        }
    }
    
    return false;
}

} // namespace seajay::eval
