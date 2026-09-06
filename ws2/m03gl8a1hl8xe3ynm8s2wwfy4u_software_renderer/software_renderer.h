#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_SOFTWARE_RENDERER_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_SOFTWARE_RENDERER_H

# include "camera.h"
# include "framebuffer.h"
# include "helpers.h"
# include "metrics.h"
# include "render_item.h"

# include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>
# include <m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h>

# include <algorithm>
# include <cstddef>
# include <cstdint>
# include <format>
# include <limits>
# include <type_traits>
# include <utility>
# include <variant>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

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
template <typename Profiler = profiling::disabled_profiler_t>
class software_renderer_t {
public:
    /** @brief Copies attachment views; later changes to the supplied view do not rebind this renderer. */
    explicit software_renderer_t(framebuffer_t framebuffer) requires (!Profiler::enabled);

    /**
     * @brief Borrows the shared profiler with its explicitly registered renderer regions.
     *
     * Construct during setup with regions from this profiler, then start capture before
     * rendering. The profiler and its capture storage outlive renderer operations.
     * Enabled and disabled renderer policies can coexist in the same executable.
     */
    software_renderer_t(framebuffer_t framebuffer, Profiler& profiler, const regions_t& regions) requires (Profiler::enabled);

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
    template <typename T = std::monostate>
    auto scope(region_t region);

