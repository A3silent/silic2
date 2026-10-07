#pragma once

#include <cmath>
#include <stdexcept>
#include <string>

namespace silic2::test {

inline void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

inline void requireNear(float actual, float expected, float epsilon, const std::string& message) {
    if (std::abs(actual - expected) > epsilon) {
        throw std::runtime_error(message);
    }
}

} // namespace silic2::test
