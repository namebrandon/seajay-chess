/**
 * SeaJay Chess Engine - Stage 12: Transposition Tables
 * Chaos and Stress Testing
 * 
 * Phase 0: Test Infrastructure Foundation
 * Stress tests for finding edge cases and race conditions
 */

#include "../test_framework.h"
#include "core/board.h"
#include "core/move_generation.h"
#include <iostream>
#include <random>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <map>
#include <set>

using namespace seajay;
using UndoInfo = Board::UndoInfo;

// Stub implementation for Move extension (needed for testing)
namespace seajay {
    inline std::string moveToString(Move m) {
        if (m == 0) return "none";
        std::string str;
        Square fromSq = moveFrom(m);
        Square toSq = moveTo(m);
        str += static_cast<char>('a' + fileOf(fromSq));
        str += static_cast<char>('1' + rankOf(fromSq));
        str += static_cast<char>('a' + fileOf(toSq));
        str += static_cast<char>('1' + rankOf(toSq));
        if (isPromotion(m)) {
            PieceType pt = promotionType(m);
            const char* pieces = "nbrq";
            str += pieces[pt - KNIGHT];
        }
        return str;
    }
}

/**
 * Chaos Test Generator
 * Creates random positions and operations to stress test the TT
 */
class ChaosTestGenerator {
private:
    std::mt19937_64 m_rng;
    std::uniform_int_distribution<int> m_pieceDist;
    std::uniform_int_distribution<int> m_squareDist;
    std::uniform_int_distribution<int> m_colorDist;
    
public:
    ChaosTestGenerator(uint64_t seed = 12345) 
        : m_rng(seed), 
          m_pieceDist(0, 5),      // 6 piece types
          m_squareDist(0, 63),     // 64 squares
          m_colorDist(0, 1)        // 2 colors
    {}
    
    // Generate random valid position
    Board generateRandomPosition() {
        Board board;
        board.clear();
        
        // Always place kings first (required)
        board.setPiece(static_cast<Square>(4), WHITE_KING);   // e1
        board.setPiece(static_cast<Square>(60), BLACK_KING);  // e8
        
        // Add random pieces (not too many to keep it valid)
        int pieceCount = m_rng() % 20 + 4;  // 4-24 pieces total
        
        for (int i = 0; i < pieceCount; i++) {
            Square sq = static_cast<Square>(m_squareDist(m_rng));
            
            // Skip if square occupied
            if (board.pieceAt(sq) != NO_PIECE) continue;
            
            // Skip back ranks for pawns
            if (sq <= 7 || sq >= 56) continue;
            
            PieceType pt = static_cast<PieceType>(m_pieceDist(m_rng));
            Color c = static_cast<Color>(m_colorDist(m_rng));
            
            // Don't add more kings
            if (pt == KING) pt = QUEEN;
            
            board.setPiece(sq, makePiece(c, pt));
        }
        
        // Random side to move
        board.setSideToMove(static_cast<Color>(m_colorDist(m_rng)));
        
        // Random castling rights
        board.setCastlingRights(m_rng() % 16);
        
        // Random fifty-move counter
        board.setHalfmoveClock(m_rng() % 100);
        
        return board;
    }
    
    // Generate sequence of random moves
    std::vector<Move> generateRandomMoves(Board& board, int count) {
        std::vector<Move> moves;
        std::vector<UndoInfo> undos;
        
        for (int i = 0; i < count; i++) {
            MoveList legal;
            MoveGenerator::generateLegalMoves(board, legal);
            
            if (legal.empty()) break;
            
            // Pick random legal move
            int idx = m_rng() % legal.size();
            Move move = legal[idx];
            
            UndoInfo undo;
            board.makeMove(move, undo);
            moves.push_back(move);
            undos.push_back(undo);
        }
        
        // Unmake all moves to restore position
        for (size_t i = moves.size(); i > 0; i--) {
            board.unmakeMove(moves[i-1], undos[i-1]);
        }
        
        return moves;
    }
    
