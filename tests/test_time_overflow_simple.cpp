#include "../src/search/time_management.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace seajay::search;
using namespace std::chrono;

void test_normal_values() {
    std::cout << "Testing normal values... ";
    auto lastTime = milliseconds(100);
    double ebf = 2.0;
    int depth = 5;
    auto predicted = predictNextIterationTime(lastTime, ebf, depth);
    
    // Should be roughly 100 * 2.0 * 1.1 = 220ms
    assert(predicted.count() >= 200 && predicted.count() <= 250);
    std::cout << "PASS (predicted: " << predicted.count() << "ms)\n";
}

void test_overflow_protection() {
    std::cout << "Testing overflow protection... ";
    auto lastTime = milliseconds(1000000);  // 1000 seconds
    double ebf = 5.0;
    int depth = 15;
    auto predicted = predictNextIterationTime(lastTime, ebf, depth);
    
    // Should be capped at 1 hour (3600000ms)
    assert(predicted.count() == 3600000);
    std::cout << "PASS (capped at 1 hour)\n";
}

void test_very_large_values() {
    std::cout << "Testing very large values... ";
    auto lastTime = milliseconds(2000000000);  // Very large
    double ebf = 10.0;
    int depth = 8;
    auto predicted = predictNextIterationTime(lastTime, ebf, depth);
    
    // Should be capped without overflow
    assert(predicted.count() == 3600000);
    std::cout << "PASS (capped without overflow)\n";
}

void test_invalid_ebf() {
    std::cout << "Testing invalid EBF... ";
    auto lastTime = milliseconds(100);
    double ebf = -1.0;  // Invalid
    int depth = 5;
    auto predicted = predictNextIterationTime(lastTime, ebf, depth);
    
    // Should use default EBF of 5.0
    assert(predicted.count() >= 500 && predicted.count() <= 600);
    std::cout << "PASS (used default EBF)\n";
}

void test_zero_time() {
    std::cout << "Testing zero last time... ";
    auto lastTime = milliseconds(0);
    double ebf = 2.0;
    int depth = 5;
    auto predicted = predictNextIterationTime(lastTime, ebf, depth);
    
    // Should use 1ms minimum
    assert(predicted.count() >= 2 && predicted.count() <= 5);
    std::cout << "PASS (used 1ms minimum)\n";
}

void test_ebf_clamping() {
    std::cout << "Testing EBF clamping... ";
    auto lastTime = milliseconds(100);
    
    // Test very low EBF
    auto predicted1 = predictNextIterationTime(lastTime, 0.5, 5);
    assert(predicted1.count() >= 150 && predicted1.count() <= 180);
    
    // Test very high EBF
    auto predicted2 = predictNextIterationTime(lastTime, 50.0, 5);
    assert(predicted2.count() >= 1000 && predicted2.count() <= 1200);
    
    std::cout << "PASS (clamped to [1.5, 10.0])\n";
}

int main() {
    std::cout << "\n=== Time Overflow Protection Tests ===\n\n";
    
    test_normal_values();
    test_overflow_protection();
    test_very_large_values();
    test_invalid_ebf();
    test_zero_time();
    test_ebf_clamping();
    
    std::cout << "\n✅ All overflow protection tests passed!\n\n";
    return 0;
}