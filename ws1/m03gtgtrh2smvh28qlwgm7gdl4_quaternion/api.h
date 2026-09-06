#ifndef M03GTGTRH2SMVH28QLWGM7GDL4_QUATERNION_API_H
# define M03GTGTRH2SMVH28QLWGM7GDL4_QUATERNION_API_H

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03glv28yaiwc5hbnvz43r14zr_matrix/api.h>

# include <cmath>
# include <concepts>
# include <cstddef>
# include <format>
# include <limits>
# include <stdexcept>

namespace m03gtgtrh2smvh28qlwgm7gdl4_quaternion {

/**
 * @brief Quaternion with freely mutable components ordered (w, x, y, z).
 *
 * Rotations are active and right-handed, acting on column vectors; q2 * q1
 * applies q1, then q2. Rotation conversions preserve orientation, not sign.
 */
template <std::floating_point T>
class quaternion_t {
public:
    using vector3_t = m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, 3>;
    using matrix3_t = m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<T, 3, 3>;

    /** @brief Constructs the identity (1, 0, 0, 0). */
    quaternion_t();
    quaternion_t(T w, T x, T y, T z);

    T& w();
    const T& w() const;
    T& x();
    const T& x() const;
    T& y();
    const T& y() const;
    T& z();
    const T& z() const;

    /** @brief Returns a unit rotation about fixed X, then Y, then Z axes in finite radians. */
    static quaternion_t from_euler_xyz(const vector3_t& radians);
    /** @brief Returns a unit rotation, normalizing the finite, nonzero axis and requiring finite radians. */
    static quaternion_t from_axis_angle(const vector3_t& axis, T radians);
    /**
     * @brief Returns a unit quaternion from a finite, proper orthonormal rotation matrix.
     *
     * Rejects inputs when any element of RᵀR - I or det(R) - 1 has absolute
     * magnitude greater than 64 * epsilon(T).
     */
    static quaternion_t from_matrix(const matrix3_t& matrix);

    /** @brief Compares components with scalar equality; opposite signs generally compare unequal. */
    bool operator==(const quaternion_t& other) const;
    /** @brief Returns the Hamilton product without normalization. */
    quaternion_t operator*(const quaternion_t& other) const;
    quaternion_t& operator*=(const quaternion_t& other);

    /** @brief Returns Euclidean magnitude, avoiding intermediate overflow/underflow; out-of-range magnitudes may be positive infinity. */
    T norm() const;
    /** @brief Normalizes every finite, nonzero quaternion, rejecting zero or non-finite components. */
    quaternion_t unit() const;
    /** @brief Returns (w, -x, -y, -z). */
    quaternion_t conjugate() const;
    /** @brief Returns the general inverse, rejecting zero or non-finite inputs and reporting result overflow. */
    quaternion_t inverse() const;

    /** @brief Applies the normalized rotation to a finite vector, rejecting invalid quaternions and reporting non-finite results. */
    vector3_t rotate(const vector3_t& vector) const;
    /** @brief Returns the normalized rotation matrix, rejecting zero or non-finite quaternions. */
    matrix3_t to_matrix() const;

private:
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, 4> m_components;
};

} // namespace m03gtgtrh2smvh28qlwgm7gdl4_quaternion

namespace std {

template <std::floating_point T>
struct formatter<m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<T>>;

} // namespace std