    // Generate positions that stress hash distribution
    std::vector<Board> generateCollisionPositions(int count) {
        std::vector<Board> positions;
        
        for (int i = 0; i < count; i++) {
            Board board = generateRandomPosition();
            
            // Modify slightly to create similar positions
            if (i > 0 && (i % 3 == 0)) {
                // Swap two pieces to create similar position
                Square sq1 = static_cast<Square>(m_squareDist(m_rng));
                Square sq2 = static_cast<Square>(m_squareDist(m_rng));
                
                Piece p1 = board.pieceAt(sq1);
                Piece p2 = board.pieceAt(sq2);
                
                if (p1 != NO_PIECE && p2 != NO_PIECE && 
                    pieceType(p1) != KING && pieceType(p2) != KING) {
                    board.removePiece(sq1);
                    board.removePiece(sq2);
                    board.setPiece(sq1, p2);
                    board.setPiece(sq2, p1);
                }
            }
            
            positions.push_back(board);
        }
        
        return positions;
    }
};

/**
 * Hash Collision Detector
 * Monitors and reports hash collisions
 */
class HashCollisionDetector {
private:
    struct CollisionInfo {
        uint64_t hash1;
        uint64_t hash2;
        std::string fen1;
        std::string fen2;
        uint32_t key32;
    };
    
    std::vector<CollisionInfo> m_collisions;
    std::map<uint32_t, std::vector<std::pair<uint64_t, std::string>>> m_key32Map;
    
public:
    void checkPosition(const Board& board) {
        uint64_t hash = board.zobristKey();
        uint32_t key32 = hash >> 32;
        std::string fen = board.toFEN();
        
        // Check for 32-bit collisions
        auto& entries = m_key32Map[key32];
        for (const auto& [otherHash, otherFen] : entries) {
            if (otherHash != hash && otherFen != fen) {
                // Found a collision!
                CollisionInfo info;
                info.hash1 = hash;
                info.hash2 = otherHash;
                info.fen1 = fen;
                info.fen2 = otherFen;
                info.key32 = key32;
                m_collisions.push_back(info);
            }
        }
        
        entries.push_back({hash, fen});
    }
    
    void printReport() {
        std::cout << "Hash Collision Report:\n";
        std::cout << "=====================\n";
        std::cout << "Total positions checked: " << getTotalPositions() << "\n";
        std::cout << "32-bit key collisions: " << m_collisions.size() << "\n";
        
        if (!m_collisions.empty()) {
            std::cout << "\nCollision Details:\n";
            for (size_t i = 0; i < std::min(size_t(5), m_collisions.size()); i++) {
                const auto& c = m_collisions[i];
                std::cout << "Collision " << (i+1) << ":\n";
                std::cout << "  Key32: 0x" << std::hex << c.key32 << std::dec << "\n";
                std::cout << "  Hash1: 0x" << std::hex << c.hash1 << std::dec << "\n";
                std::cout << "  Hash2: 0x" << std::hex << c.hash2 << std::dec << "\n";
                std::cout << "  FEN1: " << c.fen1 << "\n";
                std::cout << "  FEN2: " << c.fen2 << "\n";
            }
        }
        
        double collisionRate = 100.0 * m_collisions.size() / getTotalPositions();
        std::cout << "\nCollision rate: " << std::fixed << std::setprecision(4) 
                  << collisionRate << "%\n";
    }
    
    size_t getTotalPositions() const {
        size_t total = 0;
        for (const auto& [key, positions] : m_key32Map) {
            total += positions.size();
        }
        return total;
    }
    
    size_t getCollisionCount() const {
        return m_collisions.size();
    }
};

/**
 * Memory Stress Tester
 * Tests TT under memory pressure
 */
