#ifndef M03GINWY24NG8O487C4BEOMS6L_VECTOR_API_H
# define M03GINWY24NG8O487C4BEOMS6L_VECTOR_API_H

# include <cmath>

# include <array>
# include <functional>
# include <type_traits>
# include <limits>
# include <initializer_list>
# include <algorithm>
# include <stdexcept>
# include <format>

# include <boost/container_hash/hash.hpp>

namespace m03ginwy24ng8o487c4beoms6l_vector {

/**
 * @brief Owns N mutable components with componentwise arithmetic and vector lengths.
 *
 * Vector multiplication and division operate on corresponding components.
 * Scalar operations apply to every component and cast each result to T.
 * Arithmetic follows the underlying operators without saturation or checks
 * for overflow, zero divisors or non-finite floating point values; callers
 * must avoid undefined arithmetic and invalid conversions.
 *
 * Elements are stored contiguously in index order. References and iterators
 * borrow this object's storage for its lifetime; mutation and assignment
 * change the elements they observe. Array and initializer-list constructors
 * copy their elements. Use parentheses for scalar fill. Nonempty braces of
 * component values select the initializer-list constructor and require exactly
 * N elements; empty braces select the default constructor, which does not
 * initialize scalar elements.
 *
 * @tparam T The type of the vector elements.
 * @tparam N The positive number of dimensions of the vector.
 *
 * @code{.cpp}
 * #include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
 *
 * #include <cassert>
 * #include <stdexcept>
 *
 * int main() {
 *     using vector_t = m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>;
 *     const vector_t zero_vector(0.0f); // Fills all three components.
 *     const vector_t vector{1.0f, 2.0f, 3.0f};
 *     const vector_t scale_vector{2.0f, 3.0f, 4.0f};
 *     const auto product_vector = vector * scale_vector;
 *     assert(zero_vector.is_zero());
 *     assert((product_vector == vector_t{2.0f, 6.0f, 12.0f}));
 *
 *     bool rejected = false;
 *     try {
 *         const vector_t short_vector{0.0f}; // One element, not scalar fill.
 *     } catch (const std::invalid_argument&) {
 *         rejected = true;
 *     }
 *     assert(rejected);
 * }
 * @endcode
 */
template <typename T, std::size_t N>
class vector_t {
    static_assert(0 < N, "vector_t does not support 0-dimensional vectors.");

public:
    /** @brief Uses T for floating point lengths and double for other element types. */
    using length_t = std::conditional_t<std::is_floating_point_v<T>, T, double>;

public:
    /**
     * @brief Default-initializes the elements without initializing scalar elements.
     *
     * Assign scalar elements before reading them, including through arithmetic,
     * length queries or formatting. Empty braces also call this constructor.
     */
    vector_t();

    /**
     * @brief Constructs a vector_t with all elements set to the given value.
     * 
     * @param value The value to set all elements to.
     */
    vector_t(const T& value);

    /**
     * @brief Copies the array's elements into the vector in index order.
     *
     * @param data The array of elements to initialize the vector with.
     */
    vector_t(const std::array<T, N>& data);

    /**
     * @brief Copies exactly N initializer-list elements in index order.
     *
     * @throws std::invalid_argument If list.size() is not N.
     *
     * @param list The initializer list of elements to initialize the vector with.
     */
    vector_t(std::initializer_list<T> list);

    vector_t(const vector_t&) = default;
    vector_t(vector_t&&) = default;

    vector_t& operator=(const vector_t&) = default;
    vector_t& operator=(vector_t&&) = default;

    /**
     * @brief Returns true if the vector is a zero vector (all elements are zero).
     */
    bool is_zero() const;

    /**
     * @brief Returns a unit vector in the same direction as this vector.
     *
     * Can only be called for floating point types. Divides each component by
     * euclidean_length(). For a meaningful unit result, the components and
     * the computed length must be finite and the length nonzero; these
     * conditions are not checked and invalid inputs are not rejected.
     */
    vector_t unit() const;

    /**
     * @brief Returns the Chebyshev length of the vector.
     * 
     * The Chebyshev length is the maximum absolute value of the vector's elements.
     */
    length_t chebyshev_length() const;

