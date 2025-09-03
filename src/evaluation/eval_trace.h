#pragma once

#include "../core/types.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

namespace seajay::eval {

// Structure to trace all evaluation components
struct EvalTrace {
    // Material and PST
    Score material;
    Score pst;
    Score pstMg;  // Middlegame PST component
    Score pstEg;  // Endgame PST component
    int phase256;  // Phase value 0-256 (0=endgame, 256=middlegame)
    
    // Pawn structure
    Score passedPawns;
    Score isolatedPawns;
    Score doubledPawns;
    Score backwardPawns;
    Score pawnIslands;
    
    // Piece evaluation
    Score bishopPair;
    Score mobility;
    Score knightOutposts;
    Score rookFiles;
    Score rookKingProximity;
    
    // King safety
    Score kingSafety;
    
    // Individual component tracking for passed pawns
    struct PassedPawnDetail {
        int whiteCount = 0;
        int blackCount = 0;
        Score whiteBonus;
        Score blackBonus;
        bool hasProtected = false;
        bool hasConnected = false;
        bool hasBlockaded = false;
        bool hasUnstoppable = false;
    } passedDetail;
    
    // Individual component tracking for mobility
    struct MobilityDetail {
        int whiteKnightMoves = 0;
        int whiteBishopMoves = 0;
        int whiteRookMoves = 0;
        int whiteQueenMoves = 0;
        int blackKnightMoves = 0;
        int blackBishopMoves = 0;
        int blackRookMoves = 0;
        int blackQueenMoves = 0;
    } mobilityDetail;
    
    // Reset all values
    void reset() {
        material = Score(0);
        pst = Score(0);
        pstMg = Score(0);
        pstEg = Score(0);
        phase256 = 0;
        
        passedPawns = Score(0);
        isolatedPawns = Score(0);
        doubledPawns = Score(0);
        backwardPawns = Score(0);
        pawnIslands = Score(0);
        
        bishopPair = Score(0);
        mobility = Score(0);
        knightOutposts = Score(0);
        rookFiles = Score(0);
        rookKingProximity = Score(0);
        
        kingSafety = Score(0);
        
        passedDetail = PassedPawnDetail();
        mobilityDetail = MobilityDetail();
    }
    
    // Calculate total score
    Score total() const {
        return material + pst + passedPawns + isolatedPawns + doubledPawns + 
               backwardPawns + pawnIslands + bishopPair + mobility + 
               knightOutposts + rookFiles + rookKingProximity + kingSafety;
    }
    
    // Print formatted evaluation breakdown
    void print(Color sideToMove) const {
        std::cout << "\n=== Evaluation Breakdown ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        
        // Helper to format scores
        auto formatScore = [](Score s) -> std::string {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2);
            double cp = s.value() / 100.0;
            if (cp >= 0) ss << "+";
            ss << cp;
            return ss.str();
        };
        
        // Phase information
        std::cout << "\nGame Phase: " << phase256 << "/256 ";
        std::cout << "(MG: " << (phase256 * 100 / 256) << "%, ";
        std::cout << "EG: " << ((256 - phase256) * 100 / 256) << "%)" << std::endl;
        
        // Material
        std::cout << "\n-- Material --" << std::endl;
        std::cout << "  Material:          " << std::setw(8) << formatScore(material) << " cp" << std::endl;
        
        // PST with interpolation details
        std::cout << "\n-- Piece Square Tables --" << std::endl;
        if (phase256 < 256) {  // Show interpolation if not pure middlegame
            std::cout << "  PST (MG):          " << std::setw(8) << formatScore(pstMg) << " cp" << std::endl;
            std::cout << "  PST (EG):          " << std::setw(8) << formatScore(pstEg) << " cp" << std::endl;
            std::cout << "  PST (interpolated):" << std::setw(8) << formatScore(pst) << " cp";
            std::cout << " [" << phase256 << "*MG + " << (256-phase256) << "*EG / 256]" << std::endl;
        } else {
            std::cout << "  PST:               " << std::setw(8) << formatScore(pst) << " cp" << std::endl;
        }
        
