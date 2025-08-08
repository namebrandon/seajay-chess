#include <iostream>
#include "src/core/board.h"

using namespace seajay;

// Demonstrate the key Stage 2 enhancements
int main() {
    std::cout << "=== SeaJay Chess Engine - Stage 2 Position Management Demo ===\n\n";
    
    try {
        // 1. Demonstrate Result<T,E> error handling system
        std::cout << "1. Testing Result<T,E> error handling system:\n";
        FenResult success = true;
        std::cout << "   Success result: " << (success ? "OK" : "FAIL") << "\n";
        
        FenResult error = makeFenError(FenError::InvalidBoard, "Test error message", 5);
        std::cout << "   Error result: " << (error ? "FAIL" : "OK") << "\n";
        std::cout << "   Error message: " << error.error().message << "\n\n";
        
        // 2. Demonstrate enhanced FEN parsing with validation
        std::cout << "2. Testing enhanced FEN parsing with comprehensive validation:\n";
        
        Board board;
        
        // Test valid starting position
        std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        std::cout << "   Parsing starting position...\n";
        if (board.fromFEN(startFen)) {
            std::cout << "   ✓ Starting position parsed successfully\n";
            std::cout << "   Position hash: 0x" << std::hex << board.positionHash() << std::dec << "\n";
        } else {
            std::cout << "   ✗ Starting position parsing failed\n";
        }
        
        // 3. Demonstrate validation functions
        std::cout << "\n3. Testing comprehensive validation functions:\n";
        std::cout << "   Position valid: " << (board.validatePosition() ? "PASS" : "FAIL") << "\n";
        std::cout << "   Bitboard sync: " << (board.validateBitboardSync() ? "PASS" : "FAIL") << "\n";
        std::cout << "   Zobrist valid:  " << (board.validateZobrist() ? "PASS" : "FAIL") << "\n";
        
        // 4. Demonstrate board display with debug info
        std::cout << "\n4. Board display with debug information:\n";
        std::cout << board.toString();
        
        // 5. Test round-trip consistency (board -> FEN -> board)
        std::cout << "\n5. Testing round-trip consistency:\n";
        std::string generatedFen = board.toFEN();
        std::cout << "   Generated FEN: " << generatedFen << "\n";
        
        Board board2;
        if (board2.fromFEN(generatedFen)) {
            std::cout << "   Round-trip parsing: PASS\n";
            if (board.positionHash() == board2.positionHash()) {
                std::cout << "   Position consistency: PASS\n";
            } else {
                std::cout << "   Position consistency: FAIL\n";
            }
        } else {
            std::cout << "   Round-trip parsing: FAIL\n";
        }
        
        std::cout << "\n=== Stage 2 Position Management Demo Complete ===\n";
        std::cout << "Key enhancements implemented:\n";
        std::cout << "• Result<T,E> error handling system for C++20\n";
        std::cout << "• Enhanced FEN parser with buffer overflow protection\n";
        std::cout << "• Comprehensive validation functions\n";
        std::cout << "• Bitboard/mailbox synchronization validation\n";
        std::cout << "• Zobrist key validation and rebuilding\n";
        std::cout << "• Position hash function for testing\n";
        std::cout << "• Debug display with validation status\n";
        std::cout << "• Round-trip consistency testing\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught\n";
        return 1;
    }
}