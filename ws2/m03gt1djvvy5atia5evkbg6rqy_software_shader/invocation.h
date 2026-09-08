#ifndef M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_INVOCATION_H
# define M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_INVOCATION_H

# include "value.h"

# include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>

# include <algorithm>
# include <cstddef>
# include <cstdint>
# include <format>
# include <functional>
# include <optional>
# include <stdexcept>
# include <type_traits>
# include <unordered_map>
# include <utility>
# include <variant>
# include <vector>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
namespace texture = m03gt0l0q3l4b1k27eab5k7py1_texture;

/**
 * @brief Holds the values and CPU resources visible to shader invocations.
 *
 * Uniform, texture, and sampler indices occupy separate namespaces. Uniform values are owned by the bindings. Textures and samplers are borrowed and must outlive every `run()` that can use their binding.
 */
class bindings_t {
public:
    template <shader::shader_value T>
    void uniform(std::uint32_t binding, T uniform);

    template <shader::shader_value T>
    std::remove_cvref_t<T> uniform(std::uint32_t binding) const;

    /**
     * @brief Borrows the current uniform value; missing bindings fail.
     *
     * The reference observes subsequent replacement at this binding. It remains
     * valid until the provider is assigned, moved from, or destroyed.
     * Preparation copies the value and retains no reference to it.
     */
    const value_t& uniform_value(std::uint32_t binding) const;

    void texture(std::uint32_t binding, const texture::texture_t& texture);
    void texture(std::uint32_t binding, texture::texture_t&& texture) = delete;
    void texture(std::uint32_t binding, const texture::texture_t&& texture) = delete;
    const texture::texture_t& texture(std::uint32_t binding) const;
    void clear_texture(std::uint32_t binding);

    void sampler(std::uint32_t binding, const texture::sampler_t& sampler);
    void sampler(std::uint32_t binding, texture::sampler_t&& sampler) = delete;
    void sampler(std::uint32_t binding, const texture::sampler_t&& sampler) = delete;
    const texture::sampler_t& sampler(std::uint32_t binding) const;
    void clear_sampler(std::uint32_t binding);

private:
    std::unordered_map<std::uint32_t, value_t> m_uniforms;
    std::unordered_map<std::uint32_t, std::reference_wrapper<const texture::texture_t>> m_textures;
    std::unordered_map<std::uint32_t, std::reference_wrapper<const texture::sampler_t>> m_samplers;
};

/**
 * @brief Carries typed vertex inputs, built-ins, and results for one invocation at a time.
 */
class vertex_io_t {
public:
    vertex_io_t(std::int32_t vertex_index, std::int32_t instance_index);

    /**
     * @brief Starts reuse with new indices, empty inputs, and empty results while preserving matrix state.
     */
    void reset(std::int32_t vertex_index, std::int32_t instance_index);

    std::int32_t vertex_index() const;
    std::int32_t instance_index() const;

    void object_to_world(shader::matrix_t<float, 4, 4> matrix);
    shader::matrix_t<float, 4, 4> object_to_world() const;

    void world_to_clip(shader::matrix_t<float, 4, 4> matrix);
    shader::matrix_t<float, 4, 4> world_to_clip() const;

    template <shader::shader_value T>
    void input(std::uint32_t location, T input);

    template <shader::shader_value T>
    std::remove_cvref_t<T> input(std::uint32_t location) const;

    template <shader::shader_value T>
    void output(std::uint32_t location, T output);

    template <shader::shader_value T>
    std::optional<std::remove_cvref_t<T>> output(std::uint32_t location) const;

    void position(shader::vector_t<float, 4> position);
    shader::vector_t<float, 4> position() const;

    void clear_results();

    /** @brief Reserves numbered-output storage without changing inputs or results. */
    void reserve_outputs(std::size_t count);

private:
    std::int32_t m_vertex_index;
    std::int32_t m_instance_index;
    shader::matrix_t<float, 4, 4> m_object_to_world;
    shader::matrix_t<float, 4, 4> m_world_to_clip;
    std::vector<std::pair<std::uint32_t, value_t>> m_inputs;
    std::vector<std::pair<std::uint32_t, value_t>> m_outputs;
    std::optional<shader::vector_t<float, 4>> m_position;
};

