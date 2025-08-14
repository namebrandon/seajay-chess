/**
 * Basic TT functionality test - demonstrates hit rate with repeated positions
 */

#include <iostream>
#include "core/transposition_table.h"
#include "core/types.h"

using namespace seajay;

int main() {
    std::cout << "Testing Transposition Table Basic Functionality\n";
    std::cout << "===============================================\n\n";
    
    TranspositionTable tt(1);  // 1MB table
    
    // Test 1: Basic store and retrieve
    {
        uint64_t key = 0x123456789ABCDEF0ULL;
        Move move = makeMove(E2, E4, NORMAL);
        int16_t score = 100;
        int16_t evalScore = 50;
        uint8_t depth = 10;
        
        tt.store(key, move, score, evalScore, depth, Bound::EXACT);
        
        TTEntry* entry = tt.probe(key);
        if (entry) {
            std::cout << "✓ Basic store/retrieve successful\n";
            std::cout << "  Stored score: " << score << ", Retrieved: " << entry->score << "\n";
            std::cout << "  Stored depth: " << (int)depth << ", Retrieved: " << (int)entry->depth << "\n";
        } else {
            std::cout << "✗ Failed to retrieve entry\n";
        }
    }
    
    // Test 2: Hit rate with repeated positions
    {
        tt.resetStats();
        
        // Store 100 unique positions
        for (uint64_t i = 0; i < 100; i++) {
            Move move = makeMove(0, i % 64, NORMAL);
            tt.store(i, move, i, 0, 5, Bound::EXACT);
        }
        
        // Probe the same 100 positions (should all hit)
        for (uint64_t i = 0; i < 100; i++) {
            tt.probe(i);
        }
        
        // Probe 100 new positions (should all miss)
        for (uint64_t i = 100; i < 200; i++) {
            tt.probe(i);
        }
        
        auto& stats = tt.stats();
        std::cout << "\n✓ Hit rate test:\n";
        std::cout << "  Total probes: " << stats.probes << "\n";
        std::cout << "  Total hits: " << stats.hits << "\n";
        std::cout << "  Hit rate: " << stats.hitRate() << "%\n";
        std::cout << "  Expected: ~50% (100 hits / 200 probes)\n";
    }
    
    // Test 3: Collision handling
    {
        tt.clear();
        tt.resetStats();
        
        // Create keys that will likely collide in a small table
        uint64_t base = 0x1000;
        Move move = makeMove(E2, E4, NORMAL);
        
        // Store entries with keys that differ only in upper bits
        for (int i = 0; i < 10; i++) {
            uint64_t key = base + (static_cast<uint64_t>(i) << 32);
            tt.store(key, move, 100 + i, 50, 10, Bound::EXACT);
        }
        
        auto& stats = tt.stats();
        std::cout << "\n✓ Collision test:\n";
        std::cout << "  Stores: " << stats.stores << "\n";
        std::cout << "  Collisions: " << stats.collisions << "\n";
        if (stats.collisions > 0) {
            std::cout << "  Collision rate: " << (100.0 * stats.collisions / stats.stores) << "%\n";
        }
    }
    
    // Test 4: Enable/disable functionality
    {
        tt.clear();
        uint64_t key = 0xDEADBEEF;
        Move move = makeMove(D2, D4, NORMAL);
        
        tt.store(key, move, 100, 50, 10, Bound::EXACT);
        TTEntry* entry = tt.probe(key);
        bool enabledHit = (entry != nullptr);
        
        tt.setEnabled(false);
        entry = tt.probe(key);
        bool disabledHit = (entry != nullptr);
        
        tt.setEnabled(true);
        entry = tt.probe(key);
        bool reenabledHit = (entry != nullptr);
        
        std::cout << "\n✓ Enable/disable test:\n";
        std::cout << "  Enabled hit: " << (enabledHit ? "YES" : "NO") << " (expected: YES)\n";
        std::cout << "  Disabled hit: " << (disabledHit ? "YES" : "NO") << " (expected: NO)\n";
        std::cout << "  Re-enabled hit: " << (reenabledHit ? "YES" : "NO") << " (expected: YES)\n";
    }
    
    // Test 5: Generation management
    {
        tt.clear();
        uint64_t key = 0xCAFEBABE;
        Move move1 = makeMove(E2, E4, NORMAL);
        Move move2 = makeMove(D2, D4, NORMAL);
        
        tt.store(key, move1, 100, 50, 10, Bound::EXACT);
        TTEntry* entry = tt.probe(key);
        uint8_t gen1 = entry ? entry->generation() : 255;
        
        tt.newSearch();
        tt.store(key, move2, 200, 60, 12, Bound::LOWER);
        entry = tt.probe(key);
        uint8_t gen2 = entry ? entry->generation() : 255;
        
        std::cout << "\n✓ Generation test:\n";
        std::cout << "  Gen 1: " << (int)gen1 << "\n";
        std::cout << "  Gen 2: " << (int)gen2 << "\n";
        std::cout << "  Gen 2 = Gen 1 + 1? " << (gen2 == ((gen1 + 1) & 0x3F) ? "YES" : "NO") << "\n";
    }
    
    // Final statistics
    std::cout << "\n===============================================\n";
    std::cout << "Final TT Statistics:\n";
    auto& finalStats = tt.stats();
    std::cout << "  Total probes: " << finalStats.probes << "\n";
    std::cout << "  Total hits: " << finalStats.hits << "\n";
    std::cout << "  Overall hit rate: " << finalStats.hitRate() << "%\n";
    std::cout << "  Total stores: " << finalStats.stores << "\n";
    std::cout << "  Total collisions: " << finalStats.collisions << "\n";
    std::cout << "  Table size: " << tt.sizeInMB() << " MB\n";
    std::cout << "  Table entries: " << tt.size() << "\n";
    std::cout << "  Fill rate: " << tt.fillRate() << "%\n";
    std::cout << "  Hash full: " << tt.hashfull() << "/1000\n";
    
    return 0;
}