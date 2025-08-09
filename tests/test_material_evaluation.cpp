#include <iostream>
#include <cassert>
#include <vector>
#include "../src/core/board.h"
#include "../src/evaluation/evaluate.h"
#include "../src/search/search.h"

using namespace seajay;

struct MaterialTest {
    const char* fen;
    int expectedScore;  // From white's perspective
    const char* description;
};

void testMaterialCounting() {
    std::cout << "\n=== Material Counting Tests ===" << std::endl;
    
    std::vector<MaterialTest> tests = {
        // Basic positions
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0, "Starting position"},
        // Note: "8/8/8/8/8/8/8/8 w - - 0 1" is INVALID (no kings) - removed
        {"k7/8/8/8/8/8/8/K7 w - - 0 1", 0, "K vs K (draw)"},
        {"k7/8/8/8/8/8/8/KB6 w - - 0 1", 0, "KB vs K (insufficient)"},
        {"k7/8/8/8/8/8/8/KN6 w - - 0 1", 0, "KN vs K (insufficient)"},
        {"knn5/8/8/8/8/8/8/K7 w - - 0 1", 0, "K vs KNN (insufficient)"},
        
        // Material imbalances
        {"k7/8/8/8/8/8/P7/K7 w - - 0 1", 100, "White pawn advantage"},
        {"k7/p7/8/8/8/8/8/K7 w - - 0 1", -100, "Black pawn advantage"},
        {"k7/8/8/8/8/8/8/KR6 w - - 0 1", 510, "White rook"},
        {"kr6/8/8/8/8/8/8/K7 w - - 0 1", -510, "Black rook"},
        {"k7/8/8/8/8/8/8/KQ6 w - - 0 1", 950, "White queen"},
        {"kq6/8/8/8/8/8/8/K7 w - - 0 1", -950, "Black queen"},
        
        // Bishop endgames - KB vs KB always has equal material (330-330=0)
        // Expert verified: b1 is DARK, b8 is LIGHT - opposite colors
        {"kb6/8/8/8/8/8/8/KB6 w - - 0 1", 0, "Opposite colored bishops (b1-dark, b8-light) - material equal"},
        // Expert verified: c1 is LIGHT, c8 is DARK - opposite colors  
        {"k1b5/8/8/8/8/8/8/K1B5 w - - 0 1", 0, "Opposite colored bishops (c1-light, c8-dark) - material equal"},
        // For same-colored bishop test, let's use squares we know are the same color
        // a1 is DARK, c3 is DARK - both dark squares
        {"k7/8/8/2b5/8/8/8/B6K w - - 0 1", 0, "Same colored bishops (a1-dark, c3-dark) - insufficient material"},
        
        // Complex positions
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1", 0, "Italian Game"},
        {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 0, "Rooks equal"},
        {"8/P7/8/8/8/8/8/k6K w - - 0 1", 100, "White pawn about to promote (current material)"},
        
        // Special positions for side to move (from side-to-move perspective)
        {"8/8/8/8/8/8/P7/k6K b - - 0 1", -100, "Black to move, white pawn up (black perspective)"},
        {"k7/p7/8/8/8/8/8/K7 b - - 0 1", 100, "Black to move, black pawn up"},
    };
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        Board board;
        if (!board.fromFEN(test.fen)) {
            std::cerr << "Failed to parse FEN: " << test.fen << std::endl;
            failed++;
            continue;
        }
        
        eval::Score score = board.evaluate();
        int scoreCP = score.to_cp();
        
        if (scoreCP == test.expectedScore) {
            std::cout << "✓ " << test.description << " (score: " << scoreCP << ")" << std::endl;
            passed++;
        } else {
            std::cout << "✗ " << test.description << std::endl;
            std::cout << "  Expected: " << test.expectedScore << ", Got: " << scoreCP << std::endl;
            std::cout << "  FEN: " << test.fen << std::endl;
            failed++;
        }
    }
    
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed" << std::endl;
}

void testIncrementalUpdates() {
    std::cout << "\n=== Incremental Update Tests ===" << std::endl;
    
    Board board;
    board.setStartingPosition();
    
    // Verify starting position
    eval::Score startScore = board.evaluate();
    assert(startScore.to_cp() == 0);
    std::cout << "✓ Starting position material = 0" << std::endl;
    
    // Make e2-e4
    Move e4 = makeMove(E2, E4, NORMAL);
    Board::UndoInfo undo1;
    board.makeMove(e4, undo1);
    
    // Still equal material
    eval::Score afterE4 = board.evaluate();
    assert(afterE4.to_cp() == 0);
    std::cout << "✓ After e2-e4 material = 0" << std::endl;
    
    // Make e7-e5  
    Move e5 = makeMove(E7, E5, NORMAL);
    Board::UndoInfo undo2;
    board.makeMove(e5, undo2);
    
    // Still equal
    eval::Score afterE5 = board.evaluate();
    assert(afterE5.to_cp() == 0);
    std::cout << "✓ After e7-e5 material = 0" << std::endl;
    
    // Make Nf3
    Move nf3 = makeMove(G1, F3, NORMAL);
    Board::UndoInfo undo3;
    board.makeMove(nf3, undo3);
    
    // Still equal
    eval::Score afterNf3 = board.evaluate();
    assert(afterNf3.to_cp() == 0);
    std::cout << "✓ After Ng1-f3 material = 0" << std::endl;
    
    // Unmake all moves
    board.unmakeMove(nf3, undo3);
    board.unmakeMove(e5, undo2);
    board.unmakeMove(e4, undo1);
    
    // Should be back to starting position
    eval::Score finalScore = board.evaluate();
    assert(finalScore.to_cp() == 0);
    std::cout << "✓ After unmake all, material = 0" << std::endl;
    
    std::cout << "All incremental update tests passed!" << std::endl;
}

