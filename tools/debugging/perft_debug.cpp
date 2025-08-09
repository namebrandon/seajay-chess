#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"

using namespace seajay;

struct StockfishResult {
    std::string move;
    uint64_t nodes;
};

class PerftDebugger {
private:
    Board board;
    
    // Get Stockfish perft divide results
    std::vector<StockfishResult> getStockfishDivide(const std::string& fen, int depth) {
        std::vector<StockfishResult> results;
        
        // Create command file for Stockfish
        std::ofstream cmdFile("/tmp/sf_cmd.txt");
        cmdFile << "position fen " << fen << "\n";
        cmdFile << "go perft " << depth << "\n";
        cmdFile << "quit\n";
        cmdFile.close();
        
        // Run Stockfish and capture output
        std::string cmd = "./external/engines/stockfish/stockfish < /tmp/sf_cmd.txt 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "Failed to run Stockfish\n";
            return results;
        }
        
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            
            // Parse lines like "e2e3: 45326"
            size_t colonPos = line.find(": ");
            if (colonPos != std::string::npos) {
                std::string moveStr = line.substr(0, colonPos);
                std::string nodeStr = line.substr(colonPos + 2);
                
                // Clean up move string
                if (moveStr.length() >= 4 && moveStr.length() <= 5) {
                    uint64_t nodes = std::stoull(nodeStr);
                    results.push_back({moveStr, nodes});
                }
            }
        }
        pclose(pipe);
        
        return results;
    }
    
    // Get our engine's perft divide results
    std::map<std::string, uint64_t> getSeaJayDivide(const std::string& fen, int depth) {
        std::map<std::string, uint64_t> results;
        
        if (!board.fromFEN(fen)) {
            std::cerr << "Invalid FEN: " << fen << "\n";
            return results;
        }
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        for (size_t i = 0; i < moves.size(); ++i) {
            Move move = moves[i];
            Board::UndoInfo undo;
            board.makeMove(move, undo);
            
            uint64_t nodes = (depth > 1) ? perft(board, depth - 1) : 1;
            board.unmakeMove(move, undo);
            
            // Format move string to match Stockfish
            std::string moveStr = squareToString(moveFrom(move)) + squareToString(moveTo(move));
            if (isPromotion(move)) {
                PieceType promo = promotionType(move);
                const char promoChar[] = {'?', 'n', 'b', 'r', 'q'};
                moveStr += promoChar[promo];
            }
            
            results[moveStr] = nodes;
        }
        
        return results;
    }
    
    // Recursive perft function
    uint64_t perft(Board& b, int depth) {
        if (depth == 0) return 1;
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(b, moves);
        
        if (depth == 1) return moves.size();
        
        uint64_t nodes = 0;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move move = moves[i];
            Board::UndoInfo undo;
            b.makeMove(move, undo);
            nodes += perft(b, depth - 1);
            b.unmakeMove(move, undo);
        }
        
        return nodes;
    }
    
public:
    // Compare our results with Stockfish
    void compareWithStockfish(const std::string& fen, int depth) {
        std::cout << "Comparing SeaJay vs Stockfish at depth " << depth << "\n";
        std::cout << "FEN: " << fen << "\n";
        std::cout << std::string(80, '=') << "\n";
        
        auto stockfishResults = getStockfishDivide(fen, depth);
        auto seajayResults = getSeaJayDivide(fen, depth);
        
        std::cout << std::left << std::setw(8) << "Move" 
                  << std::right << std::setw(12) << "Stockfish"
                  << std::setw(12) << "SeaJay" 
                  << std::setw(10) << "Diff" << "\n";
        std::cout << std::string(42, '-') << "\n";
        
        uint64_t stockfishTotal = 0, seajayTotal = 0;
        int discrepancies = 0;
        
        // Create combined list of all moves
        std::set<std::string> allMoves;
        for (const auto& result : stockfishResults) {
            allMoves.insert(result.move);
        }
        for (const auto& result : seajayResults) {
            allMoves.insert(result.first);
        }
        
        for (const std::string& moveStr : allMoves) {
            uint64_t sfNodes = 0, sjNodes = 0;
            
            // Find Stockfish result
            for (const auto& result : stockfishResults) {
                if (result.move == moveStr) {
                    sfNodes = result.nodes;
                    break;
                }
            }
            
            // Find SeaJay result
            auto it = seajayResults.find(moveStr);
            if (it != seajayResults.end()) {
                sjNodes = it->second;
            }
            
            int64_t diff = static_cast<int64_t>(sjNodes) - static_cast<int64_t>(sfNodes);
            
            std::cout << std::left << std::setw(8) << moveStr
                      << std::right << std::setw(12) << sfNodes
                      << std::setw(12) << sjNodes;
            
            if (diff != 0) {
                std::cout << std::setw(10) << diff << " ❌";
                discrepancies++;
            } else {
                std::cout << std::setw(10) << "0" << " ✅";
            }
            std::cout << "\n";
            
            stockfishTotal += sfNodes;
            seajayTotal += sjNodes;
        }
        
        std::cout << std::string(42, '-') << "\n";
        std::cout << std::left << std::setw(8) << "TOTAL"
                  << std::right << std::setw(12) << stockfishTotal
                  << std::setw(12) << seajayTotal
                  << std::setw(10) << (static_cast<int64_t>(seajayTotal) - static_cast<int64_t>(stockfishTotal)) << "\n\n";
        
        if (discrepancies == 0) {
            std::cout << "✅ All moves match perfectly!\n";
        } else {
            std::cout << "❌ Found " << discrepancies << " discrepant moves\n";
            std::cout << "Total deficit: " << (static_cast<int64_t>(seajayTotal) - static_cast<int64_t>(stockfishTotal)) << " nodes\n";
        }
    }
    
    // Drill down into a specific move to find deeper discrepancies
    void drillDown(const std::string& fen, const std::string& moveStr, int depth) {
        std::cout << "\nDrilling down into move: " << moveStr << " at depth " << depth << "\n";
        std::cout << "Starting FEN: " << fen << "\n";
        
        if (!board.fromFEN(fen)) {
            std::cerr << "Invalid FEN\n";
            return;
        }
        
        // Find and make the specified move
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        Move targetMove = Move();
        for (size_t i = 0; i < moves.size(); ++i) {
            Move move = moves[i];
            std::string currentMoveStr = squareToString(moveFrom(move)) + squareToString(moveTo(move));
            if (isPromotion(move)) {
                PieceType promo = promotionType(move);
                const char promoChar[] = {'?', 'n', 'b', 'r', 'q'};
                currentMoveStr += promoChar[promo];
            }
            
            if (currentMoveStr == moveStr) {
                targetMove = move;
                break;
            }
        }
        
        if (targetMove == Move()) {
            std::cerr << "Move " << moveStr << " not found in position\n";
            return;
        }
        
        // Make the move and get resulting position
        Board::UndoInfo undo;
        board.makeMove(targetMove, undo);
        
        std::string newFen = board.toFEN();
        std::cout << "After move " << moveStr << ": " << newFen << "\n";
        
        // Compare the resulting position
        compareWithStockfish(newFen, depth - 1);
        
        board.unmakeMove(targetMove, undo);
    }
    
    // Automated analysis to find the exact point of divergence
    void findDivergence(const std::string& fen, int maxDepth = 4) {
        std::cout << "🔍 Automated divergence analysis starting...\n";
        std::cout << "Base FEN: " << fen << "\n";
        std::cout << "Max depth: " << maxDepth << "\n\n";
        
        findDivergenceRecursive(fen, maxDepth, "");
    }
    
