#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H

# include <cstddef>
# include <format>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

enum class clear_target_t { color, depth };

/** @brief Counts attachment samples actually written by one clear operation. */
struct clear_metrics_t {
    explicit clear_metrics_t(clear_target_t target) noexcept;

    clear_target_t m_target;
    std::size_t m_color_writes = 0;
    std::size_t m_depth_writes = 0;
};

struct draw_metrics_t {};
struct preparation_metrics_t {};

/**
 * @brief Counts vertex invocations entered, including an invocation that throws.
 *
 * Expected counts selected index entries, including repeated indices.
 */
struct vertex_metrics_t {
    explicit vertex_metrics_t(std::size_t expected) noexcept;

    std::size_t m_expected;
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

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {
template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_target_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_metrics_t>;

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
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_target_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid clear_target_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_target_t& clear_target, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{}", clear_target == m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_target_t::color ? "color" : "depth");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid clear_metrics_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_metrics_t& clear_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.clear_{} color_writes={}, depth_writes={}", clear_metrics.m_target, clear_metrics.m_color_writes, clear_metrics.m_depth_writes);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid draw_metrics_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_metrics_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.draw");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::preparation_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid preparation_metrics_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::preparation_metrics_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.preparation");
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

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_metrics_t& vertex_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.vertices vertex_invocations={}, expected={}", vertex_metrics.m_invocations, vertex_metrics.m_expected);
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

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_metrics_t& raster_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.rasterization fragment_invocations={}, discards={}, depth_rejections={}, color_writes={}, depth_writes={}", raster_metrics.m_invocations, raster_metrics.m_discards, raster_metrics.m_depth_rejections, raster_metrics.m_color_writes, raster_metrics.m_depth_writes);
        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H