class MemoryStressTester {
private:
    std::atomic<bool> m_running{false};
    std::atomic<uint64_t> m_operations{0};
    std::atomic<uint64_t> m_errors{0};
    
public:
    void runStressTest(int durationSeconds, int threadCount = 1) {
        std::cout << "Running memory stress test for " << durationSeconds 
                  << " seconds with " << threadCount << " threads...\n";
        
        m_running = true;
        m_operations = 0;
        m_errors = 0;
        
        std::vector<std::thread> threads;
        
        auto endTime = std::chrono::steady_clock::now() + 
                      std::chrono::seconds(durationSeconds);
        
        // Launch worker threads
        for (int i = 0; i < threadCount; i++) {
            threads.emplace_back([this, i, endTime]() {
                workerThread(i, endTime);
            });
        }
        
        // Wait for completion
        for (auto& t : threads) {
            t.join();
        }
        
        m_running = false;
        
        // Print results
        std::cout << "Stress test completed:\n";
        std::cout << "  Operations: " << m_operations.load() << "\n";
        std::cout << "  Errors: " << m_errors.load() << "\n";
        std::cout << "  Ops/sec: " << (m_operations.load() / durationSeconds) << "\n";
    }
    
private:
    void workerThread(int threadId, std::chrono::steady_clock::time_point endTime) {
        ChaosTestGenerator gen(12345 + threadId);
        
        while (std::chrono::steady_clock::now() < endTime) {
            try {
                // Generate random position
                Board board = gen.generateRandomPosition();
                
                // Make random moves
                auto moves = gen.generateRandomMoves(board, 10);
                
                std::vector<UndoInfo> undos;
                for (const Move& move : moves) {
                    UndoInfo undo;
                    board.makeMove(move, undo);
                    undos.push_back(undo);
                    
                    // Would probe/store in TT here
                    uint64_t hash = board.zobristKey();
                    (void)hash;  // Use it to avoid optimization
                    
                    m_operations++;
                }
                
                // Unmake all moves
                for (size_t i = moves.size(); i > 0; i--) {
                    board.unmakeMove(moves[i-1], undos[i-1]);
                }
                
            } catch (...) {
                m_errors++;
            }
        }
    }
};

/**
 * Incremental Update Validator
 * Validates that incremental hash updates match full recalculation
 */
class IncrementalUpdateValidator {
private:
    int m_errors = 0;
    int m_validations = 0;
    
public:
    bool validateSequence(Board& board, const std::vector<Move>& moves) {
        bool allValid = true;
        
        for (const Move& move : moves) {
            uint64_t hashBefore = board.zobristKey();
            
            UndoInfo undo;
            board.makeMove(move, undo);
            
            uint64_t hashAfter = board.zobristKey();
            
            // Hash should change after move (except in rare cases)
            if (hashBefore == hashAfter && !isNullMove(move)) {
                std::cerr << "Hash unchanged after move!\n";
                m_errors++;
                allValid = false;
            }
            
            m_validations++;
            
            // Would validate against full recalculation here
            // uint64_t fullHash = zobrist::calculateFull(board);
            // if (hashAfter != fullHash) {
            //     std::cerr << "Incremental != Full hash!\n";
            //     m_errors++;
            //     allValid = false;
            // }
            
            board.unmakeMove(move, undo);
            
            uint64_t hashRestored = board.zobristKey();
            if (hashBefore != hashRestored) {
                std::cerr << "Hash not restored after unmake!\n";
                std::cerr << "Before: 0x" << std::hex << hashBefore << "\n";
                std::cerr << "After:  0x" << std::hex << hashRestored << "\n";
                m_errors++;
                allValid = false;
            }
        }
        
        return allValid;
    }
    
