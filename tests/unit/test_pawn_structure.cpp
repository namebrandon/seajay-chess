#include <iostream>
#include <cassert>
#include "../../src/evaluation/pawn_structure.h"
#include "../../src/core/board.h"

using namespace seajay;

void testRelativeRank() {
    std::cout << "Testing relativeRank()..." << std::endl;
    
    assert(PawnStructure::relativeRank(WHITE, 0) == 0);
    assert(PawnStructure::relativeRank(WHITE, 1) == 1);
    assert(PawnStructure::relativeRank(WHITE, 7) == 7);
    
    assert(PawnStructure::relativeRank(BLACK, 0) == 7);
    assert(PawnStructure::relativeRank(BLACK, 1) == 6);
    assert(PawnStructure::relativeRank(BLACK, 7) == 0);
    
    assert(PawnStructure::relativeRank(WHITE, A1) == 0);
    assert(PawnStructure::relativeRank(WHITE, A2) == 1);
    assert(PawnStructure::relativeRank(WHITE, A8) == 7);
    
    assert(PawnStructure::relativeRank(BLACK, A1) == 7);
    assert(PawnStructure::relativeRank(BLACK, A2) == 6);
    assert(PawnStructure::relativeRank(BLACK, A8) == 0);
    
    std::cout << "  relativeRank() tests passed!" << std::endl;
}

void testPassedPawnDetection() {
    std::cout << "Testing passed pawn detection..." << std::endl;
    
    PawnStructure::initPassedPawnMasks();
    
    {
        std::cout << "  Test 1: Simple passed pawn" << std::endl;
        Board board;
        board.fromFEN("8/8/1p6/1P6/8/8/8/8 w - - 0 1");
        
        Bitboard whitePawns = board.pieces(WHITE, PAWN);
        Bitboard blackPawns = board.pieces(BLACK, PAWN);
        
        assert(PawnStructure::isPassed(WHITE, B5, blackPawns));
        assert(PawnStructure::isPassed(BLACK, B6, whitePawns));
    }
    
    {
        std::cout << "  Test 2: Connected vs isolated pawns" << std::endl;
        Board board;
        board.fromFEN("8/pp6/8/PP6/8/8/8/8 w - - 0 1");
        
        Bitboard whitePawns = board.pieces(WHITE, PAWN);
        Bitboard blackPawns = board.pieces(BLACK, PAWN);
        
        assert(!PawnStructure::isPassed(WHITE, A5, blackPawns));
        assert(!PawnStructure::isPassed(WHITE, B5, blackPawns));
        assert(!PawnStructure::isPassed(BLACK, A7, whitePawns));
        assert(!PawnStructure::isPassed(BLACK, B7, whitePawns));
    }
    
    {
        std::cout << "  Test 3: Advanced passed pawn" << std::endl;
        Board board;
        board.fromFEN("8/k7/P7/8/8/8/8/K7 w - - 0 1");
        
        Bitboard whitePawns = board.pieces(WHITE, PAWN);
        Bitboard blackPawns = board.pieces(BLACK, PAWN);
        
        assert(PawnStructure::isPassed(WHITE, A6, blackPawns));
    }
    
    {
        std::cout << "  Test 4: Blocked pawn is not passed" << std::endl;
        Board board;
        board.fromFEN("8/8/1p6/8/1P6/8/8/8 w - - 0 1");
        
        Bitboard whitePawns = board.pieces(WHITE, PAWN);
        Bitboard blackPawns = board.pieces(BLACK, PAWN);
        
        assert(!PawnStructure::isPassed(WHITE, B4, blackPawns));
        assert(!PawnStructure::isPassed(BLACK, B6, whitePawns));
    }
    
    {
        std::cout << "  Test 5: Side file passed pawns" << std::endl;
        Board board;
        board.fromFEN("8/p7/8/P7/8/8/8/8 w - - 0 1");
        
        Bitboard whitePawns = board.pieces(WHITE, PAWN);
        Bitboard blackPawns = board.pieces(BLACK, PAWN);
        
        assert(!PawnStructure::isPassed(WHITE, A5, blackPawns));
        assert(!PawnStructure::isPassed(BLACK, A7, whitePawns));
    }
    
    {
        std::cout << "  Test 6: Multiple passed pawns" << std::endl;
        Board board;
        board.fromFEN("8/8/1P3P2/8/8/1p3p2/8/8 w - - 0 1");
        
        Bitboard whitePawns = board.pieces(WHITE, PAWN);
        Bitboard blackPawns = board.pieces(BLACK, PAWN);
        
        assert(PawnStructure::isPassed(WHITE, B6, blackPawns));
        assert(PawnStructure::isPassed(WHITE, F6, blackPawns));
        assert(PawnStructure::isPassed(BLACK, B3, whitePawns));
        assert(PawnStructure::isPassed(BLACK, F3, whitePawns));
    }
    
    std::cout << "  All passed pawn detection tests passed!" << std::endl;
}

