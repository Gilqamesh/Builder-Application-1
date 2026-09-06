#include <m03gtgtrh2smvh28qlwgm7gdl4_quaternion/api.h>
#include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
#include <m03glv28yaiwc5hbnvz43r14zr_matrix/api.h>
#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <format>
#include <functional>
#include <limits>
#include <numbers>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>

namespace quaternion_api = m03gtgtrh2smvh28qlwgm7gdl4_quaternion;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;

int main() {
    return test::run([] {
        const auto validate = []<std::floating_point T>() {
            using quaternion_t = quaternion_api::quaternion_t<T>;
            using vector3_t = typename quaternion_t::vector3_t;
            using matrix3_t = typename quaternion_t::matrix3_t;
            static_assert(std::same_as<vector3_t, m03ginwy24ng8o487c4beoms6l_vector::vector_t<T, 3>>);
            static_assert(std::same_as<matrix3_t, m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<T, 3, 3>>);
            static_assert(std::same_as<decltype(std::declval<quaternion_t&>().w()), T&>);
            static_assert(std::same_as<decltype(std::declval<const quaternion_t&>().w()), const T&>);
            static_assert(std::same_as<decltype(std::declval<quaternion_t&>().x()), T&>);
            static_assert(std::same_as<decltype(std::declval<const quaternion_t&>().x()), const T&>);
            static_assert(std::same_as<decltype(std::declval<quaternion_t&>().y()), T&>);
            static_assert(std::same_as<decltype(std::declval<const quaternion_t&>().y()), const T&>);
            static_assert(std::same_as<decltype(std::declval<quaternion_t&>().z()), T&>);
            static_assert(std::same_as<decltype(std::declval<const quaternion_t&>().z()), const T&>);

            const T epsilon = std::numeric_limits<T>::epsilon();
            const T pi = std::numbers::pi_v<T>;
            const T quarter_turn = pi / T(2);
            const T maximum = std::numeric_limits<T>::max();
            const T minimum = std::numeric_limits<T>::min();
            const T subnormal = std::numeric_limits<T>::denorm_min();
            const T infinity = std::numeric_limits<T>::infinity();
            const T nan = std::numeric_limits<T>::quiet_NaN();
            const auto close = [epsilon](T actual, T expected, const std::source_location& location = std::source_location::current()) {
                test::expect_at(location, [epsilon](T left, T right) {
                    return std::isfinite(left) && std::abs(left - right) <= T(32) * epsilon;
                }, actual, expected);
            };
            const auto close_vector = [&close](const vector3_t& actual, const vector3_t& expected, const std::source_location& location = std::source_location::current()) {
                for (std::size_t index = 0; index < 3; ++index) {
                    close(actual[index], expected[index], location);
                }
            };
            const auto close_matrix = [&close](const matrix3_t& actual, const matrix3_t& expected, const std::source_location& location = std::source_location::current()) {
                for (std::size_t row = 0; row < 3; ++row) {
                    for (std::size_t column = 0; column < 3; ++column) {
                        close(actual(row, column), expected(row, column), location);
                    }
                }
            };
            const auto close_quaternion = [&close](const quaternion_t& actual, const quaternion_t& expected, const std::source_location& location = std::source_location::current()) {
                close(actual.w(), expected.w(), location);
                close(actual.x(), expected.x(), location);
                close(actual.y(), expected.y(), location);
                close(actual.z(), expected.z(), location);
            };

            const quaternion_t identity;
            const vector3_t along_x { T(1), T(0), T(0) };
            const vector3_t along_y { T(0), T(1), T(0) };
            const vector3_t along_z { T(0), T(0), T(1) };
            matrix3_t identity_matrix(T(0));
            for (std::size_t index = 0; index < 3; ++index) {
                identity_matrix(index, index) = T(1);
            }

            // Components retain their supplied values and all four accessors alias storage.
            test::expect(std::equal_to<>(), identity, quaternion_t(T(1), T(0), T(0), T(0)));
            quaternion_t components(T(2), T(3), T(4), T(5));
            T& real = components.w();
            T& imaginary_x = components.x();
            T& imaginary_y = components.y();
            T& imaginary_z = components.z();
            real = T(-2);
            imaginary_x = T(6);
            imaginary_y = T(-7);
            imaginary_z = T(8);
            const quaternion_t& constant_components = components;
            test::expect(std::identity(), &real == &constant_components.w());
            test::expect(std::identity(), &imaginary_x == &constant_components.x());
            test::expect(std::identity(), &imaginary_y == &constant_components.y());
            test::expect(std::identity(), &imaginary_z == &constant_components.z());
            test::expect(std::equal_to<>(), components, quaternion_t(T(-2), T(6), T(-7), T(8)));
            test::expect(std::not_equal_to<>(), components, quaternion_t(T(-2), T(6), T(-7), T(9)));
            test::expect(std::equal_to<>(), std::format("{}", identity), std::string("{ w: 1, x: 0, y: 0, z: 0 }"));
            test::expect(std::equal_to<>(), std::format("{}", components), std::string("{ w: -2, x: 6, y: -7, z: 8 }"));
            test::expect_throws<std::format_error>([&] {
                static_cast<void>(std::vformat("{:x}", std::make_format_args(components)));
            });
            test::expect(std::equal_to<>(), identity.norm(), T(1));
            test::expect(std::equal_to<>(), identity.to_matrix(), identity_matrix);
            const vector3_t mixed_magnitudes { maximum, minimum, subnormal };
            test::expect(std::equal_to<>(), identity.rotate(mixed_magnitudes), mixed_magnitudes);

            // Exact Hamilton basis identities and unnormalized general products.
            const quaternion_t imaginary_i(T(0), T(1), T(0), T(0));
            const quaternion_t imaginary_j(T(0), T(0), T(1), T(0));
            const quaternion_t imaginary_k(T(0), T(0), T(0), T(1));
            test::expect(std::equal_to<>(), imaginary_i * imaginary_j, imaginary_k);
            test::expect(std::equal_to<>(), imaginary_j * imaginary_i, imaginary_k.conjugate());
            test::expect(std::equal_to<>(), imaginary_j * imaginary_k, imaginary_i);
            test::expect(std::equal_to<>(), imaginary_k * imaginary_i, imaginary_j);
            const quaternion_t general(T(1), T(2), T(3), T(4));
            test::expect(std::equal_to<>(), general * identity, general);
            test::expect(std::equal_to<>(), identity * general, general);
            test::expect(std::equal_to<>(), general * quaternion_t(T(5), T(6), T(7), T(8)), quaternion_t(T(-60), T(12), T(30), T(24)));
            auto squared = general;
            test::expect(std::identity(), &(squared *= squared) == &squared);
            test::expect(std::equal_to<>(), squared, quaternion_t(T(-28), T(4), T(6), T(8)));

            // Positive quarter-turns are active and right-handed; order is observable.
            const quaternion_t around_x = quaternion_t::from_axis_angle(along_x, quarter_turn);
            const quaternion_t around_y = quaternion_t::from_axis_angle(along_y, quarter_turn);
            const quaternion_t around_z = quaternion_t::from_axis_angle(along_z, quarter_turn);
            const T half_root = std::sqrt(T(0.5));
            close_quaternion(around_x, quaternion_t(half_root, half_root, T(0), T(0)));
            close_quaternion(around_y, quaternion_t(half_root, T(0), half_root, T(0)));
            close_quaternion(around_z, quaternion_t(half_root, T(0), T(0), half_root));
            close_vector(around_x.rotate(along_y), along_z);
            close_vector(around_y.rotate(along_z), along_x);
            close_vector(around_z.rotate(along_x), along_y);
            close_vector(quaternion_t::from_axis_angle(along_z, -quarter_turn).rotate(along_x), -along_y);
            close_vector((around_y * around_x).rotate(along_y), along_x);
            close_vector((around_x * around_y).rotate(along_y), along_z);
            auto composed = around_y;
            test::expect(std::identity(), &(composed *= around_x) == &composed);
            close_vector(composed.rotate(along_y), along_x);
            auto half_turn = around_z;
            half_turn *= half_turn;
            close_vector(half_turn.rotate(along_x), -along_x);

            // Independently form Rz * Ry * Rx without quaternion conversions.
            const std::array<vector3_t, 5> euler_inputs {
                vector3_t { T(0), T(0), T(0) },
                vector3_t { T(0.31), T(-0.72), T(1.13) },
                vector3_t { T(-1.4), T(0.29), T(-2.1) },
                vector3_t { quarter_turn, quarter_turn, T(-0.63) },
                vector3_t { pi, -quarter_turn, pi }
            };
            const vector3_t probe { T(0.25), T(-0.75), T(1.5) };
            for (const vector3_t& angles : euler_inputs) {
                matrix3_t rotate_x = identity_matrix;
                rotate_x(1, 1) = std::cos(angles[0]);
                rotate_x(1, 2) = -std::sin(angles[0]);
                rotate_x(2, 1) = std::sin(angles[0]);
                rotate_x(2, 2) = std::cos(angles[0]);
                matrix3_t rotate_y = identity_matrix;
                rotate_y(0, 0) = std::cos(angles[1]);
                rotate_y(0, 2) = std::sin(angles[1]);
                rotate_y(2, 0) = -std::sin(angles[1]);
                rotate_y(2, 2) = std::cos(angles[1]);
                matrix3_t rotate_z = identity_matrix;
                rotate_z(0, 0) = std::cos(angles[2]);
                rotate_z(0, 1) = -std::sin(angles[2]);
                rotate_z(1, 0) = std::sin(angles[2]);
                rotate_z(1, 1) = std::cos(angles[2]);
                const matrix3_t expected = rotate_z * rotate_y * rotate_x;
                const quaternion_t euler = quaternion_t::from_euler_xyz(angles);
                close(euler.norm(), T(1));
                close_matrix(euler.to_matrix(), expected);
                close_vector(euler.rotate(probe), expected * probe);
                close_matrix(quaternion_t::from_matrix(expected).to_matrix(), expected);
                const quaternion_t opposite(-euler.w(), -euler.x(), -euler.y(), -euler.z());
                test::expect(std::not_equal_to<>(), euler, opposite);
                close_matrix(opposite.to_matrix(), expected);
                close_vector(opposite.rotate(probe), euler.rotate(probe));
                const quaternion_t scaled(T(7) * euler.w(), T(7) * euler.x(), T(7) * euler.y(), T(7) * euler.z());
                close_vector(scaled.rotate(probe), expected * probe);
                close_matrix(scaled.to_matrix(), expected);
            }
            close_vector((around_z * around_y * around_x).rotate(probe), around_z.rotate(around_y.rotate(around_x.rotate(probe))));
            close(quaternion_t::from_euler_xyz({ maximum, -maximum, maximum }).norm(), T(1));
            close(quaternion_t::from_axis_angle(along_x, maximum).norm(), T(1));

            // Conjugation and inverse are general quaternion operations.
            test::expect(std::equal_to<>(), general.conjugate(), quaternion_t(T(1), T(-2), T(-3), T(-4)));
            test::expect(std::equal_to<>(), general.conjugate().conjugate(), general);
            test::expect(std::equal_to<>(), general * general.conjugate(), quaternion_t(T(30), T(0), T(0), T(0)));
            const quaternion_t inverted = general.inverse();
            close_quaternion(inverted, quaternion_t(T(1) / T(30), T(-2) / T(30), T(-3) / T(30), T(-4) / T(30)));
            close_quaternion(general * inverted, identity);
            close_quaternion(inverted * general, identity);
            close_vector(inverted.rotate(general.rotate(probe)), probe);
            close_quaternion(around_x.inverse(), around_x.conjugate());
            close(general.norm(), std::sqrt(T(30)));
            close(general.unit().norm(), T(1));

            // Scaling must not turn finite, nonzero normalization inputs into zero or infinity.
            const std::array<T, 5> scales { maximum, maximum / T(4), minimum, minimum / T(4), subnormal };
            for (const T scale : scales) {
                const quaternion_t extreme(scale, -scale, scale, -scale);
                close_quaternion(extreme.unit(), quaternion_t(T(0.5), T(-0.5), T(0.5), T(-0.5)));
                close_matrix(extreme.to_matrix(), extreme.unit().to_matrix());
                close_vector(extreme.rotate(probe), extreme.unit().rotate(probe));
                const quaternion_t scaled_axis = quaternion_t::from_axis_angle({ scale, -scale, scale }, T(0.81));
                close(scaled_axis.norm(), T(1));
                close_matrix(scaled_axis.to_matrix(), quaternion_t::from_axis_angle({ T(1), T(-1), T(1) }, T(0.81)).to_matrix());
                if (scale != subnormal) {
                    const quaternion_t extreme_inverse = extreme.inverse();
                    const T expected = (T(0.25) / scale);
                    close(extreme_inverse.w() / expected, T(1));
                    close(extreme_inverse.x() / expected, T(1));
                    close(extreme_inverse.y() / expected, T(-1));
                    close(extreme_inverse.z() / expected, T(1));
                    close_quaternion(extreme * extreme_inverse, identity);
                }
            }
            close_quaternion(quaternion_t(maximum, subnormal, -minimum, T(0)).unit(), identity);
            close(quaternion_t(maximum / T(4), maximum / T(4), maximum / T(4), maximum / T(4)).norm() / maximum, T(0.5));
            test::expect(std::identity(), std::isinf(quaternion_t(maximum, maximum, maximum, maximum).norm()));
            test::expect(std::equal_to<>(), quaternion_t(subnormal, T(0), T(0), T(0)).norm(), subnormal);
            test::expect(std::equal_to<>(), quaternion_t(subnormal, subnormal, subnormal, subnormal).norm(), T(2) * subnormal);
            test::expect(std::equal_to<>(), quaternion_t(T(0), T(0), T(0), T(0)).norm(), T(0));
            close(quaternion_t(minimum, T(0), T(0), T(0)).inverse().w() * minimum, T(1));
            close(quaternion_t(maximum, T(0), T(0), T(0)).inverse().w() * maximum, T(1));
            // Small inverse components survive even when the squared norm underflows.
            const T tiny_scale = std::sqrt(minimum) / T(4);
            const quaternion_t unequal(tiny_scale, subnormal, T(0), T(0));
            const T expected_small_inverse = -(subnormal / tiny_scale) / tiny_scale;
            close(unequal.inverse().x() / expected_small_inverse, T(1));
            const quaternion_t rounding_edge(T(0.5), T(0.5), T(0.5), subnormal);
            test::expect(std::equal_to<>(), rounding_edge.inverse().z(), -subnormal);
            test::expect_throws<std::overflow_error>([&] { quaternion_t(subnormal, T(0), T(0), T(0)).inverse(); });
            test::expect_throws<std::overflow_error>([&] { quaternion_t(subnormal, subnormal, subnormal, subnormal).inverse(); });

            // Identity and each dominant imaginary component exercise conversion branches.
            const std::array<vector3_t, 6> axes {
                along_x,
                along_y,
                along_z,
                vector3_t { T(1), T(-2), T(3) },
                vector3_t { T(-4), T(2), T(1) },
                vector3_t { T(1), T(5), T(-2) }
            };
            const std::array<T, 6> angles { T(0), T(8) * epsilon, pi, pi - T(8) * epsilon, pi + T(8) * epsilon, -pi };
            for (const vector3_t& axis : axes) {
                const vector3_t direction = axis.unit();
                for (const T angle : angles) {
                    // Rodrigues' formula is an independent matrix oracle.
                    const T cosine = std::cos(angle);
                    const T sine = std::sin(angle);
                    matrix3_t expected(T(0));
                    for (std::size_t row = 0; row < 3; ++row) {
                        for (std::size_t column = 0; column < 3; ++column) {
                            expected(row, column) = (T(1) - cosine) * direction[row] * direction[column];
                            if (row == column) {
                                expected(row, column) += cosine;
                            }
                        }
                    }
                    expected(0, 1) -= sine * direction[2];
                    expected(0, 2) += sine * direction[1];
                    expected(1, 0) += sine * direction[2];
                    expected(1, 2) -= sine * direction[0];
                    expected(2, 0) -= sine * direction[1];
                    expected(2, 1) += sine * direction[0];
                    const quaternion_t recovered = quaternion_t::from_matrix(expected);
                    close(recovered.norm(), T(1));
                    close_matrix(recovered.to_matrix(), expected);
                    close_vector(recovered.rotate(probe), expected * probe);
                    close_matrix(quaternion_t::from_axis_angle(axis, angle).to_matrix(), expected);
                }
            }

            // Invalid raw components remain storable; checked rotation operations reject them.
            const quaternion_t zero(T(0), T(0), T(0), T(0));
            const auto reject_rotation = [&](const quaternion_t& invalid) {
                test::expect_throws<std::invalid_argument>([&] { invalid.unit(); });
                test::expect_throws<std::invalid_argument>([&] { invalid.inverse(); });
                test::expect_throws<std::invalid_argument>([&] { invalid.to_matrix(); });
                test::expect_throws<std::invalid_argument>([&] { invalid.rotate(probe); });
                test::expect_throws<std::invalid_argument>([&] { invalid.rotate(vector3_t(T(0))); });
            };
            reject_rotation(zero);
            for (const T invalid : { infinity, -infinity, nan }) {
                reject_rotation(quaternion_t(invalid, T(0), T(0), T(0)));
                reject_rotation(quaternion_t(T(1), invalid, T(0), T(0)));
                reject_rotation(quaternion_t(T(1), T(0), invalid, T(0)));
                reject_rotation(quaternion_t(T(1), T(0), T(0), invalid));
                for (std::size_t index = 0; index < 3; ++index) {
                    vector3_t invalid_vector = along_x;
                    invalid_vector[index] = invalid;
                    test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_euler_xyz(invalid_vector); });
                    test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_axis_angle(invalid_vector, T(0)); });
                    test::expect_throws<std::invalid_argument>([&] { identity.rotate(invalid_vector); });
                }
                test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_axis_angle(along_x, invalid); });
                for (std::size_t row = 0; row < 3; ++row) {
                    for (std::size_t column = 0; column < 3; ++column) {
                        matrix3_t invalid_matrix = identity_matrix;
                        invalid_matrix(row, column) = invalid;
                        test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_matrix(invalid_matrix); });
                    }
                }
            }
            auto mutable_invalid = identity;
            mutable_invalid.y() = infinity;
            test::expect(std::equal_to<>(), mutable_invalid.y(), infinity);
            reject_rotation(mutable_invalid);
            const quaternion_t nan_components(nan, T(0), T(0), T(0));
            test::expect(std::not_equal_to<>(), nan_components, nan_components);
            const quaternion_t infinite_components(infinity, T(0), T(0), T(0));
            test::expect(std::equal_to<>(), infinite_components, infinite_components);
            test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_axis_angle(vector3_t(T(0)), T(0)); });
            test::expect_throws<std::overflow_error>([&] {
                quaternion_t::from_axis_angle(along_z, pi / T(4)).rotate({ maximum, maximum, T(0) });
            });
            test::expect(std::equal_to<>(), around_x.rotate(vector3_t(T(0))), vector3_t(T(0)));

            // Finite scaled, sheared, reflected, singular, and huge matrices are invalid.
            for (const T scale : { T(0), T(0.5), T(2), maximum }) {
                test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_matrix(identity_matrix * scale); });
            }
            matrix3_t sheared = identity_matrix;
            sheared(0, 1) = T(0.1);
            test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_matrix(sheared); });
            matrix3_t reflected = identity_matrix;
            reflected(0, 0) = T(-1);
            test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_matrix(reflected); });
            test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_matrix(matrix3_t(maximum)); });

            // Shear gives an exactly represented off-diagonal residual and determinant 1.
            const T tolerance = T(64) * epsilon;
            const T just_inside = std::nextafter(tolerance, T(0));
            const T just_outside = std::nextafter(tolerance, infinity);
            for (const T sign : { T(-1), T(1) }) {
                for (const T accepted : { just_inside, tolerance }) {
                    matrix3_t boundary = identity_matrix;
                    boundary(0, 1) = sign * accepted;
                    close(quaternion_t::from_matrix(boundary).norm(), T(1));
                }
                matrix3_t rejected = identity_matrix;
                rejected(0, 1) = sign * just_outside;
                test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_matrix(rejected); });
            }
            // Each column passes its length test, but the determinant exceeds tolerance.
            for (const T sign : { T(-1), T(1) }) {
                const T determinant_scale = T(1) + sign * T(24) * epsilon;
                test::expect(std::less_equal<>(), std::abs(determinant_scale * determinant_scale - T(1)), tolerance);
                test::expect(std::less<>(), tolerance, std::abs(determinant_scale * determinant_scale * determinant_scale - T(1)));
                test::expect_throws<std::invalid_argument>([&] {
                    quaternion_t::from_matrix(identity_matrix * determinant_scale);
                });

                // Adjacent diagonal values straddle the determinant tolerance
                // while every orthonormality residual remains within it.
                matrix3_t boundary = identity_matrix;
                boundary(0, 0) = T(1) + sign * T(24) * epsilon;
                boundary(1, 1) = boundary(0, 0);
                const T last_diagonal = T(1) + sign * T(16) * epsilon;
                boundary(2, 2) = std::nextafter(last_diagonal, T(1));
                const T inside_determinant = boundary(0, 0) * (boundary(1, 1) * boundary(2, 2));
                test::expect(std::less_equal<>(), std::abs(inside_determinant - T(1)), tolerance);
                close(quaternion_t::from_matrix(boundary).norm(), T(1));
                boundary(2, 2) = std::nextafter(last_diagonal, sign < T(0) ? T(0) : infinity);
                const T outside_determinant = boundary(0, 0) * (boundary(1, 1) * boundary(2, 2));
                test::expect(std::less<>(), tolerance, std::abs(outside_determinant - T(1)));
                for (std::size_t index = 0; index < 3; ++index) {
                    test::expect(std::less_equal<>(), std::abs(boundary(index, index) * boundary(index, index) - T(1)), tolerance);
                }
                test::expect_throws<std::invalid_argument>([&] { quaternion_t::from_matrix(boundary); });
            }
        };
        validate.template operator()<float>();
        validate.template operator()<double>();
    });
}
