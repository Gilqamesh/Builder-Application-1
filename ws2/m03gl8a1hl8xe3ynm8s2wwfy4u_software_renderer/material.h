#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MATERIAL_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MATERIAL_H

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
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

/** @brief Selects a comparison; each use defines its left and right operands. */
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

/** @brief Selects the operation on the full stored stencil byte before write masking. */
enum class stencil_op_t {
    keep,
    zero,
    replace,
    increment_clamp,
    decrement_clamp,
    increment_wrap,
    decrement_wrap,
    invert
};

/**
 * @brief Describes stencil testing for one face, validated by material setters.
 *
 * Compares (reference & compare_mask) against (stored & compare_mask).
 * Replace uses the full reference. Operations use the full stored byte and then
 * merge through write_mask, preserving every masked-off bit. Increment/decrement
 * clamp at 0/255 or wrap modulo 256. Points and lines use the front settings.
 */
struct stencil_state_t {
    comparison_t comparison = comparison_t::always;
    std::uint8_t reference = 0;
    std::uint8_t compare_mask = 255;
    std::uint8_t write_mask = 255;
    stencil_op_t fail = stencil_op_t::keep;
    stencil_op_t depth_fail = stencil_op_t::keep;
    stencil_op_t pass = stencil_op_t::keep;
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
 * @brief Selects a component multiplier from the original source, destination, or constant.
 *
 * Color factors select the corresponding component, including alpha. Alpha factors
 * replicate alpha across RGB. Complements subtract from one.
 */
enum class blend_factor_t {
    zero,
    one,
    src_color,
    one_minus_src_color,
    dst_color,
    one_minus_dst_color,
    src_alpha,
    one_minus_src_alpha,
    dst_alpha,
    one_minus_dst_alpha,
    constant_color,
    one_minus_constant_color,
    constant_alpha,
    one_minus_constant_alpha,
    /** RGB uses min(source alpha, 1 - destination alpha); alpha uses one. */
    src_alpha_saturate
};

/** @brief Combines weighted source/destination components; min/max ignore factors. */
enum class blend_op_t {
    add,
    subtract,
    reverse_subtract,
    min,
    max
};

/**
 * @brief Describes one RGB or alpha equation, validated by the material setter.
 *
 * Add computes source * source factor + destination * destination factor;
 * subtract subtracts the destination term, reverse_subtract subtracts the source
 * term from the destination term. Min/max use the unweighted components.
 */
struct blend_equation_t {
    blend_factor_t source = blend_factor_t::one;
    blend_factor_t destination = blend_factor_t::zero;
    blend_op_t operation = blend_op_t::add;
};

/** @brief Selects stored channels; combine channels with operator|. */
enum class color_mask_t : unsigned {
    none = 0,
    red = 1,
    green = 2,
    blue = 4,
    alpha = 8,
    all = 15
};

color_mask_t operator|(color_mask_t left, color_mask_t right);
color_mask_t operator&(color_mask_t left, color_mask_t right);

/** @brief Selects the original topology vertex supplying flat inputs. */
enum class provoking_vertex_t { first, last };

/**
 * @brief Owns an immutable shader program, bindings, and mutable draw state.
 *
 * Uniform, texture, and sampler locations occupy independent namespaces. Extra
 * bindings not used by the program are accepted. Items sharing this material share
 * its draw state. Distinct materials may share a program, textures, and samplers.
 * Enum and mask setters reject invalid values before changing their properties,
 * including ignored factors and disabled blending. Copies retain valid settings independently.
 * Initial settings disable depth testing, enable depth writes, select less and
 * CCW NDC front faces, and disable culling. Blending starts disabled with one/zero/add
 * equations, a zero constant, and all color channels writable. Clears ignore draw state.
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

    /** @brief Returns the current uniform value; missing bindings and type mismatches fail. */
    template <software_shader::shader::shader_value T>
    std::remove_cvref_t<T> uniform(std::uint32_t location) const;

    /**
     * @brief Replaces an owned texture binding, or removes it when the value is null.
     */
    void texture(std::uint32_t location, std::shared_ptr<texture::texture_t> value);

    /** @brief Borrows the current texture until replacement, removal, or destruction; missing bindings fail. */
    const texture::texture_t& texture(std::uint32_t location) const;

    /**
     * @brief Replaces an owned sampler binding, or removes it when the value is null.
     */
    void sampler(std::uint32_t location, std::shared_ptr<texture::sampler_t> value);

    /** @brief Borrows the current sampler under texture()'s lifetime and failure rules. */
    const texture::sampler_t& sampler(std::uint32_t location) const;

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

    /**
     * @brief Enables late stencil testing and operations; requires an attachment when drawing.
     *
     * Disabled testing bypasses comparisons and writes. Stencil starts disabled;
     * both faces start with stencil_state_t's defaults. Setters validate even when
     * testing is disabled and preserve the previous state on failure.
     */
    void stencil_test(bool enabled);
    bool stencil_test() const;

