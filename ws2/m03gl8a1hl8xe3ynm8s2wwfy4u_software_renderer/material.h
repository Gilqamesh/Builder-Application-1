#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MATERIAL_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MATERIAL_H

# include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>
# include <m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h>

# include <cstdint>
# include <format>
# include <memory>
# include <unordered_map>
# include <utility>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace software_shader = m03gt1djvvy5atia5evkbg6rqy_software_shader;
namespace texture = m03gt0l0q3l4b1k27eab5k7py1_texture;

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
 * @brief Owns an immutable shader program, bindings, and mutable draw state.
 *
 * Uniform, texture, and sampler locations occupy independent namespaces. Extra
 * bindings not used by the program are accepted. Items sharing this material share
 * its draw state. Distinct materials may share a program, textures, and samplers.
 * Comparison, front-face, and cull setters reject invalid enum values before
 * changing their properties. Copies retain valid settings independently.
 * Initial settings disable depth testing, enable depth writes, select less and
 * CCW NDC front faces, and disable culling. Clears ignore these settings.
 */
class material_t {
public:
    /**
     * @brief Constructs a material with a required program that remains fixed for its lifetime.
     */
    explicit material_t(std::shared_ptr<const software_shader::program_t> program);

    const std::shared_ptr<const software_shader::program_t>& program() const;

    template <software_shader::shader::shader_value T>
    void uniform(std::uint32_t location, T value);

    /**
     * @brief Replaces a texture binding and its shared owner, or clears both when the value is null.
     */
    void texture(std::uint32_t location, std::shared_ptr<texture::texture_t> value);

    /**
     * @brief Replaces a sampler binding and its shared owner, or clears both when the value is null.
     */
    void sampler(std::uint32_t location, std::shared_ptr<texture::sampler_t> value);

    const software_shader::bindings_t& bindings() const;

    /**
     * @brief Enables depth comparison and permits depth writes according to depth_write().
     *
     * Disabling testing bypasses both comparison and depth writes. Enabled testing
     * requires an attachment when drawing, even with writes disabled or always selected.
     */
    void depth_test(bool enabled);
    bool depth_test() const;

    /** @brief Permits passing, non-discarded fragments to write depth when testing is enabled. */
    void depth_write(bool enabled);
    bool depth_write() const;

    /**
     * @brief Selects comparison of clamped float fragment-coordinate Z against stored depth.
     *
     * Comparisons use ordinary float operators without an epsilon. With writes
     * enabled, less preserves the first equal-depth fragment; less_equal permits
     * replacement. Against clear depth 1, less rejects far-plane samples at 1.
     * Enabled testing with always and writes enabled performs unconditional writes
     * for surviving fragments.
     */
    void depth_compare(comparison_t comparison);
    comparison_t depth_compare() const;

    void front_face(winding_t winding);
    winding_t front_face() const;

    void cull(cull_mode_t mode);
    cull_mode_t cull() const;

private:
    const std::shared_ptr<const software_shader::program_t> m_program;
    software_shader::bindings_t m_bindings;
    bool m_depth_test = false;
    bool m_depth_write = true;
    comparison_t m_depth_compare = comparison_t::less;
    winding_t m_front_face = winding_t::counter_clockwise;
    cull_mode_t m_cull = cull_mode_t::none;
    std::unordered_map<std::uint32_t, std::shared_ptr<texture::texture_t>> m_textures;
    std::unordered_map<std::uint32_t, std::shared_ptr<texture::sampler_t>> m_samplers;
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
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::material_t>;

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

template <software_shader::shader::shader_value T>
void material_t::uniform(std::uint32_t location, T value) {
    m_bindings.uniform(location, std::move(value));
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

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
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::material_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid material_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::material_t& material, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "program: {}", *material.program());
        out = std::format_to(out, ", bindings: {}", material.bindings());
        out = std::format_to(out, ", depth_test: {}", material.depth_test());
        out = std::format_to(out, ", depth_write: {}", material.depth_write());
        out = std::format_to(out, ", depth_compare: {}", material.depth_compare());
        out = std::format_to(out, ", front_face: {}", material.front_face());
        out = std::format_to(out, ", cull: {}", material.cull());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MATERIAL_H
