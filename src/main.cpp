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

int main(int argc, char* argv[]) {
    try {
        // Check for command-line arguments first
        if (argc > 1) {
            std::string command(argv[1]);
            
            if (command == "bench") {
                // Run benchmark and exit (for OpenBench compatibility)
                seajay::UCIEngine engine;
                int depth = 0;
                if (argc > 2) {
                    try {
                        depth = std::stoi(argv[2]);
                    } catch (...) {
                        depth = 0;
                    }
                }
                engine.runBenchmark(depth);
                return 0;
            }
        }
        
        // No arguments or unrecognized argument - start normal UCI loop
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