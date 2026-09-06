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
     * @brief Draws a render item into the current non-empty framebuffer.
     *
     * Requires geometry with a valid mesh, selected indices and topology, a material
     * with complete compatible bindings, and non-empty camera world bounds. Each
     * call validates framebuffer dimensions, current geometry, material bindings,
     * camera bounds, and shader interfaces in that order before vertex execution.
     * Mesh streams must match consumed vertex input locations and types; fragment
     * inputs must be floating-point scalars or vectors. Selected vertex indices
     * must fit the shader's signed 32-bit vertex index. All selected vertices run
     * before rasterization begins.
     *
     * Each dimension must be at most 2^23. Vertex positions are finite homogeneous
     * clip coordinates, clipped in X, Y, Z to [-W,W]. A surviving zero-W vertex
     * makes its primitive empty; positive W must have a reciprocal representable
     * by the float fragment-coordinate interface. Unsupported dimensions and W,
     * and non-finite shader positions, are rejected.
     * Clipping preserves equal shared endpoints and each primitive's varying values.
     *
     * Triangle X/Y positions are projected and rounded once to a 1/256-pixel grid;
     * half-grid ties go toward the greater coordinate. Samples are pixel centers
     * (x+0.5,y+0.5). Coverage is the nonzero winding fill of the snapped boundary,
     * with top/left inclusion: equivalently, classify the sample infinitesimally
     * to the right, then infinitesimally below. Each covered sample is shaded once
     * per original triangle. Collapsed or cancelling boundaries emit no fragments;
     * snapped crossings, touches and overlaps are handled without geometry errors.
     * Matching shared boundaries with filled interiors on opposite sides have
     * complementary sample ownership; overlapping interiors of separate primitives
     * retain their independent coverage.
     *
     * Simple polygons use deterministic ears: normalize screen winding, start at
     * the least (X,Y), and remove the first unblocked convex ear. Collinear and
     * coincident occurrences retain their own payloads; a zero-area occurrence
     * may contribute no samples. Non-simple polygons use winding scanline spans,
     * interpolating reciprocal W, Z/W and varying/W along their boundary edges and
     * across each span. Equal crossing positions choose the least endpoint-record
     * key with the net crossing direction (geometry, projection, then payload bits).
     * Both paths divide interpolated varying/W by interpolated reciprocal W.
     * Interpolation describes the snapped geometry and preserves primitive-local
     * payloads; distinct coincident values can cause interpolation discontinuities.
     *
     * Every generated piece has one original-primitive facing value. For a simple
     * snapped polygon, CCW NDC (negative screen winding) is front-facing. A non-simple
     * boundary uses the largest absolute fan determinant of its least cyclic grid
     * sequence over both directions, first on ties, with submitted direction restored.
     * Fragment Z is (interpolated Z/W+1)/2 clamped to [0,1]; fragment W is reciprocal W.
     * Points cover integer offsets dx*dx+dy*dy <= 9 around the floored projected
     * position. Lines use inclusive Bresenham coverage between floored projected
     * endpoints; their interpolation factor is the clamped projection of the pixel
     * center onto the projected segment. Points and lines are front-facing.
     * There is no depth test, blending or culling. Discard or an unwritten fragment
     * color leaves the destination pixel unchanged.
     *
     * Vertex invocation receives the render item's T*R*S object-to-world matrix and
     * the camera-derived world-to-clip matrix; both preserve Z and W. Camera axes
     * follow camera_t's mapping and object transforms follow render_item_t.
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
