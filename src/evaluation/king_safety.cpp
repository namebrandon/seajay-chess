#include "king_safety.h"
#include "../core/board.h"
#include "../core/bitboard.h"

namespace seajay::eval {

// Initialize static parameters
KingSafety::KingSafetyParams KingSafety::s_params;

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
    
    // Calculate raw score (will be multiplied by enableScoring)
    // Using simple midgame values for now (no phase tapering yet)
    int rawScore = directCount * s_params.directShieldMg + 
                   advancedCount * s_params.advancedShieldMg;
    
    // Phase KS1: Multiply by 0 to disable scoring
    // Phase KS3: Will set enableScoring to 1
    int finalScore = rawScore * s_params.enableScoring;
    
    // Return score from white's perspective
    return Score(side == WHITE ? finalScore : -finalScore);
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
    // Check if king is in a typical castled position
    // Based on 4ku's 0xC3D7 mask approach
    
    if (side == WHITE) {
        // White king reasonable positions:
        // Queenside: a1, b1, c1, a2, b2
        // Kingside: g1, h1, g2
        Bitboard kingBit = 1ULL << kingSquare;
        
        // Check rank 1 positions
        if (rankOf(kingSquare) == 0) {  // Rank 1
            // a1, b1, c1, g1, h1
            return (kingBit & 0xC7ULL) != 0;
        }
        // Check rank 2 positions
        else if (rankOf(kingSquare) == 1) {  // Rank 2
            // a2, b2, g2
            return (fileOf(kingSquare) <= 1 || fileOf(kingSquare) == 6);
        }
    } else {  // BLACK
        // Black king reasonable positions:
        // Queenside: a8, b8, c8, a7, b7
        // Kingside: g8, h8, g7
        Bitboard kingBit = 1ULL << kingSquare;
        
        // Check rank 8 positions
        if (rankOf(kingSquare) == 7) {  // Rank 8
            // a8, b8, c8, g8, h8
            return (kingBit & 0xC700000000000000ULL) != 0;
        }
        // Check rank 7 positions
        else if (rankOf(kingSquare) == 6) {  // Rank 7
            // a7, b7, g7
            return (fileOf(kingSquare) <= 1 || fileOf(kingSquare) == 6);
        }
    }
    
    return false;
}

Bitboard KingSafety::getShieldZone(Square kingSquare, Color side) {
    // Define the shield zone based on king position
    // This follows 4ku's approach with adjustment for king file
    
    int file = fileOf(kingSquare);
    int rank = rankOf(kingSquare);
    
    Bitboard shieldZone = 0ULL;
    
    if (side == WHITE) {
        // For white, shield is in front (higher ranks)
        if (rank < 7) {  // Can have shield
            int shieldRank = rank + 1;
            
            // Determine shield files based on king position
            if (file <= 2) {
                // Queenside castle or near - shield on files a,b,c
                shieldZone = 0x7ULL << (shieldRank * 8);  // Files a,b,c
            } else if (file >= 5) {
                // Kingside castle or near - shield on files f,g,h
                shieldZone = 0xE0ULL << (shieldRank * 8);  // Files f,g,h
            } else {
                // Central king - shield on king file and adjacent files
                if (file > 0) shieldZone |= 1ULL << (shieldRank * 8 + file - 1);
                shieldZone |= 1ULL << (shieldRank * 8 + file);
                if (file < 7) shieldZone |= 1ULL << (shieldRank * 8 + file + 1);
            }
        }
    } else {  // BLACK
        // For black, shield is in front (lower ranks)
        if (rank > 0) {  // Can have shield
            int shieldRank = rank - 1;
            
            // Determine shield files based on king position
            if (file <= 2) {
                // Queenside castle or near - shield on files a,b,c
                shieldZone = 0x7ULL << (shieldRank * 8);  // Files a,b,c
            } else if (file >= 5) {
                // Kingside castle or near - shield on files f,g,h
                shieldZone = 0xE0ULL << (shieldRank * 8);  // Files f,g,h
            } else {
                // Central king - shield on king file and adjacent files
                if (file > 0) shieldZone |= 1ULL << (shieldRank * 8 + file - 1);
                shieldZone |= 1ULL << (shieldRank * 8 + file);
                if (file < 7) shieldZone |= 1ULL << (shieldRank * 8 + file + 1);
            }
        }
    }
    
    return shieldZone;
}

} // namespace seajay::eval