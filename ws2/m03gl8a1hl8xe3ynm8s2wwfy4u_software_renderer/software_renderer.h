#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_SOFTWARE_RENDERER_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_SOFTWARE_RENDERER_H

# include "camera.h"
# include "framebuffer.h"
# include "helpers.h"
# include "render_item.h"

# include <format>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/**
 * @brief Renders camera-relative render items into a borrowed CPU framebuffer.
 */
class software_renderer_t {
public:
    explicit software_renderer_t(framebuffer_t framebuffer);

    software_renderer_t(const software_renderer_t&) = delete;
    software_renderer_t& operator=(const software_renderer_t&) = delete;
    software_renderer_t(software_renderer_t&&) = delete;
    software_renderer_t& operator=(software_renderer_t&&) = delete;

    framebuffer_t& framebuffer() noexcept;
    const framebuffer_t& framebuffer() const noexcept;

    void clear(rgba8_t color);

    /**
     * @brief Draws a render item into the current framebuffer.
     *
     * Requires valid geometry (mesh, selected indices and topology), complete
     * compatible material bindings, and non-empty camera world bounds. Resources
     * and shader interfaces are validated before vertex execution. Mesh streams
     * must match consumed vertex input locations and types; fragment inputs must
     * be floating-point scalars or vectors. Selected indices must fit signed 32-bit
     * vertex indices. All selected vertices run before rasterization begins.
     * Vertex invocation receives the matrices described by camera_t and render_item_t.
     *
     * Framebuffer dimensions must be in [1,2^23]. Shader positions must be finite
     * homogeneous clip coordinates; X, Y and Z are clipped to [-W,W], preserving
     * equal shared endpoints. A surviving zero-W vertex makes its primitive empty.
     * Positive W must have a reciprocal representable by the float fragment-coordinate
     * interface. Unsupported dimensions or W and non-finite positions are rejected.
     *
     * Triangle X/Y positions are projected and rounded once to a 1/256-pixel grid,
     * with half-grid ties toward the greater coordinate. Pixel-center samples
     * (x+0.5,y+0.5) use nonzero winding coverage with top/left inclusion. Each covered
     * sample is shaded once per original triangle. Collapsed or cancelling boundaries
     * emit no fragments; snapped crossings, touches and overlaps are supported.
     * Matching shared boundaries with interiors on opposite sides have complementary
     * sample ownership; separate primitives with overlapping interiors shade independently.
     *
     * Varyings use perspective-correct interpolation on the snapped triangle geometry,
     * preserving primitive-local values through clipping and rasterization. Distinct
     * coincident values can cause interpolation discontinuities. All pieces of an
     * original triangle share one facing value; simple snapped polygons are front-facing
     * for CCW NDC winding. Points and lines are front-facing. Fragment coordinates use
     * framebuffer X/Y, Z = (interpolated Z/W+1)/2 clamped to [0,1], and W = reciprocal W.
     *
     * Points cover integer offsets dx*dx+dy*dy <= 9 around the floored projected
     * position. Lines include both floored projected endpoints; their interpolation
     * factor is the clamped projection of the pixel center onto the projected segment.
     * Written fragment colors overwrite destination pixels without depth testing,
     * blending or culling. Discard or an unwritten color leaves the pixel unchanged.
     */
    void draw(const camera_t<float, int, 2>& camera, const render_item_t& render_item);

private:
    framebuffer_t m_framebuffer;
    scratch_t m_scratch;
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

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::software_renderer_t& renderer, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "framebuffer: {}", renderer.framebuffer());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_SOFTWARE_RENDERER_H
