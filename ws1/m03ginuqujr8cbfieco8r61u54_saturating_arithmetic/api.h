#ifndef M03GINUQUJR8CBFIECO8R61U54_SATURATING_ARITHMETIC_H
# define M03GINUQUJR8CBFIECO8R61U54_SATURATING_ARITHMETIC_H

# include <cassert>
# include <cmath>

# include <limits>
# include <format>
# include <stdexcept>
# include <type_traits>

namespace m03ginuqujr8cbfieco8r61u54_saturating_arithmetic {

/**
 * @brief Performs bound checked arithmetic operations on numeric types.
 * 
 * If the result of an operation would overflow or underflow, the result is clamped to the maximum or minimum value of the type.
 * 
 * @throws std::invalid_argument if either operand is NaN or +-infinity for floating point types.
 */
template <typename T>
T add(const T& a, const T& b);

/**
 * @brief Performs bound checked arithmetic operations on numeric types.
 * 
 * If the result of an operation would overflow or underflow, the result is clamped to the maximum or minimum value of the type.
 * 
 * @throws std::invalid_argument if either operand is NaN or +-infinity for floating point types.
 */
template <typename T>
T sub(const T& a, const T& b);

/**
 * @brief Performs bound checked arithmetic operations on numeric types.
 * 
 * If the result of an operation would overflow or underflow, the result is clamped to the maximum or minimum value of the type.
 * 
 * @throws std::invalid_argument if either operand is NaN or +-infinity for floating point types.
 */
template <typename T>
T mul(const T& a, const T& b);

/**
 * @brief Performs bound checked arithmetic operations on numeric types.
 * 
 * If the result of an operation would overflow or underflow, the result is clamped to the maximum or minimum value of the type.
 * 
 * @throws std::invalid_argument if either operand is NaN or +-infinity for floating point types.
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
    assert(false && "todo: implement");
    throw std::logic_error("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::mul: not implemented.");

    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(a) || std::isinf(a)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::mul: a {} must not be NaN or +-infinity.", a));
        }
        if (std::isnan(b) || std::isinf(b)) {
            throw std::invalid_argument(std::format("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::mul: b {} must not be NaN or +-infinity.", b));
        }
    }


    if constexpr (!std::numeric_limits<T>::is_integer) {
        return a * b;
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
    assert(false && "todo: implement");
    throw std::logic_error("m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::div: not implemented.");

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

    return a / b;
}

} // namespace m03ginuqujr8cbfieco8r61u54_saturating_arithmetic

#endif // M03GINUQUJR8CBFIECO8R61U54_SATURATING_ARITHMETIC_H