        // Pawn structure
        std::cout << "\n-- Pawn Structure --" << std::endl;
        if (passedPawns != Score(0)) {
            std::cout << "  Passed pawns:      " << std::setw(8) << formatScore(passedPawns) << " cp";
            if (passedDetail.whiteCount > 0 || passedDetail.blackCount > 0) {
                std::cout << " (W:" << passedDetail.whiteCount << " B:" << passedDetail.blackCount;
                if (passedDetail.hasProtected) std::cout << ", protected";
                if (passedDetail.hasConnected) std::cout << ", connected";
                if (passedDetail.hasBlockaded) std::cout << ", blockaded";
                if (passedDetail.hasUnstoppable) std::cout << ", unstoppable!";
                std::cout << ")";
            }
            std::cout << std::endl;
        }
        if (isolatedPawns != Score(0)) {
            std::cout << "  Isolated pawns:   " << std::setw(8) << formatScore(isolatedPawns) << " cp" << std::endl;
        }
        if (doubledPawns != Score(0)) {
            std::cout << "  Doubled pawns:     " << std::setw(8) << formatScore(doubledPawns) << " cp" << std::endl;
        }
        if (backwardPawns != Score(0)) {
            std::cout << "  Backward pawns:    " << std::setw(8) << formatScore(backwardPawns) << " cp" << std::endl;
        }
        if (pawnIslands != Score(0)) {
            std::cout << "  Pawn islands:      " << std::setw(8) << formatScore(pawnIslands) << " cp" << std::endl;
        }
        
        // Piece evaluation
        std::cout << "\n-- Piece Evaluation --" << std::endl;
        if (bishopPair != Score(0)) {
            std::cout << "  Bishop pair:       " << std::setw(8) << formatScore(bishopPair) << " cp" << std::endl;
        }
        if (mobility != Score(0)) {
            std::cout << "  Mobility:          " << std::setw(8) << formatScore(mobility) << " cp";
            std::cout << " (WN:" << mobilityDetail.whiteKnightMoves;
            std::cout << " WB:" << mobilityDetail.whiteBishopMoves;
            std::cout << " WR:" << mobilityDetail.whiteRookMoves;
            std::cout << " WQ:" << mobilityDetail.whiteQueenMoves;
            std::cout << " | BN:" << mobilityDetail.blackKnightMoves;
            std::cout << " BB:" << mobilityDetail.blackBishopMoves;
            std::cout << " BR:" << mobilityDetail.blackRookMoves;
            std::cout << " BQ:" << mobilityDetail.blackQueenMoves << ")" << std::endl;
        }
        if (knightOutposts != Score(0)) {
            std::cout << "  Knight outposts:   " << std::setw(8) << formatScore(knightOutposts) << " cp" << std::endl;
        }
        if (rookFiles != Score(0)) {
            std::cout << "  Rook on files:     " << std::setw(8) << formatScore(rookFiles) << " cp" << std::endl;
        }
        if (rookKingProximity != Score(0)) {
            std::cout << "  Rook-King prox:    " << std::setw(8) << formatScore(rookKingProximity) << " cp" << std::endl;
        }
        
        // King safety
        if (kingSafety != Score(0)) {
            std::cout << "\n-- King Safety --" << std::endl;
            std::cout << "  King safety:       " << std::setw(8) << formatScore(kingSafety) << " cp" << std::endl;
        }
        
        // Total calculation
        std::cout << "\n-- Total (White perspective) --" << std::endl;
        Score totalWhite = total();
        std::cout << "  Sum of components: " << std::setw(8) << formatScore(totalWhite) << " cp" << std::endl;
        
        // Show calculation
        std::cout << "\n  Calculation: ";
        bool first = true;
        auto addComponent = [&](const std::string& name, Score value) {
            if (value == Score(0)) return;
            if (!first) std::cout << " + ";
            std::cout << name << "(" << formatScore(value) << ")";
            first = false;
        };
        
        addComponent("mat", material);
        addComponent("pst", pst);
        addComponent("passed", passedPawns);
        addComponent("isolated", isolatedPawns);
        addComponent("doubled", doubledPawns);
        addComponent("backward", backwardPawns);
        addComponent("islands", pawnIslands);
        addComponent("bishop-pair", bishopPair);
        addComponent("mobility", mobility);
        addComponent("outposts", knightOutposts);
        addComponent("rook-files", rookFiles);
        addComponent("rook-king", rookKingProximity);
        addComponent("king-safety", kingSafety);
        std::cout << std::endl;
        
        // Side to move perspective
        std::cout << "\n-- Final Score --" << std::endl;
        Score finalScore = (sideToMove == WHITE) ? totalWhite : -totalWhite;
        std::cout << "  From " << (sideToMove == WHITE ? "White" : "Black") << "'s perspective: ";
        std::cout << formatScore(finalScore) << " cp" << std::endl;
        
        std::cout << "\n==========================" << std::endl;
    }
};

} // namespace seajay::eval