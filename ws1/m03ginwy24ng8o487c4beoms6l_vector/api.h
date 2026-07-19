#ifndef M03GINWY24NG8O487C4BEOMS6L_VECTOR_MODULE_H
# define M03GINWY24NG8O487C4BEOMS6L_VECTOR_MODULE_H

#include <cmath>

# include <array>
# include <functional>
# include <type_traits>
# include <limits>
# include <initializer_list>
# include <algorithm>
# include <stdexcept>

# include <boost/container_hash/hash.hpp>

namespace m03ginwy24ng8o487c4beoms6l_vector {

/**
 * @brief N-dimensional vector of type T.
 * 
 * Does no bound checking on its operations.
 * 
 * @tparam T The type of the vector elements.
 * @tparam N The number of dimensions of the vector.
 */
template <typename T, std::size_t N>
class vector_t {
    static_assert(0 < N, "vector_t does not support 0-dimensional vectors.");

public:
    using length_t = std::conditional_t<std::is_floating_point_v<T>, T, double>;

public:
    /**
     * @brief Constructs a vector_t with uninitialized elements.
     */
    vector_t();

    /**
     * @brief Constructs a vector_t with all elements set to the given value.
     * 
     * @param value The value to set all elements to.
     */
    vector_t(const T& value);

    /**
     * @brief Constructs a vector_t with the given elements.
     * 
     * @param data The array of elements to initialize the vector with.
     */
    vector_t(const std::array<T, N>& data);

    /**
     * @brief Constructs a vector_t with the given elements.
     * 
     * The number of elements in the initializer list must be equal to N.
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
     * Can only be called for floating point types.
     * No bound checking is performed, so the caller must ensure that the vector is not zero.
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

    T& operator[](std::size_t index); // does no bound checking
    const T& operator[](std::size_t index) const; // does no bound checking

    /**
     * @brief Compares two vectors for equality.
     * 
     * For floating point types, if either vector contains NaN, the vectors are considered not equal.
     */
    bool operator==(const vector_t& other) const;

    vector_t& operator+=(const vector_t& other); // element-wise, does no bound checking
    vector_t& operator-=(const vector_t& other); // element-wise, does no bound checking
    vector_t& operator*=(const vector_t& other); // element-wise, does no bound checking
    vector_t& operator/=(const vector_t& other); // element-wise, does no bound checking
    vector_t operator+(const vector_t& other) const; // element-wise, does no bound checking
    vector_t operator-(const vector_t& other) const; // element-wise, does no bound checking
    vector_t operator*(const vector_t& other) const; // element-wise, does no bound checking
    vector_t operator/(const vector_t& other) const; // element-wise, does no bound checking

    template <typename U>
    vector_t& operator+=(U value); // element-wise, does no bound checking, operation relies on implicit conversion
    template <typename U>
    vector_t& operator-=(U value); // element-wise, does no bound checking, operation relies on implicit conversion
    template <typename U>
    vector_t& operator*=(U value); // element-wise, does no bound checking, operation relies on implicit conversion
    template <typename U>
    vector_t& operator/=(U value); // element-wise, does no bound checking, operation relies on implicit conversion
    template <typename U>
    vector_t operator+(U value) const; // element-wise, does no bound checking, operation relies on implicit conversion
    template <typename U>
    vector_t operator-(U value) const; // element-wise, does no bound checking, operation relies on implicit conversion
    template <typename U>
    vector_t operator*(U value) const; // element-wise, does no bound checking, operation relies on implicit conversion
    template <typename U>
    vector_t operator/(U value) const; // element-wise, does no bound checking, operation relies on implicit conversion

private:
    std::array<T, N> m_data;
};

} // namespace m03ginwy24ng8o487c4beoms6l_vector

namespace std {

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

#endif // M03GINWY24NG8O487C4BEOMS6L_VECTOR_MODULE_H