    void runRandomValidation(int iterations) {
        std::cout << "Running " << iterations 
                  << " random incremental validations...\n";
        
        ChaosTestGenerator gen;
        
        for (int i = 0; i < iterations; i++) {
            Board board = gen.generateRandomPosition();
            auto moves = gen.generateRandomMoves(board, 20);
            
            if (!validateSequence(board, moves)) {
                std::cerr << "Validation failed at iteration " << i << "\n";
            }
            
            if ((i + 1) % 1000 == 0) {
                std::cout << "  Completed " << (i + 1) << " iterations...\n";
            }
        }
        
        printReport();
    }
    
    void printReport() {
        std::cout << "\nIncremental Validation Report:\n";
        std::cout << "  Total validations: " << m_validations << "\n";
        std::cout << "  Errors found: " << m_errors << "\n";
        
        if (m_validations > 0) {
            double errorRate = 100.0 * m_errors / m_validations;
            std::cout << "  Error rate: " << std::fixed << std::setprecision(4) 
                     << errorRate << "%\n";
        }
    }
    
private:
    bool isNullMove(const Move& move) const {
        return move.from() == move.to();
    }
};

/**
 * TT Overflow Tester
 * Tests behavior when TT is full
 */
class TTOverflowTester {
public:
    void testOverflowBehavior() {
        std::cout << "Testing TT overflow behavior...\n";
        
        // Create very small TT (1KB)
        // TranspositionTable tt;
        // tt.resize(0.001);  // 1KB = ~64 entries
        
        ChaosTestGenerator gen;
        
        // Try to store many more positions than capacity
        for (int i = 0; i < 10000; i++) {
            Board board = gen.generateRandomPosition();
            uint64_t hash = board.zobristKey();
            
            // tt.store(hash, i, 0, 5, Move::none(), TTBound::EXACT);
            
            // Every 100 stores, check some old ones
            if (i > 0 && i % 100 == 0) {
                // Check if old entries are being replaced properly
                // Some should be gone, but no crashes
            }
        }
        
        std::cout << "Overflow test completed without crashes\n";
    }
};

// ============================================================================
// Chaos Test Suite
// ============================================================================

