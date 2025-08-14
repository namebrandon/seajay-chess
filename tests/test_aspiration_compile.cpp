#include "../src/search/aspiration_window.h"
#include <cassert>

using namespace seajay::search;
using namespace seajay::eval;

// Simple compile test for aspiration window types
int main() {
    // Test AspirationWindow construction
    AspirationWindow window;
    assert(window.isInfinite());
    assert(!window.exceedsMaxAttempts());
    
    // Test with specific values
    window.alpha = Score(100);
    window.beta = Score(200);
    window.attempts = 3;
    assert(!window.isInfinite());
    assert(!window.exceedsMaxAttempts());
    
    // Test max attempts
    window.attempts = AspirationConstants::MAX_ATTEMPTS;
    assert(window.exceedsMaxAttempts());
    
    // Test makeInfinite
    window.makeInfinite();
    assert(window.isInfinite());
    
    // Test constants
    static_assert(AspirationConstants::INITIAL_DELTA == 16);
    static_assert(AspirationConstants::MAX_ATTEMPTS == 5);
    static_assert(AspirationConstants::MIN_DEPTH == 4);
    
    return 0;
}