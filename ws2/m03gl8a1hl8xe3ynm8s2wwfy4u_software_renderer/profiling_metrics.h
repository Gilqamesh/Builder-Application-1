#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H

# include <cstddef>
# include <format>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

// Each metric path owns independent counters; repeated measurements of that path
// accumulate in the same object. The profiling module owns path identity.

/** @brief Counts color samples written across all measured color clears. */
struct clear_color_metrics_t {
    std::size_t m_color_writes = 0;
};

/** @brief Counts depth samples written across all measured depth clears. */
struct clear_depth_metrics_t {
    std::size_t m_depth_writes = 0;
};

/** @brief Counts stencil byte assignments across all measured stencil clears. */
struct clear_stencil_metrics_t {
    std::size_t m_stencil_writes = 0;
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
 * samples reaching depth testing that fail it. Stencil rejections count stencil failures. Writes count actual sample
 * assignments, including repeated assignments to overlapping framebuffer locations.
 * A color sample counts once when at least one channel is assigned, even if its bytes
 * are unchanged; a fully disabled color mask counts zero.
 * Stencil keep and a zero write mask perform no assignment; other operations count
 * an assignment even when the stored byte is unchanged.
 * Unwinding preserves counts up to the failed operation. Clear writes are separate.
 * Reported discard/depth-rejection percentages use invocations as the denominator;
 * no invocations is reported as n/a.
 */
struct raster_metrics_t {
    std::size_t m_invocations = 0;
    std::size_t m_discards = 0;
    std::size_t m_stencil_rejections = 0;
    std::size_t m_stencil_writes = 0;
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
        out = std::format_to(out, " color_writes={}", clear_color_metrics.m_color_writes);
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
        out = std::format_to(out, " depth_writes={}", clear_depth_metrics.m_depth_writes);
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
        out = std::format_to(out, " stencil_writes={}", clear_stencil_metrics.m_stencil_writes);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
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
        return ctx.begin();
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
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_metrics_t& vertex_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.vertices");
        out = std::format_to(out, " vertex_invocations={}", vertex_metrics.m_invocations);
        out = std::format_to(out, ", expected={}", vertex_metrics.m_expected);
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
        out = std::format_to(out, " fragment_invocations={}", raster_metrics.m_invocations);
        out = std::format_to(out, ", discards={}", raster_metrics.m_discards);
        out = std::format_to(out, ", stencil_rejections={}", raster_metrics.m_stencil_rejections);
        out = std::format_to(out, ", stencil_writes={}", raster_metrics.m_stencil_writes);
        out = std::format_to(out, ", depth_rejections={}", raster_metrics.m_depth_rejections);
        out = std::format_to(out, ", color_writes={}", raster_metrics.m_color_writes);
        out = std::format_to(out, ", depth_writes={}", raster_metrics.m_depth_writes);
        if (raster_metrics.m_invocations == 0) {
            out = std::format_to(out, ", discarded=n/a, depth_rejected=n/a");
        } else {
            const auto percent = 100.0L / raster_metrics.m_invocations;
            out = std::format_to(out, ", discarded={:.1f}%", raster_metrics.m_discards * percent);
            out = std::format_to(out, ", depth_rejected={:.1f}%", raster_metrics.m_depth_rejections * percent);
        }
        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_PROFILING_METRICS_H