    framebuffer_t m_framebuffer;
    scratch_t m_scratch;
    [[no_unique_address]] std::conditional_t<Profiler::enabled, std::pair<Profiler&, regions_t>, Profiler> m_profiling;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <typename Profiler>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::software_renderer_t<Profiler>>;

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

template <typename Profiler>
software_renderer_t<Profiler>::software_renderer_t(framebuffer_t framebuffer) requires (!Profiler::enabled):
    m_framebuffer(framebuffer)
{
}

template <typename Profiler>
software_renderer_t<Profiler>::software_renderer_t(framebuffer_t framebuffer, Profiler& profiler, const regions_t& regions) requires (Profiler::enabled):
    m_framebuffer(framebuffer),
    m_profiling(profiler, regions)
{
    for (const auto region : regions.m_ids) {
        if (region.m_owner != &profiler) {
            throw std::invalid_argument("software_renderer_t requires regions registered with its profiler");
        }
    }
}

template <typename Profiler>
framebuffer_t& software_renderer_t<Profiler>::framebuffer() noexcept {
    return m_framebuffer;
}

template <typename Profiler>
const framebuffer_t& software_renderer_t<Profiler>::framebuffer() const noexcept {
    return m_framebuffer;
}

template <typename Profiler>
void software_renderer_t<Profiler>::clear_color(rgba8_t color) {
    [[maybe_unused]] auto clear_scope = scope<clear_metrics_t>(region_t::clear_color);
    std::ranges::fill(m_framebuffer.pixels(), color);
    if constexpr (Profiler::enabled) {
        clear_scope.metrics().m_color_writes = m_framebuffer.pixels().size();
    }
}

template <typename Profiler>
void software_renderer_t<Profiler>::clear_color(const camera_t& camera, rgba8_t color) {
    [[maybe_unused]] auto clear_scope = scope<clear_metrics_t>(region_t::clear_color);
    const raster_bounds_t bounds(m_framebuffer.width(), m_framebuffer.height(), camera.view_rect());
    if (bounds.empty()) {
        return;
    }
    const auto pixels = m_framebuffer.pixels();
    for (auto y = bounds.m_first_y; y < bounds.m_end_y; ++y) {
        const auto offset = std::size_t(y + bounds.m_y) * std::size_t(bounds.m_width) + std::size_t(bounds.m_first_x + bounds.m_x);
        std::ranges::fill(pixels.subspan(offset, std::size_t(bounds.m_end_x - bounds.m_first_x)), color);
        if constexpr (Profiler::enabled) {
            clear_scope.metrics().m_color_writes += std::size_t(bounds.m_end_x - bounds.m_first_x);
        }
    }
}

template <typename Profiler>
void software_renderer_t<Profiler>::clear_depth(float depth) {
    [[maybe_unused]] auto clear_scope = scope<clear_metrics_t>(region_t::clear_depth);
    if (m_framebuffer.pixels().empty()) {
        return;
    }
    if (m_framebuffer.depth().empty()) {
        throw std::invalid_argument("software_renderer_t::clear_depth requires a depth attachment");
    }
    std::ranges::fill(m_framebuffer.depth(), depth_clear_value(depth));
    if constexpr (Profiler::enabled) {
        clear_scope.metrics().m_depth_writes = m_framebuffer.depth().size();
    }
}

template <typename Profiler>
void software_renderer_t<Profiler>::clear_depth(const camera_t& camera, float depth) {
    [[maybe_unused]] auto clear_scope = scope<clear_metrics_t>(region_t::clear_depth);
    const raster_bounds_t bounds(m_framebuffer.width(), m_framebuffer.height(), camera.view_rect());
    if (bounds.empty()) {
        return;
    }
    const auto samples = m_framebuffer.depth();
    if (samples.empty()) {
        throw std::invalid_argument("software_renderer_t::clear_depth requires a depth attachment");
    }
    depth = depth_clear_value(depth);
    for (auto y = bounds.m_first_y; y < bounds.m_end_y; ++y) {
        const auto offset = std::size_t(y + bounds.m_y) * std::size_t(bounds.m_width) + std::size_t(bounds.m_first_x + bounds.m_x);
        std::ranges::fill(samples.subspan(offset, std::size_t(bounds.m_end_x - bounds.m_first_x)), depth);
        if constexpr (Profiler::enabled) {
            clear_scope.metrics().m_depth_writes += std::size_t(bounds.m_end_x - bounds.m_first_x);
        }
    }
}

template <typename Profiler>
void software_renderer_t<Profiler>::draw(
    const camera_t& camera,
    const render_item_t& render_item
) {
    [[maybe_unused]] auto draw_scope = scope(region_t::draw);
    [[maybe_unused]] auto preparation = scope(region_t::preparation);
    const raster_bounds_t bounds(m_framebuffer.width(), m_framebuffer.height(), camera.view_rect());
    if (bounds.empty()) {
        return;
    }

    if (maximum_extent < m_framebuffer.width() || maximum_extent < m_framebuffer.height()) {
        throw std::out_of_range(std::format("software_renderer_t::draw dimensions {}x{} exceed the supported limit {}", m_framebuffer.width(), m_framebuffer.height(), maximum_extent));
    }

    const auto geometry = render_item.geometry();
    if (!geometry) {
        throw std::invalid_argument("software_renderer_t::draw requires geometry");
    }
    geometry->finalize();
    const auto material = render_item.material();
    if (!material) {
        throw std::invalid_argument("software_renderer_t::draw requires a material");
    }
    if (material->depth_test() && m_framebuffer.depth().empty()) {
        throw std::invalid_argument("software_renderer_t::draw requires a depth attachment when depth testing is enabled");
    }
    const auto& program = *material->program();
    const auto& bindings = material->bindings();
    program.validate_bindings(bindings);

    const auto object_to_world = render_item.object_to_world();
    const auto world_to_clip = camera.world_to_clip();
    const auto& framebuffer = m_framebuffer;
    const auto mesh = geometry->mesh();
    const auto& streams = mesh->vertex_streams();
    const auto attributes = mesh->vertex_attributes();
    for (const auto& input : program.vertex_interface().inputs()) {
        if (streams.size() <= input.index) {
            throw std::invalid_argument("shader vertex input location has no corresponding mesh stream");
        }
        validate_vertex_attribute(attributes[input.index], input.type);
    }
    for (const auto& input : program.fragment_interface().inputs()) {
        if (!supported_fragment_input(input.type)) {
            throw std::invalid_argument("software renderer cannot interpolate this fragment input type");
        }
    }

    if constexpr (Profiler::enabled) {
        preparation.close();
    }
    [[maybe_unused]] auto vertices = scope<vertex_metrics_t>(region_t::vertices);
    const auto indices = geometry->indices();
    auto& scratch = m_scratch;
    scratch.m_vertex_results.clear();
    scratch.m_vertex_values.clear();
    scratch.m_fragment_inputs.clear();
    scratch.m_vertex_results.reserve(indices.size());
    scratch.m_vertex_io.object_to_world(object_to_world);
    scratch.m_vertex_io.world_to_clip(world_to_clip);
    for (const std::uint32_t vertex_index : indices) {
        if (static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) < vertex_index) {
            throw std::out_of_range("vertex index cannot be represented by software shader vertex_io_t");
        }

        auto& io = scratch.m_vertex_io;
        io.reset(static_cast<std::int32_t>(vertex_index), 0);
        for (const auto& input : program.vertex_interface().inputs()) {
            set_vertex_input(io, input, streams[input.index], attributes[input.index], vertex_index);
        }
        if constexpr (Profiler::enabled) {
            ++vertices.metrics().m_invocations;
        }
        program.run(bindings, io);
        const vector4f_t clip_position = io.position();
        if (!finite(clip_position)) {
            throw std::runtime_error("vertex shader produced a non-finite clip position");
        }

        const std::size_t output_offset = scratch.m_vertex_values.size();
        for (const auto& input : program.fragment_interface().inputs()) {
            scratch.m_vertex_values.emplace_back(input.index, vertex_output(io, input));
        }
        scratch.m_vertex_results.push_back({
            .m_clip_position = clip_position,
            .m_outputs = {output_offset, program.fragment_interface().inputs().size()}
        });
    }

