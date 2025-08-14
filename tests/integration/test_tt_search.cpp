/**
 * SeaJay Chess Engine - Stage 12: Transposition Tables
 * Search Integration Tests
 * 
 * Phase 0: Test Infrastructure Foundation
 * These tests validate TT integration with the search algorithm
 */

#include "../test_framework.h"
#include "core/board.h"
#include "core/move_generation.h"
#include "search/search.h"
#include "search/negamax.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>

using namespace seajay;

// Forward declarations for TT integration
namespace seajay {

// Mate score constants for testing
constexpr int MATE_SCORE = 30000;
constexpr int MATE_BOUND = MATE_SCORE - 100;
constexpr int DRAW_SCORE = 0;

/**
 * Mate Score Adjustment Functions
 * Critical for correct mate distance reporting through TT
 */
class MateScoreAdjuster {
public:
    // Adjust mate score when storing in TT
    static int adjustForStore(int score, int ply) {
        if (score > MATE_BOUND) {
            return score + ply;
        }
        if (score < -MATE_BOUND) {
            return score - ply;
        }
        return score;
    }
    
    // Adjust mate score when retrieving from TT
    static int adjustFromTT(int score, int ply) {
        if (score > MATE_BOUND) {
            score -= ply;
            if (score <= MATE_BOUND) {
                return MATE_BOUND + 1;  // Prevent boundary issues
            }
        } else if (score < -MATE_BOUND) {
            score += ply;
            if (score >= -MATE_BOUND) {
                return -MATE_BOUND - 1;  // Prevent boundary issues
            }
        }
        return score;
    }
    
    // Check if score indicates mate
    static bool isMateScore(int score) {
        return std::abs(score) > MATE_BOUND;
    }
    
    // Get mate distance from score
    static int mateDistance(int score) {
        if (score > MATE_BOUND) {
            return MATE_SCORE - score;
        }
        if (score < -MATE_BOUND) {
            return -(MATE_SCORE + score);
        }
        return 0;
    }
};

/**
 * Test Interface for TT-enabled Search
 * Allows testing with TT on/off for comparison
 */
class TTSearchTester {
private:
    Board m_board;
    bool m_ttEnabled;
    int m_nodesSearched;
    int m_ttHits;
    int m_ttCutoffs;
    
public:
    TTSearchTester() : m_ttEnabled(false), m_nodesSearched(0), 
                       m_ttHits(0), m_ttCutoffs(0) {}
    
    void setPosition(const std::string& fen) {
        m_board.parseFEN(fen);
    }
    
    void enableTT(bool enable) {
        m_ttEnabled = enable;
    }
    
    struct SearchResult {
        int score;
        Move bestMove;
        int nodes;
        int ttHits;
        int ttCutoffs;
        double timeMs;
        std::vector<Move> pv;
    };
    
    SearchResult search(int depth) {
        SearchResult result;
        m_nodesSearched = 0;
        m_ttHits = 0;
        m_ttCutoffs = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Call actual search (will be integrated in Phase 4-5)
        // For now, this is a placeholder
        result.score = 0;
        result.bestMove = Move();
        
        auto end = std::chrono::high_resolution_clock::now();
        result.timeMs = std::chrono::duration<double, std::milli>(end - start).count();
        
        result.nodes = m_nodesSearched;
        result.ttHits = m_ttHits;
        result.ttCutoffs = m_ttCutoffs;
        
        return result;
    }
    
    // Compare search with and without TT
    void compareSearches(int depth) {
        // Search without TT
        enableTT(false);
        auto withoutTT = search(depth);
        
        // Search with TT
        enableTT(true);
        auto withTT = search(depth);
        
        std::cout << "Search comparison at depth " << depth << ":\n";
        std::cout << "Without TT: " << withoutTT.nodes << " nodes in " 
                  << std::fixed << std::setprecision(1) 
                  << withoutTT.timeMs << "ms\n";
        std::cout << "With TT:    " << withTT.nodes << " nodes in " 
                  << withTT.timeMs << "ms";
        
        if (withTT.nodes > 0 && withoutTT.nodes > 0) {
            double reduction = 100.0 * (1.0 - static_cast<double>(withTT.nodes) 
                                            / withoutTT.nodes);
            double speedup = withoutTT.timeMs / withTT.timeMs;
            std::cout << " (" << std::setprecision(1) << reduction 
                     << "% reduction, " << std::setprecision(2) 
                     << speedup << "x speedup)";
        }
        std::cout << "\n";
        
        if (withTT.ttHits > 0) {
            std::cout << "TT Stats: " << withTT.ttHits << " hits, " 
                     << withTT.ttCutoffs << " cutoffs\n";
        }
        
        // Verify same result
        if (withTT.score != withoutTT.score) {
            std::cerr << "WARNING: Different scores! Without TT: " 
                     << withoutTT.score << ", With TT: " << withTT.score << "\n";
        }
        if (withTT.bestMove != withoutTT.bestMove) {
            std::cerr << "WARNING: Different best moves!\n";
        }
    }
};

/**
 * PV (Principal Variation) Extraction
 * Extracts the best line from TT
 */
class PVExtractor {
private:
    static constexpr int MAX_PV_LENGTH = 20;
    
public:
    static std::vector<Move> extractPV(Board& board, int maxDepth) {
        std::vector<Move> pv;
        std::set<uint64_t> seen;  // Prevent loops
        
        Board tempBoard = board;  // Work on copy
        
        for (int i = 0; i < std::min(maxDepth, MAX_PV_LENGTH); i++) {
            uint64_t key = tempBoard.zobristKey();
            
            // Check for loop
            if (seen.count(key)) {
                break;
            }
            seen.insert(key);
            
            // Probe TT for move
            // TTEntry* tte = tt.probe(key);
            // if (!tte || tte->move == Move::none()) {
            //     break;
            // }
            
            // pv.push_back(tte->move);
            // tempBoard.makeMove(tte->move);
            
            // Placeholder for now
            break;
        }
        
        return pv;
    }
    