    /**
     * @brief Returns the Euclidean length of the vector.
     * 
     * The Euclidean length is the square root of the sum of the squares of the vector's elements.
     */
    length_t euclidean_length() const;

    /**
     * @brief Returns the Manhattan length of the vector.
     * 
     * The Manhattan length is the sum of the absolute values of the vector's elements.
     */
    length_t manhattan_length() const;

    /**
     * @brief Returns the taxicab length of the vector.
     * 
     * The taxicab length is the sum of the absolute values of the vector's elements.
     */
    length_t taxicab_length() const;

    /**
     * @brief Returns the squared Euclidean length of the vector.
     * 
     * The squared Euclidean length is the sum of the squares of the vector's elements.
     */
    length_t euclidean_length_squared() const;

    /** @brief Returns a read-only iterator to the first component. */
    std::array<T, N>::const_iterator begin() const;
    /** @brief Returns the read-only iterator past the N components. */
    std::array<T, N>::const_iterator end() const;
    /** @brief Returns a mutable iterator to the first component. */
    std::array<T, N>::iterator begin();
    /** @brief Returns the mutable iterator past the N components. */
    std::array<T, N>::iterator end();

    /**
     * @brief Borrows the component at the given index for mutation.
     * @pre index < N; no bounds check is performed.
     */
    T& operator[](std::size_t index);
    /**
     * @brief Borrows the component at the given index for reading.
     * @pre index < N; no bounds check is performed.
     */
    const T& operator[](std::size_t index) const;

    /**
     * @brief Compares two vectors for equality.
     * 
     * For floating point types, if either vector contains NaN, the vectors are considered not equal.
     */
    bool operator==(const vector_t& other) const;

    /** @brief Adds corresponding components in place and returns *this. */
    vector_t& operator+=(const vector_t& other);
    /** @brief Subtracts corresponding components in place and returns *this. */
    vector_t& operator-=(const vector_t& other);
    /** @brief Multiplies corresponding components in place and returns *this. */
    vector_t& operator*=(const vector_t& other);
    /** @brief Divides corresponding components in place and returns *this. */
    vector_t& operator/=(const vector_t& other);
    /** @brief Returns the componentwise sum. */
    vector_t operator+(const vector_t& other) const;
    /** @brief Returns the componentwise difference. */
    vector_t operator-(const vector_t& other) const;
    /** @brief Returns a vector with each component negated. */
    vector_t operator-() const;
    /** @brief Returns the componentwise product. */
    vector_t operator*(const vector_t& other) const;
    /** @brief Returns the componentwise quotient. */
    vector_t operator/(const vector_t& other) const;

    /** @brief Adds the scalar to every component in place and returns *this. */
    template <typename U>
    vector_t& operator+=(U value);
    /** @brief Subtracts the scalar from every component in place and returns *this. */
    template <typename U>
    vector_t& operator-=(U value);
    /** @brief Multiplies every component by the scalar in place and returns *this. */
    template <typename U>
    vector_t& operator*=(U value);
    /** @brief Divides every component by the scalar in place and returns *this. */
    template <typename U>
    vector_t& operator/=(U value);
    /** @brief Returns a vector with the scalar added to each component. */
    template <typename U>
    vector_t operator+(U value) const;
    /** @brief Returns a vector with the scalar subtracted from each component. */
    template <typename U>
    vector_t operator-(U value) const;
    /** @brief Returns a vector with each component multiplied by the scalar. */
    template <typename U>
    vector_t operator*(U value) const;
    /** @brief Returns a vector with each component divided by the scalar. */
    template <typename U>
    vector_t operator/(U value) const;

private:
    std::array<T, N> m_data;
};

} // namespace m03ginwy24ng8o487c4beoms6l_vector

namespace std {

template <typename T, std::size_t N>
struct formatter<m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>>;

template <typename T, std::size_t N>
struct hash<m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>>;

} // namespace std

