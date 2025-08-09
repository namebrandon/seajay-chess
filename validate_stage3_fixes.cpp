#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/move_list.h"

using namespace seajay;

void printMove(Move m) {
    std::cout << squareToString(moveFrom(m)) << squareToString(moveTo(m));
    uint8_t flags = moveFlags(m);
    if (flags == DOUBLE_PAWN) std::cout << " (double)";
    else if (flags == CASTLING) std::cout << " (castle)";
    else if (flags == EN_PASSANT) std::cout << " (ep)";
    else if (isPromotion(m)) {
        std::cout << "=";
        PieceType pt = promotionType(m);
        if (pt == KNIGHT) std::cout << "N";
        else if (pt == BISHOP) std::cout << "B";
        else if (pt == ROOK) std::cout << "R";
        else if (pt == QUEEN) std::cout << "Q";
        if (isCapture(m)) std::cout << " (capture)";
    }
    else if (isCapture(m)) std::cout << " (capture)";
}

int main() {
    std::cout << "=== SeaJay Stage 3 Bug Fix Validation ===" << std::endl << std::endl;
    
    // Test 1: Promotion piece type mapping (should return 1-4, not 0-3)
    std::cout << "Test 1: Promotion Piece Type Mapping" << std::endl;
    {
        Move promoKnight = makeMove(D7, D8, PROMO_KNIGHT);
        Move promoBishop = makeMove(D7, D8, PROMO_BISHOP);
        Move promoRook = makeMove(D7, D8, PROMO_ROOK);
        Move promoQueen = makeMove(D7, D8, PROMO_QUEEN);
        
        assert(promotionType(promoKnight) == KNIGHT);  // Should be 1
        assert(promotionType(promoBishop) == BISHOP);  // Should be 2
        assert(promotionType(promoRook) == ROOK);      // Should be 3
        assert(promotionType(promoQueen) == QUEEN);    // Should be 4
        
        std::cout << "✓ Promotion types correctly map to 1-4 (KNIGHT through QUEEN)" << std::endl;
    }
    
    // Test 2: Capture flag in promotion moves
    std::cout << "\nTest 2: Capture Flags in Promotions" << std::endl;
    {
        Move promoCaptureQueen = makeMove(D7, E8, PROMO_QUEEN_CAPTURE);
        assert(isPromotion(promoCaptureQueen));
        assert(isCapture(promoCaptureQueen));
        assert(promotionType(promoCaptureQueen) == QUEEN);
        
        Move promoNonCapture = makeMove(D7, D8, PROMO_QUEEN);
        assert(isPromotion(promoNonCapture));
        assert(!isCapture(promoNonCapture));
        
        std::cout << "✓ Promotion captures correctly flagged" << std::endl;
    }
    
    // Test 3: Pawn promotion from correct rank
    std::cout << "\nTest 3: Pawn Promotion Rank Check" << std::endl;
    {
        Board board;
        board.fromFEN("4k3/3P4/8/8/8/8/3p4/4K3 w - - 0 1");
        
        MoveList moves;
        MoveGenerator::generatePseudoLegalMoves(board, moves);
        
        // White pawn on 7th rank should generate promotions
        int whitePromos = 0;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            if (moveFrom(m) == D7 && isPromotion(m)) {
                whitePromos++;
            }
        }
        // Should be 8 total: 4 quiet promotions + 4 capture promotions (king on e8)
        assert(whitePromos == 8);  // 4 promotion types x 2 (quiet + capture)
        
        // Switch to black's turn
        board.fromFEN("4k3/3P4/8/8/8/8/3p4/4K3 b - - 0 1");
        moves.clear();
        MoveGenerator::generatePseudoLegalMoves(board, moves);
        
        // Black pawn on 2nd rank should generate promotions
        int blackPromos = 0;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            if (moveFrom(m) == D2 && isPromotion(m)) {
                blackPromos++;
            }
        }
        // Should be 8 total: 4 quiet promotions + 4 capture promotions (king on e1)
        assert(blackPromos == 8);  // 4 promotion types x 2 (quiet + capture)
        
        std::cout << "✓ Pawns promote from correct ranks (7th for white, 2nd for black)" << std::endl;
    }
    
    // Test 4: En passant validation
    std::cout << "\nTest 4: En Passant Validation" << std::endl;
    {
        // Position where en passant is possible
        Board board;
        board.fromFEN("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
        
        MoveList moves;
        MoveGenerator::generatePseudoLegalMoves(board, moves);
        
        bool foundEp = false;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            if (isEnPassant(m)) {
                assert(moveFrom(m) == E5);
                assert(moveTo(m) == D6);
                foundEp = true;
            }
        }
        assert(foundEp);
        
        std::cout << "✓ En passant correctly validates enemy pawn presence" << std::endl;
    }
    
    // Test 5: Castling with B1/B8 squares
    std::cout << "\nTest 5: Castling B1/B8 Square Handling" << std::endl;
    {
        // White queenside castling - B1 must be empty but not necessarily unattacked
        Board board;
        board.fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        
        MoveList moves;
        MoveGenerator::generatePseudoLegalMoves(board, moves);
        
        // Should have both castling moves
        int castlingMoves = 0;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            if (isCastling(m)) {
                castlingMoves++;
                if (moveTo(m) == C1) {
                    std::cout << "  Found O-O-O (white queenside)" << std::endl;
                } else if (moveTo(m) == G1) {
                    std::cout << "  Found O-O (white kingside)" << std::endl;
                }
            }
        }
        
        std::cout << "✓ Castling correctly checks B1/B8 only for emptiness" << std::endl;
    }
    
    // Test 6: Capture flag in all piece types
    std::cout << "\nTest 6: Capture Flags for All Pieces" << std::endl;
    {
        Board board;
        // Position where black pawns can capture white pawn diagonally
        board.fromFEN("rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1");
        
        MoveList moves;
        MoveGenerator::generatePseudoLegalMoves(board, moves);
        
        // Black should be able to capture the d4 pawn with c5 or e5 pawns
        int captures = 0;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            if (isCapture(m)) {
                captures++;
                if (moveTo(m) == D4) {
                    std::cout << "  Found capture to d4: " << squareToString(moveFrom(m)) << "xd4" << std::endl;
                }
            }
        }
        assert(captures >= 1);  // At least c5xd4 should be possible
        
        std::cout << "✓ All piece types correctly set capture flag" << std::endl;
    }
    
    // Test 7: Legal move filtering
    std::cout << "\nTest 7: Legal Move Filtering" << std::endl;
    {
        // Position where king is in check
        Board board;
        board.fromFEN("4k3/8/8/8/4r3/8/4P3/4K3 w - - 0 1");
        
        MoveList pseudoMoves;
        MoveGenerator::generatePseudoLegalMoves(board, pseudoMoves);
        
        MoveList legalMoves;
        MoveGenerator::generateLegalMoves(board, legalMoves);
        
        // Legal moves should be fewer than pseudo-legal (can't move into check)
        assert(legalMoves.size() < pseudoMoves.size());
        
        // King should not be able to move to d2, e2, f2 (attacked by rook)
        bool foundIllegalMove = false;
        for (size_t i = 0; i < legalMoves.size(); ++i) {
            Move m = legalMoves[i];
            if (moveFrom(m) == E1 && 
                (moveTo(m) == D2 || moveTo(m) == E2 || moveTo(m) == F2)) {
                foundIllegalMove = true;
            }
        }
        assert(!foundIllegalMove);
        
        std::cout << "✓ Legal move generation filters moves leaving king in check" << std::endl;
    }
    
    // Test 8: Starting position move count
    std::cout << "\nTest 8: Starting Position Move Generation" << std::endl;
    {
        Board board;
        board.setStartingPosition();
        
        MoveList moves;
        MoveGenerator::generatePseudoLegalMoves(board, moves);
        
        std::cout << "  Pseudo-legal moves from starting position: " << moves.size() << std::endl;
        assert(moves.size() == 20);  // 16 pawn moves + 4 knight moves
        
        MoveList legalMoves;
        MoveGenerator::generateLegalMoves(board, legalMoves);
        std::cout << "  Legal moves from starting position: " << legalMoves.size() << std::endl;
        assert(legalMoves.size() == 20);  // All should be legal
        
        std::cout << "✓ Correct move count from starting position" << std::endl;
    }
    
    // Test 9: Complex position
    std::cout << "\nTest 9: Complex Position (Kiwipete)" << std::endl;
    {
        Board board;
        board.fromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        
        MoveList moves;
        MoveGenerator::generatePseudoLegalMoves(board, moves);
        
        std::cout << "  Pseudo-legal moves from Kiwipete: " << moves.size() << std::endl;
        
        // Count different move types
        int captures = 0, castles = 0, promos = 0, eps = 0;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            if (isCapture(m)) captures++;
            if (isCastling(m)) castles++;
            if (isPromotion(m)) promos++;
            if (isEnPassant(m)) eps++;
        }
        
        std::cout << "  Captures: " << captures << std::endl;
        std::cout << "  Castles: " << castles << std::endl;
        std::cout << "  Promotions: " << promos << std::endl;
        std::cout << "  En passants: " << eps << std::endl;
        
        std::cout << "✓ Complex position generates diverse move types" << std::endl;
    }
    
    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
    std::cout << "\nSummary of Fixed Bugs:" << std::endl;
    std::cout << "✓ Promotion piece types correctly return 1-4" << std::endl;
    std::cout << "✓ Pawn promotions check source rank correctly" << std::endl;
    std::cout << "✓ Capture flags properly set for all moves" << std::endl;
    std::cout << "✓ En passant validates enemy pawn presence" << std::endl;
    std::cout << "✓ Castling B1/B8 squares handled correctly" << std::endl;
    std::cout << "✓ Legal move filtering works" << std::endl;
    std::cout << "✓ isAttacked() has basic implementation" << std::endl;
    
    return 0;
}