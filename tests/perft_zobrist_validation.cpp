/**
 * Perft with Zobrist Hash Validation
 * Ensures hash consistency throughout the entire perft tree
 */

#include <iostream>
#include <chrono>
#include <iomanip>
#include <set>
#include <map>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"

using namespace seajay;
using namespace std::chrono;

// Global counters for validation
struct ZobristStats {
    uint64_t nodesVisited = 0;
    uint64_t hashesValidated = 0;
    uint64_t uniquePositions = 0;
    uint64_t collisions = 0;
    uint64_t validationErrors = 0;
    std::set<uint64_t> seenHashes;
    std::map<uint64_t, std::string> hashToFEN;  // For collision detection
};

// Helper to validate zobrist consistency
bool validateZobrist(Board& board, ZobristStats& stats) {
    stats.hashesValidated++;
    
    // Rebuild hash from scratch and compare
    uint64_t currentHash = board.zobristKey();
    board.rebuildZobristKey();
    uint64_t rebuiltHash = board.zobristKey();
    
    if (currentHash != rebuiltHash) {
        std::cerr << "\nZobrist mismatch detected!\n";
        std::cerr << "Position: " << board.toFEN() << "\n";
        std::cerr << "Current:  0x" << std::hex << currentHash << "\n";
        std::cerr << "Rebuilt:  0x" << rebuiltHash << std::dec << "\n";
        stats.validationErrors++;
        return false;
    }
    
    // Track unique positions and collisions
    if (stats.seenHashes.count(currentHash) == 0) {
        stats.seenHashes.insert(currentHash);
        stats.uniquePositions++;
        stats.hashToFEN[currentHash] = board.toFEN();
    } else {
        // Check if it's a true collision or same position
        std::string currentFEN = board.toFEN();
        if (stats.hashToFEN[currentHash] != currentFEN) {
            std::cerr << "\nHash collision detected!\n";
            std::cerr << "Hash: 0x" << std::hex << currentHash << std::dec << "\n";
            std::cerr << "Position 1: " << stats.hashToFEN[currentHash] << "\n";
            std::cerr << "Position 2: " << currentFEN << "\n";
            stats.collisions++;
        }
    }
    
    return true;
}

// Enhanced perft with zobrist validation
uint64_t perftWithValidation(Board& board, int depth, ZobristStats& stats, bool validate = true) {
    if (depth == 0) {
        stats.nodesVisited++;
        if (validate) {
            validateZobrist(board, stats);
        }
        return 1;
    }
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    if (depth == 1) {
        stats.nodesVisited += moves.size();
        if (validate) {
            // Validate each leaf position
            for (size_t i = 0; i < moves.size(); ++i) {
                Move move = moves[i];
                Board::UndoInfo undo;
                board.makeMove(move, undo);
                validateZobrist(board, stats);
                board.unmakeMove(move, undo);
            }
        }
        return moves.size();
    }
    
    uint64_t nodes = 0;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        Board::UndoInfo undo;
        
        // Save hash before move
        uint64_t hashBefore = board.zobristKey();
        
        board.makeMove(move, undo);
        
        // Validate incremental update
        if (validate && depth > 2) {  // Only validate at higher depths to save time
            validateZobrist(board, stats);
        }
        
        nodes += perftWithValidation(board, depth - 1, stats, validate);
        
        board.unmakeMove(move, undo);
        
        // Verify hash is restored correctly
        uint64_t hashAfter = board.zobristKey();
        if (hashBefore != hashAfter) {
            std::cerr << "\nHash not restored after unmake!\n";
            std::cerr << "Move: " << squareToString(moveFrom(move)) 
                     << squareToString(moveTo(move)) << "\n";
            std::cerr << "Before: 0x" << std::hex << hashBefore << "\n";
            std::cerr << "After:  0x" << hashAfter << std::dec << "\n";
            stats.validationErrors++;
        }
    }
    
    return nodes;
}

// Standard test positions
struct TestPosition {
    const char* fen;
    const char* description;
    int maxDepth;
    uint64_t expected[7];  // Expected nodes at depths 1-6
};

const TestPosition testPositions[] = {
    {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Starting position",
        6,
        {20, 400, 8902, 197281, 4865609, 119060324}
    },
    {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "Kiwipete",
        5,
        {48, 2039, 97862, 4085603, 193690690, 0}
    },
    {
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "Position 3",
        6,
        {14, 191, 2812, 43238, 674624, 11030083}
    },
    {
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "Position 4",
        5,
        {6, 264, 9467, 422333, 15833292, 0}
    },
    {
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "Position 5",
        5,
        {44, 1486, 62379, 2103487, 89941194, 0}
    }
};