namespace m03gtgtrh2smvh28qlwgm7gdl4_quaternion {

template <std::floating_point T>
quaternion_t<T>::quaternion_t():
    m_components { T(1), T(0), T(0), T(0) }
{
}

template <std::floating_point T>
quaternion_t<T>::quaternion_t(T w, T x, T y, T z):
    m_components { w, x, y, z }
{
}

template <std::floating_point T>
T& quaternion_t<T>::w() {
    return m_components[0];
}

template <std::floating_point T>
const T& quaternion_t<T>::w() const {
    return m_components[0];
}

template <std::floating_point T>
T& quaternion_t<T>::x() {
    return m_components[1];
}

template <std::floating_point T>
const T& quaternion_t<T>::x() const {
    return m_components[1];
}

template <std::floating_point T>
T& quaternion_t<T>::y() {
    return m_components[2];
}

template <std::floating_point T>
const T& quaternion_t<T>::y() const {
    return m_components[2];
}

template <std::floating_point T>
T& quaternion_t<T>::z() {
    return m_components[3];
}

template <std::floating_point T>
const T& quaternion_t<T>::z() const {
    return m_components[3];
}

template <std::floating_point T>
quaternion_t<T> quaternion_t<T>::from_euler_xyz(const vector3_t& radians) {
    for (const T angle : radians) {
        if (!std::isfinite(angle)) {
            throw std::invalid_argument(std::format("quaternion_t::from_euler_xyz: expected finite radians, got {}", radians));
        }
    }

    const T half_x = radians[0] / T(2);
    const T half_y = radians[1] / T(2);
    const T half_z = radians[2] / T(2);
    const quaternion_t around_x(std::cos(half_x), std::sin(half_x), T(0), T(0));
    const quaternion_t around_y(std::cos(half_y), T(0), std::sin(half_y), T(0));
    const quaternion_t around_z(std::cos(half_z), T(0), T(0), std::sin(half_z));
    return (around_z * around_y * around_x).unit();
}

template <std::floating_point T>
quaternion_t<T> quaternion_t<T>::from_axis_angle(const vector3_t& axis, T radians) {
    if (!std::isfinite(radians)) {
        throw std::invalid_argument(std::format("quaternion_t::from_axis_angle: expected finite radians, got {}", radians));
    }
    for (const T component : axis) {
        if (!std::isfinite(component)) {
            throw std::invalid_argument(std::format("quaternion_t::from_axis_angle: expected a finite axis, got {}", axis));
        }
    }
    const T scale = axis.chebyshev_length();
    if (scale == T(0)) {
        throw std::invalid_argument("quaternion_t::from_axis_angle: expected a nonzero axis");
    }

    // Scale before normalizing so even subnormal and maximal finite axes work.
    const vector3_t direction = (axis / scale).unit();
    const T half_angle = radians / T(2);
    const vector3_t imaginary = direction * std::sin(half_angle);
    return quaternion_t(std::cos(half_angle), imaginary[0], imaginary[1], imaginary[2]).unit();
}

template <std::floating_point T>
quaternion_t<T> quaternion_t<T>::from_matrix(const matrix3_t& matrix) {
    for (const T element : matrix) {
        if (!std::isfinite(element)) {
            throw std::invalid_argument(std::format("quaternion_t::from_matrix: expected finite elements, got {}", matrix));
        }
    }

    const T tolerance = T(64) * std::numeric_limits<T>::epsilon();
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            T product = T(0);
            for (std::size_t index = 0; index < 3; ++index) {
                product += matrix(index, row) * matrix(index, column);
            }
            const T residual = product - (row == column ? T(1) : T(0));
            if (!std::isfinite(residual) || tolerance < std::abs(residual)) {
                throw std::invalid_argument(std::format("quaternion_t::from_matrix: orthonormality residual ({}, {}) is {}, expected magnitude <= {}", row, column, residual, tolerance));
            }
        }
    }
    const T determinant = matrix(0, 0) * (matrix(1, 1) * matrix(2, 2) - matrix(1, 2) * matrix(2, 1))
        - matrix(0, 1) * (matrix(1, 0) * matrix(2, 2) - matrix(1, 2) * matrix(2, 0))
        + matrix(0, 2) * (matrix(1, 0) * matrix(2, 1) - matrix(1, 1) * matrix(2, 0));
    if (!std::isfinite(determinant) || tolerance < std::abs(determinant - T(1))) {
        throw std::invalid_argument(std::format("quaternion_t::from_matrix: determinant is {}, expected distance from 1 <= {}", determinant, tolerance));
    }

    // Select the largest squared component to avoid dividing by a small value
    // near identity or a half-turn. Cyclic indices preserve the Hamilton signs.
    const T trace = matrix(0, 0) + matrix(1, 1) + matrix(2, 2);
    quaternion_t result;
    if (T(0) < trace) {
        const T divisor = T(2) * std::sqrt(T(1) + trace);
        result.w() = divisor / T(4);
        result.x() = (matrix(2, 1) - matrix(1, 2)) / divisor;
        result.y() = (matrix(0, 2) - matrix(2, 0)) / divisor;
        result.z() = (matrix(1, 0) - matrix(0, 1)) / divisor;
    } else {
        std::size_t largest = 0;
        if (matrix(largest, largest) < matrix(1, 1)) {
            largest = 1;
        }
        if (matrix(largest, largest) < matrix(2, 2)) {
            largest = 2;
        }
        const std::size_t next = (largest + 1) % 3;
        const std::size_t last = (largest + 2) % 3;
        const T divisor = T(2) * std::sqrt(T(1) + matrix(largest, largest) - matrix(next, next) - matrix(last, last));
        result.w() = (matrix(last, next) - matrix(next, last)) / divisor;
        result.m_components[largest + 1] = divisor / T(4);
        result.m_components[next + 1] = (matrix(next, largest) + matrix(largest, next)) / divisor;
        result.m_components[last + 1] = (matrix(last, largest) + matrix(largest, last)) / divisor;
    }
    return result.unit();
}

template <std::floating_point T>
bool quaternion_t<T>::operator==(const quaternion_t& other) const {
    return m_components == other.m_components;
}

template <std::floating_point T>
quaternion_t<T> quaternion_t<T>::operator*(const quaternion_t& other) const {
    return quaternion_t(
        w() * other.w() - x() * other.x() - y() * other.y() - z() * other.z(),
        w() * other.x() + x() * other.w() + y() * other.z() - z() * other.y(),
        w() * other.y() - x() * other.z() + y() * other.w() + z() * other.x(),
        w() * other.z() + x() * other.y() - y() * other.x() + z() * other.w()
    );
}

