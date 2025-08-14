/**
 * SeaJay Chess Engine - Stage 12: Transposition Tables
 * Zobrist Hashing Validation Tests
 * 
 * Phase 0: Test Infrastructure Foundation
 * These tests will validate the enhanced Zobrist implementation
 */

#include "../test_framework.h"
#include "core/board.h"
#include "core/types.h"
#include <iostream>
#include <random>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <sstream>

using namespace seajay;

// Forward declarations for future Zobrist implementation
namespace seajay {
namespace zobrist {
    
    // These will be implemented in Phase 1
    extern uint64_t pieceKeys[15][64];      // [piece][square]
    extern uint64_t castlingKeys[16];       // All castling combinations
    extern uint64_t enPassantKeys[8];       // EP file keys
    extern uint64_t sideToMoveKey;          // Black to move XOR
    extern uint64_t fiftyMoveKeys[100];     // Fifty-move counter keys
    
    // Validation functions
    bool validateKeysUnique();
    bool validateKeysNonZero();
    uint64_t calculateFull(const Board& board);
    
} // namespace zobrist
} // namespace seajay

/**
 * Differential Testing Framework
 * Validates that incremental updates match full recalculation
 */
class DifferentialTester {
public:
    bool validateIncremental(const Board& pos) {
        uint64_t incremental = pos.zobristKey();
        uint64_t full = zobrist::calculateFull(pos);
        
        if (incremental != full) {
            dumpMismatch(incremental, full, pos);
            return false;
        }
        return true;
    }
    
    bool compareWithFull(const Board& pos) {
        return validateIncremental(pos);
    }
    
    void dumpMismatch(uint64_t incremental, uint64_t full, const Board& pos) {
        std::cerr << "Zobrist mismatch detected!\n";
        std::cerr << "Position: " << pos.toFEN() << "\n";
        std::cerr << "Incremental: 0x" << std::hex << incremental << "\n";
        std::cerr << "Full calc:   0x" << std::hex << full << "\n";
        std::cerr << "XOR diff:    0x" << std::hex << (incremental ^ full) << "\n";
        std::cerr << std::dec;
    }
    
    // Test that multiple paths to same position give same hash
    bool testTranspositionProperty() {
        Board b1, b2;
        b1.setStartingPosition();
        b2.setStartingPosition();
        
        // Path 1: e2-e4, Nf6, Nf3
        b1.parseFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
        b1.parseFEN("rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2");
        b1.parseFEN("rnbqkb1r/pppppppp/5n2/8/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 2 2");
        
        // Path 2: Nf3, Nf6, e2-e4
        b2.parseFEN("rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1");
        b2.parseFEN("rnbqkb1r/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 2 2");
        b2.parseFEN("rnbqkb1r/pppppppp/5n2/8/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 0 2");
        
        // Different paths to same position should have same hash
        // Note: fifty-move counter might differ, so we need to account for that
        return true; // Will implement fully in Phase 1
    }
};

/**
 * Killer Test Positions from Implementation Plan
 * These expose common Zobrist/TT bugs
 */
struct KillerPosition {
    const char* fen;
    const char* description;
    bool requiresSpecialHandling;
};

const KillerPosition killerPositions[] = {
    // Critical positions from stage12_transposition_tables_plan.md
    {"8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1", 
     "Bratko-Kopec BK.24 - Exposes TT mate bugs", false},
    
    {"r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
     "The Lasker Trap - Tests repetition + TT interaction", true},
    
    {"8/2P5/8/8/8/8/8/k6K w - - 0 1",
     "The Promotion Horizon - Tests promotion + TT", false},
    
    {"8/8/3p4/KPp4r/1R2Pp1k/8/6P1/8 b - e3 0 1",
     "The En Passant Mirage - Only looks like EP is possible", true},
    
    {"8/8/p1p5/1p5p/1P5p/8/PPP2K1p/4R1rk w - - 0 1",
     "The Zugzwang Special - TT must not break zugzwang detection", true},
    
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
     "SMP Stress Position - High collision rate", false},
    
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
     "Fine #70 - En passant edge cases", false},
    
    // Additional critical positions
    {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
     "The Transposition Trap - Same position after Ke1-e2-e1", false},
    
    {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 2 2",
     "Same position, different fifty-move counter", false},
    
    {"8/8/8/2k5/3Pp3/8/8/3K4 b - d3 0 1",
     "The False En Passant - Looks possible but isn't", true},
    
    {"8/2P5/8/8/8/8/2p5/8 w - - 0 1",
     "The Underpromotion Hash - Tests promotion handling", false},
    
    {"8/8/1p1p1p2/p1p1p1p1/P1P1P1P1/1P1P1P2/8/8 w - - 0 1",
     "The Null Move Critical - Where null move fails", false},
    
    {"8/8/8/8/1k6/8/1K6/4Q3 w - - 0 1",
     "The Deep Mate - Tests mate score adjustment (Mate in 8)", false},
    
    {"2b2rk1/p1p2ppp/1p1p4/3Pp3/1PP1P3/P3KP2/6PP/8 w - - 0 1",
     "The Fortress - High collision position", false},
    
    {"rnbqkb1r/pp1p1ppp/4pn2/2p5/2PP4/5N2/PP2PPPP/RNBQKB1R w KQkq c6 0 4",
     "The PV Corruption Special", false},
    
    {"k7/8/KP6/8/8/8/8/8 w - - 0 1",
     "The Hash Collision Generator", false},
    
    {"8/8/8/3k4/8/8/8/R2K2R1 w - - 0 1",
     "The Repetition Maze", false},
    
    {"r1b1kb1r/pp2qppp/2n1p3/3p4/2PP4/2N2N2/PP2QPPP/R1B1KB1R w KQkq - 0 8",
     "The Quiescence Explosion", false}
};