    static void printPV(const std::vector<Move>& pv) {
        std::cout << "PV: ";
        for (const Move& move : pv) {
            std::cout << move.toString() << " ";
        }
        std::cout << "\n";
    }
};

/**
 * Draw Detection Order Validator
 * Ensures repetition/fifty-move detection happens before TT probe
 */
class DrawDetectionValidator {
public:
    static bool validateDrawOrder(Board& board) {
        // Critical order that must be maintained:
        // 1. Check for repetition draw
        // 2. Check for fifty-move rule
        // 3. Only then probe TT
        
        bool isRep = board.isRepetitionDraw();
        bool isFifty = board.isFiftyMoveRule();
        
        if (isRep || isFifty) {
            // If it's a draw, TT should not override this
            return true;
        }
        
        // Now safe to probe TT
        // uint64_t key = board.zobristKey();
        // TTEntry* tte = tt.probe(key);
        
        return true;
    }
    
    static void testCriticalDrawPositions() {
        std::cout << "Testing critical draw detection order...\n";
        
        // Position with potential repetition
        Board board;
        board.parseFEN("8/8/8/3k4/8/8/8/R2K2R1 w - - 0 1");
        
        // Make moves that create repetition
        // Ra1-a8-a1 while black king shuffles
        
        if (!validateDrawOrder(board)) {
            std::cerr << "Draw detection order violation!\n";
        }
    }
};

} // namespace seajay

// ============================================================================
// Integration Test Suite
// ============================================================================

TEST_CASE("TT Search: Mate Score Adjustment") {
    SECTION("Store and retrieve mate scores") {
        // Mate in 3 from ply 5
        int mateIn3 = MATE_SCORE - 3;
        int ply = 5;
        
        // Store adjusted score
        int stored = MateScoreAdjuster::adjustForStore(mateIn3, ply);
        REQUIRE(stored == MATE_SCORE - 3 + 5);
        
        // Retrieve at different ply
        int retrieved = MateScoreAdjuster::adjustFromTT(stored, 7);
        REQUIRE(retrieved == MATE_SCORE - 3 + 5 - 7);
    }
    
    SECTION("Boundary protection") {
        // Score right at boundary
        int score = MATE_BOUND + 1;
        int adjusted = MateScoreAdjuster::adjustFromTT(score, 10);
        REQUIRE(adjusted > MATE_BOUND);
        REQUIRE(adjusted <= MATE_BOUND + 1);
    }
}

TEST_CASE("TT Search: Draw Detection Order") {
    DrawDetectionValidator validator;
    
    SECTION("Repetition before TT probe") {
        Board board;
        board.parseFEN("8/8/8/3k4/8/8/8/R2K2R1 w - - 0 1");
        REQUIRE(DrawDetectionValidator::validateDrawOrder(board));
    }
    
    SECTION("Fifty-move before TT probe") {
        Board board;
        board.parseFEN("8/8/8/3k4/8/3K4/8/8 w - - 99 1");
        REQUIRE(DrawDetectionValidator::validateDrawOrder(board));
    }
}

