#include "perft.h"
#include "move_generation.h"
#include "move_list.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace seajay {

// Standard perft test positions
static const std::vector<Perft::TestPosition> STANDARD_POSITIONS = {
    {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Starting position",
        {{1, 20}, {2, 400}, {3, 8902}, {4, 197281}, {5, 4865609}, {6, 119060324}}
    },
    {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "Kiwipete",
        {{1, 48}, {2, 2039}, {3, 97862}, {4, 4085603}, {5, 193690690}}
    },
    {
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "Position 3",
        {{1, 14}, {2, 191}, {3, 2812}, {4, 43238}, {5, 674624}, {6, 11030083}}
    },
    {
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "Position 4",
        {{1, 6}, {2, 264}, {3, 9467}, {4, 422333}, {5, 15833292}}
    },
    {
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "Position 5",
        {{1, 44}, {2, 1486}, {3, 62379}, {4, 2103487}, {5, 89941194}}
    },
    {
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "Position 6",
        {{1, 46}, {2, 2079}, {3, 89890}, {4, 3894594}, {5, 164075551}}
    }
};

const std::vector<Perft::TestPosition>& Perft::getStandardPositions() {
    return STANDARD_POSITIONS;
}

// Basic perft without TT
uint64_t Perft::perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    // Bulk counting optimization for depth 1
    if (depth == 1) {
        return moves.size();
    }
    
    uint64_t nodes = 0;
    for (Move move : moves) {
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        nodes += perft(board, depth - 1);
        board.unmakeMove(move, undo);
    }
    
    return nodes;
}

// Perft with TT caching - main entry point
uint64_t Perft::perftWithTT(Board& board, int depth, TranspositionTable& tt) {
    // For perft, we want a completely clean TT to ensure accurate caching
    // Each position at each depth should be cached separately
    return perftTTInternal(board, depth, tt);
}

// Internal TT perft implementation
uint64_t Perft::perftTTInternal(Board& board, int depth, TranspositionTable& tt) {
    if (depth == 0) return 1;
    
    // Probe TT first
    Hash key = board.zobristKey();
    TTEntry* entry = tt.probe(key);
    
    // Check if we have a cached result for this exact depth
    if (entry && entry->depth == depth && entry->bound() == Bound::EXACT) {
        // Check if this is a valid cached value (not -1)
        if (entry->score >= 0) {
            return decodeNodeCount(entry->score);
        }
        // If score is -1, it means the value was too large to cache
        // Fall through to recalculate
    }
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    // Bulk counting optimization for depth 1
    if (depth == 1) {
        uint64_t nodes = moves.size();
        // Store in TT for future use
        tt.store(key, 0, encodeNodeCount(nodes), 0, static_cast<uint8_t>(depth), Bound::EXACT);
        return nodes;
    }
    
    uint64_t nodes = 0;
    for (Move move : moves) {
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        nodes += perftTTInternal(board, depth - 1, tt);
        board.unmakeMove(move, undo);
    }
    
    // Store the result in TT only if it fits in our encoding
    // For perft, we don't have a best move, so we store 0
    int16_t encodedNodes = encodeNodeCount(nodes);
    if (encodedNodes >= 0) {
        // Only store if we can encode the value properly
        tt.store(key, 0, encodedNodes, 0, static_cast<uint8_t>(depth), Bound::EXACT);
    }
    // If encodedNodes is -1, we don't store it (value too large)
    
    return nodes;
}

// Helper to encode node count in 16-bit score field
// Since perft can have very large node counts, we use a special encoding:
// - For values <= 32767: store directly
// - For larger values: return -1 to indicate "too large to cache"
int16_t Perft::encodeNodeCount(uint64_t nodes) {
    if (nodes <= 32767) {
        return static_cast<int16_t>(nodes);
    }
    // For larger values, we can't store them accurately in 16 bits
    // Return a special value indicating "too large to cache"
    return -1;  // Special marker for "do not cache"
}

uint64_t Perft::decodeNodeCount(int16_t score) {
    if (score >= 0) {
        return static_cast<uint64_t>(score);
    }
    // Special value -1 means "not cached" - should never reach here
    return 0;  // Invalid - should not be used
}

