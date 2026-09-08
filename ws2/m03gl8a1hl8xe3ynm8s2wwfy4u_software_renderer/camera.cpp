#include "camera.h"

#include <m03glv28yaiwc5hbnvz43r14zr_matrix/api.h>
#include <m03gtgtrh2smvh28qlwgm7gdl4_quaternion/api.h>
#include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

perspective_t::perspective_t(float vertical_fov, float near_distance, float far_distance):
    m_vertical_fov(vertical_fov),
    m_near_distance(near_distance),
    m_far_distance(far_distance)
{
    if (!std::isfinite(vertical_fov) || !(0 < vertical_fov && double(vertical_fov) < std::numbers::pi)) {
        throw std::invalid_argument("perspective_t requires a finite vertical field of view in (0, pi) radians");
    }
    if (!std::isfinite(near_distance) || !std::isfinite(far_distance) || !(0 < near_distance && near_distance < far_distance)) {
        throw std::invalid_argument("perspective_t requires finite distances with 0 < near < far");
    }
}

float perspective_t::vertical_fov() const { return m_vertical_fov; }
float perspective_t::near_distance() const { return m_near_distance; }
float perspective_t::far_distance() const { return m_far_distance; }

orthographic_t::orthographic_t(const projection_bounds_t& bounds, float near_distance, float far_distance):
    m_bounds(bounds),
    m_near_distance(near_distance),
    m_far_distance(far_distance)
{
    if (bounds.is_empty()) {
        throw std::invalid_argument("orthographic_t requires positive-area projection bounds");
    }
    if (!std::isfinite(near_distance) || !std::isfinite(far_distance) || !(0 <= near_distance && near_distance < far_distance)) {
        throw std::invalid_argument("orthographic_t requires finite distances with 0 <= near < far");
    }
}

const projection_bounds_t& orthographic_t::bounds() const { return m_bounds; }
float orthographic_t::near_distance() const { return m_near_distance; }
float orthographic_t::far_distance() const { return m_far_distance; }

camera_t::camera_t(const view_rect_t& view_rect, projection_t projection):
    m_position(0.0F),
    m_rotation(),
    m_projection(std::move(projection)),
    m_view_rect(view_rect)
{
}

m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& camera_t::position() { return m_position; }
const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& camera_t::position() const { return m_position; }

void camera_t::rotation(const m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>& rotation) {
    m_rotation = rotation.unit();
}
void camera_t::rotation(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& euler_xyz) {
    m_rotation = m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>::from_euler_xyz(euler_xyz);
}
const m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>& camera_t::rotation() const { return m_rotation; }

void camera_t::look_at(
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& eye,
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& target,
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& up
) {
    const auto finite = [](float component) { return std::isfinite(component); };
    if (!std::ranges::all_of(eye, finite) || !std::ranges::all_of(target, finite) || !std::ranges::all_of(up, finite)) {
        throw std::invalid_argument("camera_t::look_at requires finite eye, target, and up directions");
    }
    // Work in double so subtraction and normalization cover all finite float inputs.
    const auto normalize = [](std::array<double, 3> direction) {
        const double length = std::hypot(direction[0], direction[1], direction[2]);
        if (length == 0) {
            throw std::invalid_argument("camera_t::look_at requires distinct eye/target and a nonzero, nonparallel up direction");
        }
        for (auto& component : direction) { component /= length; }
        return direction;
    };
    const std::array displacement {double(eye[0]) - target[0], double(eye[1]) - target[1], double(eye[2]) - target[2]};
    const auto back = normalize(displacement);
    // Cross before normalizing the displacement so exact parallel directions
    // do not acquire a spurious perpendicular component from unit-vector rounding.
    const auto right = normalize({
        double(up[1]) * displacement[2] - double(up[2]) * displacement[1],
        double(up[2]) * displacement[0] - double(up[0]) * displacement[2],
        double(up[0]) * displacement[1] - double(up[1]) * displacement[0]
    });
    const std::array corrected_up {
        back[1] * right[2] - back[2] * right[1],
        back[2] * right[0] - back[0] * right[2],
        back[0] * right[1] - back[1] * right[0]
    };
    // These columns map the camera-local basis into world space.
    const m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 3, 3> basis {
        float(right[0]),
        float(corrected_up[0]),
        float(back[0]),
        float(right[1]),
        float(corrected_up[1]),
        float(back[1]),
        float(right[2]),
        float(corrected_up[2]),
        float(back[2])
    };
    const auto rotation = m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>::from_matrix(basis);
    m_position = eye;
    m_rotation = rotation;
}