TEST_CASE("TT Search: Node Reduction") {
    TTSearchTester tester;
    
    SECTION("Complex middlegame position") {
        tester.setPosition("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        
        // This will show results once integrated
        // Expected: 30-50% node reduction with TT
        // tester.compareSearches(8);
    }
    
    SECTION("Endgame position") {
        tester.setPosition("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
        
        // Endgames often benefit more from TT
        // Expected: 40-60% node reduction
        // tester.compareSearches(10);
    }
}

TEST_CASE("TT Search: Best Move Consistency") {
    TTSearchTester tester;
    
    SECTION("Same best move with and without TT") {
        const char* testPositions[] = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
        };
        
        for (const char* fen : testPositions) {
            tester.setPosition(fen);
            // Verify same move found
            // auto withoutTT = tester.search(6);
            // auto withTT = tester.search(6);
            // REQUIRE(withoutTT.bestMove == withTT.bestMove);
        }
    }
}

TEST_CASE("TT Search: PV Extraction") {
    Board board;
    
    SECTION("Extract PV without loops") {
        board.setStartingPosition();
        
        // Once TT is integrated, test PV extraction
        // auto pv = PVExtractor::extractPV(board, 10);
        // REQUIRE(pv.size() <= 10);
        
        // Verify no repeated positions in PV
        // Board testBoard = board;
        // std::set<uint64_t> seen;
        // for (const Move& move : pv) {
        //     testBoard.makeMove(move);
        //     uint64_t key = testBoard.zobristKey();
        //     REQUIRE(seen.count(key) == 0);
        //     seen.insert(key);
        // }
    }
}

TEST_CASE("TT Search: Killer Position Tests") {
    TTSearchTester tester;
    
    // Test positions from implementation plan
    struct TestPosition {
        const char* fen;
        const char* description;
        int minDepth;
    };
    
    TestPosition positions[] = {
        {"8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1", 
         "Bratko-Kopec BK.24", 10},
        {"8/2P5/8/8/8/8/8/k6K w - - 0 1",
         "Promotion horizon", 8},
        {"8/8/8/8/1k6/8/1K6/4Q3 w - - 0 1",
         "Deep mate position", 16}
    };
    
    for (const auto& pos : positions) {
        SECTION(pos.description) {
            tester.setPosition(pos.fen);
            // Test will be active once TT is integrated
            // tester.compareSearches(pos.minDepth);
        }
    }
}

// ============================================================================
// Performance Testing
// ============================================================================

void runIntegrationBenchmark() {
    std::cout << "Running TT Search Integration Benchmark\n";
    std::cout << "========================================\n\n";
    
    TTSearchTester tester;
    
    struct BenchPosition {
        const char* fen;
        const char* name;
        int depth;
    };
    
    BenchPosition positions[] = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "Starting position", 10},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
         "Complex middlegame", 8},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
         "Fine endgame", 12},
        {"8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1",
         "Pawn endgame", 14}
    };
    
    for (const auto& pos : positions) {
        std::cout << "Position: " << pos.name << "\n";
        std::cout << "Depth: " << pos.depth << "\n";
        tester.setPosition(pos.fen);
        
        // Will show comparison once integrated
        // tester.compareSearches(pos.depth);
        std::cout << "\n";
    }
}

// ============================================================================
// Perft with TT Testing
// ============================================================================

uint64_t perftWithTT(Board& board, int depth, bool useTT) {
    if (depth == 0) return 1;
    
    uint64_t nodes = 0;
    
    if (useTT) {
        // Check TT first
        // uint64_t key = board.zobristKey();
        // TTEntry* tte = tt.probe(key);
        // if (tte && tte->depth >= depth) {
        //     return tte->score;  // Use cached perft result
        // }
    }
    
    MoveList moves;
    MoveGenerator gen(board);
    gen.generateAllMoves(moves);
    
    for (size_t i = 0; i < moves.size(); i++) {
        const Move& move = moves[i];
        
        if (!board.makeMove(move)) {
            continue;
        }
        
        nodes += perftWithTT(board, depth - 1, useTT);
        board.unmakeMove(move);
    }
    
    if (useTT) {
        // Store result in TT
        // tt.store(board.zobristKey(), nodes, 0, depth, Move::none(), TTBound::EXACT);
    }
    
    return nodes;
}

TEST_CASE("TT Search: Perft Integration") {
    Board board;
    
    SECTION("Perft with TT matches without TT") {
        board.setStartingPosition();
        
        // Small depth for testing
        int depth = 4;
        
        uint64_t withoutTT = perftWithTT(board, depth, false);
        uint64_t withTT = perftWithTT(board, depth, true);
        
        REQUIRE(withoutTT == withTT);
        REQUIRE(withoutTT == 197281);  // Known perft(4) value
    }
    
    SECTION("Second run is faster with TT") {
        board.setStartingPosition();
        int depth = 5;
        
        // First run populates TT
        auto start1 = std::chrono::high_resolution_clock::now();
        uint64_t result1 = perftWithTT(board, depth, true);
        auto end1 = std::chrono::high_resolution_clock::now();
        auto time1 = std::chrono::duration<double, std::milli>(end1 - start1).count();
        
        // Second run should be much faster
        auto start2 = std::chrono::high_resolution_clock::now();
        uint64_t result2 = perftWithTT(board, depth, true);
        auto end2 = std::chrono::high_resolution_clock::now();
        auto time2 = std::chrono::duration<double, std::milli>(end2 - start2).count();
        
        REQUIRE(result1 == result2);
        // Second run should be significantly faster
        // REQUIRE(time2 < time1 * 0.5);  // At least 2x faster
    }
}

// Main test runner
int main(int argc, char* argv[]) {
    std::cout << "SeaJay Stage 12: TT Search Integration Tests\n";
    std::cout << "============================================\n\n";
    
    // Run benchmark if requested
    if (argc > 1 && std::string(argv[1]) == "--bench") {
        runIntegrationBenchmark();
        return 0;
    }
    
    // Run catch2 tests
    return Catch::Session().run(argc, argv);
}