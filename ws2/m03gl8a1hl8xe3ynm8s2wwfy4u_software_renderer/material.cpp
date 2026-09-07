#include "material.h"
#include "helpers.h"

#include <cstddef>
#include <format>
#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

color_mask_t operator|(color_mask_t left, color_mask_t right) {
    return static_cast<color_mask_t>(std::to_underlying(left) | std::to_underlying(right));
}

color_mask_t operator&(color_mask_t left, color_mask_t right) {
    return static_cast<color_mask_t>(std::to_underlying(left) & std::to_underlying(right));
}

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

void material_t::blend(bool enabled) {
    m_blend = enabled;
}

bool material_t::blend() const {
    return m_blend;
}

void material_t::blend_color(blend_equation_t blend_equation) {
    validate_blend_equation(blend_equation);
    m_blend_color = blend_equation;
}

blend_equation_t material_t::blend_color() const {
    return m_blend_color;
}

void material_t::blend_alpha(blend_equation_t blend_equation) {
    validate_blend_equation(blend_equation);
    m_blend_alpha = blend_equation;
}

blend_equation_t material_t::blend_alpha() const {
    return m_blend_alpha;
}

void material_t::blend_constant(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4>& blend_constant) {
    for (std::size_t component = 0; component < 4; ++component) {
        m_blend_constant[component] = sanitize_unorm(blend_constant[component]);
    }
}

const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4>& material_t::blend_constant() const {
    return m_blend_constant;
}

void material_t::color_write(color_mask_t color_mask) {
    if ((std::to_underlying(color_mask) & ~std::to_underlying(color_mask_t::all)) != 0) {
        throw std::invalid_argument(std::format("material_t::color_write rejects invalid mask {}", color_mask));
    }
    m_color_write = color_mask;
}

color_mask_t material_t::color_write() const {
    return m_color_write;
}

void material_t::validate_blend_equation(const blend_equation_t& blend_equation) {
    for (const auto factor : {blend_equation.source, blend_equation.destination}) {
        switch (factor) {
            case blend_factor_t::zero:
            case blend_factor_t::one:
            case blend_factor_t::src_color:
            case blend_factor_t::one_minus_src_color:
            case blend_factor_t::dst_color:
            case blend_factor_t::one_minus_dst_color:
            case blend_factor_t::src_alpha:
            case blend_factor_t::one_minus_src_alpha:
            case blend_factor_t::dst_alpha:
            case blend_factor_t::one_minus_dst_alpha:
            case blend_factor_t::constant_color:
            case blend_factor_t::one_minus_constant_color:
            case blend_factor_t::constant_alpha:
            case blend_factor_t::one_minus_constant_alpha:
            case blend_factor_t::src_alpha_saturate:
                break;
            default: {
                throw std::invalid_argument(std::format("material_t rejects invalid blend factor in {}", blend_equation));
            }
        }
    }
    switch (blend_equation.operation) {
        case blend_op_t::add:
        case blend_op_t::subtract:
        case blend_op_t::reverse_subtract:
        case blend_op_t::min:
        case blend_op_t::max:
            break;
        default: {
            throw std::invalid_argument(std::format("material_t rejects invalid blend operation in {}", blend_equation));
        }
    }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
