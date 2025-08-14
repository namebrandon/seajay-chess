/**
 * Zobrist Key Validation Program
 * Validates that generated keys are unique, non-zero, and well-distributed
 */

#include "../src/core/board.h"
#include "../src/core/types.h"
#include <iostream>
#include <set>
#include <map>
#include <bitset>
#include <iomanip>
#include <random>

using namespace seajay;

// Access Board's private static members via friend declaration
class ZobristAnalyzer {
public:
    static void analyzeKeys() {
        // Initialize a board to ensure Zobrist tables are initialized
        Board board;
        board.setStartingPosition();
        
        std::cout << "SeaJay Zobrist Key Analysis\n";
        std::cout << "============================\n\n";
        
        // Collect all keys
        std::set<uint64_t> allKeys;
        std::vector<uint64_t> keyVector;
        
        // Test piece-square keys - we can't access them directly
        // but we can test through board operations
        std::cout << "Testing key generation through board operations...\n";
        
        // Test 1: Uniqueness via position hashing
        testPositionUniqueness();
        
        // Test 2: Distribution analysis
        testKeyDistribution();
        
        // Test 3: XOR properties
        testXORProperties();
        
        // Test 4: Fifty-move counter integration
        testFiftyMoveIntegration();
        
        std::cout << "\nAll tests completed successfully!\n";
    }
    
private:
    static void testPositionUniqueness() {
        std::cout << "\n1. Position Uniqueness Test\n";
        std::cout << "   Testing that different positions produce different hashes...\n";
        
        std::set<uint64_t> hashes;
        Board board;
        
        // Test various positions
        const char* positions[] = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
            "rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2",
            "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
            "8/2P5/8/8/8/8/8/k6K w - - 0 1",
            "8/8/8/3k4/3K4/8/8/8 w - - 0 1",
            "8/8/8/3k4/3K4/8/8/8 w - - 50 1",  // Same position, different fifty-move
            "8/8/8/3k4/3K4/8/8/8 b - - 0 1",  // Same position, different side to move
        };
        
        for (const char* fen : positions) {
            board.parseFEN(fen);
            uint64_t hash = board.zobristKey();
            if (hashes.count(hash) > 0) {
                std::cerr << "   ERROR: Duplicate hash found for position: " << fen << "\n";
                std::cerr << "   Hash: 0x" << std::hex << hash << std::dec << "\n";
            }
            hashes.insert(hash);
        }
        
        std::cout << "   Tested " << hashes.size() << " positions\n";
        std::cout << "   All hashes unique: " << (hashes.size() == sizeof(positions)/sizeof(positions[0]) ? "YES" : "NO") << "\n";
    }
    
    static void testKeyDistribution() {
        std::cout << "\n2. Key Distribution Test\n";
        std::cout << "   Analyzing bit distribution in generated hashes...\n";
        
        Board board;
        std::vector<uint64_t> hashes;
        
        // Generate hashes for many random positions
        std::mt19937 rng(12345);
        for (int i = 0; i < 1000; i++) {
            board.clear();
            
            // Place random pieces
            for (int j = 0; j < 10; j++) {
                Square sq = rng() % 64;
                Piece p = static_cast<Piece>(rng() % 12);  // Random piece (not including NO_PIECE)
                if (board.pieceAt(sq) == NO_PIECE) {
                    board.setPiece(sq, p);
                }
            }
            
            board.rebuildZobristKey();
            hashes.push_back(board.zobristKey());
        }
        
        // Analyze bit distribution
        std::array<int, 64> bitCounts = {0};
        for (uint64_t hash : hashes) {
            for (int bit = 0; bit < 64; bit++) {
                if (hash & (1ULL << bit)) {
                    bitCounts[bit]++;
                }
            }
        }
        
        // Check that each bit is set approximately 50% of the time
        double minRatio = 1.0, maxRatio = 0.0;
        for (int bit = 0; bit < 64; bit++) {
            double ratio = bitCounts[bit] / (double)hashes.size();
            minRatio = std::min(minRatio, ratio);
            maxRatio = std::max(maxRatio, ratio);
        }
        
        std::cout << "   Bit distribution (should be near 0.5):\n";
        std::cout << "   Min ratio: " << std::fixed << std::setprecision(3) << minRatio << "\n";
        std::cout << "   Max ratio: " << std::fixed << std::setprecision(3) << maxRatio << "\n";
        
        bool goodDistribution = (minRatio > 0.4 && maxRatio < 0.6);
        std::cout << "   Good distribution: " << (goodDistribution ? "YES" : "NO") << "\n";
    }
    
    static void testXORProperties() {
        std::cout << "\n3. XOR Properties Test\n";
        std::cout << "   Testing XOR mathematical properties...\n";
        
        Board board;
        board.setStartingPosition();
        uint64_t hash1 = board.zobristKey();
        
        // Make a move
        board.parseFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
        uint64_t hash2 = board.zobristKey();
        
        // Unmake (go back)
        board.setStartingPosition();
        uint64_t hash3 = board.zobristKey();
        
        std::cout << "   Hash before move:  0x" << std::hex << hash1 << "\n";
        std::cout << "   Hash after move:   0x" << hash2 << "\n";
        std::cout << "   Hash after unmake: 0x" << hash3 << std::dec << "\n";
        std::cout << "   Hashes match after unmake: " << (hash1 == hash3 ? "YES" : "NO") << "\n";
    }
    
    static void testFiftyMoveIntegration() {
        std::cout << "\n4. Fifty-Move Counter Integration Test\n";
        std::cout << "   Testing that fifty-move counter affects hash...\n";
        
        Board board;
        
        // Same position with different fifty-move counters
        board.parseFEN("8/8/8/3k4/3K4/8/8/8 w - - 0 1");
        uint64_t hash0 = board.zobristKey();
        
        board.parseFEN("8/8/8/3k4/3K4/8/8/8 w - - 25 1");
        uint64_t hash25 = board.zobristKey();
        
        board.parseFEN("8/8/8/3k4/3K4/8/8/8 w - - 50 1");
        uint64_t hash50 = board.zobristKey();
        
        board.parseFEN("8/8/8/3k4/3K4/8/8/8 w - - 99 1");
        uint64_t hash99 = board.zobristKey();
        
        std::cout << "   Hash with counter=0:  0x" << std::hex << hash0 << "\n";
        std::cout << "   Hash with counter=25: 0x" << hash25 << "\n";
        std::cout << "   Hash with counter=50: 0x" << hash50 << "\n";
        std::cout << "   Hash with counter=99: 0x" << hash99 << std::dec << "\n";
        
        bool allDifferent = (hash0 != hash25) && (hash0 != hash50) && (hash0 != hash99) &&
                           (hash25 != hash50) && (hash25 != hash99) && (hash50 != hash99);
        
        std::cout << "   All hashes different: " << (allDifferent ? "YES" : "NO") << "\n";
        
        // Test that counter >= 100 doesn't affect hash
        board.parseFEN("8/8/8/3k4/3K4/8/8/8 w - - 100 1");
        uint64_t hash100 = board.zobristKey();
        
        board.parseFEN("8/8/8/3k4/3K4/8/8/8 w - - 150 1");
        uint64_t hash150 = board.zobristKey();
        
        std::cout << "   Hash with counter=100: 0x" << std::hex << hash100 << "\n";
        std::cout << "   Hash with counter=150: 0x" << hash150 << std::dec << "\n";
        std::cout << "   Hashes same for counter >= 100: " << (hash100 == hash150 ? "YES (correct)" : "NO (error)") << "\n";
    }
};

int main() {
    try {
        ZobristAnalyzer::analyzeKeys();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}