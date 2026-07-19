#ifndef M03GIN6LTE1AZ5KJ36AJ9SUK6T_INTERVAL_MODULE_H
# define M03GIN6LTE1AZ5KJ36AJ9SUK6T_INTERVAL_MODULE_H

# include <cmath>

# include <algorithm>
# include <limits>
# include <numeric>
# include <type_traits>
# include <stdexcept>

# include <m03ginuqujr8cbfieco8r61u54_saturating_arithmetic/api.h>

namespace m03gin6lte1az5kj36aj9suk6t_interval {

/**
 * @brief Half-open interval [start, end) of type T.
 * 
 * Invariants:
 *   start <= end.
 *   operations always produce finite values, no NaN or +-infinity for floating point types.
 */
template <typename T>
class interval_t {
public:
    /**
     * @brief Constructs an empty interval [0, 0).
     */
    interval_t();

    /**
     * @brief Constructs an interval with the given bounds.
     * 
     * @throws std::invalid_argument if end < start.
     * @throws if either values are NaN or +-infinity for floating point types.
     */
    interval_t(const T& start, const T& end);

    /**
     * @brief Sets the bounds of the interval.
     * 
     * @throws std::invalid_argument if end < start.
     * @throws if either values are NaN or +-infinity for floating point types.
     */
    void bounds(const T& start, const T& end);

    const T& start() const;
    const T& end() const;

    /**
     * @brief Adds a value to both bounds using saturating arithmetic.
     * 
     * If the addition would overflow or underflow, the result is clamped to the maximum or minimum value of the type.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    interval_t& operator+=(const T& value);

    /**
     * @brief Subtracts a value from both bounds using saturating arithmetic.
     * 
     * If the subtraction would overflow or underflow, the result is clamped to the maximum or minimum value of the type.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    interval_t& operator-=(const T& value);

    /**
     * @brief Returns a new interval that is the result of adding a value to both bounds using saturating arithmetic.
     * 
     * If the addition would overflow or underflow, the result is clamped to the maximum or minimum value of the type.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    interval_t operator+(const T& value) const;

    /**
     * @brief Returns a new interval that is the result of subtracting a value from both bounds using saturating arithmetic.
     * 
     * If the subtraction would overflow or underflow, the result is clamped to the maximum or minimum value of the type.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    interval_t operator-(const T& value) const;

    /**
     * @brief Clamps the given value to the interval.
     * 
     * If the value is greater or equal to the end of the interval, the end of the interval is returned, which is not part of the interval.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    T clamp(const T& value) const;

    /**
     * @brief Returns a new interval that is inflated by the given value on both sides.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    interval_t inflate(const T& value) const;

    /**
     * @brief Returns a new interval that is deflated by the given value on both sides.
     * 
     * If the deflation would result in an empty interval, the result is an empty interval with the midpoint of the original interval as its bounds.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    interval_t deflate(const T& value) const;

    /**
     * @brief Returns the intersection of this interval with another interval.
     * 
     * If the intervals do not intersect, the result is an empty interval with the greater start value.
     */
    interval_t intersect(const interval_t& other) const;

    /**
     * @brief Returns true if the interval is empty (start == end).
     */
    bool is_empty() const;

    /**
     * @brief Returns true if the interval contains the given value.
     * 
     * Empty intervals do not contain any values.
     */
    bool contains(const T& value) const;

    /**
     * @brief Returns true if the interval overlaps with another interval.
     * 
     * Empty intervals do not overlap with any intervals.
     */
    bool overlaps(const interval_t& other) const;

    /**
     * @brief Returns the length of the interval (end - start).
     * 
     * Guaranteed to be non-negative and finite, no NaN or +-infinity for floating point types.
     */
    T length() const;

private:
    T m_start;
    T m_end;
};

} // namespace m03gin6lte1az5kj36aj9suk6t_interval

