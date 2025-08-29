#include <iostream>
#include "../src/core/board.h"
#include "../src/search/search.h"
#include "../src/search/negamax.h"
#include "../src/search/types.h"

using namespace seajay;

int main() {
    // Black to move, mate in 1
    Board board;
    board.fromFEN("3r2k1/5ppp/8/8/8/8/5PPP/6K1 b - - 0 1");
    
    std::cout << "Testing Black mate in 1\n";
    std::cout << "FEN: 3r2k1/5ppp/8/8/8/8/5PPP/6K1 b - - 0 1\n";
    std::cout << "Side to move: " << (board.sideToMove() == BLACK ? "BLACK" : "WHITE") << "\n\n";
    
    // Set up search
    search::SearchLimits limits;
    limits.maxDepth = 3;
    limits.infinite = false;
    limits.useQuiescence = false;
    
    // Do a simple search
    SearchInfo searchInfo;
    searchInfo.clear();
    
    search::SearchData info;
    info.rootSideToMove = board.sideToMove();
    
    // Call negamax directly at depth 1
    eval::Score score = search::negamax(board, 1, 0, 
                                       eval::Score::minus_infinity(), 
                                       eval::Score::infinity(),
                                       searchInfo, info, limits);
    
    std::cout << "Negamax returned score: " << score.value() << "\n";
    std::cout << "Is mate score: " << (score.is_mate_score() ? "YES" : "NO") << "\n";
    
    if (score.is_mate_score()) {
        int mateIn = 0;
        if (score > eval::Score::zero()) {
            mateIn = (eval::Score::mate().value() - score.value() + 1) / 2;
            std::cout << "Positive mate score, mate in " << mateIn << " moves\n";
        } else {
            mateIn = -(eval::Score::mate().value() + score.value()) / 2;
            std::cout << "Negative mate score, mated in " << -mateIn << " moves\n";
        }
    }
    
    return 0;
}