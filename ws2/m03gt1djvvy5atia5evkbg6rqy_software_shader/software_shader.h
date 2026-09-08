#ifndef M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_SOFTWARE_SHADER_H
# define M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_SOFTWARE_SHADER_H

# include "helpers.h"
# include "invocation.h"

# include <concepts>
# include <cstddef>
# include <variant>
# include <cstdint>
# include <format>
# include <optional>
# include <span>
# include <stdexcept>
# include <vector>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

/**
 * @brief Borrows current uniform values and resources during preparation.
 *
 * Uniform references need remain valid only until copied during prepare();
 * preparation retains their values, never references to provider uniform storage.
 */
template <typename T>
concept binding_provider = requires(const T& bindings, std::uint32_t location) {
    { bindings.uniform_value(location) } -> std::same_as<const value_t&>;
    { bindings.texture(location) } -> std::same_as<const texture::texture_t&>;
    { bindings.sampler(location) } -> std::same_as<const texture::sampler_t&>;
};

/**
 * @brief Owns two immutable compiled shader stages and their reflected interfaces.
 *
 * Compilation borrows the ASTs only during construction. Successful construction
 * establishes stage/link compatibility and executable code; expression evaluation
 * and invocation-dependent failures remain runtime behavior.
 */
class program_t {
public:
    class execution_context_t;

    program_t(shader::shader_ast_t vertex, shader::shader_ast_t fragment);

    program_t(const program_t&) = delete;
    program_t& operator=(const program_t&) = delete;
    program_t(program_t&&) = default;
    program_t& operator=(program_t&&) = default;

    const shader::shader_interface_t& vertex_interface() const;
    const shader::shader_interface_t& fragment_interface() const;
    /** @brief Maps each reflected fragment input to its linked vertex output index. */
    std::span<const std::size_t> fragment_sources() const;

    /** @brief Validates reflected bindings in both stages, accepting unused extras. */
    void validate_bindings(const bindings_t& bindings) const;

    /**
     * @brief Runs a fresh vertex invocation using temporary execution storage.
     *
     * Preserves caller inputs, indices, and matrices. Validates the invoked stage's
     * inputs and bindings. Normal completion requires position. Results are cleared
     * before any potentially failing preparation and again on failure.
     */
    void run(const bindings_t& bindings, vertex_io_t& io) const;

    /**
     * @brief Runs a fresh fragment invocation using temporary execution storage.
     *
     * Preserves caller inputs and built-ins. Validates the invoked stage's inputs
     * and bindings. Color and numbered outputs are optional. Discard terminates
     * execution and invalidates all outputs. Results are cleared before any
     * potentially failing preparation and again on failure.
     */
    void run(const bindings_t& bindings, fragment_io_t& io) const;

    /**
     * @brief Runs a vertex invocation with reusable execution storage.
     *
     * Follows the convenience overload's result and validation rules. Successful
     * invocations allocate no execution or IO storage once both have sufficient
     * capacity. Preparation reserves all reflected numbered outputs, including
     * outputs on untaken paths. This guarantee does not cover exception construction
     * or resource operations. Context, IO, and bindings are borrowed for this call.
     */
    void run(const bindings_t& bindings, vertex_io_t& io, execution_context_t& context) const;

    /** @brief Runs a fragment invocation with the same storage-reuse guarantee and its convenience overload's semantics. */
    void run(const bindings_t& bindings, fragment_io_t& io, execution_context_t& context) const;

private:
    stage_code_t m_vertex;
    stage_code_t m_fragment;
    std::vector<std::size_t> m_fragment_sources;
};

/**
 * @brief Owns reusable storage for sequential shader invocations across stages and programs.
 *
 * Each simultaneous invocation needs independent context and IO. Storage grows when
 * required and retains its capacity. Locals are logically fresh on every run;
 * temporary storage need not be cleared. Failure leaves the context reusable.
 * No program, binding, resource, or IO references are retained after a call.
 */
class program_t::execution_context_t {
public:
    class prepared_t;

    execution_context_t();

    std::size_t slot_capacity() const;
    std::size_t local_capacity() const;

    /** @brief Runs the selected program stage with validated location-based IO and current bindings. */
    void run(const program_t& program, const bindings_t& bindings, vertex_io_t& io);
    void run(const program_t& program, const bindings_t& bindings, fragment_io_t& io);

private:
    std::vector<value_t> m_slots;
    std::vector<std::uint8_t> m_local_initialized;
};

