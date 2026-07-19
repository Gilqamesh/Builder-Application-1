#ifndef M03GINTXCZOHR63Y44O77B4PYJ_HYPERRECTANGLE_H
# define M03GINTXCZOHR63Y44O77B4PYJ_HYPERRECTANGLE_H

# include <array>
# include <type_traits>
# include <stdexcept>
# include <algorithm>
# include <initializer_list>

# include <m03gin6lte1az5kj36aj9suk6t_interval/api.h>
# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

namespace m03gintxczohr63y44o77b4pyj_hyperrectangle {

/**
 * @brief N-dimensional hyperrectangle of type T.
 * 
 * Invariants:
 *   dimensions are represented as half-open intervals [start, end).
 *   operations always produce finite values, no NaN or +-infinity for floating point types.
 */
template <typename T, std::size_t N>
class hyperrectangle_t {
    static_assert(0 < N, "hyperrectangle_t does not support 0-dimensional hyperrectangles.");

public:
    /**
     * @brief Constructs a hyperrectangle with empty intervals [0, 0).
     */
    hyperrectangle_t() noexcept;

    /**
     * @brief Constructs a hyperrectangle with the given intervals.
     */
    hyperrectangle_t(const std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>& intervals) noexcept;

    /**
     * @brief Constructs a hyperrectangle with the given intervals.
     * 
     * @throws std::invalid_argument if the number of intervals is not equal to N.
     */
    hyperrectangle_t(std::initializer_list<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>> list);

    /**
     * @brief Constructs a hyperrectangle with the given corner and opposite corner.
     * 
     * @throws std::invalid_argument if any element of the corner or opposite corner is NaN or +-infinity for floating point types.
     */
    hyperrectangle_t(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& corner, const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& opposite_corner);

    /**
     * @brief Returns the corner of the hyperrectangle.
     */
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> corner() const noexcept;

    /**
     * @brief Returns the opposite corner of the hyperrectangle.
     */
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> opposite_corner() const noexcept;

    /**
     * @brief Sets the intervals of the hyperrectangle.
     */
    void bounds(const std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>& intervals) noexcept;

    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>::const_iterator begin() const noexcept;
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>::const_iterator end() const noexcept;

    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>::iterator begin() noexcept;
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>::iterator end() noexcept;

    /**
     * @brief Returns a reference to the interval at the given index.
     * 
     * Does no bound checking.
     */
    m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>& operator[](std::size_t index) noexcept;

    /**
     * @brief Returns a const reference to the interval at the given index.
     * 
     * Does no bound checking.
     */
    const m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>& operator[](std::size_t index) const noexcept;

    /**
     * @brief Adds a vector to the hyperrectangle using saturating arithmetic.
     * 
     * @throws std::invalid_argument if any element of the vector is NaN or +-infinity for floating point types.
     */
    hyperrectangle_t& operator+=(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector);

    /**
     * @brief Subtracts a vector from the hyperrectangle using saturating arithmetic.
     * 
     * @throws std::invalid_argument if any element of the vector is NaN or +-infinity for floating point types.
     */
    hyperrectangle_t& operator-=(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector);

    /**
     * @brief Returns a new hyperrectangle that is the result of adding a vector to the hyperrectangle using saturating arithmetic.
     * 
     * @throws std::invalid_argument if any element of the vector is NaN or +-infinity for floating point types.
     */
    hyperrectangle_t operator+(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) const;

    /**
     * @brief Returns a new hyperrectangle that is the result of subtracting a vector from the hyperrectangle using saturating arithmetic.
     * 
     * @throws std::invalid_argument if any element of the vector is NaN or +-infinity for floating point types.
     */
    hyperrectangle_t operator-(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) const;

    /**
     * @brief Clamps the given vector to the hyperrectangle.
     * 
     * If any element of the vector is greater or equal to the end of the corresponding interval, the end of the interval is returned, which is not part of the interval.
     * 
     * @throws std::invalid_argument if any element of the vector is NaN or +-infinity for floating point types.
     */
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> clamp(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) const;

    /**
     * @brief Inflates the hyperrectangle by the given value in all dimensions using saturating arithmetic.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    hyperrectangle_t inflate(const T& value) const;

    /**
     * @brief Inflates the hyperrectangle by the given values in each dimension using saturating arithmetic.
     * 
     * @throws std::invalid_argument if any element of the vector is NaN or +-infinity for floating point types.
     */
    hyperrectangle_t inflate(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& values) const;