/**
 * Property-Based Testing Framework
 * Tests mathematical properties that must hold for Zobrist hashing
 */
class PropertyBasedTester {
public:
    // XOR is its own inverse: A XOR B XOR B = A
    bool testXorInverseProperty() {
        uint64_t a = 0x123456789ABCDEF0ULL;
        uint64_t b = 0xFEDCBA9876543210ULL;
        
        uint64_t result = a ^ b ^ b;
        if (result != a) {
            std::cerr << "XOR inverse property failed!\n";
            return false;
        }
        return true;
    }
    
    // XOR is commutative: A XOR B = B XOR A
    bool testXorCommutativeProperty() {
        uint64_t a = 0x123456789ABCDEF0ULL;
        uint64_t b = 0xFEDCBA9876543210ULL;
        
        if ((a ^ b) != (b ^ a)) {
            std::cerr << "XOR commutative property failed!\n";
            return false;
        }
        return true;
    }
    
    // Uniqueness: All keys should be unique
    bool testUniquenessProperty() {
        // Will be implemented in Phase 1 when we have actual keys
        return true;
    }
    
    // Distribution: Keys should be well-distributed
    bool testDistributionProperty() {
        // Will analyze bit distribution in Phase 1
        return true;
    }
    
    // Test that removing and re-adding same piece gives same hash
    bool testAddRemoveInvariant() {
        // Board b;
        // b.setStartingPosition();
        // uint64_t original = b.zobristKey();
        // 
        // // Remove and re-add white pawn on e2
        // b.removePiece(E2);
        // b.setPiece(E2, WHITE_PAWN);
        // 
        // return b.zobristKey() == original;
        
        return true; // Will implement in Phase 1
    }
};

/**
 * Zobrist Validator Class
 * Main validation infrastructure for differential testing
 */
class ZobristValidator {
private:
    bool m_shadowMode = false;
    uint64_t m_shadowHash = 0;
    
public:
    uint64_t calculateFull(const Board& board) {
        // Will be implemented in Phase 1
        // This will calculate hash from scratch based on board state
        return 0;
    }
    
    bool validateIncremental(uint64_t incremental, const Board& board) {
        uint64_t full = calculateFull(board);
        if (incremental != full) {
            std::cerr << "Validation failed!\n";
            std::cerr << "Incremental: 0x" << std::hex << incremental << "\n";
            std::cerr << "Full calc:   0x" << std::hex << full << "\n";
            return false;
        }
        return true;
    }
    
    void enableShadowMode(bool enable) {
        m_shadowMode = enable;
        if (enable) {
            m_shadowHash = 0;
        }
    }
    
    bool verifyShadowHash(uint64_t primary) const {
        if (!m_shadowMode) return true;
        return primary == m_shadowHash;
    }
};

// ============================================================================
// Test Suite
// ============================================================================

TEST_CASE("Zobrist: Basic XOR Properties") {
    PropertyBasedTester tester;
    
    SECTION("XOR is its own inverse") {
        REQUIRE(tester.testXorInverseProperty());
    }
    
    SECTION("XOR is commutative") {
        REQUIRE(tester.testXorCommutativeProperty());
    }
}

TEST_CASE("Zobrist: Key Generation Validation") {
    SECTION("All keys are unique") {
        // Will be implemented in Phase 1
        // REQUIRE(zobrist::validateKeysUnique());
    }
    
    SECTION("All keys are non-zero") {
        // Will be implemented in Phase 1
        // REQUIRE(zobrist::validateKeysNonZero());
    }
    
    SECTION("Keys have good distribution") {
        // Will analyze bit distribution in Phase 1
    }
}

TEST_CASE("Zobrist: Incremental Update Correctness") {
    DifferentialTester tester;
    Board board;
    
    SECTION("Starting position") {
        board.setStartingPosition();
        // REQUIRE(tester.validateIncremental(board));
    }
    
    SECTION("After single move") {
        board.parseFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
        // REQUIRE(tester.validateIncremental(board));
    }
    
    SECTION("Complex middlegame position") {
        board.parseFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        // REQUIRE(tester.validateIncremental(board));
    }
}

