#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_DRAW_STATE_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_DRAW_STATE_H

# include <format>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/** @brief Compares the incoming window depth against the stored float sample. */
enum class comparison_t {
    never,
    less,
    equal,
    less_equal,
    greater,
    not_equal,
    greater_equal,
    always
};

/** @brief Selects front-facing triangle winding in NDC, before the framebuffer Y inversion. */
enum class winding_t {
    counter_clockwise,
    clockwise
};

/** @brief Selects triangle faces to omit; points and lines are unaffected. */
enum class cull_mode_t {
    none,
    front,
    back,
    both
};

/**
 * @brief Describes the material's depth and face-culling policy for each draw.
 *
 * Disabling depth testing bypasses both comparison and depth writes. Enabling it
 * requires a depth attachment, even when writes are disabled or comparison is
 * always. Enabled testing with always and writes enabled performs unconditional
 * depth writes for surviving fragments.
 *
 * Comparisons use the clamped float fragment-coordinate Z and ordinary float
 * comparisons against stored depth, without an epsilon. With writes enabled,
 * less preserves the first equal-depth fragment; less_equal allows replacement.
 * Against a clear depth of 1, less rejects samples at the far-plane depth of 1.
 * Default settings preserve color-only rendering. Clears ignore this state.
 */
struct draw_state_t {
    bool m_depth_test = false;
    bool m_depth_write = true;
    comparison_t m_depth_compare = comparison_t::less;
    winding_t m_front_face = winding_t::counter_clockwise;
    cull_mode_t m_cull = cull_mode_t::none;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::winding_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::cull_mode_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_state_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid comparison_t format specifier");
        }
        return it;
    }

    auto format(m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t option, auto& ctx) const {
        auto out = ctx.out();
        switch (option) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t::never: {
                out = std::format_to(out, "never");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t::less: {
                out = std::format_to(out, "less");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t::equal: {
                out = std::format_to(out, "equal");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t::less_equal: {
                out = std::format_to(out, "less_equal");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t::greater: {
                out = std::format_to(out, "greater");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t::not_equal: {
                out = std::format_to(out, "not_equal");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t::greater_equal: {
                out = std::format_to(out, "greater_equal");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t::always: {
                out = std::format_to(out, "always");
            } break;
            default: {
                out = std::format_to(out, "invalid({})", static_cast<int>(option));
            } break;
        }
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::winding_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid winding_t format specifier");
        }
        return it;
    }

    auto format(m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::winding_t option, auto& ctx) const {
        auto out = ctx.out();
        switch (option) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::winding_t::counter_clockwise: {
                out = std::format_to(out, "counter_clockwise");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::winding_t::clockwise: {
                out = std::format_to(out, "clockwise");
            } break;
            default: {
                out = std::format_to(out, "invalid({})", static_cast<int>(option));
            } break;
        }
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::cull_mode_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid cull_mode_t format specifier");
        }
        return it;
    }

    auto format(m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::cull_mode_t option, auto& ctx) const {
        auto out = ctx.out();
        switch (option) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::cull_mode_t::none: {
                out = std::format_to(out, "none");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::cull_mode_t::front: {
                out = std::format_to(out, "front");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::cull_mode_t::back: {
                out = std::format_to(out, "back");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::cull_mode_t::both: {
                out = std::format_to(out, "both");
            } break;
            default: {
                out = std::format_to(out, "invalid({})", static_cast<int>(option));
            } break;
        }
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_state_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid draw_state_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_state_t& state, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "depth_test: {}", state.m_depth_test);
        out = std::format_to(out, ", depth_write: {}", state.m_depth_write);
        out = std::format_to(out, ", depth_compare: {}", state.m_depth_compare);
        out = std::format_to(out, ", front_face: {}", state.m_front_face);
        out = std::format_to(out, ", cull: {}", state.m_cull);
        out = std::format_to(out, " }}");
        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_DRAW_STATE_H
