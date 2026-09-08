#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_CAMERA_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_CAMERA_H

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03glv28yaiwc5hbnvz43r14zr_matrix/api.h>
# include <m03gtgtrh2smvh28qlwgm7gdl4_quaternion/api.h>
# include <m03gintxczohr63y44o77b4pyj_hyperrectangle/api.h>

# include <format>
# include <variant>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

using view_rect_t = m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>;
using projection_bounds_t = m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<float, 2>;

/**
 * @brief Describes perspective projection with a vertical field of view in radians.
 *
 * Requires finite parameters, 0 < field of view < pi, and 0 < near < far.
 * The camera derives aspect from its original view rectangle.
 */
class perspective_t {
public:
    perspective_t(float vertical_fov, float near_distance, float far_distance);

    float vertical_fov() const;
    float near_distance() const;
    float far_distance() const;

private:
    float m_vertical_fov;
    float m_near_distance;
    float m_far_distance;
};

/**
 * @brief Describes orthographic bounds in camera-local X/Y and forward near/far distances.
 *
 * Requires positive-area bounds and finite distances with 0 <= near < far.
 */
class orthographic_t {
public:
    orthographic_t(const projection_bounds_t& bounds, float near_distance, float far_distance);

    const projection_bounds_t& bounds() const;
    float near_distance() const;
    float far_distance() const;

private:
    projection_bounds_t m_bounds;
    float m_near_distance;
    float m_far_distance;
};

using projection_t = std::variant<perspective_t, orthographic_t>;

/**
 * @brief Owns a 3D viewing pose, projection, and one framebuffer rendering rectangle.
 *
 * Local +X is right, +Y is up, and -Z is forward. The quaternion maps local
 * directions into world space; the view transform is the inverse pose. Near/far
 * map to NDC Z -1/+1. Viewport mapping handles the top-left framebuffer origin.
 *
 * The half-open view rectangle supplies both viewport mapping and pixel bounds.
 * Empty rectangles are valid. Framebuffer intersections restrict writes without
 * changing mapping or perspective aspect. Position must be finite when deriving
 * matrices; projection descriptions validate their own inputs.
 *
 * Rotation setters store a normalized copy, rejecting zero or non-finite
 * quaternions. Euler input uses finite radians about fixed X, then Y, then Z
 * axes. Failed rotation updates preserve the previous orientation.
 */
class camera_t {
public:
    camera_t(const view_rect_t& view_rect, projection_t projection);

    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& position();
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& position() const;

    void rotation(const m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>& rotation);
    void rotation(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& euler_xyz);
    const m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>& rotation() const;

    /**
     * @brief Replaces position and rotation to look from eye toward target with the supplied world up direction.
     *
     * Rejects non-finite inputs, coincident eye/target, and zero or parallel up
     * directions. Failed updates preserve the previous pose.
     */
    void look_at(
        const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& eye,
        const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& target,
        const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& up
    );

    projection_t& projection();
    const projection_t& projection() const;

    view_rect_t& view_rect();
    const view_rect_t& view_rect() const;

    m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 4, 4> world_to_view() const;

    /**
     * @brief Derives projection times inverse pose, requiring a nonempty view rectangle and representable finite matrix coefficients.
     */
    m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 4, 4> world_to_clip() const;

    /**
     * @brief Projects a world position to unclipped framebuffer X/Y and depth (NDC Z + 1) / 2.
     *
     * Requires a finite input, nonempty view rectangle, positive clip W, and
     * representable finite results. This operation does not perform visibility tests.
     */
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> world_to_framebuffer(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>& world_position) const;

private:
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> m_position;
    m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float> m_rotation;
    projection_t m_projection;
    view_rect_t m_view_rect;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::perspective_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::orthographic_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::camera_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::perspective_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid perspective_t format specifier");
        }
        return it;
    }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::perspective_t& projection, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ vertical_fov: {}", projection.vertical_fov());
        out = std::format_to(out, ", near: {}", projection.near_distance());
        out = std::format_to(out, ", far: {} }}", projection.far_distance());
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::orthographic_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid orthographic_t format specifier");
        }
        return it;
    }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::orthographic_t& projection, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ bounds: {}", projection.bounds());
        out = std::format_to(out, ", near: {}", projection.near_distance());
        out = std::format_to(out, ", far: {} }}", projection.far_distance());
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::camera_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid camera_t format specifier");
        }
        return it;
    }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::camera_t& camera, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ position: {}", camera.position());
        out = std::format_to(out, ", rotation: {}", camera.rotation());
        std::visit([&](const auto& projection) { out = std::format_to(out, ", projection: {}", projection); }, camera.projection());
        out = std::format_to(out, ", view_rect: {} }}", camera.view_rect());
        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_CAMERA_H