template <std::floating_point T>
quaternion_t<T>& quaternion_t<T>::operator*=(const quaternion_t& other) {
    *this = *this * other;
    return *this;
}

template <std::floating_point T>
T quaternion_t<T>::norm() const {
    const T scale = m_components.chebyshev_length();
    if (scale == T(0) || !std::isfinite(scale)) {
        return m_components.euclidean_length();
    }
    // Scale before accumulating lengths so subnormal rounding happens only
    // when restoring the magnitude, rather than at every hypot operation.
    return (m_components / scale).euclidean_length() * scale;
}

template <std::floating_point T>
quaternion_t<T> quaternion_t<T>::unit() const {
    for (const T component : m_components) {
        if (!std::isfinite(component)) {
            throw std::invalid_argument(std::format("quaternion_t::unit: expected finite components, got {}", *this));
        }
    }
    const T scale = m_components.chebyshev_length();
    if (scale == T(0)) {
        throw std::invalid_argument("quaternion_t::unit: expected a nonzero quaternion");
    }

    // A largest component of magnitude one keeps vector normalization finite,
    // even when the original norm overflows or every component is subnormal.
    quaternion_t result;
    result.m_components = (m_components / scale).unit();
    return result;
}

template <std::floating_point T>
quaternion_t<T> quaternion_t<T>::conjugate() const {
    return quaternion_t(w(), -x(), -y(), -z());
}

template <std::floating_point T>
quaternion_t<T> quaternion_t<T>::inverse() const {
    for (const T component : m_components) {
        if (!std::isfinite(component)) {
            throw std::invalid_argument(std::format("quaternion_t::inverse: expected finite components, got {}", *this));
        }
    }
    const T scale = m_components.chebyshev_length();
    if (scale == T(0)) {
        throw std::invalid_argument("quaternion_t::inverse: expected a nonzero quaternion");
    }

    const T scaled_squared_norm = (m_components / scale).euclidean_length_squared();
    // Keep powers of two separate: neither scale² nor its reciprocal need fit
    // in T. Decompose each original component to preserve small components of
    // the inverse that an intermediate scaled division could round to zero.
    int scale_exponent;
    const T scale_fraction = std::frexp(scale, &scale_exponent);
    quaternion_t result = conjugate();
    for (T& component : result.m_components) {
        int component_exponent;
        const T component_fraction = std::frexp(component, &component_exponent);
        const T quotient = component_fraction / scale_fraction / scale_fraction / scaled_squared_norm;
        component = std::scalbn(quotient, component_exponent - 2 * scale_exponent);
        if (!std::isfinite(component)) {
            throw std::overflow_error(std::format("quaternion_t::inverse: result overflows for {}", *this));
        }
    }
    return result;
}

template <std::floating_point T>
typename quaternion_t<T>::vector3_t quaternion_t<T>::rotate(const vector3_t& vector) const {
    for (const T component : vector) {
        if (!std::isfinite(component)) {
            throw std::invalid_argument(std::format("quaternion_t::rotate: expected a finite vector, got {}", vector));
        }
    }
    const matrix3_t rotation = to_matrix();
    const vector3_t result = rotation * vector;
    for (const T component : result) {
        if (!std::isfinite(component)) {
            throw std::overflow_error(std::format("quaternion_t::rotate: result is non-finite for quaternion {} and vector {}", *this, vector));
        }
    }
    return result;
}

template <std::floating_point T>
typename quaternion_t<T>::matrix3_t quaternion_t<T>::to_matrix() const {
    const quaternion_t rotation = unit();
    const T xx = rotation.x() * rotation.x();
    const T yy = rotation.y() * rotation.y();
    const T zz = rotation.z() * rotation.z();
    const T xy = rotation.x() * rotation.y();
    const T xz = rotation.x() * rotation.z();
    const T yz = rotation.y() * rotation.z();
    const T wx = rotation.w() * rotation.x();
    const T wy = rotation.w() * rotation.y();
    const T wz = rotation.w() * rotation.z();
    return matrix3_t {
        T(1) - T(2) * (yy + zz),
        T(2) * (xy - wz),
        T(2) * (xz + wy),
        T(2) * (xy + wz),
        T(1) - T(2) * (xx + zz),
        T(2) * (yz - wx),
        T(2) * (xz - wy),
        T(2) * (yz + wx),
        T(1) - T(2) * (xx + yy)
    };
}

} // namespace m03gtgtrh2smvh28qlwgm7gdl4_quaternion

namespace std {

template <std::floating_point T>
struct formatter<m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<T>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid quaternion_t format specifier");
        }
        return it;
    }

    auto format(const m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<T>& quaternion, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ w: {}", quaternion.w());
        out = std::format_to(out, ", x: {}", quaternion.x());
        out = std::format_to(out, ", y: {}", quaternion.y());
        out = std::format_to(out, ", z: {}", quaternion.z());
        out = std::format_to(out, " }}");
        return out;
    }
};

} // namespace std

#endif // M03GTGTRH2SMVH28QLWGM7GDL4_QUATERNION_API_H
