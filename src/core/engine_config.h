#pragma once

/**
 * Global Engine Configuration
 * 
 * Runtime configuration options that can be set via UCI
 * to control engine behavior without recompilation.
 */

namespace seajay {

class EngineConfig {
public:
    // Get singleton instance
    static EngineConfig& getInstance() {
        static EngineConfig instance;
        return instance;
    }
    
    // Delete copy constructor and assignment
    EngineConfig(const EngineConfig&) = delete;
    EngineConfig& operator=(const EngineConfig&) = delete;
    
    // Configuration options
    bool useMagicBitboards = true;  // Stage 10: Default ON for 79x speedup
    bool usePSTInterpolation = true; // PST Phase Interpolation: Default ON for smooth evaluation tapering
    
    // Futility Pruning configuration (SPSA-tuned for 1.1M NPS)
    bool useFutilityPruning = true;     // Enable/disable futility pruning
    int futilityMaxDepth = 7;           // Maximum depth for futility pruning (extended from 4 to 7 for Phase 1.1)
    int futilityBase = 150;             // Base margin for futility pruning (reset to original for exponential scaling)
    int futilityScale = 79;             // Scale factor per depth for futility margin (SPSA-tuned from 60)
    int futilitySeeMargin = 40;         // SEE guard margin (cp) to skip futility when tactical opportunities exist

    // King attack scaling (applied to offensive king-safety evaluation)
    int kingAttackScale = 2;            // Percentage boost (2 = default boost for king attack scoring)

    // Null-move desperation guard
    int nullMoveDesperationMargin = 0;   // Skip null move when static eval trails alpha by this margin (cp)

    // Phase SE1: Singular extension runtime controls (mirrored from UCI for SPSA sweeps)
    bool useSingularExtensions = true;           // Master singular toggle (enabled post-SE4 tuning)
    bool allowStackedExtensions = true;          // Permit recapture stacking alongside singular
    bool bypassSingularTTExact = false;          // Debug hook to bypass TT exact cutoffs in verification
    bool disableCheckDuringSingular = false;     // Suppress check extensions during verification nodes
    int singularDepthMin = 7;                    // Minimum depth threshold for singular verification
    int singularMarginBase = 51;                 // Margin scaling factor (cp)
    int singularVerificationReduction = 4;       // Depth reduction when launching verification search
    int singularExtensionDepth = 2;              // Extension plies applied on singular fail-low

    // Evaluation experimentation toggles
    bool usePasserPhaseP4 = true;                // Advanced passed-pawn scaling (Phase P4)
    bool profileSquareAttacks = false;           // Instrument MoveGenerator::isSquareAttacked usage

    // Passed pawn Phase P4 tuning parameters (SPSA-ready via UCI)
    int passerPathFreeBonus = 4;                 // Bonus when promotion path is empty
    int passerPathSafeBonus = -1;                // Bonus when enemy lacks control on path squares
    int passerPathDefendedBonus = -1;            // Bonus when own pieces control path squares
    int passerPathAttackedPenalty = 22;          // Penalty when enemy controls path squares
    int passerStopDefendedBonus = 13;            // Bonus when stop square is defended
    int passerStopAttackedPenalty = 15;          // Penalty when stop square is attacked
    int passerRookSupportBonus = 9;              // Bonus for friendly rook/queen behind passer
    int passerEnemyRookBehindPenalty = 0;        // Penalty for enemy heavy piece behind passer
    int passerKingDistanceScale = 10;            // Scale for king distance differential

    // Semi-open liability tuning parameters
    int semiOpenLiabilityPenalty = 12;           // Extra penalty when enemy heavies target isolated/backward pawns
    int semiOpenGuardRebate = 4;                 // Rebate when friendly pawns still guard the file

    // Loose pawn tuning parameters
    int loosePawnOwnHalfPenalty = 6;             // Penalty for unsupported pawns on our side of the board
    int loosePawnEnemyHalfPenalty = 13;          // Penalty for unsupported pawns in enemy territory
    int loosePawnPhalanxRebate = 3;              // Rebate when adjacent friendly pawn forms a phalanx

    // Passed pawn phalanx tuning parameters
    int passerPhalanxSupportBonus = 3;           // Bonus when a passer has a same-rank phalanx mate
    int passerPhalanxAdvanceBonus = 16;          // Additional bonus when the phalanx can safely advance
    int passerPhalanxRookBonus = 4;              // Bonus when a rook/queen backs a phalanx pair

    // Candidate passed pawn tuning parameters
    int candidateLeverBaseBonus = 5;             // Base bonus for a lever-based pawn candidate
    int candidateLeverAdvanceBonus = 9;          // Extra when the lever can push safely
    int candidateLeverSupportBonus = 5;          // Bonus for sufficient support ratio
    int candidateLeverRankBonus = 4;             // Bonus per rank beyond origin

    // Bishop/pawn color complex parameters
    int bishopColorHarmonyBonus = 2;             // Bonus per opposite-color pawn pairing
    int bishopColorTensionPenalty = 2;           // Penalty per same-color pawn pairing
    int bishopColorBlockedPenalty = 3;           // Penalty per blocked central pawn sharing bishop color

    // Pawn span & tension parameters
    int pawnInfiltrationBonus = 25;              // Bonus per heavy piece penetrating enemy pawn span
    int pawnTensionPenalty = 3;                  // Penalty per pawn tension instance
    int pawnPushThreatBonus = 6;                 // Bonus per safe single-step pawn push threat

    // Threat evaluation tuning parameters
    int threatHangingPawnBonus = 12;             // Bonus when a pawn hangs to a cheaper attacker
    int threatHangingKnightBonus = 18;           // Bonus when a knight hangs to a cheaper attacker
    int threatHangingBishopBonus = 18;           // Bonus when a bishop hangs to a cheaper attacker
    int threatHangingRookBonus = 26;             // Bonus when a rook hangs to a cheaper attacker
    int threatHangingQueenBonus = 40;            // Bonus when a queen hangs to a cheaper attacker
    int threatDoublePawnBonus = 8;               // Bonus when a pawn is forked by a cheaper attacker
    int threatDoubleKnightBonus = 14;            // Bonus when a knight is forked by a cheaper attacker
    int threatDoubleBishopBonus = 14;            // Bonus when a bishop is forked by a cheaper attacker
    int threatDoubleRookBonus = 22;              // Bonus when a rook is forked by a cheaper attacker
    int threatDoubleQueenBonus = 32;             // Bonus when a queen is forked by a cheaper attacker

    // QS3 king-safety experimentation toggles
    bool useQS3KingSafety = true;                // Enable enhanced king-danger heuristics for queen sacs
    int qs3SafeQueenContactPenalty = 48;         // Penalty per safe queen contact-check (cp)
    int qs3ShieldHolePenalty = 28;               // Penalty per missing flank shield square (cp)
    int qs3SliderSupportPenalty = 20;            // Extra penalty when sliders support the contact square (cp)
    int qs3NoMinorDefenderPenalty = 24;          // Flat penalty when no minor defends the king ring (cp)
    int qs3KingExposurePenalty = 32;             // Additional penalty when king stays exposed after contact capture
    int qs3AttackerCompensationPercent = 60;     // Percentage of defender danger penalty granted back to attacker

    // Future options can be added here
    // bool useMVVLVA = true;        // Stage 11
    // etc.

private:
    EngineConfig() = default;
};

// Convenience function
inline EngineConfig& getConfig() {
    return EngineConfig::getInstance();
}

} // namespace seajay
