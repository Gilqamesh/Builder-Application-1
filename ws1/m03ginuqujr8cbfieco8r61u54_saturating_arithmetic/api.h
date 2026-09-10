#ifndef M03GINUQUJR8CBFIECO8R61U54_SATURATING_ARITHMETIC_API_H
# define M03GINUQUJR8CBFIECO8R61U54_SATURATING_ARITHMETIC_API_H

# include <cmath>
# include <format>
# include <limits>
# include <stdexcept>
# include <type_traits>

namespace m03ginuqujr8cbfieco8r61u54_saturating_arithmetic {

/**
 * @brief Returns a + b, saturating at the lowest or maximum representable value of T.
 *
 * Floating point overflow is clamped to a finite bound; rounding and underflow
 * toward zero follow ordinary floating point arithmetic.
 * @throws std::invalid_argument If a floating point operand is NaN or +-infinity.
 */
template <typename T>
T add(const T& a, const T& b);

/**
 * @brief Returns a - b, saturating at the lowest or maximum representable value of T.
 *
 * Unsigned subtraction returns zero when a < b. Floating point overflow is
 * clamped to a finite bound; rounding and underflow toward zero are unchanged.
 * @throws std::invalid_argument If a floating point operand is NaN or +-infinity.
 */
template <typename T>
T sub(const T& a, const T& b);

/**
 * @brief Returns a * b, saturating at the lowest or maximum representable value of T.
 *
 * Floating point overflow is clamped to a finite bound; rounding and underflow
 * toward zero follow ordinary floating point arithmetic.
 * @throws std::invalid_argument If a floating point operand is NaN or +-infinity.
 */
template <typename T>
T mul(const T& a, const T& b);

/**
 * @brief Returns a / b with saturation for overflow and a zero divisor.
 *
 * A zero divisor returns std::numeric_limits<T>::lowest() when a < 0, and
 * std::numeric_limits<T>::max() otherwise, including 0/0 and unsigned inputs.
 * For floating point inputs this also applies to a divisor of -0.0; the sign
 * of zero does not affect the choice, and a numerator of -0.0 is not negative.
 * Signed integer lowest() divided by -1 returns max(); other integer quotients
 * truncate toward zero. Floating point overflow is clamped to a finite bound;
 * rounding and underflow toward zero follow ordinary floating point arithmetic.
 * @throws std::invalid_argument If a floating point operand is NaN or +-infinity,
 * even when the divisor is zero.
 *
 * @code{.cpp}
 * #include <m03ginuqujr8cbfieco8r61u54_saturating_arithmetic/api.h>
 *
 * #include <cassert>
 * #include <limits>
 *
 * int main() {
 *     namespace arithmetic = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic;
 *     const int lowest = std::numeric_limits<int>::lowest();
 *     const int highest = std::numeric_limits<int>::max();
 *     assert(arithmetic::add(highest, 1) == highest);
 *     assert(arithmetic::sub(0U, 1U) == 0U);
 *     assert(arithmetic::mul(lowest, -1) == highest);
 *     assert(arithmetic::div(0, 0) == highest);
 *     assert(arithmetic::div(-1, 0) == lowest);
 *     assert(arithmetic::div(lowest, -1) == highest);
 * }
 * @endcode
 */
template <typename T>
T div(const T& a, const T& b);

} // namespace m03ginuqujr8cbfieco8r61u54_saturating_arithmetic

namespace m03ginuqujr8cbfieco8r61u54_saturating_arithmetic {

template <typename T>
T add(const T& a, const T& b) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(a) || std::isinf(a)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add: a {} must not be NaN or +-infinity.", a));
        }
        if (std::isnan(b) || std::isinf(b)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add: b {} must not be NaN or +-infinity.", b));
        }

        const T result = a + b;
        if (std::isinf(result)) {
            return std::signbit(result) ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
        }
        return result;
    } else if constexpr (std::is_unsigned_v<T>) {
        if (std::numeric_limits<T>::max() - b < a) {
            return std::numeric_limits<T>::max();
        }
    } else {
        if (static_cast<T>(0) < b && std::numeric_limits<T>::max() - b < a) {
            return std::numeric_limits<T>::max();
        }

        if (b < static_cast<T>(0) && a < std::numeric_limits<T>::lowest() - b) {
            return std::numeric_limits<T>::lowest();
        }
    }

    return a + b;
}

template <typename T>
T sub(const T& a, const T& b) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(a) || std::isinf(a)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub: a {} must not be NaN or +-infinity.", a));
        }
        if (std::isnan(b) || std::isinf(b)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub: b {} must not be NaN or +-infinity.", b));
        }

        const T result = a - b;
        if (std::isinf(result)) {
            return std::signbit(result) ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
        }
        return result;
    } else if constexpr (std::is_unsigned_v<T>) {
        if (a < b) {
            return std::numeric_limits<T>::lowest();
        }
    } else {
        if (static_cast<T>(0) < b && a < std::numeric_limits<T>::lowest() + b) {
            return std::numeric_limits<T>::lowest();
        }

        if (b < static_cast<T>(0) && std::numeric_limits<T>::max() + b < a) {
            return std::numeric_limits<T>::max();
        }
    }

    return a - b;
}

template <typename T>
T mul(const T& a, const T& b) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(a) || std::isinf(a)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::mul: a {} must not be NaN or +-infinity.", a));
        }
        if (std::isnan(b) || std::isinf(b)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::mul: b {} must not be NaN or +-infinity.", b));
        }

        const T result = a * b;
        if (std::isinf(result)) {
            return std::signbit(result) ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
        }
        return result;
    }

    const T zero = static_cast<T>(0);

    if (a == zero || b == zero) {
        return zero;
    }

    const T lowest = std::numeric_limits<T>::lowest();
    const T highest = std::numeric_limits<T>::max();

    if constexpr (!std::numeric_limits<T>::is_signed) {
        if (highest / b < a) {
            return highest;
        }
    } else if (zero < a) {
        if (zero < b) {
            if (highest / b < a) {
                return highest;
            }
        } else {
            if (b < lowest / a) {
                return lowest;
            }
        }
    } else {
        if (zero < b) {
            if (a < lowest / b) {
                return lowest;
            }
        } else {
            if (a < highest / b) {
                return highest;
            }
        }
    }

    return a * b;
}

template <typename T>
T div(const T& a, const T& b) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(a) || std::isinf(a)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::div: a {} must not be NaN or +-infinity.", a));
        }
        if (std::isnan(b) || std::isinf(b)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::div: b {} must not be NaN or +-infinity.", b));
        }
    }

    const T zero = static_cast<T>(0);

    if (b == zero) {
        if constexpr (std::numeric_limits<T>::is_signed) {
            if (a < zero) {
                return std::numeric_limits<T>::lowest();
            } else {
                return std::numeric_limits<T>::max();
            }
        } else {
            return std::numeric_limits<T>::max();
        }
    }

    if constexpr (std::numeric_limits<T>::is_signed && std::numeric_limits<T>::is_integer) {
        if (a == std::numeric_limits<T>::lowest() && b == static_cast<T>(-1)) {
            return std::numeric_limits<T>::max();
        }
    }

    const T result = a / b;
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isinf(result)) {
            return std::signbit(result) ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
        }
    }
    return result;
}

} // namespace m03ginuqujr8cbfieco8r61u54_saturating_arithmetic

#endif // M03GINUQUJR8CBFIECO8R61U54_SATURATING_ARITHMETIC_API_H
