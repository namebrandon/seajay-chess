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

    // King attack scaling (applied to offensive king-safety evaluation)
    int kingAttackScale = 2;            // Percentage boost (2 = default boost for king attack scoring)

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
    bool usePasserPhaseP4 = false;               // Advanced passed-pawn scaling (Phase P4)
    bool profileSquareAttacks = false;           // Instrument MoveGenerator::isSquareAttacked usage

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