    void stencil_front(stencil_state_t stencil_state);
    stencil_state_t stencil_front() const;

    void stencil_back(stencil_state_t stencil_state);
    stencil_state_t stencil_back() const;

    void front_face(winding_t winding);
    winding_t front_face() const;

    /**
     * @brief Selects the flat-input source; defaults to first and preserves state on invalid input.
     *
     * Lists and strips select the first/last vertex in assembly order before winding
     * adjustment. A fan triangle (v0, vi, vi+1) selects vi or vi+1. A closing loop
     * segment (v_last, v0) selects v_last or v0. Points select their only vertex.
     * Clipping never changes the selection, including when that vertex is removed.
     */
    void provoking_vertex(provoking_vertex_t provoking_vertex);
    provoking_vertex_t provoking_vertex() const;

    void cull(cull_mode_t mode);
    cull_mode_t cull() const;

    void blend(bool enabled);
    bool blend() const;

    void blend_color(blend_equation_t blend_equation);
    blend_equation_t blend_color() const;

    void blend_alpha(blend_equation_t blend_equation);
    blend_equation_t blend_alpha() const;

    /**
     * @brief Stores a sanitized and clamped linear blend constant.
     *
     * NaN and negative infinity become zero, positive infinity becomes one;
     * finite components clamp to [0,1]. Constants are independent of attachment encoding.
     */
    void blend_constant(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4>& blend_constant);
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4>& blend_constant() const;

    /**
     * @brief Selects channels to write without affecting shader execution, blending inputs, or depth.
     */
    void color_write(color_mask_t color_mask);
    color_mask_t color_write() const;

private:

    static void validate_stencil_state(const stencil_state_t& stencil_state);
    static void validate_blend_equation(const blend_equation_t& blend_equation);

