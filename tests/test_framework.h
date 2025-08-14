/**
 * SeaJay Chess Engine - Test Framework Header
 * Common testing infrastructure for all tests
 */

#pragma once

// For now, we'll use simple assertion-based testing
// In the future, this can be replaced with Catch2 or Google Test

#include <iostream>
#include <string>
#include <chrono>
#include <cassert>
#include <cmath>

// Simple test macros that mimic Catch2 syntax
// Note: name should be a valid C++ identifier, not a string
#define TEST_CASE(name, ...) void test_##name()
#define SECTION(name) if(true)
#define REQUIRE(expr) assert(expr)
#define CHECK(expr) assert(expr)

// Approx for floating point comparisons
class Approx {
private:
    double m_value;
    double m_margin;
    
public:
    explicit Approx(double value) : m_value(value), m_margin(0.001) {}
    
    Approx& margin(double m) {
        m_margin = m;
        return *this;
    }
    
    friend bool operator==(double lhs, const Approx& rhs) {
        return std::abs(lhs - rhs.m_value) <= rhs.m_margin;
    }
    
    friend bool operator==(const Approx& lhs, double rhs) {
        return std::abs(lhs.m_value - rhs) <= lhs.m_margin;
    }
};

// Simple test session runner
namespace Catch {
    class Session {
    public:
        int run(int argc, char* argv[]) {
            std::cout << "Running tests...\n";
            // In a real implementation, this would discover and run all TEST_CASE functions
            // For now, tests will need to be called manually in main()
            return 0;
        }
    };
}

// Test timing utilities
class TestTimer {
private:
    std::chrono::high_resolution_clock::time_point m_start;
    std::string m_name;
    
public:
    explicit TestTimer(const std::string& name) : m_name(name) {
        m_start = std::chrono::high_resolution_clock::now();
    }
    
    ~TestTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start);
        std::cout << m_name << " took " << duration.count() << "ms\n";
    }
};

// Test result reporting
class TestReporter {
private:
    int m_passed = 0;
    int m_failed = 0;
    
public:
    void recordPass() { m_passed++; }
    void recordFail() { m_failed++; }
    
    void printSummary() const {
        std::cout << "\nTest Summary:\n";
        std::cout << "  Passed: " << m_passed << "\n";
        std::cout << "  Failed: " << m_failed << "\n";
        
        if (m_failed == 0) {
            std::cout << "All tests passed!\n";
        } else {
            std::cout << "Some tests failed.\n";
        }
    }
    
    int getExitCode() const {
        return m_failed > 0 ? 1 : 0;
    }
};