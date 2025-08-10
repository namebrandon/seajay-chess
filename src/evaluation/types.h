#pragma once

#include <cstdint>
#include <limits>
#include <algorithm>
#include <compare>

namespace seajay::eval {

class Score {
public:
    using value_type = int32_t;  // Internal representation to prevent overflow
    
    constexpr Score() noexcept = default;
    constexpr explicit Score(value_type v) noexcept : m_value(v) {}
    
    constexpr auto operator<=>(const Score&) const noexcept = default;
    
    constexpr Score operator+(Score rhs) const noexcept {
        return Score(saturate_add(m_value, rhs.m_value));
    }
    
    constexpr Score operator-(Score rhs) const noexcept {
        return Score(saturate_sub(m_value, rhs.m_value));
    }
    
    constexpr Score operator-() const noexcept {
        if (m_value == std::numeric_limits<value_type>::min()) {
            return Score(std::numeric_limits<value_type>::max());
        }
        return Score(-m_value);
    }
    
    constexpr Score& operator+=(Score rhs) noexcept {
        m_value = saturate_add(m_value, rhs.m_value);
        return *this;
    }
    
    constexpr Score& operator-=(Score rhs) noexcept {
        m_value = saturate_sub(m_value, rhs.m_value);
        return *this;
    }
    
    constexpr Score operator*(int factor) const noexcept {
        int64_t result = static_cast<int64_t>(m_value) * factor;
        if (result > std::numeric_limits<value_type>::max()) {
            return Score(std::numeric_limits<value_type>::max());
        }
        if (result < std::numeric_limits<value_type>::min()) {
            return Score(std::numeric_limits<value_type>::min());
        }
        return Score(static_cast<value_type>(result));
    }
    
    constexpr value_type value() const noexcept { return m_value; }
    
    constexpr int16_t to_cp() const noexcept {
        return static_cast<int16_t>(std::clamp(m_value, 
            static_cast<value_type>(-32000), 
            static_cast<value_type>(32000)));
    }
    
    static consteval Score zero() noexcept { return Score(0); }
    static consteval Score draw() noexcept { return Score(0); }
    static consteval Score mate() noexcept { return Score(32000); }
    static consteval Score mate_in(int ply) noexcept { 
        return Score(32000 - ply); 
    }
    static consteval Score mated_in(int ply) noexcept { 
        return Score(-32000 + ply); 
    }
    static consteval Score infinity() noexcept { 
        // Use a value that can be safely negated without overflow
        // Reserve the actual max/min for special purposes
        return Score(1000000); 
    }
    static consteval Score minus_infinity() noexcept { 
        // Use a value that can be safely negated without overflow
        return Score(-1000000); 
    }
    
    constexpr bool is_mate_score() const noexcept {
        return std::abs(m_value) >= 31000;
    }
    
private:
    value_type m_value{0};
    
    static constexpr value_type saturate_add(value_type a, value_type b) noexcept {
        if (b > 0 && a > std::numeric_limits<value_type>::max() - b) {
            return std::numeric_limits<value_type>::max();
        }
        if (b < 0 && a < std::numeric_limits<value_type>::min() - b) {
            return std::numeric_limits<value_type>::min();
        }
        return a + b;
    }
    
    static constexpr value_type saturate_sub(value_type a, value_type b) noexcept {
        if (b == std::numeric_limits<value_type>::min()) {
            if (a >= 0) {
                return std::numeric_limits<value_type>::max();
            }
            return saturate_add(a, std::numeric_limits<value_type>::max());
        }
        return saturate_add(a, -b);
    }
};

inline constexpr Score operator*(int factor, Score s) noexcept {
    return s * factor;
}

constexpr Score SCORE_ZERO = Score::zero();
constexpr Score SCORE_DRAW = Score::draw();
constexpr Score SCORE_MATE = Score::mate();

} // namespace seajay::eval