    const std::shared_ptr<const software_shader::program_t> m_program;
    std::unordered_map<std::uint32_t, software_shader::value_t> m_uniforms;
    bool m_depth_test = false;
    bool m_depth_write = true;
    comparison_t m_depth_compare = comparison_t::less;
    bool m_stencil_test = false;
    stencil_state_t m_stencil_front;
    stencil_state_t m_stencil_back;
    winding_t m_front_face = winding_t::counter_clockwise;
    provoking_vertex_t m_provoking_vertex = provoking_vertex_t::first;
    cull_mode_t m_cull = cull_mode_t::none;
    bool m_blend = false;
    blend_equation_t m_blend_color;
    blend_equation_t m_blend_alpha;
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4> m_blend_constant {0, 0, 0, 0};
    color_mask_t m_color_write = color_mask_t::all;
    std::unordered_map<std::uint32_t, std::shared_ptr<texture::texture_t>> m_textures;
    std::unordered_map<std::uint32_t, std::shared_ptr<texture::sampler_t>> m_samplers;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::comparison_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_state_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::winding_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::cull_mode_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_op_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_equation_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_mask_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::provoking_vertex_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::material_t>;

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

template <software_shader::shader::shader_value T>
void material_t::uniform(std::uint32_t location, T value) {
    m_uniforms.insert_or_assign(location, software_shader::value_t(std::move(value)));
}

template <software_shader::shader::shader_value T>
std::remove_cvref_t<T> material_t::uniform(std::uint32_t location) const {
    const auto iterator = m_uniforms.find(location);
    if (iterator == m_uniforms.end()) { throw std::invalid_argument(std::format("material uniform binding {} is missing", location)); }
    const auto* uniform = std::get_if<std::remove_cvref_t<T>>(&iterator->second);
    if (!uniform) { throw std::invalid_argument(std::format("material uniform binding {} has the wrong type", location)); }
    return *uniform;
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
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') { throw std::format_error("invalid stencil_op_t format specifier"); }
        return it;
    }
    auto format(m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t operation, auto& ctx) const {
        auto out = ctx.out();
        switch (operation) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t::keep: { out = std::format_to(out, "keep"); } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t::zero: { out = std::format_to(out, "zero"); } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t::replace: { out = std::format_to(out, "replace"); } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t::increment_clamp: { out = std::format_to(out, "increment_clamp"); } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t::decrement_clamp: { out = std::format_to(out, "decrement_clamp"); } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t::increment_wrap: { out = std::format_to(out, "increment_wrap"); } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t::decrement_wrap: { out = std::format_to(out, "decrement_wrap"); } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_op_t::invert: { out = std::format_to(out, "invert"); } break;
            default: { out = std::format_to(out, "invalid({})", std::to_underlying(operation)); } break;
        }
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_state_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') { throw std::format_error("invalid stencil_state_t format specifier"); }
        return it;
    }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::stencil_state_t& stencil_state, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ comparison: {}", stencil_state.comparison);
        out = std::format_to(out, ", reference: {}", stencil_state.reference);
        out = std::format_to(out, ", compare_mask: {}", stencil_state.compare_mask);
        out = std::format_to(out, ", write_mask: {}", stencil_state.write_mask);
        out = std::format_to(out, ", fail: {}", stencil_state.fail);
        out = std::format_to(out, ", depth_fail: {}", stencil_state.depth_fail);
        out = std::format_to(out, ", pass: {}", stencil_state.pass);
        out = std::format_to(out, " }}");
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
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid blend_factor_t format specifier");
        }
        return it;
    }

    auto format(m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t option, auto& ctx) const {
        auto out = ctx.out();
        switch (option) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::zero: {
                out = std::format_to(out, "zero");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::one: {
                out = std::format_to(out, "one");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::src_color: {
                out = std::format_to(out, "src_color");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::one_minus_src_color: {
                out = std::format_to(out, "one_minus_src_color");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::dst_color: {
                out = std::format_to(out, "dst_color");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::one_minus_dst_color: {
                out = std::format_to(out, "one_minus_dst_color");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::src_alpha: {
                out = std::format_to(out, "src_alpha");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::one_minus_src_alpha: {
                out = std::format_to(out, "one_minus_src_alpha");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::dst_alpha: {
                out = std::format_to(out, "dst_alpha");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::one_minus_dst_alpha: {
                out = std::format_to(out, "one_minus_dst_alpha");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::constant_color: {
                out = std::format_to(out, "constant_color");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::one_minus_constant_color: {
                out = std::format_to(out, "one_minus_constant_color");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::constant_alpha: {
                out = std::format_to(out, "constant_alpha");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::one_minus_constant_alpha: {
                out = std::format_to(out, "one_minus_constant_alpha");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_factor_t::src_alpha_saturate: {
                out = std::format_to(out, "src_alpha_saturate");
            } break;
            default: {
                out = std::format_to(out, "invalid({})", std::to_underlying(option));
            } break;
        }
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_op_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid blend_op_t format specifier");
        }
        return it;
    }

    auto format(m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_op_t option, auto& ctx) const {
        auto out = ctx.out();
        switch (option) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_op_t::add: {
                out = std::format_to(out, "add");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_op_t::subtract: {
                out = std::format_to(out, "subtract");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_op_t::reverse_subtract: {
                out = std::format_to(out, "reverse_subtract");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_op_t::min: {
                out = std::format_to(out, "min");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_op_t::max: {
                out = std::format_to(out, "max");
            } break;
            default: {
                out = std::format_to(out, "invalid({})", std::to_underlying(option));
            } break;
        }
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_equation_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid blend_equation_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::blend_equation_t& blend_equation, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ source: {}", blend_equation.source);
        out = std::format_to(out, ", destination: {}", blend_equation.destination);
        out = std::format_to(out, ", operation: {} }}", blend_equation.operation);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_mask_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid color_mask_t format specifier");
        }
        return it;
    }

    auto format(m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_mask_t color_mask, auto& ctx) const {
        auto out = ctx.out();
        const auto bits = std::to_underlying(color_mask);
        if ((bits & ~15U) != 0) {
            out = std::format_to(out, "invalid({})", bits);
        } else {
            out = std::format_to(out, "{}{}{}{}", bits & 1 ? 'r' : '-', bits & 2 ? 'g' : '-', bits & 4 ? 'b' : '-', bits & 8 ? 'a' : '-');
        }
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::provoking_vertex_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') { throw std::format_error("invalid provoking_vertex_t format specifier"); }
        return it;
    }
    auto format(m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::provoking_vertex_t provoking_vertex, auto& ctx) const {
        auto out = ctx.out();
        switch (provoking_vertex) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::provoking_vertex_t::first: { out = std::format_to(out, "first"); } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::provoking_vertex_t::last: { out = std::format_to(out, "last"); } break;
            default: { out = std::format_to(out, "invalid({})", static_cast<int>(provoking_vertex)); } break;
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
        out = std::format_to(out, ", depth_test: {}", material.depth_test());
        out = std::format_to(out, ", depth_write: {}", material.depth_write());
        out = std::format_to(out, ", depth_compare: {}", material.depth_compare());
        out = std::format_to(out, ", stencil_test: {}", material.stencil_test());
        out = std::format_to(out, ", stencil_front: {}", material.stencil_front());
        out = std::format_to(out, ", stencil_back: {}", material.stencil_back());
        out = std::format_to(out, ", front_face: {}", material.front_face());
        out = std::format_to(out, ", provoking_vertex: {}", material.provoking_vertex());
        out = std::format_to(out, ", cull: {}", material.cull());
        out = std::format_to(out, ", blend: {}", material.blend());
        out = std::format_to(out, ", blend_color: {}", material.blend_color());
        out = std::format_to(out, ", blend_alpha: {}", material.blend_alpha());
        out = std::format_to(out, ", blend_constant: {}", material.blend_constant());
        out = std::format_to(out, ", color_write: {}", material.color_write());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MATERIAL_H
