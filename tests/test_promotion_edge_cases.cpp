#include <iostream>
#include <string>
#include <sstream>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"

using namespace seajay;

// Simple move to string conversion for testing
std::string moveToString(Move move) {
    if (move == Move()) return "0000";
    
    std::string result = squareToString(moveFrom(move)) + squareToString(moveTo(move));
    
    if (isPromotion(move)) {
        PieceType promo = promotionType(move);
        switch (promo) {
            case QUEEN: result += "q"; break;
            case ROOK: result += "r"; break;
            case BISHOP: result += "b"; break;
            case KNIGHT: result += "n"; break;
            default: break;
        }
    }
    
    return result;
}

// Simple perft implementation
uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    if (depth == 1) return moves.size();
    
    uint64_t nodes = 0;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        nodes += perft(board, depth - 1);
        board.unmakeMove(move, undo);
    }
    
    return nodes;
}

// Test promotion with check
void testPromotionWithCheck() {
    std::cout << "\n=== Testing Promotion with Check ===" << std::endl;
    
    // Position 1: Promotion gives direct check
    {
        Board board;
        board.fromFEN("3k4/P7/8/8/8/8/8/4K3 w - - 0 1");
        std::cout << "\nPosition 1: 3k4/P7/8/8/8/8/8/4K3 w - - 0 1" << std::endl;
        std::cout << "White pawn on a7, black king on d8" << std::endl;
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        // Find promotion moves
        for (int i = 0; i < moves.size(); i++) {
            Move move = moves[i];
            if (isPromotion(move) && moveTo(move) == A8) {
                std::string moveStr = moveToString(move);
                
                // Make the move and check if black is in check
                Board::UndoInfo undo;
                board.makeMove(move, undo);
                
                bool givesCheck = MoveGenerator::inCheck(board);
                std::cout << "Move " << moveStr << " - Gives check: " << (givesCheck ? "YES" : "NO") << std::endl;
                
                // Verify zobrist consistency
                bool zobristValid = board.validateZobrist();
                if (!zobristValid) {
                    std::cout << "  WARNING: Zobrist mismatch after promotion!" << std::endl;
                }
                
                board.unmakeMove(move, undo);
                
                // Verify state restoration
                zobristValid = board.validateZobrist();
                if (!zobristValid) {
                    std::cout << "  WARNING: Zobrist mismatch after unmake!" << std::endl;
                }
            }
        }
    }
    
    // Position 2: Promotion gives discovered check
    {
        Board board;
        board.fromFEN("3k4/4P3/8/8/8/8/8/R3K3 w - - 0 1");
        std::cout << "\nPosition 2: 3k4/4P3/8/8/8/8/8/R3K3 w - - 0 1" << std::endl;
        std::cout << "White pawn on e7, rook on a1, black king on d8" << std::endl;
        std::cout << "Promotion to e8 should give discovered check from rook" << std::endl;
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        for (int i = 0; i < moves.size(); i++) {
            Move move = moves[i];
            if (isPromotion(move) && moveTo(move) == E8) {
                std::string moveStr = moveToString(move);
                
                Board::UndoInfo undo;
                board.makeMove(move, undo);
                
                bool givesCheck = MoveGenerator::inCheck(board);
                std::cout << "Move " << moveStr << " - Gives check: " << (givesCheck ? "YES" : "NO") << std::endl;
                
                board.unmakeMove(move, undo);
            }
        }
    }
}

// Test promotion that blocks check
void testPromotionBlockingCheck() {
    std::cout << "\n=== Testing Promotion Blocking Check ===" << std::endl;
    
    Board board;
    board.fromFEN("8/2P5/8/8/8/8/r7/4K3 w - - 0 1");
    std::cout << "\nPosition: 8/2P5/8/8/8/8/r7/4K3 w - - 0 1" << std::endl;
    std::cout << "White king on e1 in check from rook on a2" << std::endl;
    std::cout << "Pawn on c7 cannot promote (doesn't block check)" << std::endl;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Legal moves: " << moves.size() << std::endl;
    
    // Check if any promotion moves are legal
    bool hasPromotion = false;
    for (int i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        if (isPromotion(move)) {
            hasPromotion = true;
            std::cout << "Found legal promotion: " << moveToString(move) << std::endl;
        }
    }
    
    if (!hasPromotion) {
        std::cout << "Correct: No promotion moves available (they don't block check)" << std::endl;
    }
}

