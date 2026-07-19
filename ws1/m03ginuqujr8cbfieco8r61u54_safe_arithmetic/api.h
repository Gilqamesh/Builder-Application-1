#ifndef M03GINUQUJR8CBFIECO8R61U54_SAFE_ARITHMETIC_MODULE_H
# define M03GINUQUJR8CBFIECO8R61U54_SAFE_ARITHMETIC_MODULE_H

# include <limits>

namespace m03ginuqujr8cbfieco8r61u54_safe_arithmetic {

template <typename T>
T add(const T& a, const T& b);

template <typename T>
T sub(const T& a, const T& b);

template <typename T>
T mul(const T& a, const T& b);

template <typename T>
T div(const T& a, const T& b);

} // namespace m03ginuqujr8cbfieco8r61u54_safe_arithmetic

namespace m03ginuqujr8cbfieco8r61u54_safe_arithmetic {

template <typename T>
T add(const T& a, const T& b) {
    const auto is_b_negative = b < static_cast<T>(0);
    if (!is_b_negative && std::numeric_limits<T>::max() - b < a) {
        return std::numeric_limits<T>::max();
    } else if (is_b_negative && a < std::numeric_limits<T>::lowest() - b) {
        return std::numeric_limits<T>::lowest();
    }

    return a + b;
}

template <typename T>
T sub(const T& a, const T& b) {
    const auto is_b_negative = b < static_cast<T>(0);
    if (!is_b_negative && a < std::numeric_limits<T>::lowest() + b) {
        return std::numeric_limits<T>::lowest();
    } else if (is_b_negative && std::numeric_limits<T>::max() + b < a) {
        return std::numeric_limits<T>::max();
    }

    return a - b;
}

template <typename T>
T mul(const T& a, const T& b) {
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

} // namespace m03ginuqujr8cbfieco8r61u54_safe_arithmetic

#endif // M03GINUQUJR8CBFIECO8R61U54_SAFE_ARITHMETIC_MODULE_H
