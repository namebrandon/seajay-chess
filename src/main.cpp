#include <iostream>
#include "uci/uci.h"

/**
 * SeaJay Chess Engine - Main Entry Point
 * 
 * Stage 3 Implementation:
 * - Complete UCI protocol handler
 * - Legal move generation (99.974% accuracy)
 * - Random move selection for gameplay
 * - GUI compatibility (Arena, Cute Chess, etc.)
 */

int main() {
    try {
        seajay::UCIEngine engine;
        engine.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}