// Perft divide for debugging
Perft::DivideResult Perft::perftDivide(Board& board, int depth) {
    DivideResult result;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    for (Move move : moves) {
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        uint64_t nodes = (depth > 1) ? perft(board, depth - 1) : 1;
        
        board.unmakeMove(move, undo);
        
        // Format move string
        std::stringstream ss;
        ss << squareToString(moveFrom(move)) << squareToString(moveTo(move));
        if (isPromotion(move)) {
            char promoChar = '?';
            switch (promotionType(move)) {
                case QUEEN:  promoChar = 'q'; break;
                case ROOK:   promoChar = 'r'; break;
                case BISHOP: promoChar = 'b'; break;
                case KNIGHT: promoChar = 'n'; break;
                default: break;
            }
            ss << promoChar;
        }
        
        result.moveNodes[ss.str()] = nodes;
        result.totalNodes += nodes;
    }
    
    return result;
}

// Perft divide with TT
Perft::DivideResult Perft::perftDivideWithTT(Board& board, int depth, TranspositionTable& tt) {
    DivideResult result;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    for (Move move : moves) {
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        uint64_t nodes = (depth > 1) ? perftTTInternal(board, depth - 1, tt) : 1;
        
        board.unmakeMove(move, undo);
        
        // Format move string
        std::stringstream ss;
        ss << squareToString(moveFrom(move)) << squareToString(moveTo(move));
        if (isPromotion(move)) {
            char promoChar = '?';
            switch (promotionType(move)) {
                case QUEEN:  promoChar = 'q'; break;
                case ROOK:   promoChar = 'r'; break;
                case BISHOP: promoChar = 'b'; break;
                case KNIGHT: promoChar = 'n'; break;
                default: break;
            }
            ss << promoChar;
        }
        
        result.moveNodes[ss.str()] = nodes;
        result.totalNodes += nodes;
    }
    
    return result;
}

// Run perft with timing
Perft::Result Perft::runPerft(Board& board, int depth, bool useTT, TranspositionTable* tt) {
    Result result;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    if (useTT && tt) {
        result.nodes = perftWithTT(board, depth, *tt);
    } else {
        result.nodes = perft(board, depth);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    result.timeSeconds = duration.count() / 1e9;
    
    return result;
}

// Validate a position
bool Perft::validatePosition(const std::string& fen, int depth, 
                             uint64_t expectedNodes, bool useTT,
                             TranspositionTable* tt) {
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Failed to parse FEN: " << fen << std::endl;
        return false;
    }
    
    Result result = runPerft(board, depth, useTT, tt);
    
    if (result.nodes != expectedNodes) {
        std::cerr << "Perft mismatch at depth " << depth << ":\n";
        std::cerr << "  Expected: " << expectedNodes << "\n";
        std::cerr << "  Got:      " << result.nodes << "\n";
        std::cerr << "  Diff:     " << static_cast<int64_t>(result.nodes - expectedNodes) << "\n";
        return false;
    }
    
    return true;
}

// Run standard test suite
bool Perft::runStandardTests(int maxDepth, bool useTT, TranspositionTable* tt) {
    bool allPassed = true;
    
    std::cout << "\nRunning Perft Test Suite" 
              << (useTT ? " (with TT)" : " (without TT)") << "\n";
    std::cout << "================================\n\n";
    
    for (const auto& pos : STANDARD_POSITIONS) {
        std::cout << pos.description << ":\n";
        std::cout << "FEN: " << pos.fen << "\n";
        
        Board board;
        if (!board.fromFEN(pos.fen)) {
            std::cerr << "Failed to parse FEN\n";
            allPassed = false;
            continue;
        }
        
        for (const auto& [depth, expected] : pos.expectedNodes) {
            if (depth > maxDepth) continue;
            
            std::cout << "  Depth " << depth << ": ";
            std::cout.flush();
            
            Result result = runPerft(board, depth, useTT, tt);
            
            if (result.nodes == expected) {
                std::cout << "✓ " << result.nodes << " nodes";
                std::cout << " (" << std::fixed << std::setprecision(3) 
                         << result.timeSeconds << "s, ";
                std::cout << std::fixed << std::setprecision(0) 
                         << result.nps() << " nps)\n";
            } else {
                std::cout << "✗ Expected " << expected 
                         << ", got " << result.nodes << "\n";
                allPassed = false;
            }
        }
        std::cout << "\n";
    }
    
    return allPassed;
}

} // namespace seajay