    /**
     * @brief Deflates the hyperrectangle by the given value in all dimensions using saturating arithmetic.
     * 
     * @throws std::invalid_argument if value is NaN or +-infinity for floating point types.
     */
    hyperrectangle_t deflate(const T& value) const;

    /**
     * @brief Deflates the hyperrectangle by the given values in each dimension using saturating arithmetic.
     * 
     * @throws std::invalid_argument if any element of the vector is NaN or +-infinity for floating point types.
     */
    hyperrectangle_t deflate(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& values) const;

    /**
     * @brief Returns the intersection of this hyperrectangle with another hyperrectangle.
     * 
     * If the hyperrectangles do not intersect, the result is an empty hyperrectangle with the greater start values of the corresponding intervals.
     */
    hyperrectangle_t intersect(const hyperrectangle_t& other) const;

    /**
     * @brief Returns true if the hyperrectangle is empty (any interval is empty).
     */
    bool is_empty() const;

    /**
     * @brief Returns true if the hyperrectangle contains the given vector.
     */
    bool contains(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) const;

    /**
     * @brief Returns true if the hyperrectangle overlaps with another hyperrectangle.
     */
    bool overlaps(const hyperrectangle_t& other) const;

    /**
     * @brief Returns the lengths of the hyperrectangle in each dimension.
     */
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> lengths() const;

private:
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N> m_intervals;
};

} // namespace m03gintxczohr63y44o77b4pyj_hyperrectangle

namespace std {

template <typename T, std::size_t N>
struct std::formatter<m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<T, N>>;

} // namespace std

