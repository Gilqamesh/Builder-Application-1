#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_SOFTWARE_RENDERER_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_SOFTWARE_RENDERER_H

# include "camera.h"
# include "framebuffer.h"
# include "helpers.h"
# include "profiling_metrics.h"
# include "render_item.h"

# include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

# include <cstddef>
# include <format>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;

/**
 * @brief Renders camera-relative render items into a borrowed CPU framebuffer.
 *
 * Mesh streams must match consumed vertex input locations and types; selected
 * indices must fit signed 32-bit vertex indices. Fragment inputs must be floating-point
 * scalars or vectors, with perspective-correct interpolation of primitive-local values.
 *
 * Vertex invocations receive render_item_t's object-to-world transform and a
 * world-to-clip matrix derived from camera_t's pose and projection. The camera's
 * view rectangle supplies both viewport mapping and half-open pixel bounds.
 * Partial framebuffer overlap restricts writes without changing that mapping.
 *
 * Shader positions must be finite homogeneous clip coordinates; X, Y and Z are
 * clipped to [-W,W]. Surviving zero-W vertices make their primitive empty; positive W
 * requires a reciprocal representable as float. Non-finite positions and unsupported W
 * are rejected.
 *
 * Triangle X/Y positions are projected and rounded once to a 1/256-pixel grid
 * (ties toward the greater coordinate) for coverage and interpolation. Nonzero winding
 * with top/left inclusion shades each covered pixel-center sample once per original
 * triangle, including degenerate boundaries. Matching shared boundaries with interiors
 * on opposite sides have complementary sample ownership; overlapping primitives shade
 * independently. Facing is constant per original triangle; simple snapped polygons
 * are front-facing for CCW NDC winding under the default front-face selection.
 *
 * Points cover integer offsets dx*dx+dy*dy <= 9 around the floored projected position.
 * Lines include both floored projected endpoints. Points and lines are front-facing.
 * Fragment coordinates use framebuffer X/Y = (x+0.5,y+0.5),
 * Z = (interpolated Z/W+1)/2 clamped to [0,1], and W = interpolated reciprocal W.
 * The material supplies depth and culling state. Effective facing follows its
 * front-face selection without changing coverage; points and lines stay front-facing.
 * Covered, unculled samples execute the fragment shader before depth testing.
 * Discard prevents color and depth writes. A failed depth test also prevents both.
 * Passing samples write depth when enabled and overwrite color when supplied;
 * an unwritten color preserves color while still permitting depth writes.
 * There is no blending.
 */
class software_renderer_t {
public:
    /** @brief Copies attachment views; later changes to the supplied view do not rebind this renderer. */
    explicit software_renderer_t(framebuffer_t framebuffer);

    /**
     * @brief Exposes this renderer's owned profiler, which starts disabled.
     *
     * Applications may measure their own work with this profiler. Each metric type
     * retains the latest data and accumulates timing statistics. Borrowed use of
     * the profiler ends with this renderer.
     */
    profiling::profiler_t& profiler() noexcept;
    const profiling::profiler_t& profiler() const noexcept;

    software_renderer_t(const software_renderer_t&) = delete;
    software_renderer_t& operator=(const software_renderer_t&) = delete;
    software_renderer_t(software_renderer_t&&) = delete;
    software_renderer_t& operator=(software_renderer_t&&) = delete;

    framebuffer_t& framebuffer() noexcept;
    const framebuffer_t& framebuffer() const noexcept;

    void clear_color(rgba8_t color);

    /**
     * @brief Fills the intersection of the camera rectangle and framebuffer.
     *
     * Camera pose, projection, materials, and shader state do not affect clearing.
     * Empty intersections do no work.
     */
    void clear_color(const camera_t& camera, rgba8_t color);

    /**
     * @brief Fills the depth attachment independently of draw state, preserving color.
     *
     * Requires a depth attachment for a nonempty framebuffer. Values are clamped
     * to [0,1], including infinities; NaN is rejected before writing any samples.
     * Empty framebuffers do no work, including validation.
     */
    void clear_depth(float depth);

    /**
     * @brief Clears depth within the intersection of the camera rectangle and framebuffer.
     *
     * Uses clear_depth's value and attachment rules. Camera pose and projection
     * do not affect clearing; empty intersections do no work, including validation.
     */
    void clear_depth(const camera_t& camera, float depth);

    /**
     * @brief Draws a render item using its material's program and the camera.
     *
     * Empty framebuffers, empty camera rectangles, and empty intersections return
     * before validating draw resources or deriving matrices. Otherwise requires
     * geometry, material, finite transforms, and framebuffer dimensions in [1, 2^23].
     * Enabled depth testing requires an attached depth buffer.
     * Validates current geometry, material bindings, and shader interfaces before
     * vertex execution. Camera rectangles support the full signed-int endpoint range.
     */
    void draw(const camera_t& camera, const render_item_t& render_item);

private:
    framebuffer_t m_framebuffer;
    scratch_t m_scratch;
    profiling::profiler_t m_profiler;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::software_renderer_t>;

} // namespace std


namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::software_renderer_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid software_renderer_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::software_renderer_t& software_renderer, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "framebuffer: {}", software_renderer.framebuffer());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_SOFTWARE_RENDERER_H