void testCandidatePassers() {
    std::cout << "Testing candidate passer detection..." << std::endl;
    
    PawnStructure::initPassedPawnMasks();
    
    {
        std::cout << "  Test 1: Pawn that can become passed with one push" << std::endl;
        Board board;
        board.fromFEN("8/2p5/8/1P6/8/8/8/8 w - - 0 1");
        
        Bitboard whitePawns = board.pieces(WHITE, PAWN);
        Bitboard blackPawns = board.pieces(BLACK, PAWN);
        
        assert(PawnStructure::isCandidate(WHITE, B5, whitePawns, blackPawns));
    }
    
    {
        std::cout << "  Test 2: Already passed pawn is not a candidate" << std::endl;
        Board board;
        board.fromFEN("8/8/8/1P6/8/8/8/8 w - - 0 1");
        
        Bitboard whitePawns = board.pieces(WHITE, PAWN);
        Bitboard blackPawns = board.pieces(BLACK, PAWN);
        
        assert(!PawnStructure::isCandidate(WHITE, B5, whitePawns, blackPawns));
    }
    
    std::cout << "  All candidate passer tests passed!" << std::endl;
}

void testPawnHash() {
    std::cout << "Testing pawn hash table..." << std::endl;
    
    PawnStructure pawnStruct;
    
    uint64_t testKey = 0x123456789ABCDEF0ULL;
    PawnEntry entry;
    entry.key = testKey;
    entry.passedPawns[WHITE] = 0x0000000000001000ULL;
    entry.passedPawns[BLACK] = 0x0000100000000000ULL;
    entry.score = 42;
    entry.valid = true;
    
    pawnStruct.store(testKey, entry);
    
    PawnEntry* retrieved = pawnStruct.probe(testKey);
    assert(retrieved != nullptr);
    assert(retrieved->key == testKey);
    assert(retrieved->passedPawns[WHITE] == 0x0000000000001000ULL);
    assert(retrieved->passedPawns[BLACK] == 0x0000100000000000ULL);
    assert(retrieved->score == 42);
    
    PawnEntry* notFound = pawnStruct.probe(0xDEADBEEFULL);
    assert(notFound == nullptr);
    
    std::cout << "  Pawn hash table tests passed!" << std::endl;
}

void testGetPassedPawns() {
    std::cout << "Testing getPassedPawns()..." << std::endl;
    
    PawnStructure::initPassedPawnMasks();
    PawnStructure pawnStruct;
    
    {
        Board board;
        board.fromFEN("8/8/1P3P2/8/8/1p3p2/8/8 w - - 0 1");
        
        Bitboard whitePawns = board.pieces(WHITE, PAWN);
        Bitboard blackPawns = board.pieces(BLACK, PAWN);
        
        Bitboard whitePassers = pawnStruct.getPassedPawns(WHITE, whitePawns, blackPawns);
        Bitboard blackPassers = pawnStruct.getPassedPawns(BLACK, blackPawns, whitePawns);
        
        assert(whitePassers == (squareBB(B6) | squareBB(F6)));
        assert(blackPassers == (squareBB(B3) | squareBB(F3)));
    }
    
    std::cout << "  getPassedPawns() tests passed!" << std::endl;
}

int main() {
    std::cout << "Running pawn structure tests..." << std::endl;
    std::cout << "================================" << std::endl;
    
    testRelativeRank();
    testPassedPawnDetection();
    testCandidatePassers();
    testPawnHash();
    testGetPassedPawns();
    
    std::cout << "================================" << std::endl;
    std::cout << "All pawn structure tests passed!" << std::endl;
    
    return 0;
}