/**
 * @brief Carries typed fragment inputs, built-ins, and results for one invocation at a time.
 */
class fragment_io_t {
public:
    fragment_io_t(shader::vector_t<float, 4> fragment_coordinate, bool front_facing);

    /**
     * @brief Starts reuse with new built-ins and empty inputs and results.
     */
    void reset(shader::vector_t<float, 4> fragment_coordinate, bool front_facing);

    shader::vector_t<float, 4> fragment_coordinate() const;
    bool front_facing() const;

    template <shader::shader_value T>
    void input(std::uint32_t location, T input);

    template <shader::shader_value T>
    std::remove_cvref_t<T> input(std::uint32_t location) const;

    template <shader::shader_value T>
    void output(std::uint32_t location, T output);

    template <shader::shader_value T>
    std::optional<std::remove_cvref_t<T>> output(std::uint32_t location) const;

    void color(shader::vector_t<float, 4> color);
    std::optional<shader::vector_t<float, 4>> color() const;

    bool discarded() const;
    void discard();
    void clear_results();

    /** @brief Reserves numbered-output storage without changing inputs or results. */
    void reserve_outputs(std::size_t count);

private:
    shader::vector_t<float, 4> m_fragment_coordinate;
    bool m_front_facing;
    bool m_discarded;
    std::vector<std::pair<std::uint32_t, value_t>> m_inputs;
    std::vector<std::pair<std::uint32_t, value_t>> m_outputs;
    std::optional<shader::vector_t<float, 4>> m_color;
};


} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::bindings_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::vertex_io_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::fragment_io_t>;

} // namespace std

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

template <shader::shader_value T>
void bindings_t::uniform(std::uint32_t binding, T uniform) {
    using type_t = std::remove_cvref_t<T>;
    m_uniforms.insert_or_assign(binding, value_t(type_t(std::move(uniform))));
}

template <shader::shader_value T>
std::remove_cvref_t<T> bindings_t::uniform(std::uint32_t binding) const {
    const auto& stored_uniform = uniform_value(binding);
    const auto* uniform = std::get_if<std::remove_cvref_t<T>>(&stored_uniform);
    if (!uniform) {
        throw std::invalid_argument(std::format("software shader uniform binding {} has type {}; expected {}", binding, std::visit([]<typename V>(const V&) { return shader::shader_data_type<V>(); }, stored_uniform), shader::shader_data_type<std::remove_cvref_t<T>>()));
    }
    return *uniform;
}

template <shader::shader_value T>
void vertex_io_t::input(std::uint32_t location, T input) {
    const auto entry = std::ranges::find(m_inputs, location, &std::pair<std::uint32_t, value_t>::first);
    if (entry == m_inputs.end()) {
        m_inputs.emplace_back(location, value_t(std::move(input)));
    } else {
        entry->second = std::move(input);
    }
}

template <shader::shader_value T>
std::remove_cvref_t<T> vertex_io_t::input(std::uint32_t location) const {
    const auto entry = std::ranges::find(m_inputs, location, &std::pair<std::uint32_t, value_t>::first);
    if (entry == m_inputs.end()) {
        throw std::invalid_argument(std::format("software shader vertex input location {} is missing", location));
    }
    const auto* input = std::get_if<std::remove_cvref_t<T>>(&entry->second);
    if (!input) {
        throw std::invalid_argument(std::format("software shader vertex input location {} has type {}; expected {}", location, std::visit([]<typename V>(const V&) { return shader::shader_data_type<V>(); }, entry->second), shader::shader_data_type<std::remove_cvref_t<T>>()));
    }
    return *input;
}

template <shader::shader_value T>
void vertex_io_t::output(std::uint32_t location, T output) {
    const auto entry = std::ranges::find(m_outputs, location, &std::pair<std::uint32_t, value_t>::first);
    if (entry == m_outputs.end()) {
        m_outputs.emplace_back(location, value_t(std::move(output)));
    } else {
        entry->second = std::move(output);
    }
}

