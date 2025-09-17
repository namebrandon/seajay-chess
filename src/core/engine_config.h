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

    // Fast evaluation toggles (Phase 3)
    bool useFastEvalForQsearch = false;   // Gate pawn cache usage in qsearch stand-pat
    bool useFastEvalForPruning = false;   // Gate pawn cache usage in pruning heuristics
    
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
