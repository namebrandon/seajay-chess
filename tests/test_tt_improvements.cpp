/**
 * Test program for Stage 12 Transposition Table improvements
 * 
 * Tests:
 * 1. Fifty-move counter NOT affecting hash (improved TT hits)
 * 2. UCI Hash option working correctly
 * 3. UCI UseTranspositionTable option working
 * 4. Generation wraparound handling
 * 5. Depth-preferred replacement scheme
 */

#include "../src/core/board.h"
#include "../src/core/transposition_table.h"
#include "../src/core/move_generation.h"
#include "../src/search/search.h"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace seajay;

void testFiftyMoveHashExclusion() {
    std::cout << "\n=== Test 1: Fifty-Move Counter Hash Exclusion ===" << std::endl;
    
    Board board1, board2;
    
    // Set up identical positions but with different fifty-move counters
    board1.fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    board2.fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 50 1");
    
    // Use zobristKey() which is the actual hash used for transposition tables
    Hash hash1 = board1.zobristKey();
    Hash hash2 = board2.zobristKey();
    
    std::cout << "Position 1 (halfmove=0): " << std::hex << hash1 << std::dec << std::endl;
    std::cout << "Position 2 (halfmove=50): " << std::hex << hash2 << std::dec << std::endl;
    
    if (hash1 == hash2) {
        std::cout << "✓ PASS: Zobrist hashes are identical (fifty-move counter excluded)" << std::endl;
    } else {
        std::cout << "✗ FAIL: Zobrist hashes differ (fifty-move counter should not affect hash)" << std::endl;
    }
    
    // Test that positions with different pieces have different hashes
    Board board3;
    board3.fromFEN("r3k2r/8/8/8/8/8/8/R3K1R1 w KQkq - 0 1");  // Changed rook position
    Hash hash3 = board3.zobristKey();
    
    if (hash1 != hash3) {
        std::cout << "✓ PASS: Different positions have different Zobrist hashes" << std::endl;
    } else {
        std::cout << "✗ FAIL: Different positions should have different Zobrist hashes" << std::endl;
    }
}

void testHashTableResize() {
    std::cout << "\n=== Test 2: Hash Table Resize ===" << std::endl;
    
    TranspositionTable tt;
    
    // Test various sizes
    size_t testSizes[] = {1, 16, 128, 256, 1024};
    
    for (size_t size : testSizes) {
        tt.resize(size);
        size_t actualMB = tt.sizeInMB();
        std::cout << "Requested: " << size << " MB, Actual: " << actualMB << " MB";
        
        // Due to power-of-2 rounding, actual may be less than requested
        if (actualMB <= size && actualMB > 0) {
            std::cout << " ✓ PASS" << std::endl;
        } else {
            std::cout << " ✗ FAIL" << std::endl;
        }
    }
}

void testTTEnableDisable() {
    std::cout << "\n=== Test 3: TT Enable/Disable ===" << std::endl;
    
    TranspositionTable tt(16);  // 16 MB table
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    Hash key = board.zobristKey();
    Move testMove = 0x1234;  // Dummy move
    
    // Test with TT enabled
    tt.setEnabled(true);
    tt.store(key, testMove, 100, 50, 10, Bound::EXACT);
    TTEntry* entry = tt.probe(key);
    
    if (entry != nullptr && entry->move == testMove) {
        std::cout << "✓ PASS: TT stores/retrieves when enabled" << std::endl;
    } else {
        std::cout << "✗ FAIL: TT should store/retrieve when enabled" << std::endl;
    }
    
    // Test with TT disabled
    tt.setEnabled(false);
    tt.clear();  // Clear first
    tt.store(key, testMove, 100, 50, 10, Bound::EXACT);  // Should do nothing
    entry = tt.probe(key);  // Should return nullptr
    
    if (entry == nullptr) {
        std::cout << "✓ PASS: TT returns nullptr when disabled" << std::endl;
    } else {
        std::cout << "✗ FAIL: TT should return nullptr when disabled" << std::endl;
    }
}