namespace m03gintxczohr63y44o77b4pyj_hyperrectangle {

template <typename T, std::size_t N>
hyperrectangle_t<T, N>::hyperrectangle_t() noexcept {
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N>::hyperrectangle_t(const std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>& intervals) noexcept:
    m_intervals(intervals)
{
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N>::hyperrectangle_t(std::initializer_list<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>> list) {
    if (list.size() != N) {
        throw std::invalid_argument("hyperrectangle_t does not support initializer lists of size different than N.");
    }
    std::copy(list.begin(), list.end(), m_intervals.begin());
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N>::hyperrectangle_t(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& corner, const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& opposite_corner) {
    for (std::size_t i = 0; i < N; ++i) {
        m_intervals[i] = m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>(corner[i], opposite_corner[i]);
    }
}

template <typename T, std::size_t N>
m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> hyperrectangle_t<T, N>::corner() const noexcept {
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> result;
    for (std::size_t i = 0; i < N; ++i) {
        result[i] = m_intervals[i][0];
    }
    return result;
}

template <typename T, std::size_t N>
m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> hyperrectangle_t<T, N>::opposite_corner() const noexcept {
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> result;
    for (std::size_t i = 0; i < N; ++i) {
        result[i] = m_intervals[i][1];
    }
    return result;
}

template <typename T, std::size_t N>
void hyperrectangle_t<T, N>::bounds(const std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>& intervals) noexcept {
    m_intervals = intervals;
}

template <typename T, std::size_t N>
std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>::const_iterator hyperrectangle_t<T, N>::begin() const noexcept {
    return m_intervals.begin();
}

template <typename T, std::size_t N>
std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>::const_iterator hyperrectangle_t<T, N>::end() const noexcept {
    return m_intervals.end();
}

template <typename T, std::size_t N>
std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>::iterator hyperrectangle_t<T, N>::begin() noexcept {
    return m_intervals.begin();
}

template <typename T, std::size_t N>
std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N>::iterator hyperrectangle_t<T, N>::end() noexcept {
    return m_intervals.end();
}

template <typename T, std::size_t N>
m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>& hyperrectangle_t<T, N>::operator[](std::size_t index) noexcept {
    return m_intervals[index];
}

template <typename T, std::size_t N>
const m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>& hyperrectangle_t<T, N>::operator[](std::size_t index) const noexcept {
    return m_intervals[index];
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N>& hyperrectangle_t<T, N>::operator+=(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) {
    for (std::size_t i = 0; i < N; ++i) {
        m_intervals[i] += vector[i];
    }
    return *this;
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N>& hyperrectangle_t<T, N>::operator-=(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) {
    for (std::size_t i = 0; i < N; ++i) {
        m_intervals[i] -= vector[i];
    }
    return *this;
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N> hyperrectangle_t<T, N>::operator+(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) const {
    hyperrectangle_t<T, N> result = *this;
    result += vector;
    return result;
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N> hyperrectangle_t<T, N>::operator-(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) const {
    hyperrectangle_t<T, N> result = *this;
    result -= vector;
    return result;
}

template <typename T, std::size_t N>
m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> hyperrectangle_t<T, N>::clamp(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) const {
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> clamped_vector;
    for (std::size_t i = 0; i < N; ++i) {
        clamped_vector[i] = m_intervals[i].clamp(vector[i]);
    }
    return clamped_vector;
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N> hyperrectangle_t<T, N>::inflate(const T& value) const {
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N> inflated_intervals;
    for (std::size_t i = 0; i < N; ++i) {
        inflated_intervals[i] = m_intervals[i].inflate(value);
    }
    return hyperrectangle_t<T, N>(inflated_intervals);
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N> hyperrectangle_t<T, N>::inflate(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& values) const {
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N> inflated_intervals;
    for (std::size_t i = 0; i < N; ++i) {
        inflated_intervals[i] = m_intervals[i].inflate(values[i]);
    }
    return hyperrectangle_t<T, N>(inflated_intervals);
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N> hyperrectangle_t<T, N>::deflate(const T& value) const {
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N> deflated_intervals;
    for (std::size_t i = 0; i < N; ++i) {
        deflated_intervals[i] = m_intervals[i].deflate(value);
    }
    return hyperrectangle_t<T, N>(deflated_intervals);
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N> hyperrectangle_t<T, N>::deflate(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& values) const {
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N> deflated_intervals;
    for (std::size_t i = 0; i < N; ++i) {
        deflated_intervals[i] = m_intervals[i].deflate(values[i]);
    }
    return hyperrectangle_t<T, N>(deflated_intervals);
}

template <typename T, std::size_t N>
hyperrectangle_t<T, N> hyperrectangle_t<T, N>::intersect(const hyperrectangle_t<T, N>& other) const {
    std::array<m03gin6lte1az5kj36aj9suk6t_interval::interval_t<T>, N> intersected_intervals;
    for (std::size_t i = 0; i < N; ++i) {
        intersected_intervals[i] = m_intervals[i].intersect(other.m_intervals[i]);
    }
    return hyperrectangle_t<T, N>(intersected_intervals);
}

template <typename T, std::size_t N>
bool hyperrectangle_t<T, N>::is_empty() const {
    for (const auto& interval : m_intervals) {
        if (interval.is_empty()) {
            return true;
        }
    }
    return false;
}

template <typename T, std::size_t N>
bool hyperrectangle_t<T, N>::contains(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector) const {
    for (std::size_t i = 0; i < N; ++i) {
        if (!m_intervals[i].contains(vector[i])) {
            return false;
        }
    }
    return true;
}

template <typename T, std::size_t N>
bool hyperrectangle_t<T, N>::overlaps(const hyperrectangle_t<T, N>& other) const {
    for (std::size_t i = 0; i < N; ++i) {
        if (!m_intervals[i].overlaps(other.m_intervals[i])) {
            return false;
        }
    }
    return true;
}

template <typename T, std::size_t N>
m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> hyperrectangle_t<T, N>::lengths() const {
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> result;
    for (std::size_t i = 0; i < N; ++i) {
        result[i] = m_intervals[i].length();
    }
    return result;
}

} // namespace m03gintxczohr63y44o77b4pyj_hyperrectangle

namespace std {

template <typename T, std::size_t N>
struct std::formatter<m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<T, N>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid hyperrectangle_t format specifier");
        }

        return it;
    }

    auto format(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<T, N>& hyperrectangle, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        for (std::size_t i = 0; i < N; ++i) {
            if (i > 0) {
                out = std::format_to(out, ", ");
            }
            const auto& interval = hyperrectangle[i];
            out = std::format_to(out, "{}", interval);
        }
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GINTXCZOHR63Y44O77B4PYJ_HYPERRECTANGLE_H