namespace m03ginwy24ng8o487c4beoms6l_vector {

template <typename T, std::size_t N>
vector_t<T, N>::vector_t()
{
}

template <typename T, std::size_t N>
vector_t<T, N>::vector_t(const T& value) {
    m_data.fill(value);
}

template <typename T, std::size_t N>
vector_t<T, N>::vector_t(const std::array<T, N>& data):
    m_data(data)
{
}

template <typename T, std::size_t N>
vector_t<T, N>::vector_t(std::initializer_list<T> list) {
    if (list.size() != N) {
        throw std::invalid_argument("vector_t does not support initializer lists of size different than N.");
    }
    std::copy(list.begin(), list.end(), m_data.begin());
}

template <typename T, std::size_t N>
bool vector_t<T, N>::is_zero() const {
    for (const auto& data : m_data) {
        if (data != static_cast<T>(0)) {
            return false;
        }
    }
    return true;
}

template <typename T, std::size_t N>
vector_t<T, N> vector_t<T, N>::unit() const {
    static_assert(std::is_floating_point_v<T>, "vector_t::unit() does not support non-floating point types.");

    const auto length = euclidean_length();
    vector_t result(*this);
    for (auto& data : result.m_data) {
        const auto converted_data = static_cast<length_t>(data);
        data = static_cast<T>(converted_data / length);
    }
    return result;
}

template <typename T, std::size_t N>
typename vector_t<T, N>::length_t vector_t<T, N>::chebyshev_length() const {
    length_t result = static_cast<length_t>(0);
    for (const auto& data : m_data) {
        const auto converted_data = static_cast<length_t>(data);
        if constexpr (std::is_floating_point_v<T>) {
            if (std::isnan(converted_data)) {
                return std::numeric_limits<length_t>::quiet_NaN();
            }
        }
        result = std::max(result, std::abs(converted_data));
    }
    return result;
}

template <typename T, std::size_t N>
typename vector_t<T, N>::length_t vector_t<T, N>::euclidean_length() const {
    length_t result = std::abs(static_cast<length_t>(m_data[0]));
    for (std::size_t i = 1; i < N; ++i) {
        const auto converted_data = static_cast<length_t>(m_data[i]);
        result = std::hypot(result, converted_data);
    }
    return result;
}

template <typename T, std::size_t N>
typename vector_t<T, N>::length_t vector_t<T, N>::manhattan_length() const {
    length_t result = static_cast<length_t>(0);
    for (const auto& data : m_data) {
        const auto converted_data = static_cast<length_t>(data);
        result += std::abs(converted_data);
    }
    return result;
}

template <typename T, std::size_t N>
typename vector_t<T, N>::length_t vector_t<T, N>::taxicab_length() const {
    return manhattan_length();
}

template <typename T, std::size_t N>
typename vector_t<T, N>::length_t vector_t<T, N>::euclidean_length_squared() const {
    length_t result = static_cast<length_t>(0);
    for (const auto& data : m_data) {
        const auto converted_data = static_cast<length_t>(data);
        result += converted_data * converted_data;
    }
    return result;
}

template <typename T, std::size_t N>
std::array<T, N>::const_iterator vector_t<T, N>::begin() const {
    return m_data.begin();
}

template <typename T, std::size_t N>
std::array<T, N>::const_iterator vector_t<T, N>::end() const {
    return m_data.end();
}

template <typename T, std::size_t N>
std::array<T, N>::iterator vector_t<T, N>::begin() {
    return m_data.begin();
}

template <typename T, std::size_t N>
std::array<T, N>::iterator vector_t<T, N>::end() {
    return m_data.end();
}

template <typename T, std::size_t N>
T& vector_t<T, N>::operator[](std::size_t index) {
    return m_data[index];
}

template <typename T, std::size_t N>
const T& vector_t<T, N>::operator[](std::size_t index) const {
    return m_data[index];
}

template <typename T, std::size_t N>
bool vector_t<T, N>::operator==(const vector_t<T, N>& other) const {
    for (std::size_t i = 0; i < N; ++i) {
        if constexpr (std::is_floating_point_v<T>) {
            if (std::isnan(m_data[i]) || std::isnan(other.m_data[i])) {
                return false;
            }
        }
        if (m_data[i] != other.m_data[i]) {
            return false;
        }
    }
    return true;
}

template <typename T, std::size_t N>
vector_t<T, N>& vector_t<T, N>::operator+=(const vector_t<T, N>& other) {
    for (std::size_t i = 0; i < N; ++i) {
        m_data[i] += other.m_data[i];
    }
    return *this;
}

template <typename T, std::size_t N>
vector_t<T, N>& vector_t<T, N>::operator-=(const vector_t<T, N>& other) {
    for (std::size_t i = 0; i < N; ++i) {
        m_data[i] -= other.m_data[i];
    }
    return *this;
}

template <typename T, std::size_t N>
vector_t<T, N>& vector_t<T, N>::operator*=(const vector_t<T, N>& other) {
    for (std::size_t i = 0; i < N; ++i) {
        m_data[i] *= other.m_data[i];
    }
    return *this;
}

template <typename T, std::size_t N>
vector_t<T, N>& vector_t<T, N>::operator/=(const vector_t<T, N>& other) {
    for (std::size_t i = 0; i < N; ++i) {
        m_data[i] /= other.m_data[i];
    }
    return *this;
}

template <typename T, std::size_t N>
vector_t<T, N> vector_t<T, N>::operator+(const vector_t<T, N>& other) const {
    vector_t result = *this;
    result += other;
    return result;
}

template <typename T, std::size_t N>
vector_t<T, N> vector_t<T, N>::operator-(const vector_t<T, N>& other) const {
    vector_t result = *this;
    result -= other;
    return result;
}

template <typename T, std::size_t N>
vector_t<T, N> vector_t<T, N>::operator-() const {
    vector_t result;
    for (std::size_t i = 0; i < N; ++i) {
        result[i] = -m_data[i];
    }
    return result;
}

template <typename T, std::size_t N>
vector_t<T, N> vector_t<T, N>::operator*(const vector_t<T, N>& other) const {
    vector_t result = *this;
    result *= other;
    return result;
}

template <typename T, std::size_t N>
vector_t<T, N> vector_t<T, N>::operator/(const vector_t<T, N>& other) const {
    vector_t result = *this;
    result /= other;
    return result;
}

template <typename T, std::size_t N>
template <typename U>
vector_t<T, N>& vector_t<T, N>::operator+=(U value) {
    for (auto& data : m_data) {
        data = static_cast<T>(data + value);
    }
    return *this;
}

template <typename T, std::size_t N>
template <typename U>
vector_t<T, N>& vector_t<T, N>::operator-=(U value) {
    for (auto& data : m_data) {
        data = static_cast<T>(data - value);
    }
    return *this;
}

template <typename T, std::size_t N>
template <typename U>
vector_t<T, N>& vector_t<T, N>::operator*=(U value) {
    for (auto& data : m_data) {
        data = static_cast<T>(data * value);
    }
    return *this;
}

template <typename T, std::size_t N>
template <typename U>
vector_t<T, N>& vector_t<T, N>::operator/=(U value) {
    for (auto& data : m_data) {
        data = static_cast<T>(data / value);
    }
    return *this;
}

template <typename T, std::size_t N>
template <typename U>
vector_t<T, N> vector_t<T, N>::operator+(U value) const {
    vector_t result = *this;
    result += value;
    return result;
}

template <typename T, std::size_t N>
template <typename U>
vector_t<T, N> vector_t<T, N>::operator-(U value) const {
    vector_t result = *this;
    result -= value;
    return result;
}

template <typename T, std::size_t N>
template <typename U>
vector_t<T, N> vector_t<T, N>::operator*(U value) const {
    vector_t result = *this;
    result *= value;
    return result;
}

template <typename T, std::size_t N>
template <typename U>
vector_t<T, N> vector_t<T, N>::operator/(U value) const {
    vector_t result = *this;
    result /= value;
    return result;
}

} // namespace m03ginwy24ng8o487c4beoms6l_vector

namespace std {

template <typename T, std::size_t N>
struct formatter<m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>> {

    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid vector_t format specifier");
        }

        return it;
    }

    auto format(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& vector, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        for (std::size_t i = 0; i < N; ++i) {
            if (i > 0) {
                out = std::format_to(out, ", ");
            }
            out = std::format_to(out, "{}", vector[i]);
        }
        out = std::format_to(out, " }}");

        return out;
    }
};

template <typename T, std::size_t N>
struct hash<m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>> {
    std::size_t operator()(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N>& v) const {
        std::size_t result = 0;
        for (std::size_t i = 0; i < N; ++i) {
            boost::hash_combine(result, v[i]);
        }
        return result;
    }
};

} // namespace std

#endif // M03GINWY24NG8O487C4BEOMS6L_VECTOR_API_H
