#include "material.h"

#include <stdexcept>
#include <utility>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

material_t::material_t(std::shared_ptr<const software_shader::program_t> program):
    m_program(std::move(program))
{
    if (!m_program) {
        throw std::invalid_argument("material_t requires a shader program");
    }
}

const std::shared_ptr<const software_shader::program_t>& material_t::program() const {
    return m_program;
}

void material_t::texture(std::uint32_t location, std::shared_ptr<texture::texture_t> value) {
    if (!value) {
        m_bindings.clear_texture(location);
        m_textures.erase(location);
        return;
    }

    auto textures = m_textures;
    auto bindings = m_bindings;
    textures.insert_or_assign(location, std::move(value));
    bindings.texture(location, *textures.at(location));

    static_assert(noexcept(m_textures.swap(textures)));
    static_assert(noexcept(std::swap(m_bindings, bindings)));
    m_textures.swap(textures);
    std::swap(m_bindings, bindings);
}

void material_t::sampler(std::uint32_t location, std::shared_ptr<texture::sampler_t> value) {
    if (!value) {
        m_bindings.clear_sampler(location);
        m_samplers.erase(location);
        return;
    }

    auto samplers = m_samplers;
    auto bindings = m_bindings;
    samplers.insert_or_assign(location, std::move(value));
    bindings.sampler(location, *samplers.at(location));

    static_assert(noexcept(m_samplers.swap(samplers)));
    static_assert(noexcept(std::swap(m_bindings, bindings)));
    m_samplers.swap(samplers);
    std::swap(m_bindings, bindings);
}

const software_shader::bindings_t& material_t::bindings() const {
    return m_bindings;
}

draw_state_t& material_t::draw_state() {
    return m_draw_state;
}

const draw_state_t& material_t::draw_state() const {
    return m_draw_state;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
