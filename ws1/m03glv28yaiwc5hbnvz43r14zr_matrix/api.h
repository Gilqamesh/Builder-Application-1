#ifndef M03GLV28YAIWC5HBNVZ43R14ZR_MATRIX_API_H
# define M03GLV28YAIWC5HBNVZ43R14ZR_MATRIX_API_H

# include <array>
# include <algorithm>
# include <cstddef>
# include <format>
# include <initializer_list>
# include <stdexcept>

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

namespace m03glv28yaiwc5hbnvz43r14zr_matrix {

/**
 * @brief Owns an N-row by M-column matrix with arithmetic and matrix/vector products.
 *
 * N and M must be positive. Elements are stored contiguously in row-major
 * order: element (row, column) has offset row * M + column. Iterators visit
 * all N * M elements in that order. References and iterators borrow this
 * object's storage for its lifetime; mutation and assignment change the
 * elements they observe. Array and initializer-list constructors copy their elements.
 *
 * Addition, subtraction, negation and scalar scaling operate elementwise.
 * Arithmetic uses the underlying operators without saturation or checks for
 * overflow, zero divisors or non-finite floating point values; callers must
 * avoid undefined arithmetic and invalid conversions. Scalar results and
 * each accumulated product sum are cast to T.
 *
 * @code{.cpp}
 * #include <m03glv28yaiwc5hbnvz43r14zr_matrix/api.h>
 * #include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
 *
 * #include <cassert>
 * #include <stdexcept>
 *
 * int main() {
 *     using matrix_t = m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<int, 2, 3>;
 *     const matrix_t zero_matrix(0); // Fills all six elements.
 *     const matrix_t matrix{
 *         1, 2, 3, // Row 0.
 *         4, 5, 6  // Row 1.
 *     };
 *     const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 3> column_vector{1, 2, 3};
 *     const auto product_vector = matrix * column_vector;
 *     assert(product_vector[0] == 14); // 1*1 + 2*2 + 3*3.
 *     assert(product_vector[1] == 32); // 4*1 + 5*2 + 6*3.
 *     assert(matrix(1, 0) == 4);
 *     assert(zero_matrix(1, 2) == 0);
 *
 *     bool rejected = false;
 *     try {
 *         const matrix_t short_matrix{0}; // One element, not scalar fill.
 *     } catch (const std::invalid_argument&) {
 *         rejected = true;
 *     }
 *     assert(rejected);
 * }
 * @endcode
 */
template <typename T, std::size_t N, std::size_t M>
class matrix_t {
    static_assert(0 < N, "matrix_t does not support matrices with 0 rows.");
    static_assert(0 < M, "matrix_t does not support matrices with 0 columns.");

public:
    using value_type = T;
    static constexpr std::size_t row_count = N;
    static constexpr std::size_t column_count = M;

public:
    /**
     * @brief Default-initializes the elements without initializing scalar elements.
     *
     * Assign scalar elements before reading them, including through arithmetic
     * or formatting. Empty braces also call this constructor.
     */
    matrix_t();
    /**
     * @brief Fills every element with the supplied scalar.
     *
     * Use parentheses for scalar fill; nonempty braces of scalar elements
     * select the list constructor.
     */
    matrix_t(const T& value);
    /** @brief Copies the array's N * M elements in row-major order. */
    matrix_t(const std::array<T, N * M>& data);
    /**
     * @brief Copies exactly N * M initializer-list elements in row-major order.
     * @throws std::invalid_argument If list.size() is not N * M.
     */
    matrix_t(std::initializer_list<T> list);

    matrix_t(const matrix_t&) = default;
    matrix_t(matrix_t&&) = default;

    matrix_t& operator=(const matrix_t&) = default;
    matrix_t& operator=(matrix_t&&) = default;

    /** @brief Returns a read-only iterator to the first row-major element. */
    typename std::array<T, N * M>::const_iterator begin() const;
    /** @brief Returns the read-only iterator past the N * M elements. */
    typename std::array<T, N * M>::const_iterator end() const;
    /** @brief Returns a mutable iterator to the first row-major element. */
    typename std::array<T, N * M>::iterator begin();
    /** @brief Returns the mutable iterator past the N * M elements. */
    typename std::array<T, N * M>::iterator end();

