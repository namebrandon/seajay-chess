#include <iostream>

int main() {
    // File indices
    int FILE_D = 3;
    int FILE_E = 4;
    
    std::cout << "D file (3) mirrors to: " << (7 - FILE_D) << " (should be 4=E)\n";
    std::cout << "E file (4) mirrors to: " << (7 - FILE_E) << " (should be 3=D)\n";
    
    std::cout << "\nThis means:\n";
    std::cout << "- Setting pawn_eg_r5_d updates BOTH d5 and e5!\n";
    std::cout << "- Setting pawn_eg_r5_e ALSO updates BOTH e5 and d5!\n";
    std::cout << "- They're overwriting each other!\n";
    
    return 0;
}