projection_t& camera_t::projection() { return m_projection; }
const projection_t& camera_t::projection() const { return m_projection; }
view_rect_t& camera_t::view_rect() { return m_view_rect; }
const view_rect_t& camera_t::view_rect() const { return m_view_rect; }

m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 4, 4> camera_t::world_to_view() const {
    if (!std::ranges::all_of(m_position, [](float component) { return std::isfinite(component); })) {
        throw std::invalid_argument("camera_t::world_to_view requires a finite position");
    }
    const auto rotation = m_rotation.to_matrix();
    m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 4, 4> view(0.0F);
    for (std::size_t row = 0; row < 3; ++row) {
        double offset = 0;
        for (std::size_t column = 0; column < 3; ++column) {
            view(row, column) = rotation(column, row);
            offset -= double(rotation(column, row)) * m_position[column];
        }
        view(row, 3) = float(offset);
    }
    view(3, 3) = 1;
    if (!std::ranges::all_of(view, [](float component) { return std::isfinite(component); })) {
        throw std::out_of_range("camera_t::world_to_view matrix cannot be represented as finite floats");
    }
    return view;
}

m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 4, 4> camera_t::world_to_clip() const {
    if (m_view_rect.is_empty()) {
        throw std::invalid_argument("camera_t::world_to_clip requires a nonempty view rectangle");
    }
    m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 4, 4> projection(0.0F);
    std::visit([&](const auto& description) {
        const double near = description.near_distance(), far = description.far_distance();
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(description)>, perspective_t>) {
            const double width = std::int64_t(m_view_rect[0][1]) - std::int64_t(m_view_rect[0][0]);
            const double height = std::int64_t(m_view_rect[1][1]) - std::int64_t(m_view_rect[1][0]);
            const double vertical_scale = 1.0 / std::tan(double(description.vertical_fov()) / 2);
            projection(0, 0) = float(vertical_scale * height / width);
            projection(1, 1) = float(vertical_scale);
            projection(2, 2) = float(-(far + near) / (far - near));
            projection(2, 3) = float(-2 * far * near / (far - near));
            projection(3, 2) = -1;
        } else {
            const auto& bounds = description.bounds();
            const double left = bounds[0][0], right = bounds[0][1];
            const double bottom = bounds[1][0], top = bounds[1][1];
            projection(0, 0) = float(2 / (right - left));
            projection(1, 1) = float(2 / (top - bottom));
            projection(2, 2) = float(-2 / (far - near));
            projection(0, 3) = float(-(right + left) / (right - left));
            projection(1, 3) = float(-(top + bottom) / (top - bottom));
            projection(2, 3) = float(-(far + near) / (far - near));
            projection(3, 3) = 1;
        }
    }, m_projection);
    const auto view = world_to_view();
    m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 4, 4> result;
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            double coefficient = 0;
            for (std::size_t k = 0; k < 4; ++k) { coefficient += double(projection(row, k)) * view(k, column); }
            result(row, column) = float(coefficient);
        }
    }
    if (!std::ranges::all_of(result, [](float component) { return std::isfinite(component); })) {
        throw std::out_of_range("camera_t::world_to_clip matrix cannot be represented as finite floats");
    }
    return result;
}

m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> camera_t::world_to_framebuffer(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& world_position) const {
    if (!std::ranges::all_of(world_position, [](float component) { return std::isfinite(component); })) {
        throw std::invalid_argument("camera_t::world_to_framebuffer requires a finite world position");
    }
    const auto matrix = world_to_clip();
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4> point {world_position[0], world_position[1], world_position[2], 1};
    std::array<double, 4> clip {};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) { clip[row] += double(matrix(row, column)) * point[column]; }
    }
    if (!(0 < clip[3])) {
        throw std::invalid_argument("camera_t::world_to_framebuffer requires positive clip W");
    }
    const double width = std::int64_t(m_view_rect[0][1]) - std::int64_t(m_view_rect[0][0]);
    const double height = std::int64_t(m_view_rect[1][1]) - std::int64_t(m_view_rect[1][0]);
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> result {
        float(double(m_view_rect[0][0]) + (clip[0] / clip[3] + 1) * width / 2),
        float(double(m_view_rect[1][0]) + (1 - clip[1] / clip[3]) * height / 2),
        float((clip[2] / clip[3] + 1) / 2)
    };
    if (!std::ranges::all_of(result, [](float component) { return std::isfinite(component); })) {
        throw std::out_of_range("camera_t::world_to_framebuffer result cannot be represented as finite floats");
    }
    return result;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