namespace m03gin6lte1az5kj36aj9suk6t_interval {

template <typename T>
interval_t<T>::interval_t():
    m_start(static_cast<T>(0)),
    m_end(static_cast<T>(0))
{
}

template <typename T>
interval_t<T>::interval_t(const T& start, const T& end):
    m_start(start),
    m_end(end)
{
    if (end < start) {
        throw std::invalid_argument("interval_t: end must be greater than or equal to start.");
    }

    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(start) || std::isnan(end) || std::isinf(start) || std::isinf(end)) {
            throw std::invalid_argument("interval_t: start and end must not be NaN or +-infinity.");
        }
    }
}

template <typename T>
void interval_t<T>::bounds(const T& start, const T& end) {
    if (end < start) {
        throw std::invalid_argument("interval_t::bounds: end must be greater than or equal to start.");
    }

    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(start) || std::isnan(end) || std::isinf(start) || std::isinf(end)) {
            throw std::invalid_argument("interval_t::bounds: start and end must not be NaN or +-infinity.");
        }
    }

    m_start = start;
    m_end = end;
}

template <typename T>
const T& interval_t<T>::start() const {
    return m_start;
}

template <typename T>
const T& interval_t<T>::end() const {
    return m_end;
}

template <typename T>
interval_t<T>& interval_t<T>::operator+=(const T& value) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value) || std::isinf(value)) {
            throw std::invalid_argument("interval_t::operator+=: value must not be NaN or +-infinity.");
        }
    }

    m_start = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add(m_start, value);
    m_end = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add(m_end, value);
    return *this;
}

template <typename T>
interval_t<T>& interval_t<T>::operator-=(const T& value) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value) || std::isinf(value)) {
            throw std::invalid_argument("interval_t::operator-=: value must not be NaN or +-infinity.");
        }
    }

    m_start = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_start, value);
    m_end = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_end, value);
    return *this;
}

template <typename T>
interval_t<T> interval_t<T>::operator+(const T& value) const {
    interval_t<T> result = *this;
    result += value;
    return result;
}

template <typename T>
interval_t<T> interval_t<T>::operator-(const T& value) const {
    interval_t<T> result = *this;
    result -= value;
    return result;
}

template <typename T>
T interval_t<T>::clamp(const T& value) const {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value) || std::isinf(value)) {
            throw std::invalid_argument("interval_t::clamp: value must not be NaN or +-infinity.");
        }
    }

    if (value < m_start) {
        return m_start;
    } else if (m_end < value) {
        return m_end;
    }
    return value;
}

template <typename T>
interval_t<T> interval_t<T>::inflate(const T& value) const {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value) || std::isinf(value)) {
            throw std::invalid_argument("interval_t::inflate: value must not be NaN or +-infinity.");
        }
    }

    const T new_start = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_start, value);
    const T new_end = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add(m_end, value);

    if (new_end < new_start) {
        const T mid = std::midpoint(m_start, m_end);
        return interval_t<T>(mid, mid);
    }

    return interval_t<T>(new_start, new_end);
}

template <typename T>
interval_t<T> interval_t<T>::deflate(const T& value) const {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value) || std::isinf(value)) {
            throw std::invalid_argument("interval_t::deflate: value must not be NaN or +-infinity.");
        }
    }

    const T new_start = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add(m_start, value);
    const T new_end = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_end, value);

    if (new_end < new_start) {
        const T mid = std::midpoint(m_start, m_end);
        return interval_t<T>(mid, mid);
    }

    return interval_t<T>(new_start, new_end);
}

template <typename T>
interval_t<T> interval_t<T>::intersect(const interval_t<T>& other) const {
    interval_t<T> result = *this;
    result.m_start = std::max(result.m_start, other.m_start);
    result.m_end = std::max(result.m_start, std::min(result.m_end, other.m_end));
    return result;
}

template <typename T>
bool interval_t<T>::is_empty() const {
    return m_start == m_end;
}

template <typename T>
bool interval_t<T>::contains(const T& value) const {
    return m_start <= value && value < m_end;
}

template <typename T>
bool interval_t<T>::overlaps(const interval_t<T>& other) const {
    return m_start < other.m_end && other.m_start < m_end;
}

template <typename T>
T interval_t<T>::length() const {
    return m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_end, m_start);
}

} // namespace m03gin6lte1az5kj36aj9suk6t_interval

#endif // M03GIN6LTE1AZ5KJ36AJ9SUK6T_INTERVAL_MODULE_H
