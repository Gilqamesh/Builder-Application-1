#ifndef M03GIN6LTE1AZ5KJ36AJ9SUK6T_INTERVAL_MODULE_H
# define M03GIN6LTE1AZ5KJ36AJ9SUK6T_INTERVAL_MODULE_H

# include <cmath>

# include <algorithm>
# include <array>
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
    interval_t() noexcept;

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

    std::array<T, 2>::const_iterator begin() const noexcept;
    std::array<T, 2>::const_iterator end() const noexcept;

    const T& operator[](std::size_t index) const noexcept;

    /**
     * @brief Adds a value to both bounds using saturating arithmetic.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    interval_t& operator+=(const T& value);

    /**
     * @brief Subtracts a value from both bounds using saturating arithmetic.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    interval_t& operator-=(const T& value);

    /**
     * @brief Returns a new interval that is the result of adding a value to both bounds using saturating arithmetic.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    interval_t operator+(const T& value) const;

    /**
     * @brief Returns a new interval that is the result of subtracting a value from both bounds using saturating arithmetic.
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
    bool is_empty() const noexcept;

    /**
     * @brief Returns true if the interval contains the given value.
     * 
     * Empty intervals do not contain any values.
     */
    bool contains(const T& value) const noexcept;

    /**
     * @brief Returns true if the interval overlaps with another interval.
     * 
     * Empty intervals do not overlap with any intervals.
     */
    bool overlaps(const interval_t& other) const noexcept;

    /**
     * @brief Returns the length of the interval (end - start).
     * 
     * Guaranteed to be non-negative and finite, no NaN or +-infinity for floating point types.
     */
    T length() const;

private:
    std::array<T, 2> m_data;
};

} // namespace m03gin6lte1az5kj36aj9suk6t_interval

namespace std {

template <typename T>
struct std::formatter<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>>;

} // namespace std

namespace m03gin6lte1az5kj36aj9suk6t_interval {

template <typename T>
interval_t<T>::interval_t() noexcept:
    m_data{}
{
}

template <typename T>
interval_t<T>::interval_t(const T& start, const T& end):
    m_data{start, end}
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

    m_data[0] = start;
    m_data[1] = end;
}

template <typename T>
std::array<T, 2>::const_iterator interval_t<T>::begin() const noexcept {
    return m_data.begin();
}

template <typename T>
std::array<T, 2>::const_iterator interval_t<T>::end() const noexcept {
    return m_data.end();
}

template <typename T>
const T& interval_t<T>::operator[](std::size_t index) const noexcept {
    return m_data[index];
}

template <typename T>
interval_t<T>& interval_t<T>::operator+=(const T& value) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value) || std::isinf(value)) {
            throw std::invalid_argument("interval_t::operator+=: value must not be NaN or +-infinity.");
        }
    }

    m_data[0] = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add(m_data[0], value);
    m_data[1] = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add(m_data[1], value);
    return *this;
}

template <typename T>
interval_t<T>& interval_t<T>::operator-=(const T& value) {
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value) || std::isinf(value)) {
            throw std::invalid_argument("interval_t::operator-=: value must not be NaN or +-infinity.");
        }
    }

    m_data[0] = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_data[0], value);
    m_data[1] = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_data[1], value);
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

    if (value < m_data[0]) {
        return m_data[0];
    } else if (m_data[1] < value) {
        return m_data[1];
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

    const T new_start = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_data[0], value);
    const T new_end = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add(m_data[1], value);

    if (new_end < new_start) {
        const T mid = std::midpoint(m_data[0], m_data[1]);
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

    const T new_start = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::add(m_data[0], value);
    const T new_end = m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_data[1], value);

    if (new_end < new_start) {
        const T mid = std::midpoint(m_data[0], m_data[1]);
        return interval_t<T>(mid, mid);
    }

    return interval_t<T>(new_start, new_end);
}

template <typename T>
interval_t<T> interval_t<T>::intersect(const interval_t<T>& other) const {
    interval_t<T> result = *this;
    result.m_data[0] = std::max(result.m_data[0], other.m_data[0]);
    result.m_data[1] = std::max(result.m_data[0], std::min(result.m_data[1], other.m_data[1]));
    return result;
}

template <typename T>
bool interval_t<T>::is_empty() const noexcept {
    return m_data[0] == m_data[1];
}

template <typename T>
bool interval_t<T>::contains(const T& value) const noexcept {
    return m_data[0] <= value && value < m_data[1];
}

template <typename T>
bool interval_t<T>::overlaps(const interval_t<T>& other) const noexcept {
    return m_data[0] < other.m_data[1] && other.m_data[0] < m_data[1];
}

template <typename T>
T interval_t<T>::length() const {
    return m03ginuqujr8cbfieco8r61u54_saturating_arithmetic::sub(m_data[1], m_data[0]);
}

} // namespace m03gin6lte1az5kj36aj9suk6t_interval

namespace std {

template <typename T>
struct std::formatter<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid interval_t format specifier");
        }

        return it;
    }

    auto format(const m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>& interval, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "[{}, {})", interval[0], interval[1]);

        return out;
    }
};

} // namespace std

#endif // M03GIN6LTE1AZ5KJ36AJ9SUK6T_INTERVAL_MODULE_H
