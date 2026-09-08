#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H

# include <cstddef>
# include <format>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

// Each metric path owns independent counters; repeated measurements of that path
// accumulate in the same object. The profiling module owns path identity.

/** @brief Counts color samples written across all measured color clears. */
struct clear_color_metrics_t {
    std::size_t color_writes = 0;
};

/** @brief Counts depth samples written across all measured depth clears. */
struct clear_depth_metrics_t {
    std::size_t depth_writes = 0;
};

/** @brief Counts stencil byte assignments across all measured stencil clears. */
struct clear_stencil_metrics_t {
    std::size_t stencil_writes = 0;
};

/** @brief Counts empty intersections and validated draws by their original topology. */
struct draw_metrics_t {
    std::size_t empty_draws = 0;
    std::size_t point_draws = 0;
    std::size_t line_draws = 0;
    std::size_t line_strip_draws = 0;
    std::size_t line_loop_draws = 0;
    std::size_t triangle_draws = 0;
    std::size_t triangle_strip_draws = 0;
    std::size_t triangle_fan_draws = 0;
};
/**
 * @brief Describes successfully prepared draws, accumulated once per draw.
 *
 * Bindings count reflected entries in both stages, including shared locations.
 * Interpolation counts scalar components of reflected fragment inputs, not samples.
 * Color paths count prepared draw state, including draws later clipped or culled.
 * Replacement includes blending equations equivalent to source replacement.
 * Disabled color writes are separate; masked counts partial masks on enabled paths.
 */
struct preparation_metrics_t {
    std::size_t vertex_inputs = 0;
    std::size_t uniform_bindings = 0;
    std::size_t texture_bindings = 0;
    std::size_t sampler_bindings = 0;
    std::size_t perspective_components = 0;
    std::size_t noperspective_components = 0;
    std::size_t flat_components = 0;
    std::size_t color_disabled_draws = 0;
    std::size_t replacement_draws = 0;
    std::size_t blended_draws = 0;
    std::size_t masked_draws = 0;
    std::size_t linear_draws = 0;
    std::size_t srgb_draws = 0;
};

/**
 * @brief Counts vertex invocations across all measurements, including calls that throw.
 *
 * Expected accumulates selected index entries across measurements, including repeated indices.
 */
struct vertex_metrics_t {
    explicit vertex_metrics_t(std::size_t expected = 0) noexcept;

    void accumulate(const vertex_metrics_t& counters) noexcept;

    std::size_t expected;
    std::size_t invocations = 0;
};

/**
 * @brief Counts fragment invocations and rasterization results across all measurements.
 *
 * Discards count completed invocations reporting discard. Depth rejections count
 * samples reaching depth testing that fail it. Stencil rejections count stencil failures. Writes count actual sample
 * assignments, including repeated assignments to overlapping framebuffer locations.
 * A color sample counts once when at least one channel is assigned, even if its bytes
 * are unchanged; a fully disabled color mask counts zero.
 * Stencil keep and a zero write mask perform no assignment; other operations count
 * an assignment even when the stored byte is unchanged.
 * Unwinding preserves counts up to the failed operation. Clear writes are separate.
 * Reported discard/depth-rejection percentages use invocations as the denominator;
 * no invocations is reported as n/a.
 *
 * Points/lines/triangles count original primitives submitted to rasterization.
 * Clipped_out includes surviving zero-W primitives; degenerate_triangles counts
 * polygons collapsed by projection/snapping. Clipping_intersections counts vertices
 * generated at all clip planes, including vertices removed by later planes.
 * Facing counts nondegenerate triangles before culling; rasterized_polygons and
 * algorithm counts follow culling, even when the camera intersection covers no samples.
 * Generated_triangles counts the triangulation used by those polygons.
 * Triangle_candidates counts planned bounding-rectangle sample visits on entry to
 * each generated triangle, including overlap and unvisited candidates on exception.
 * Winding scanlines/events/spans count reached work; spans count nonempty intervals.
 * All other counts preserve completed events and attempted invocations on exceptions.
 */
struct raster_metrics_t {
    void accumulate(const raster_metrics_t& counters) noexcept;

