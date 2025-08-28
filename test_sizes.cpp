#include <iostream>
#include <type_traits>
#include "src/search/types.h"
#include "src/search/iterative_search_data.h"

int main() {
    using namespace seajay::search;
    
    std::cout << "===== OBJECT SIZES =====" << std::endl;
    std::cout << "SearchData size: " << sizeof(SearchData) << " bytes" << std::endl;
    std::cout << "IterativeSearchData size: " << sizeof(IterativeSearchData) << " bytes" << std::endl;
    std::cout << "Is SearchData polymorphic: " << (std::is_polymorphic<SearchData>::value ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
    
    // Check individual component sizes
    std::cout << "===== COMPONENT SIZES =====" << std::endl;
    std::cout << "KillerMoves size: " << sizeof(KillerMoves) << " bytes" << std::endl;
    std::cout << "HistoryHeuristic size: " << sizeof(HistoryHeuristic) << " bytes" << std::endl;
    std::cout << "CounterMoves size: " << sizeof(seajay::CounterMoves) << " bytes" << std::endl;
    std::cout << std::endl;
    
    // Check pointer sizes in SearchData
    std::cout << "===== POINTER SIZES IN SEARCHDATA =====" << std::endl;
    std::cout << "KillerMoves* size: " << sizeof(void*) << " bytes" << std::endl;
    std::cout << "HistoryHeuristic* size: " << sizeof(void*) << " bytes" << std::endl;
    std::cout << "CounterMoves* size: " << sizeof(void*) << " bytes" << std::endl;
    std::cout << std::endl;
    
    // Summary
    std::cout << "===== SUMMARY =====" << std::endl;
    if (sizeof(SearchData) > 2000) {
        std::cout << "[PROBLEM] SearchData is still too large at " << sizeof(SearchData) 
                  << " bytes! Cache thrashing likely persists." << std::endl;
    } else {
        std::cout << "[GOOD] SearchData size is reasonable at " << sizeof(SearchData) 
                  << " bytes (down from 42KB)." << std::endl;
    }
    
    return 0;
}