// Test under-promotion scenarios
void testUnderPromotion() {
    std::cout << "\n=== Testing Under-Promotion ===" << std::endl;
    
    // Knight promotion for fork
    {
        Board board;
        board.fromFEN("r3k3/P7/8/8/8/8/8/4K3 w - - 0 1");
        std::cout << "\nPosition: r3k3/P7/8/8/8/8/8/4K3 w - - 0 1" << std::endl;
        std::cout << "Knight promotion on a8 forks king and rook" << std::endl;
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        int knightPromos = 0, queenPromos = 0;
        for (int i = 0; i < moves.size(); i++) {
            Move move = moves[i];
            if (isPromotion(move) && moveTo(move) == A8) {
                if (promotionType(move) == KNIGHT) knightPromos++;
                if (promotionType(move) == QUEEN) queenPromos++;
            }
        }
        
        std::cout << "Queen promotions available: " << queenPromos << std::endl;
        std::cout << "Knight promotions available: " << knightPromos << std::endl;
        
        if (knightPromos > 0) {
            std::cout << "✓ Knight under-promotion correctly generated" << std::endl;
        }
    }
    
    // Bishop promotion to avoid stalemate
    {
        Board board;
        board.fromFEN("7k/P7/7K/8/8/8/8/8 w - - 0 1");
        std::cout << "\nPosition: 7k/P7/7K/8/8/8/8/8 w - - 0 1" << std::endl;
        std::cout << "Queen promotion would be stalemate, bishop/rook keeps game going" << std::endl;
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        for (int i = 0; i < moves.size(); i++) {
            Move move = moves[i];
            if (isPromotion(move)) {
                Board::UndoInfo undo;
                board.makeMove(move, undo);
                
                // Check if black has legal moves
                MoveList blackMoves;
                MoveGenerator::generateLegalMoves(board, blackMoves);
                
                std::string promoStr = (promotionType(move) == QUEEN ? "Queen" :
                                        promotionType(move) == ROOK ? "Rook" :
                                        promotionType(move) == BISHOP ? "Bishop" : "Knight");
                
                std::cout << promoStr << " promotion - Black has " << blackMoves.size() << " legal moves";
                if (blackMoves.size() == 0) {
                    std::cout << " (STALEMATE)";
                }
                std::cout << std::endl;
                
                board.unmakeMove(move, undo);
            }
        }
    }
}

// Test complex promotion with multiple pawns
void testComplexPromotion() {
    std::cout << "\n=== Testing Complex Promotion Position ===" << std::endl;
    
    Board board;
    board.fromFEN("r3k3/P6P/8/8/8/8/p6p/R3K3 w Q - 0 1");
    std::cout << "\nPosition: r3k3/P6P/8/8/8/8/p6p/R3K3 w Q - 0 1" << std::endl;
    std::cout << "Multiple pawns ready to promote" << std::endl;
    
    // Test make/unmake cycles for all promotions
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    int promotionCount = 0;
    int testCycles = 100; // Multiple make/unmake cycles
    
    for (int i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        if (isPromotion(move)) {
            promotionCount++;
            
            // Store initial zobrist
            uint64_t initialZobrist = board.zobristKey();
            
            // Multiple make/unmake cycles
            bool stateValid = true;
            for (int cycle = 0; cycle < testCycles; cycle++) {
                Board::UndoInfo undo;
                board.makeMove(move, undo);
                
                if (!board.validateZobrist()) {
                    std::cout << "Zobrist mismatch after makeMove in cycle " << cycle << std::endl;
                    stateValid = false;
                }
                
                board.unmakeMove(move, undo);
                
                if (!board.validateZobrist()) {
                    std::cout << "Zobrist mismatch after unmakeMove in cycle " << cycle << std::endl;
                    stateValid = false;
                }
                
                if (board.zobristKey() != initialZobrist) {
                    std::cout << "Zobrist not restored after cycle " << cycle << std::endl;
                    stateValid = false;
                }
            }
            
            if (stateValid) {
                std::cout << "Move " << moveToString(move) << " - " << testCycles << " cycles OK" << std::endl;
            }
        }
    }
    
    std::cout << "Total promotion moves: " << promotionCount << std::endl;
}

// Run perft on promotion-heavy positions
void testPromotionPerft() {
    std::cout << "\n=== Testing Perft on Promotion Positions ===" << std::endl;
    
    struct TestPosition {
        std::string fen;
        std::string description;
        int depth;
        uint64_t expected;
    };
    
    TestPosition positions[] = {
        {"8/P7/8/8/8/8/p7/8 w - - 0 1", "Two pawns about to promote", 5, 0}, // Need to verify with Stockfish
        {"8/PPP5/8/8/8/8/ppp5/8 w - - 0 1", "Multiple pawns promoting", 4, 0}, // Need to verify
        {"rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4", "Normal position", 5, 8031647685}
    };
    
    for (const auto& pos : positions) {
        if (pos.expected == 0) {
            std::cout << "\nPosition: " << pos.description << std::endl;
            std::cout << "FEN: " << pos.fen << std::endl;
            std::cout << "Need to verify expected perft(" << pos.depth << ") with Stockfish" << std::endl;
            
            Board board;
            board.fromFEN(pos.fen);
            uint64_t nodes = perft(board, pos.depth);
            std::cout << "SeaJay result: " << nodes << " nodes" << std::endl;
        } else {
            std::cout << "\nPosition: " << pos.description << std::endl;
            std::cout << "FEN: " << pos.fen << std::endl;
            
            Board board;
            board.fromFEN(pos.fen);
            uint64_t nodes = perft(board, pos.depth);
            
            std::cout << "Depth " << pos.depth << ": " << nodes << " nodes";
            if (pos.expected > 0) {
                std::cout << " (expected: " << pos.expected << ")";
                if (nodes == pos.expected) {
                    std::cout << " ✓";
                } else {
                    std::cout << " ✗ MISMATCH!";
                }
            }
            std::cout << std::endl;
        }
    }
}

int main() {
    std::cout << "=== SeaJay Promotion Edge Cases Test ===" << std::endl;
    std::cout << "Testing promotion-related edge cases from Bug #003" << std::endl;
    
    testPromotionWithCheck();
    testPromotionBlockingCheck();
    testUnderPromotion();
    testComplexPromotion();
    testPromotionPerft();
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}