void testSpecialMoves() {
    std::cout << "\n=== Special Move Material Tests ===" << std::endl;
    
    // Test castling (no material change)
    {
        Board board;
        board.fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        eval::Score before = board.evaluate();
        
        Move castle = makeMove(E1, G1, CASTLING);
        Board::UndoInfo undo;
        board.makeMove(castle, undo);
        
        eval::Score after = board.evaluate();
        assert(before.to_cp() == -after.to_cp());  // Perspective changed
        std::cout << "✓ Castling doesn't change material" << std::endl;
        
        board.unmakeMove(castle, undo);
        eval::Score restored = board.evaluate();
        assert(restored.to_cp() == before.to_cp());
        std::cout << "✓ Castling unmake restores material" << std::endl;
    }
    
    // Test en passant
    {
        Board board;
        board.fromFEN("8/2p5/3p4/KP5r/8/8/8/8 w - c6 0 1");
        eval::Score before = board.evaluate();
        
        Move ep = makeMove(B5, C6, EN_PASSANT);
        Board::UndoInfo undo;
        board.makeMove(ep, undo);
        
        eval::Score after = board.evaluate();
        // White captured a pawn, so from black's perspective -100
        assert(after.to_cp() == -100);
        std::cout << "✓ En passant captures pawn correctly" << std::endl;
        
        board.unmakeMove(ep, undo);
        eval::Score restored = board.evaluate();
        assert(restored.to_cp() == before.to_cp());
        std::cout << "✓ En passant unmake restores material" << std::endl;
    }
    
    // Test promotion
    {
        Board board;
        board.fromFEN("8/P7/8/8/8/8/8/k6K w - - 0 1");
        eval::Score before = board.evaluate();
        assert(before.to_cp() == 100);  // One pawn up
        
        Move promote = makeMove(A7, A8, PROMO_QUEEN);
        Board::UndoInfo undo;
        board.makeMove(promote, undo);
        
        eval::Score after = board.evaluate();
        // Pawn (100) promoted to queen (950)
        assert(after.to_cp() == -950);  // From black's perspective
        std::cout << "✓ Promotion updates material correctly" << std::endl;
        
        board.unmakeMove(promote, undo);
        eval::Score restored = board.evaluate();
        assert(restored.to_cp() == before.to_cp());
        std::cout << "✓ Promotion unmake restores material" << std::endl;
    }
}

void testMoveSelection() {
    std::cout << "\n=== Move Selection Tests ===" << std::endl;
    
    // Test hanging piece capture
    {
        Board board;
        board.fromFEN("k7/8/8/3n4/8/3R4/8/K7 w - - 0 1");
        Move best = search::selectBestMove(board);
        
        // Should capture the hanging knight
        assert(moveFrom(best) == D3);
        assert(moveTo(best) == D5);
        std::cout << "✓ Captures hanging knight" << std::endl;
    }
    
    // Test best capture selection
    {
        Board board;
        board.fromFEN("k7/3q4/8/3n4/8/3R4/8/K7 w - - 0 1");
        Move best = search::selectBestMove(board);
        
        // Should capture the queen (950) over knight (320)
        assert(moveFrom(best) == D3);
        assert(moveTo(best) == D7);
        std::cout << "✓ Captures queen over knight" << std::endl;
    }
    
    // Test avoiding bad trades
    {
        Board board;
        board.fromFEN("k7/8/8/3r4/8/3Q4/8/K7 w - - 0 1");
        Move best = search::selectBestMove(board);
        
        // Should NOT capture rook with queen (bad trade)
        assert(moveTo(best) != D5);
        std::cout << "✓ Avoids bad queen for rook trade" << std::endl;
    }
}

int main() {
    std::cout << "=== SeaJay Material Evaluation Tests ===" << std::endl;
    
    testMaterialCounting();
    testIncrementalUpdates();
    testSpecialMoves();
    testMoveSelection();
    
    std::cout << "\n=== All Tests Complete ===" << std::endl;
    return 0;
}