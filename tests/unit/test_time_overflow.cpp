#include <gtest/gtest.h>
#include "../../src/search/time_management.h"
#include <chrono>
#include <limits>

using namespace seajay::search;
using namespace std::chrono;

// Test overflow protection in predictNextIterationTime
TEST(TimeManagement, PredictNextIterationTime_NoOverflow) {
    // Test with normal values
    {
        auto lastTime = milliseconds(100);
        double ebf = 2.0;
        int depth = 5;
        auto predicted = predictNextIterationTime(lastTime, ebf, depth);
        
        // Should be roughly 100 * 2.0 * 1.1 = 220ms
        EXPECT_GE(predicted.count(), 200);
        EXPECT_LE(predicted.count(), 250);
    }
    
    // Test with large values that would overflow without protection
    {
        auto lastTime = milliseconds(1000000);  // 1000 seconds
        double ebf = 5.0;
        int depth = 15;
        auto predicted = predictNextIterationTime(lastTime, ebf, depth);
        
        // Should be capped at 1 hour (3600000ms)
        EXPECT_EQ(predicted.count(), 3600000);
    }
    
    // Test with very large last time
    {
        auto lastTime = milliseconds(std::numeric_limits<int>::max() / 2);
        double ebf = 3.0;
        int depth = 8;
        auto predicted = predictNextIterationTime(lastTime, ebf, depth);
        
        // Should be capped at 1 hour without overflow
        EXPECT_EQ(predicted.count(), 3600000);
    }
    
    // Test with invalid EBF (should use default)
    {
        auto lastTime = milliseconds(100);
        double ebf = -1.0;  // Invalid
        int depth = 5;
        auto predicted = predictNextIterationTime(lastTime, ebf, depth);
        
        // Should use default EBF of 5.0, giving ~100 * 5.0 * 1.1 = 550ms
        EXPECT_GE(predicted.count(), 500);
        EXPECT_LE(predicted.count(), 600);
    }
    
    // Test with zero last time (should use minimum 1ms)
    {
        auto lastTime = milliseconds(0);
        double ebf = 2.0;
        int depth = 5;
        auto predicted = predictNextIterationTime(lastTime, ebf, depth);
        
        // Should use 1ms minimum, giving ~1 * 2.0 * 1.1 = 2.2ms
        EXPECT_GE(predicted.count(), 2);
        EXPECT_LE(predicted.count(), 5);
    }
    
    // Test depth factor at high depths
    {
        auto lastTime = milliseconds(100);
        double ebf = 2.0;
        int depth = 12;  // High depth, should apply 0.9 factor
        auto predicted = predictNextIterationTime(lastTime, ebf, depth);
        
        // Should be roughly 100 * 2.0 * 0.9 * 1.1 = 198ms
        EXPECT_GE(predicted.count(), 180);
        EXPECT_LE(predicted.count(), 220);
    }
}

// Test that EBF clamping works correctly
TEST(TimeManagement, PredictNextIterationTime_EBFClamping) {
    auto lastTime = milliseconds(100);
    
    // Test with very low EBF (should clamp to 1.5)
    {
        double ebf = 0.5;
        int depth = 5;
        auto predicted = predictNextIterationTime(lastTime, ebf, depth);
        
        // Should use clamped 1.5, giving ~100 * 1.5 * 1.1 = 165ms
        EXPECT_GE(predicted.count(), 150);
        EXPECT_LE(predicted.count(), 180);
    }
    
    // Test with very high EBF (should clamp to 10.0)
    {
        double ebf = 50.0;
        int depth = 5;
        auto predicted = predictNextIterationTime(lastTime, ebf, depth);
        
        // Should use clamped 10.0, giving ~100 * 10.0 * 1.1 = 1100ms
        EXPECT_GE(predicted.count(), 1000);
        EXPECT_LE(predicted.count(), 1200);
    }
}