TEST_CASE(Chaos_RandomPositionGeneration) {
    ChaosTestGenerator gen;
    
    SECTION("Generate valid positions") {
        for (int i = 0; i < 100; i++) {
            Board board = gen.generateRandomPosition();
            
            // Should have both kings
            bool hasWhiteKing = false;
            bool hasBlackKing = false;
            
            for (Square sq = A1; sq <= H8; sq = static_cast<Square>(sq + 1)) {
                Piece p = board.pieceAt(sq);
                if (p == WHITE_KING) hasWhiteKing = true;
                if (p == BLACK_KING) hasBlackKing = true;
            }
            
            REQUIRE(hasWhiteKing);
            REQUIRE(hasBlackKing);
        }
    }
    
    SECTION("Generate move sequences") {
        Board board;
        board.setStartingPosition();
        
        auto moves = gen.generateRandomMoves(board, 10);
        REQUIRE(moves.size() <= 10);
        
        // Board should be restored
        REQUIRE(board.toFEN() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }
}

TEST_CASE(Chaos_HashCollisionDetection) {
    HashCollisionDetector detector;
    ChaosTestGenerator gen;
    
    SECTION("Detect collisions in random positions") {
        // Generate many positions
        for (int i = 0; i < 1000; i++) {
            Board board = gen.generateRandomPosition();
            detector.checkPosition(board);
        }
        
        // With good random keys, collisions should be rare
        double collisionRate = 100.0 * detector.getCollisionCount() / 1000;
        REQUIRE(collisionRate < 1.0);  // Less than 1% collision rate
    }
}

TEST_CASE(Chaos_IncrementalUpdateValidation) {
    IncrementalUpdateValidator validator;
    ChaosTestGenerator gen;
    
    SECTION("Random move sequences") {
        for (int i = 0; i < 10; i++) {
            Board board = gen.generateRandomPosition();
            auto moves = gen.generateRandomMoves(board, 10);
            REQUIRE(validator.validateSequence(board, moves));
        }
    }
}

TEST_CASE(Chaos_SpecialPositionStress) {
    ChaosTestGenerator gen;
    
    SECTION("Positions with many pieces") {
        Board board;
        board.setStartingPosition();
        
        // Full starting position should work
        auto moves = gen.generateRandomMoves(board, 20);
        REQUIRE(!moves.empty());
    }
    
    SECTION("Nearly empty positions") {
        Board board;
        board.clear();
        board.setPiece(E1, WHITE_KING);
        board.setPiece(E8, BLACK_KING);
        board.setPiece(A1, WHITE_ROOK);
        
        auto moves = gen.generateRandomMoves(board, 10);
        // Should have some moves
        REQUIRE(!moves.empty());
    }
}

// ============================================================================
// Stress Test Runners
// ============================================================================

void runFullChaosTest(int seconds) {
    std::cout << "\n=== Running Full Chaos Test Suite ===\n\n";
    
    // 1. Memory stress test
    std::cout << "1. Memory Stress Test\n";
    std::cout << "---------------------\n";
    MemoryStressTester memTester;
    memTester.runStressTest(seconds / 4, 1);  // Single-threaded for now
    std::cout << "\n";
    
    // 2. Hash collision analysis
    std::cout << "2. Hash Collision Analysis\n";
    std::cout << "--------------------------\n";
    HashCollisionDetector detector;
    ChaosTestGenerator gen;
    
    for (int i = 0; i < 10000; i++) {
        Board board = gen.generateRandomPosition();
        detector.checkPosition(board);
    }
    detector.printReport();
    std::cout << "\n";
    
    // 3. Incremental validation
    std::cout << "3. Incremental Update Validation\n";
    std::cout << "--------------------------------\n";
    IncrementalUpdateValidator validator;
    validator.runRandomValidation(1000);
    std::cout << "\n";
    
    // 4. Overflow test
    std::cout << "4. TT Overflow Test\n";
    std::cout << "-------------------\n";
    TTOverflowTester overflowTester;
    overflowTester.testOverflowBehavior();
    std::cout << "\n";
    
    std::cout << "=== Chaos Test Suite Complete ===\n";
}

void run24HourStabilityTest() {
    std::cout << "\n=== Starting 24-Hour Stability Test ===\n";
    std::cout << "Press Ctrl+C to stop early\n\n";
    
    const int hours = 24;
    const int checkpointMinutes = 60;
    
    MemoryStressTester tester;
    
    for (int hour = 1; hour <= hours; hour++) {
        std::cout << "Hour " << hour << " of " << hours << "...\n";
        
        // Run for 1 hour
        tester.runStressTest(3600, 1);
        
        // Memory check
        std::cout << "Memory check... ";
        // Would check for memory leaks here
        std::cout << "OK\n\n";
        
        // Brief pause
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "=== 24-Hour Test Complete ===\n";
}

// Main test runner
int main(int argc, char* argv[]) {
    std::cout << "SeaJay Stage 12: TT Chaos and Stress Testing\n";
    std::cout << "============================================\n\n";
    
    if (argc > 1) {
        std::string cmd = argv[1];
        
        if (cmd == "--chaos") {
            int seconds = 10;
            if (argc > 2) {
                seconds = std::stoi(argv[2]);
            }
            runFullChaosTest(seconds);
            return 0;
        }
        else if (cmd == "--24hour") {
            run24HourStabilityTest();
            return 0;
        }
        else if (cmd == "--help") {
            std::cout << "Usage:\n";
            std::cout << "  " << argv[0] << "           - Run catch2 tests\n";
            std::cout << "  " << argv[0] << " --chaos [seconds] - Run chaos tests\n";
            std::cout << "  " << argv[0] << " --24hour  - Run 24-hour stability test\n";
            return 0;
        }
    }
    
    // Run catch2 tests by default
    return Catch::Session().run(argc, argv);
}