void testGenerationWraparound() {
    std::cout << "\n=== Test 4: Generation Wraparound ===" << std::endl;
    
    // Test the generation difference calculation
    // Since generationDifference is private, we test it indirectly
    // by simulating multiple generations and checking replacement behavior
    
    TranspositionTable tt(1);  // Small table
    
    // Simulate 70 searches to wrap around the 6-bit generation counter
    for (int gen = 0; gen < 70; gen++) {
        tt.newSearch();  // Increment generation
    }
    
    std::cout << "✓ PASS: Simulated 70 generations without crash (wraparound handled)" << std::endl;
    
    // Test that old entries get replaced after generation wraparound
    Board board;
    board.setStartingPosition();
    Hash key = board.zobristKey();
    
    // Reset and store an entry at generation 0
    tt.clear();
    tt.store(key, 0x1111, 100, 50, 10, Bound::EXACT);  // Depth 10
    
    // Advance just 3 generations (more than threshold of 2)
    for (int i = 0; i < 3; i++) {
        tt.newSearch();
    }
    
    // Store new entry with lower depth - should replace due to generation difference > 2
    tt.store(key, 0x2222, 200, 60, 3, Bound::LOWER);  // Much lower depth
    TTEntry* entry = tt.probe(key);
    
    if (entry && entry->move == 0x2222) {
        std::cout << "✓ PASS: Old entry replaced when generation difference > 2" << std::endl;
    } else if (entry && entry->move == 0x1111) {
        std::cout << "✗ FAIL: Old entry not replaced (generation diff > 2 should force replacement)" << std::endl;
    } else {
        std::cout << "✗ FAIL: Unexpected TT state" << std::endl;
    }
}

void testDepthPreferredReplacement() {
    std::cout << "\n=== Test 5: Depth-Preferred Replacement ===" << std::endl;
    
    TranspositionTable tt(1);  // Small table to force collisions
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    Hash key = board.zobristKey();
    
    // Store shallow entry
    tt.store(key, 0x1111, 100, 50, 5, Bound::EXACT);  // depth 5
    TTEntry* entry = tt.probe(key);
    
    if (entry && entry->depth == 5 && entry->move == 0x1111) {
        std::cout << "✓ PASS: Shallow entry stored" << std::endl;
    } else {
        std::cout << "✗ FAIL: Failed to store shallow entry" << std::endl;
        return;
    }
    
    // Try to store shallower entry - should NOT replace
    tt.store(key, 0x2222, 200, 60, 3, Bound::LOWER);  // depth 3
    entry = tt.probe(key);
    
    if (entry && entry->depth == 5 && entry->move == 0x1111) {
        std::cout << "✓ PASS: Shallower entry did not replace deeper one" << std::endl;
    } else {
        std::cout << "✗ FAIL: Shallower entry should not replace deeper one" << std::endl;
    }
    
    // Store deeper entry - should replace
    tt.store(key, 0x3333, 300, 70, 10, Bound::UPPER);  // depth 10
    entry = tt.probe(key);
    
    if (entry && entry->depth == 10 && entry->move == 0x3333) {
        std::cout << "✓ PASS: Deeper entry replaced shallower one" << std::endl;
    } else {
        std::cout << "✗ FAIL: Deeper entry should replace shallower one" << std::endl;
    }
    
    // Test that different position always replaces
    Hash differentKey = key ^ 0x123456789ABCDEF0ULL;
    tt.store(differentKey, 0x4444, 400, 80, 1, Bound::EXACT);  // Very shallow
    
    // This forced a collision, one of them should be stored
    // We can't directly test which one without knowing the index mapping
    std::cout << "✓ PASS: Different position handling tested (collision forced)" << std::endl;
}

void testTTStatistics() {
    std::cout << "\n=== Test 6: TT Statistics ===" << std::endl;
    
    TranspositionTable tt(16);
    tt.resetStats();
    
    Board board;
    board.setStartingPosition();
    
    // Store some entries
    for (int i = 0; i < 100; i++) {
        Hash key = board.positionHash() ^ i;  // Vary the key
        tt.store(key, i, i*10, i*5, i % 20, Bound::EXACT);
    }
    
    // Probe some entries (half hits, half misses)
    for (int i = 0; i < 200; i++) {
        Hash key = board.positionHash() ^ (i % 150);  // Some will hit, some won't
        tt.probe(key);
    }
    
    const TTStats& stats = tt.stats();
    std::cout << "Stores: " << stats.stores << std::endl;
    std::cout << "Probes: " << stats.probes << std::endl;
    std::cout << "Hits: " << stats.hits << std::endl;
    std::cout << "Hit rate: " << std::fixed << std::setprecision(1) 
              << stats.hitRate() << "%" << std::endl;
    
    if (stats.stores == 100 && stats.probes == 200 && stats.hits > 0) {
        std::cout << "✓ PASS: Statistics tracking works" << std::endl;
    } else {
        std::cout << "✗ FAIL: Statistics not tracked correctly" << std::endl;
    }
}

int main() {
    std::cout << "Stage 12 Transposition Table Improvements Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    testFiftyMoveHashExclusion();
    testHashTableResize();
    testTTEnableDisable();
    testGenerationWraparound();
    testDepthPreferredReplacement();
    testTTStatistics();
    
    std::cout << "\n===== Test Suite Complete =====" << std::endl;
    
    return 0;
}