    if constexpr (Profiler::enabled) {
        vertices.close();
    }
    auto rasterization = scope<raster_metrics_t>(region_t::rasterization);
    const auto vertex = [&](std::size_t index) {
        return view(scratch.m_vertex_results[index], scratch.m_vertex_values);
    };
    switch (geometry->primitive_topology()) {
        case vertex_primitive_topology_t::point: {
            for (std::size_t index = 0; index < indices.size(); ++index) {
                rasterize_point(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    rasterization
                );
            }
        } break;
        case vertex_primitive_topology_t::line: {
            for (std::size_t index = 0; index + 1 < indices.size(); index += 2) {
                rasterize_line(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    vertex(index + 1),
                    scratch.m_clipping,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    rasterization
                );
            }
        } break;
        case vertex_primitive_topology_t::line_strip: {
            for (std::size_t index = 0; index + 1 < indices.size(); ++index) {
                rasterize_line(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    vertex(index + 1),
                    scratch.m_clipping,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    rasterization
                );
            }
        } break;
        case vertex_primitive_topology_t::line_loop: {
            for (std::size_t index = 0; index < indices.size(); ++index) {
                rasterize_line(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    vertex((index + 1) % indices.size()),
                    scratch.m_clipping,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    rasterization
                );
            }
        } break;
        case vertex_primitive_topology_t::triangle: {
            for (std::size_t index = 0; index + 2 < indices.size(); index += 3) {
                rasterize_triangle(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    vertex(index + 1),
                    vertex(index + 2),
                    scratch.m_raster,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    rasterization
                );
            }
        } break;
        case vertex_primitive_topology_t::triangle_strip: {
            for (std::size_t index = 0; index + 2 < indices.size(); ++index) {
                if (index % 2 == 0) {
                    rasterize_triangle(
                        *material,
                        bounds,
                        framebuffer,
                        vertex(index + 1),
                        vertex(index),
                        vertex(index + 2),
                        scratch.m_raster,
                        scratch.m_fragment_inputs,
                        scratch.m_fragment_io,
                        rasterization
                    );
                } else {
                    rasterize_triangle(
                        *material,
                        bounds,
                        framebuffer,
                        vertex(index),
                        vertex(index + 1),
                        vertex(index + 2),
                        scratch.m_raster,
                        scratch.m_fragment_inputs,
                        scratch.m_fragment_io,
                        rasterization
                    );
                }
            }
        } break;
        case vertex_primitive_topology_t::triangle_fan: {
            for (std::size_t index = 1; index + 1 < indices.size(); ++index) {
                rasterize_triangle(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(0),
                    vertex(index),
                    vertex(index + 1),
                    scratch.m_raster,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    rasterization
                );
            }
        } break;
    }
}

template <typename Profiler>
template <typename T>
auto software_renderer_t<Profiler>::scope(region_t region) {
    if constexpr (Profiler::enabled) {
        return m_profiling.first.template scope<T>(m_profiling.second.m_ids[static_cast<std::size_t>(region)]);
    } else {
        return m_profiling.template scope<T>({});
    }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <typename Profiler>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::software_renderer_t<Profiler>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid software_renderer_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::software_renderer_t<Profiler>& renderer, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "framebuffer: {}", renderer.framebuffer());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_SOFTWARE_RENDERER_H