    std::size_t invocations = 0;
    std::size_t discards = 0;
    std::size_t stencil_rejections = 0;
    std::size_t stencil_writes = 0;
    std::size_t depth_rejections = 0;
    std::size_t color_writes = 0;
    std::size_t depth_writes = 0;
    std::size_t points = 0;
    std::size_t lines = 0;
    std::size_t triangles = 0;
    std::size_t clipped_out = 0;
    std::size_t clipping_intersections = 0;
    std::size_t degenerate_triangles = 0;
    std::size_t front_triangles = 0;
    std::size_t back_triangles = 0;
    std::size_t culled_triangles = 0;
    std::size_t rasterized_polygons = 0;
    std::size_t triangulated_polygons = 0;
    std::size_t winding_polygons = 0;
    std::size_t generated_triangles = 0;
    std::size_t triangle_candidates = 0;
    std::size_t winding_scanlines = 0;
    std::size_t winding_events = 0;
    std::size_t winding_spans = 0;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {
template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_color_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_depth_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_stencil_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::preparation_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_metrics_t>;

} // namespace std

namespace std {
template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_color_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_color_metrics_t& clear_color_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.clear_color");
        out = std::format_to(out, " color_writes={}", clear_color_metrics.color_writes);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_depth_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_depth_metrics_t& clear_depth_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.clear_depth");
        out = std::format_to(out, " depth_writes={}", clear_depth_metrics.depth_writes);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_stencil_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_stencil_metrics_t& clear_stencil_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.clear_stencil");
        out = std::format_to(out, " stencil_writes={}", clear_stencil_metrics.stencil_writes);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.draw");
        out = std::format_to(out, " empty_draws={}", metrics.empty_draws);
        out = std::format_to(out, " point_draws={}", metrics.point_draws);
        out = std::format_to(out, " line_draws={}", metrics.line_draws);
        out = std::format_to(out, " line_strip_draws={}", metrics.line_strip_draws);
        out = std::format_to(out, " line_loop_draws={}", metrics.line_loop_draws);
        out = std::format_to(out, " triangle_draws={}", metrics.triangle_draws);
        out = std::format_to(out, " triangle_strip_draws={}", metrics.triangle_strip_draws);
        out = std::format_to(out, " triangle_fan_draws={}", metrics.triangle_fan_draws);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::preparation_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::preparation_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.preparation");
        out = std::format_to(out, " vertex_inputs={}", metrics.vertex_inputs);
        out = std::format_to(out, " uniform_bindings={}", metrics.uniform_bindings);
        out = std::format_to(out, " texture_bindings={}", metrics.texture_bindings);
        out = std::format_to(out, " sampler_bindings={}", metrics.sampler_bindings);
        out = std::format_to(out, " perspective_components={}", metrics.perspective_components);
        out = std::format_to(out, " noperspective_components={}", metrics.noperspective_components);
        out = std::format_to(out, " flat_components={}", metrics.flat_components);
        out = std::format_to(out, " color_disabled_draws={}", metrics.color_disabled_draws);
        out = std::format_to(out, " replacement_draws={}", metrics.replacement_draws);
        out = std::format_to(out, " blended_draws={}", metrics.blended_draws);
        out = std::format_to(out, " masked_draws={}", metrics.masked_draws);
        out = std::format_to(out, " linear_draws={}", metrics.linear_draws);
        out = std::format_to(out, " srgb_draws={}", metrics.srgb_draws);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_metrics_t& vertex_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.vertices");
        out = std::format_to(out, " vertex_invocations={}", vertex_metrics.invocations);
        out = std::format_to(out, ", expected={}", vertex_metrics.expected);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_metrics_t& raster_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.rasterization");
        out = std::format_to(out, " fragment_invocations={}", raster_metrics.invocations);
        out = std::format_to(out, ", discards={}", raster_metrics.discards);
        out = std::format_to(out, ", stencil_rejections={}", raster_metrics.stencil_rejections);
        out = std::format_to(out, ", stencil_writes={}", raster_metrics.stencil_writes);
        out = std::format_to(out, ", depth_rejections={}", raster_metrics.depth_rejections);
        out = std::format_to(out, ", color_writes={}", raster_metrics.color_writes);
        out = std::format_to(out, ", depth_writes={}", raster_metrics.depth_writes);
        out = std::format_to(out, ", points={}", raster_metrics.points);
        out = std::format_to(out, ", lines={}", raster_metrics.lines);
        out = std::format_to(out, ", triangles={}", raster_metrics.triangles);
        out = std::format_to(out, ", clipped_out={}", raster_metrics.clipped_out);
        out = std::format_to(out, ", clipping_intersections={}", raster_metrics.clipping_intersections);
        out = std::format_to(out, ", degenerate_triangles={}", raster_metrics.degenerate_triangles);
        out = std::format_to(out, ", front_triangles={}", raster_metrics.front_triangles);
        out = std::format_to(out, ", back_triangles={}", raster_metrics.back_triangles);
        out = std::format_to(out, ", culled_triangles={}", raster_metrics.culled_triangles);
        out = std::format_to(out, ", rasterized_polygons={}", raster_metrics.rasterized_polygons);
        out = std::format_to(out, ", triangulated_polygons={}", raster_metrics.triangulated_polygons);
        out = std::format_to(out, ", winding_polygons={}", raster_metrics.winding_polygons);
        out = std::format_to(out, ", generated_triangles={}", raster_metrics.generated_triangles);
        out = std::format_to(out, ", triangle_candidates={}", raster_metrics.triangle_candidates);
        out = std::format_to(out, ", winding_scanlines={}", raster_metrics.winding_scanlines);
        out = std::format_to(out, ", winding_events={}", raster_metrics.winding_events);
        out = std::format_to(out, ", winding_spans={}", raster_metrics.winding_spans);
        if (raster_metrics.invocations == 0) {
            out = std::format_to(out, ", discarded=n/a, depth_rejected=n/a");
        } else {
            const auto percent = 100.0L / raster_metrics.invocations;
            out = std::format_to(out, ", discarded={:.1f}%", raster_metrics.discards * percent);
            out = std::format_to(out, ", depth_rejected={:.1f}%", raster_metrics.depth_rejections * percent);
        }
        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H