TEST_CASE("Zobrist: Special Cases") {
    Board board;
    
    SECTION("Fifty-move counter affects hash") {
        board.parseFEN("8/8/8/3k4/8/3K4/8/8 w - - 0 1");
        uint64_t hash1 = board.zobristKey();
        
        board.parseFEN("8/8/8/3k4/8/3K4/8/8 w - - 50 1");
        uint64_t hash2 = board.zobristKey();
        
        // Different fifty-move counters should give different hashes
        // REQUIRE(hash1 != hash2);
    }
    
    SECTION("En passant only when capturable") {
        // Position where e3 is set but no black pawn can capture
        board.parseFEN("8/8/8/2k5/3P4/8/8/3K4 b - e3 0 1");
        uint64_t hash1 = board.zobristKey();
        
        // Same position without en passant square
        board.parseFEN("8/8/8/2k5/3P4/8/8/3K4 b - - 0 1");
        uint64_t hash2 = board.zobristKey();
        
        // Should be same hash since EP not actually possible
        // REQUIRE(hash1 == hash2);
    }
    
    SECTION("Castling rights removed correctly") {
        board.parseFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        uint64_t hashBefore = board.zobristKey();
        
        // Move rook, losing kingside castling rights
        board.parseFEN("r3k2r/8/8/8/8/8/8/R2K3R b Qkq - 1 1");
        uint64_t hashAfter = board.zobristKey();
        
        // REQUIRE(hashBefore != hashAfter);
    }
}

TEST_CASE("Zobrist: Killer Positions") {
    Board board;
    DifferentialTester tester;
    
    for (const auto& killer : killerPositions) {
        SECTION(killer.description) {
            auto result = board.parseFEN(killer.fen);
            if (result == FenResult::OK) {
                // Test that hash is calculated correctly
                // REQUIRE(tester.validateIncremental(board));
                
                if (killer.requiresSpecialHandling) {
                    // These positions need extra validation
                    // Will be implemented in Phase 1
                }
            }
        }
    }
}

TEST_CASE("Zobrist: Hash Collision Analysis") {
    std::map<uint32_t, int> collisionCounts;
    Board board;
    
    SECTION("Measure collision rate") {
        // Test various positions and track 32-bit key collisions
        for (const auto& killer : killerPositions) {
            board.parseFEN(killer.fen);
            uint64_t hash = board.zobristKey();
            uint32_t key32 = hash >> 32;
            collisionCounts[key32]++;
        }
        
        // Check collision rate
        int collisions = 0;
        for (const auto& [key, count] : collisionCounts) {
            if (count > 1) collisions++;
        }
        
        // With good random keys, collision rate should be very low
        // REQUIRE(collisions == 0);  // No collisions in this small set
    }
}

TEST_CASE("Zobrist: Perft Integration Preparation") {
    Board board;
    
    SECTION("Hash consistency through move sequence") {
        board.setStartingPosition();
        std::vector<uint64_t> hashes;
        
        // Make a sequence of moves and track hashes
        const char* positions[] = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
            "rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2",
            "rnbqkb1r/pppppppp/5n2/8/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 2 2"
        };
        
        for (const char* fen : positions) {
            board.parseFEN(fen);
            hashes.push_back(board.zobristKey());
        }
        
        // Verify all hashes are different
        std::set<uint64_t> uniqueHashes(hashes.begin(), hashes.end());
        // REQUIRE(uniqueHashes.size() == hashes.size());
    }
}

TEST_CASE("Zobrist: Shadow Hashing Framework") {
    ZobristValidator validator;
    Board board;
    
    SECTION("Shadow mode tracks correctly") {
        validator.enableShadowMode(true);
        board.setStartingPosition();
        
        // In shadow mode, we maintain two hashes and verify they match
        // This will be fully implemented in Phase 1
        // REQUIRE(validator.verifyShadowHash(board.zobristKey()));
    }
}

// ============================================================================
// Performance and Stress Testing Helpers
// ============================================================================

void runZobristPerformanceTest() {
    Board board;
    DifferentialTester tester;
    
    std::cout << "Running Zobrist performance validation...\n";
    
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        board.setStartingPosition();
        // Make random moves and validate
        // Will implement in Phase 1
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Completed " << iterations << " validations in " 
              << duration.count() << "ms\n";
}

// Main test runner
int main(int argc, char* argv[]) {
    std::cout << "SeaJay Stage 12: Zobrist Validation Tests\n";
    std::cout << "=========================================\n\n";
    
    // Run performance tests if requested
    if (argc > 1 && std::string(argv[1]) == "--perf") {
        runZobristPerformanceTest();
        return 0;
    }
    
    // Run catch2 tests
    return Catch::Session().run(argc, argv);
}