// Helper function to format move
std::string formatMove(Move move) {
    std::string result = squareToString(moveFrom(move)) + squareToString(moveTo(move));
    if (isPromotion(move)) {
        const char* pieces = " nbrq";
        result += pieces[promotionType(move)];
    }
    return result;
}

// Perft divide with validation
void perftDivideWithValidation(Board& board, int depth) {
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    ZobristStats stats;
    uint64_t total = 0;
    
    std::cout << "\nPerft divide at depth " << depth << " with Zobrist validation:\n";
    std::cout << "--------------------------------------------------------\n";
    
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        Board::UndoInfo undo;
        
        uint64_t hashBefore = board.zobristKey();
        board.makeMove(move, undo);
        
        ZobristStats moveStats;
        uint64_t nodes = (depth > 1) ? perftWithValidation(board, depth - 1, moveStats, true) : 1;
        
        board.unmakeMove(move, undo);
        uint64_t hashAfter = board.zobristKey();
        
        // Verify hash restoration
        if (hashBefore != hashAfter) {
            std::cout << "ERROR: Hash mismatch for move " << formatMove(move) << "\n";
        }
        
        std::cout << std::left << std::setw(8) << formatMove(move) 
                  << std::right << std::setw(12) << nodes;
        
        if (moveStats.validationErrors > 0) {
            std::cout << " [" << moveStats.validationErrors << " ERRORS]";
        }
        std::cout << "\n";
        
        total += nodes;
        stats.nodesVisited += moveStats.nodesVisited;
        stats.hashesValidated += moveStats.hashesValidated;
        stats.validationErrors += moveStats.validationErrors;
    }
    
    std::cout << "--------------------------------------------------------\n";
    std::cout << "Total nodes: " << total << "\n";
    std::cout << "Hashes validated: " << stats.hashesValidated << "\n";
    if (stats.validationErrors > 0) {
        std::cout << "VALIDATION ERRORS: " << stats.validationErrors << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::cout << "SeaJay Perft with Zobrist Validation\n";
    std::cout << "=====================================\n";
    
    bool quickMode = false;
    bool divideMode = false;
    int positionIndex = 0;
    int depth = 4;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--quick") {
            quickMode = true;
        } else if (arg == "--divide") {
            divideMode = true;
        } else if (arg == "--position" && i + 1 < argc) {
            positionIndex = std::stoi(argv[++i]);
        } else if (arg == "--depth" && i + 1 < argc) {
            depth = std::stoi(argv[++i]);
        }
    }
    
    if (divideMode) {
        // Divide mode for debugging
        Board board;
        board.parseFEN(testPositions[positionIndex].fen);
        std::cout << "\nPosition: " << testPositions[positionIndex].description << "\n";
        std::cout << "FEN: " << testPositions[positionIndex].fen << "\n";
        perftDivideWithValidation(board, depth);
        return 0;
    }
    
    // Run standard perft tests with validation
    for (const auto& test : testPositions) {
        Board board;
        board.parseFEN(test.fen);
        
        std::cout << "\n" << test.description << ":\n";
        std::cout << "FEN: " << test.fen << "\n";
        std::cout << "Initial hash: 0x" << std::hex << board.zobristKey() << std::dec << "\n";
        std::cout << "\nDepth | Nodes       | Expected    | Time (ms) | Validated | Errors\n";
        std::cout << "------|-------------|-------------|-----------|-----------|-------\n";
        
        int maxDepth = quickMode ? std::min(4, test.maxDepth) : test.maxDepth;
        
        for (int d = 1; d <= maxDepth; ++d) {
            ZobristStats stats;
            auto start = high_resolution_clock::now();
            
            uint64_t nodes = perftWithValidation(board, d, stats, true);
            
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start);
            
            std::cout << std::setw(5) << d << " | "
                     << std::setw(11) << nodes << " | "
                     << std::setw(11) << test.expected[d-1] << " | "
                     << std::setw(9) << duration.count() << " | "
                     << std::setw(9) << stats.hashesValidated << " | "
                     << std::setw(5) << stats.validationErrors;
            
            if (nodes != test.expected[d-1] && test.expected[d-1] != 0) {
                std::cout << " FAIL";
            }
            std::cout << "\n";
            
            // Stop if errors detected
            if (stats.validationErrors > 0) {
                std::cout << "\nStopping due to validation errors!\n";
                break;
            }
            
            // Skip very deep tests if they take too long
            if (duration.count() > 5000 && d < maxDepth) {
                std::cout << "(Skipping deeper depths due to time)\n";
                break;
            }
        }
    }
    
    std::cout << "\n=== Perft Zobrist Validation Complete ===\n";
    return 0;
}