/**
 * @brief Resolves immutable program bindings for repeated invocations with indexed IO.
 *
 * prepare() copies uniform values and borrows the program, textures and samplers.
 * Keep these objects alive and unmoved until the next preparation or destruction;
 * changes to source bindings require preparation again. Texture contents remain live.
 * The binding provider is borrowed only during prepare(); no references to its
 * uniform storage are retained. Preparation reuses
 * binding capacity; once sufficient, it allocates no binding storage. This excludes
 * allocations inside provider getters and exception construction.
 * Copies own independent uniform snapshots and share the same resource borrows.
 * Copy assignment preserves the previous snapshot on failure.
 * Moving leaves the source unprepared. Failed preparation leaves this object
 * unprepared, with retained storage capacity.
 *
 * Inputs and numbered outputs follow their stage's reflection order. The caller
 * supplies every input with its declared type; no location search or full input
 * validation is performed. Output entries are empty when unwritten. Built-ins and
 * special results use the supplied IO; its numbered inputs and outputs are unused.
 * Result invalidation, discard, local freshness and warmed execution-storage rules
 * are the same as program_t::run(). Each concurrent invocation needs independent
 * context, IO and output storage. Preparation and execution are not concurrent.
 */
class program_t::execution_context_t::prepared_t {
public:
    prepared_t();
    prepared_t(const prepared_t&) = default;
    prepared_t& operator=(const prepared_t& other);
    prepared_t(prepared_t&& other) noexcept;
    prepared_t& operator=(prepared_t&& other) noexcept;

    /** @brief Resolves current bindings, retaining capacity across reset and preparation. */
    template <binding_provider T>
    void prepare(const program_t& program, const T& bindings);
    /** @brief Releases all borrows and retains binding storage for reuse. */
    void reset();
    void run(std::span<const value_t> inputs, std::span<std::optional<value_t>> outputs, vertex_io_t& io, execution_context_t& context) const;
    void run(std::span<const value_t> inputs, std::span<std::optional<value_t>> outputs, fragment_io_t& io, execution_context_t& context) const;

private:
    template <binding_provider T>
    static void resolve(const shader::shader_interface_t& interface, const T& bindings, std::vector<binding_value_t>& resolved);

    const program_t* m_program = nullptr;
    std::vector<binding_value_t> m_vertex_bindings;
    std::vector<binding_value_t> m_fragment_bindings;
};

using execution_context_t = program_t::execution_context_t;
using prepared_program_t = execution_context_t::prepared_t;

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t>;
template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t::execution_context_t>;
template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t::execution_context_t::prepared_t>;

} // namespace std

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

template <binding_provider T>
void program_t::execution_context_t::prepared_t::prepare(const program_t& program, const T& bindings) {
    reset();
    try {
        resolve(program.vertex_interface(), bindings, m_vertex_bindings);
        resolve(program.fragment_interface(), bindings, m_fragment_bindings);
        m_program = &program;
    } catch (...) {
        reset();
        throw;
    }
}

template <binding_provider T>
void program_t::execution_context_t::prepared_t::resolve(const shader::shader_interface_t& interface, const T& bindings, std::vector<binding_value_t>& resolved) {
    resolved.clear();
    resolved.reserve(interface.bindings().size());
    for (const auto& binding : interface.bindings()) {
        switch (binding.type.category()) {
            case shader::shader_data_category_t::texture_2d: { resolved.emplace_back(&bindings.texture(binding.index)); } break;
            case shader::shader_data_category_t::sampler: { resolved.emplace_back(&bindings.sampler(binding.index)); } break;
            default: {
                const auto& uniform = bindings.uniform_value(binding.index);
                const auto type = std::visit([]<typename V>(const V&) { return shader::shader_data_type<V>(); }, uniform);
                if (type != binding.type) {
                    throw std::invalid_argument(std::format("prepared shader uniform binding {} has the wrong type", binding.index));
                }
                resolved.emplace_back(uniform);
            } break;
        }
    }
}

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto iterator = ctx.begin();
        if (iterator != ctx.end() && *iterator != '}') {
            throw std::format_error("invalid program_t format specifier");
        }
        return iterator;
    }

    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t& program, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "vertex_inputs: {}", program.vertex_interface().inputs().size());
        out = std::format_to(out, ", vertex_outputs: {}", program.vertex_interface().outputs().size());
        out = std::format_to(out, ", fragment_inputs: {}", program.fragment_interface().inputs().size());
        out = std::format_to(out, ", fragment_outputs: {}", program.fragment_interface().outputs().size());
        out = std::format_to(out, " }}");
        return out;
    }
};


template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t::execution_context_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t::execution_context_t& execution_context, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "slot_capacity={} local_capacity={}", execution_context.slot_capacity(), execution_context.local_capacity());
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t::execution_context_t::prepared_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t::execution_context_t::prepared_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "prepared software shader program");
        return out;
    }
};

} // namespace std

#endif // M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_SOFTWARE_SHADER_H
