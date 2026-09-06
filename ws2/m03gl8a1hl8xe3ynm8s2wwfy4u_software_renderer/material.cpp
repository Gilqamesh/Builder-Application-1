#include "material.h"

#include <format>
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

void material_t::depth_test(bool enabled) {
    m_depth_test = enabled;
}

bool material_t::depth_test() const {
    return m_depth_test;
}

void material_t::depth_write(bool enabled) {
    m_depth_write = enabled;
}

bool material_t::depth_write() const {
    return m_depth_write;
}

void material_t::depth_compare(comparison_t comparison) {
    switch (comparison) {
        case comparison_t::never:
        case comparison_t::less:
        case comparison_t::equal:
        case comparison_t::less_equal:
        case comparison_t::greater:
        case comparison_t::not_equal:
        case comparison_t::greater_equal:
        case comparison_t::always: {
            m_depth_compare = comparison;
        } break;
        default: {
            throw std::invalid_argument(std::format("material_t::depth_compare rejects invalid comparison {}", comparison));
        }
    }
}

comparison_t material_t::depth_compare() const {
    return m_depth_compare;
}

void material_t::front_face(winding_t winding) {
    switch (winding) {
        case winding_t::counter_clockwise:
        case winding_t::clockwise: {
            m_front_face = winding;
        } break;
        default: {
            throw std::invalid_argument(std::format("material_t::front_face rejects invalid winding {}", winding));
        }
    }
}

winding_t material_t::front_face() const {
    return m_front_face;
}

void material_t::cull(cull_mode_t mode) {
    switch (mode) {
        case cull_mode_t::none:
        case cull_mode_t::front:
        case cull_mode_t::back:
        case cull_mode_t::both: {
            m_cull = mode;
        } break;
        default: {
            throw std::invalid_argument(std::format("material_t::cull rejects invalid mode {}", mode));
        }
    }
}

cull_mode_t material_t::cull() const {
    return m_cull;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