    /**
     * @brief Borrows the element at the given row and column for mutation.
     * @pre row < N and column < M; no bounds check is performed.
     */
    T& operator()(std::size_t row, std::size_t column);
    /**
     * @brief Borrows the element at the given row and column for reading.
     * @pre row < N and column < M; no bounds check is performed.
     */
    const T& operator()(std::size_t row, std::size_t column) const;

    /** @brief Returns whether all corresponding elements compare equal. */
    bool operator==(const matrix_t& other) const;

    /** @brief Adds corresponding elements in place and returns *this. */
    matrix_t& operator+=(const matrix_t& other);
    /** @brief Subtracts corresponding elements in place and returns *this. */
    matrix_t& operator-=(const matrix_t& other);
    /** @brief Returns the elementwise sum. */
    matrix_t operator+(const matrix_t& other) const;
    /** @brief Returns the elementwise difference. */
    matrix_t operator-(const matrix_t& other) const;
    /** @brief Returns a matrix with every element negated. */
    matrix_t operator-() const;

    /** @brief Multiplies every element by the scalar in place and returns *this. */
    template <typename U>
    matrix_t& operator*=(U value);
    /** @brief Divides every element by the scalar in place and returns *this. */
    template <typename U>
    matrix_t& operator/=(U value);
    /** @brief Returns a matrix with every element multiplied by the scalar. */
    template <typename U>
    matrix_t operator*(U value) const;
    /** @brief Returns a matrix with every element divided by the scalar. */
    template <typename U>
    matrix_t operator/(U value) const;

    /**
     * @brief Multiplies by an M-component column vector to return an N-component vector.
     *
     * Result row r sums (*this)(r, c) * vector[c] in increasing column order.
     */
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> operator*(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, M>& vector) const;

    /**
     * @brief Multiplies by an M-row by P-column matrix to return an N-row by P-column matrix.
     *
     * Result (r, c) sums (*this)(r, i) * other(i, c) in increasing i order.
     */
    template <std::size_t P>
    matrix_t<T, N, P> operator*(const matrix_t<T, M, P>& other) const;

private:
    std::array<T, N * M> m_data;
};

} // namespace m03glv28yaiwc5hbnvz43r14zr_matrix

namespace std {

template <typename T, std::size_t N, std::size_t M>
struct formatter<m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<T, N, M>>;

} // namespace std

