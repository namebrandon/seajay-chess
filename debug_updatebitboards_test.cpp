#include <iostream>
#include "src/core/board.h"

// We need access to private methods, so let's use friend class hack
class BoardTester {
public:
    static void testUpdateBitboards() {
        std::cout << "Testing updateBitboards" << std::endl;
        
        Board board;
        std::cout << "Board created" << std::endl;
        
        Square s = E2;
        Piece p = WHITE_PAWN;
        
        std::cout << "About to test manual setPiece steps..." << std::endl;
        
        // Manually do what setPiece does
        Piece oldPiece = board.pieceAt(s);
        std::cout << "oldPiece = " << static_cast<int>(oldPiece) << std::endl;
        
        if (oldPiece != NO_PIECE) {
            std::cout << "Would call updateBitboards for oldPiece - but skipping since oldPiece is NO_PIECE" << std::endl;
        } else {
            std::cout << "Skipping oldPiece updateBitboards as expected" << std::endl;
        }\n        \n        // Now the part that should run\n        std::cout << \"Setting mailbox directly...\" << std::endl;\n        // We can't access m_mailbox directly, so let's just test the update functions\n        \n        if (p != NO_PIECE) {\n            std::cout << \"About to call updateBitboards for new piece...\" << std::endl;\n            // Can't call private method directly\n            std::cout << \"Would call updateBitboards(s, p, true)\" << std::endl;\n            std::cout << \"Would call updateZobristKey(s, p)\" << std::endl;\n        }\n        \n        std::cout << \"Manual setPiece simulation completed\" << std::endl;\n    }\n};\n\nusing namespace seajay;\n\nint main() {\n    BoardTester::testUpdateBitboards();\n    return 0;
}