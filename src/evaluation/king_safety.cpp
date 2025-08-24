#include "king_safety.h"
#include "../core/board.h"
#include "../core/bitboard.h"
#include "../search/game_phase.h"  // Phase KS3: For phase tapering

namespace seajay::eval {

// Initialize static parameters
// Phase KS3: Enable scoring with initial values from 4ku
KingSafety::KingSafetyParams KingSafety::s_params = {
    .directShieldMg = 33,    // S(33, -10) midgame value
    .directShieldEg = -10,   // S(33, -10) endgame value  
    .advancedShieldMg = 25,  // S(25, -7) midgame value
    .advancedShieldEg = -7,  // S(25, -7) endgame value
    .enableScoring = 1       // Phase KS3: ENABLED
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
    
    // Apply phase-appropriate values
    switch (phase) {
        case search::GamePhase::OPENING:
        case search::GamePhase::MIDDLEGAME:
            // Use middlegame values (king safety more important)
            rawScore = directCount * s_params.directShieldMg + 
                      advancedCount * s_params.advancedShieldMg;
            break;
            
        case search::GamePhase::ENDGAME:
            // Use endgame values (king safety less important, can be negative)
            rawScore = directCount * s_params.directShieldEg + 
                      advancedCount * s_params.advancedShieldEg;
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
    // Get pawns two squares in front of the king
    Bitboard friendlyPawns = board.pieces(side, PAWN);
    
    // Get the shield zone and shift it forward one rank
    Bitboard shieldZone = getShieldZone(kingSquare, side);
    Bitboard advancedZone = (side == WHITE) ? shieldZone << 8 : shieldZone >> 8;
    
    return friendlyPawns & advancedZone;
}

bool KingSafety::isReasonableKingPosition(Square kingSquare, Color side) {
    // 4ku's exact mask: 0xC3D7 for white
    // This covers: a1,b1,c1,d1,e1,g1,h1 on rank 1 + a2,b2,g2,h2 on rank 2
    
    Bitboard kingBit = 1ULL << kingSquare;
    
    if (side == WHITE) {
        // Use 4ku's exact mask
        return (kingBit & 0xC3D7ULL) != 0;
    } else {  // BLACK
        // Mirror of white's mask for rank 7-8
        // 0xC3D7 shifted to ranks 7-8
        constexpr Bitboard BLACK_REASONABLE = 0xD7C3000000000000ULL;
        return (kingBit & BLACK_REASONABLE) != 0;
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
            return 0xE00000000000ULL;
        } else {
            // Queenside: a7, b7, c7
            return 0x70000000000ULL;
        }
    }
}

} // namespace seajay::eval