private:
    void findDivergenceRecursive(const std::string& fen, int depth, const std::string& path) {
        if (depth <= 0) return;
        
        auto stockfishResults = getStockfishDivide(fen, depth);
        auto seajayResults = getSeaJayDivide(fen, depth);
        
        // Check for discrepancies
        for (const auto& sfResult : stockfishResults) {
            auto it = seajayResults.find(sfResult.move);
            uint64_t sjNodes = (it != seajayResults.end()) ? it->second : 0;
            
            if (sjNodes != sfResult.nodes) {
                std::cout << "🎯 DISCREPANCY FOUND!\n";
                std::cout << "Path: " << path << " -> " << sfResult.move << "\n";
                std::cout << "Depth: " << depth << "\n";
                std::cout << "Stockfish: " << sfResult.nodes << " nodes\n";
                std::cout << "SeaJay: " << sjNodes << " nodes\n";
                std::cout << "Difference: " << (static_cast<int64_t>(sjNodes) - static_cast<int64_t>(sfResult.nodes)) << "\n\n";
                
                // If difference is significant and we have depth left, drill deeper
                if (depth > 1 && abs(static_cast<int64_t>(sjNodes) - static_cast<int64_t>(sfResult.nodes)) > 10) {
                    std::string newPath = path.empty() ? sfResult.move : path + " " + sfResult.move;
                    
                    // Make the move and get new FEN
                    if (board.fromFEN(fen)) {
                        MoveList moves;
                        MoveGenerator::generateLegalMoves(board, moves);
                        
                        for (size_t i = 0; i < moves.size(); ++i) {
                            Move move = moves[i];
                            std::string moveStr = squareToString(moveFrom(move)) + squareToString(moveTo(move));
                            if (isPromotion(move)) {
                                PieceType promo = promotionType(move);
                                const char promoChar[] = {'?', 'n', 'b', 'r', 'q'};
                                moveStr += promoChar[promo];
                            }
                            
                            if (moveStr == sfResult.move) {
                                Board::UndoInfo undo;
                                board.makeMove(move, undo);
                                std::string newFen = board.toFEN();
                                findDivergenceRecursive(newFen, depth - 1, newPath);
                                board.unmakeMove(move, undo);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  perft_debug compare <fen> <depth>    - Compare with Stockfish\n";
        std::cout << "  perft_debug drill <fen> <move> <depth> - Drill down into specific move\n";
        std::cout << "  perft_debug find <fen> [maxdepth]    - Find exact divergence point\n";
        std::cout << "\nExample for Position 3:\n";
        std::cout << "  perft_debug compare \"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1\" 5\n";
        return 1;
    }
    
    PerftDebugger debugger;
    std::string command = argv[1];
    
    if (command == "compare" && argc >= 4) {
        std::string fen = argv[2];
        int depth = std::atoi(argv[3]);
        debugger.compareWithStockfish(fen, depth);
    }
    else if (command == "drill" && argc >= 5) {
        std::string fen = argv[2];
        std::string move = argv[3];
        int depth = std::atoi(argv[4]);
        debugger.drillDown(fen, move, depth);
    }
    else if (command == "find" && argc >= 3) {
        std::string fen = argv[2];
        int maxDepth = (argc >= 4) ? std::atoi(argv[3]) : 4;
        debugger.findDivergence(fen, maxDepth);
    }
    else {
        std::cerr << "Invalid command or arguments\n";
        return 1;
    }
    
    return 0;
}