template <shader::shader_value T>
std::optional<std::remove_cvref_t<T>> vertex_io_t::output(std::uint32_t location) const {
    const auto entry = std::ranges::find(m_outputs, location, &std::pair<std::uint32_t, value_t>::first);
    if (entry == m_outputs.end()) {
        return std::nullopt;
    }
    const auto* output = std::get_if<std::remove_cvref_t<T>>(&entry->second);
    if (!output) {
        throw std::invalid_argument(std::format("software shader vertex output location {} has type {}; expected {}", location, std::visit([]<typename V>(const V&) { return shader::shader_data_type<V>(); }, entry->second), shader::shader_data_type<std::remove_cvref_t<T>>()));
    }
    return *output;
}

template <shader::shader_value T>
void fragment_io_t::input(std::uint32_t location, T input) {
    const auto entry = std::ranges::find(m_inputs, location, &std::pair<std::uint32_t, value_t>::first);
    if (entry == m_inputs.end()) {
        m_inputs.emplace_back(location, value_t(std::move(input)));
    } else {
        entry->second = std::move(input);
    }
}

template <shader::shader_value T>
std::remove_cvref_t<T> fragment_io_t::input(std::uint32_t location) const {
    const auto entry = std::ranges::find(m_inputs, location, &std::pair<std::uint32_t, value_t>::first);
    if (entry == m_inputs.end()) {
        throw std::invalid_argument(std::format("software shader fragment input location {} is missing", location));
    }
    const auto* input = std::get_if<std::remove_cvref_t<T>>(&entry->second);
    if (!input) {
        throw std::invalid_argument(std::format("software shader fragment input location {} has type {}; expected {}", location, std::visit([]<typename V>(const V&) { return shader::shader_data_type<V>(); }, entry->second), shader::shader_data_type<std::remove_cvref_t<T>>()));
    }
    return *input;
}

template <shader::shader_value T>
void fragment_io_t::output(std::uint32_t location, T output) {
    const auto entry = std::ranges::find(m_outputs, location, &std::pair<std::uint32_t, value_t>::first);
    if (entry == m_outputs.end()) {
        m_outputs.emplace_back(location, value_t(std::move(output)));
    } else {
        entry->second = std::move(output);
    }
}

template <shader::shader_value T>
std::optional<std::remove_cvref_t<T>> fragment_io_t::output(std::uint32_t location) const {
    const auto entry = std::ranges::find(m_outputs, location, &std::pair<std::uint32_t, value_t>::first);
    if (entry == m_outputs.end()) {
        return std::nullopt;
    }
    const auto* output = std::get_if<std::remove_cvref_t<T>>(&entry->second);
    if (!output) {
        throw std::invalid_argument(std::format("software shader fragment output location {} has type {}; expected {}", location, std::visit([]<typename V>(const V&) { return shader::shader_data_type<V>(); }, entry->second), shader::shader_data_type<std::remove_cvref_t<T>>()));
    }
    return *output;
}

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::bindings_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto iterator = ctx.begin();
        if (iterator != ctx.end() && *iterator != '}') {
            throw std::format_error("invalid bindings_t format specifier");
        }
        return iterator;
    }

    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::bindings_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "software shader bindings");
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::vertex_io_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto iterator = ctx.begin();
        if (iterator != ctx.end() && *iterator != '}') {
            throw std::format_error("invalid vertex_io_t format specifier");
        }
        return iterator;
    }

    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::vertex_io_t& io, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "vertex_index: {}", io.vertex_index());
        out = std::format_to(out, ", instance_index: {}", io.instance_index());
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::fragment_io_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto iterator = ctx.begin();
        if (iterator != ctx.end() && *iterator != '}') {
            throw std::format_error("invalid fragment_io_t format specifier");
        }
        return iterator;
    }

    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::fragment_io_t& io, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "fragment_coordinate: {}", io.fragment_coordinate());
        out = std::format_to(out, ", front_facing: {}", io.front_facing());
        out = std::format_to(out, ", discarded: {}", io.discarded());
        out = std::format_to(out, " }}");
        return out;
    }
};


} // namespace std

#endif // M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_INVOCATION_H
