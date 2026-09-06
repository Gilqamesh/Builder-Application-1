#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_METRICS_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_METRICS_H

# include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

# include <array>
# include <cstddef>
# include <format>
# include <string_view>
# include <variant>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;

/** @brief Counts attachment samples actually written by one clear operation. */
struct clear_metrics_t {
    std::size_t m_color_writes = 0;
    std::size_t m_depth_writes = 0;
};

/** @brief Counts vertex invocations entered, including an invocation that throws. */
struct vertex_metrics_t {
    std::size_t m_invocations = 0;
};

/**
 * @brief Counts fragment invocations entered and subsequent results of rasterization.
 *
 * Discards count completed invocations reporting discard. Depth rejections count
 * non-discarded samples that fail enabled depth testing. Writes count actual sample
 * assignments, including repeated assignments to overlapping framebuffer locations.
 * Unwinding preserves counts up to the failed operation. Clear writes are separate.
 */
struct raster_metrics_t {
    std::size_t m_invocations = 0;
    std::size_t m_discards = 0;
    std::size_t m_depth_rejections = 0;
    std::size_t m_color_writes = 0;
    std::size_t m_depth_writes = 0;
};

using metrics_t = std::variant<clear_metrics_t, vertex_metrics_t, raster_metrics_t>;

enum class region_t { clear_color, clear_depth, draw, preparation, vertices, rasterization };

/**
 * @brief Registers renderer regions during application setup before profiler.start().
 *
 * The supplied profiler copies and owns region names through reporting. Retain this
 * mapping for constructing renderers sharing that profiler; capture resets preserve it.
 * An application's profiler payload variant includes all three renderer metric types.
 */
struct regions_t {
    std::array<profiling::region_id_t, 6> m_ids;

    template <typename profiler_type_t>
    explicit regions_t(profiler_type_t& profiler);
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::region_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::regions_t>;

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

template <typename profiler_type_t>
regions_t::regions_t(profiler_type_t& profiler) {
    constexpr std::array<std::string_view, 6> names {
        "renderer.clear_color", "renderer.clear_depth", "renderer.draw",
        "renderer.preparation", "renderer.vertices", "renderer.rasterization"
    };
    for (std::size_t i = 0; i < names.size(); ++i) {
        m_ids[i] = profiler.register_region(names[i]);
    }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid clear_metrics_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "color_writes={}, depth_writes={}", metrics.m_color_writes, metrics.m_depth_writes);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid vertex_metrics_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "vertex_invocations={}", metrics.m_invocations);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid raster_metrics_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "fragment_invocations={}, discards={}, depth_rejections={}, color_writes={}, depth_writes={}", metrics.m_invocations, metrics.m_discards, metrics.m_depth_rejections, metrics.m_color_writes, metrics.m_depth_writes);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::region_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid region_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::region_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "region({})", static_cast<std::size_t>(metrics));
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::regions_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid regions_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::regions_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer_regions");
        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_METRICS_H
