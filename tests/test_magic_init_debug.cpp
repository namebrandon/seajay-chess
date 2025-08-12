/**
 * Debug test for magic bitboards initialization
 */

#include <iostream>
#include <cstdlib>

// Forward declare the initialization function
namespace seajay {
namespace magic {
    void initMagics();
}
}

// Segfault handler
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>

void handler(int sig) {
    std::cerr << "Error: signal " << sig << "\n";
    void *array[10];
    size_t size = backtrace(array, 10);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main() {
    signal(SIGSEGV, handler);
    signal(SIGILL, handler);
    
    std::cout << "Starting magic bitboards initialization...\n" << std::flush;
    
    try {
        // Call initialization
        seajay::magic::initMagics();
        std::cout << "Initialization completed!\n" << std::flush;
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught\n";
        return 1;
    }
    
    std::cout << "Done!\n";
    return 0;
}