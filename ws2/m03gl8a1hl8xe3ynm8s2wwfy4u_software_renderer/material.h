#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MATERIAL_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MATERIAL_H

# include "draw_state.h"

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

/**
 * @brief Owns an immutable shader program, bindings, and mutable draw state.
 *
 * Uniform, texture, and sampler locations occupy independent namespaces. Extra
 * bindings not used by the program are accepted. Items sharing this material share
 * its draw state. Distinct materials may share a program, textures, and samplers.
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

    draw_state_t& draw_state();
    const draw_state_t& draw_state() const;

private:
    const std::shared_ptr<const software_shader::program_t> m_program;
    software_shader::bindings_t m_bindings;
    draw_state_t m_draw_state;
    std::unordered_map<std::uint32_t, std::shared_ptr<texture::texture_t>> m_textures;
    std::unordered_map<std::uint32_t, std::shared_ptr<texture::sampler_t>> m_samplers;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

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
        out = std::format_to(out, ", draw_state: {}", material.draw_state());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MATERIAL_H
