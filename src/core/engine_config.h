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
    
    // Futility Pruning configuration (Phase 4 investigation)
    bool useFutilityPruning = true;     // Enable/disable futility pruning
    int futilityMaxDepth = 4;           // Maximum depth for futility pruning (default: 4, tested optimal)
    int futilityBase = 150;             // Base margin for futility pruning
    int futilityScale = 60;             // Scale factor per depth for futility margin
    
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