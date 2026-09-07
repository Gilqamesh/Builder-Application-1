#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H

# include <cstddef>
# include <format>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/** @brief Counts color samples written across all measured color clears. */
struct clear_color_metrics_t {
    std::size_t m_color_writes = 0;
};

/** @brief Counts depth samples written across all measured depth clears. */
struct clear_depth_metrics_t {
    std::size_t m_depth_writes = 0;
};

struct draw_metrics_t {};
struct preparation_metrics_t {};

/**
 * @brief Counts vertex invocations across all measurements, including calls that throw.
 *
 * Expected accumulates selected index entries across measurements, including repeated indices.
 */
struct vertex_metrics_t {
    explicit vertex_metrics_t(std::size_t expected = 0) noexcept;

    std::size_t m_expected;
    std::size_t m_invocations = 0;
};

/**
 * @brief Counts fragment invocations and rasterization results across all measurements.
 *
 * Discards count completed invocations reporting discard. Depth rejections count
 * non-discarded samples that fail enabled depth testing. Writes count actual sample
 * assignments, including repeated assignments to overlapping framebuffer locations.
 * Unwinding preserves counts up to the failed operation. Clear writes are separate.
 * Reported discard/depth-rejection percentages use invocations as the denominator;
 * no invocations is reported as n/a.
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
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_color_metrics_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_depth_metrics_t>;

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
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid clear_color_metrics_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_color_metrics_t& clear_color_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.clear_color color_writes={}", clear_color_metrics.m_color_writes);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_depth_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid clear_depth_metrics_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clear_depth_metrics_t& clear_depth_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.clear_depth depth_writes={}", clear_depth_metrics.m_depth_writes);
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
        if (raster_metrics.m_invocations == 0) {
            out = std::format_to(out, ", discarded=n/a, depth_rejected=n/a");
        } else {
            const auto percent = 100.0L / raster_metrics.m_invocations;
            out = std::format_to(out, ", discarded={:.1f}%, depth_rejected={:.1f}%", raster_metrics.m_discards * percent, raster_metrics.m_depth_rejections * percent);
        }
        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H