namespace m03glv28yaiwc5hbnvz43r14zr_matrix {

template <typename T, std::size_t N, std::size_t M>
matrix_t<T, N, M>::matrix_t()
{
}

template <typename T, std::size_t N, std::size_t M>
matrix_t<T, N, M>::matrix_t(const T& value) {
    m_data.fill(value);
}

template <typename T, std::size_t N, std::size_t M>
matrix_t<T, N, M>::matrix_t(const std::array<T, N * M>& data):
    m_data(data)
{
}

template <typename T, std::size_t N, std::size_t M>
matrix_t<T, N, M>::matrix_t(std::initializer_list<T> list) {
    if (list.size() != N * M) {
        throw std::invalid_argument("matrix_t does not support initializer lists of size different than N * M.");
    }
    std::copy(list.begin(), list.end(), m_data.begin());
}

template <typename T, std::size_t N, std::size_t M>
typename std::array<T, N * M>::const_iterator matrix_t<T, N, M>::begin() const {
    return m_data.begin();
}

template <typename T, std::size_t N, std::size_t M>
typename std::array<T, N * M>::const_iterator matrix_t<T, N, M>::end() const {
    return m_data.end();
}

template <typename T, std::size_t N, std::size_t M>
typename std::array<T, N * M>::iterator matrix_t<T, N, M>::begin() {
    return m_data.begin();
}

template <typename T, std::size_t N, std::size_t M>
typename std::array<T, N * M>::iterator matrix_t<T, N, M>::end() {
    return m_data.end();
}

template <typename T, std::size_t N, std::size_t M>
T& matrix_t<T, N, M>::operator()(std::size_t row, std::size_t column) {
    return m_data[row * M + column];
}

template <typename T, std::size_t N, std::size_t M>
const T& matrix_t<T, N, M>::operator()(std::size_t row, std::size_t column) const {
    return m_data[row * M + column];
}

template <typename T, std::size_t N, std::size_t M>
bool matrix_t<T, N, M>::operator==(const matrix_t& other) const {
    return m_data == other.m_data;
}

template <typename T, std::size_t N, std::size_t M>
matrix_t<T, N, M>& matrix_t<T, N, M>::operator+=(const matrix_t& other) {
    for (std::size_t i = 0; i < m_data.size(); ++i) {
        m_data[i] += other.m_data[i];
    }
    return *this;
}

template <typename T, std::size_t N, std::size_t M>
matrix_t<T, N, M>& matrix_t<T, N, M>::operator-=(const matrix_t& other) {
    for (std::size_t i = 0; i < m_data.size(); ++i) {
        m_data[i] -= other.m_data[i];
    }
    return *this;
}

template <typename T, std::size_t N, std::size_t M>
matrix_t<T, N, M> matrix_t<T, N, M>::operator+(const matrix_t& other) const {
    matrix_t result(*this);
    result += other;
    return result;
}

template <typename T, std::size_t N, std::size_t M>
matrix_t<T, N, M> matrix_t<T, N, M>::operator-(const matrix_t& other) const {
    matrix_t result(*this);
    result -= other;
    return result;
}

template <typename T, std::size_t N, std::size_t M>
matrix_t<T, N, M> matrix_t<T, N, M>::operator-() const {
    matrix_t result;
    for (std::size_t i = 0; i < m_data.size(); ++i) {
        result.m_data[i] = -m_data[i];
    }
    return result;
}

template <typename T, std::size_t N, std::size_t M>
template <typename U>
matrix_t<T, N, M>& matrix_t<T, N, M>::operator*=(U value) {
    for (auto& element : m_data) {
        element = static_cast<T>(element * value);
    }
    return *this;
}

template <typename T, std::size_t N, std::size_t M>
template <typename U>
matrix_t<T, N, M>& matrix_t<T, N, M>::operator/=(U value) {
    for (auto& element : m_data) {
        element = static_cast<T>(element / value);
    }
    return *this;
}

template <typename T, std::size_t N, std::size_t M>
template <typename U>
matrix_t<T, N, M> matrix_t<T, N, M>::operator*(U value) const {
    matrix_t result(*this);
    result *= value;
    return result;
}

template <typename T, std::size_t N, std::size_t M>
template <typename U>
matrix_t<T, N, M> matrix_t<T, N, M>::operator/(U value) const {
    matrix_t result(*this);
    result /= value;
    return result;
}

template <typename T, std::size_t N, std::size_t M>
m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> matrix_t<T, N, M>::operator*(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, M>& vector) const {
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, N> result(static_cast<T>(0));
    for (std::size_t row = 0; row < N; ++row) {
        for (std::size_t column = 0; column < M; ++column) {
            result[row] = static_cast<T>(result[row] + (*this)(row, column) * vector[column]);
        }
    }
    return result;
}

template <typename T, std::size_t N, std::size_t M>
template <std::size_t P>
matrix_t<T, N, P> matrix_t<T, N, M>::operator*(const matrix_t<T, M, P>& other) const {
    matrix_t<T, N, P> result(static_cast<T>(0));
    for (std::size_t row = 0; row < N; ++row) {
        for (std::size_t column = 0; column < P; ++column) {
            for (std::size_t i = 0; i < M; ++i) {
                result(row, column) = static_cast<T>(result(row, column) + (*this)(row, i) * other(i, column));
            }
        }
    }
    return result;
}

} // namespace m03glv28yaiwc5hbnvz43r14zr_matrix

namespace std {

template <typename T, std::size_t N, std::size_t M>
struct formatter<m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<T, N, M>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid matrix_t format specifier");
        }
        return it;
    }

    auto format(const m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<T, N, M>& matrix, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        for (std::size_t row = 0; row < N; ++row) {
            if (0 < row) {
                out = std::format_to(out, ", ");
            }
            out = std::format_to(out, "{{ ");
            for (std::size_t column = 0; column < M; ++column) {
                if (0 < column) {
                    out = std::format_to(out, ", ");
                }
                out = std::format_to(out, "{}", matrix(row, column));
            }
            out = std::format_to(out, " }}");
        }
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GLV28YAIWC5HBNVZ43R14ZR_MATRIX_API_H
