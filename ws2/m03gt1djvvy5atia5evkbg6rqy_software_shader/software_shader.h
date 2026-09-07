#ifndef M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_SOFTWARE_SHADER_H
# define M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_SOFTWARE_SHADER_H

# include "helpers.h"
# include "invocation.h"

# include <cstddef>
# include <cstdint>
# include <format>
# include <vector>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

/**
 * @brief Owns reusable storage for sequential shader invocations across stages and programs.
 *
 * Each simultaneous invocation needs independent context and IO. Storage grows when
 * required and retains its capacity. Locals are logically fresh on every run;
 * temporary storage need not be cleared. Failure leaves the context reusable.
 * No program, binding, resource, or IO references are retained after a call.
 */
class execution_context_t {
public:
    execution_context_t();

    std::size_t slot_capacity() const;
    std::size_t local_capacity() const;

    // Module-local execution entry points for immutable, AST-compiled stages.
    // A stage/IO mismatch fails with cleared results.
    void execute(const stage_code_t& code, const bindings_t& bindings, vertex_io_t& io);
    void execute(const stage_code_t& code, const bindings_t& bindings, fragment_io_t& io);

private:
    std::vector<value_t> m_slots;
    std::vector<std::uint8_t> m_local_initialized;
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
    program_t(shader::shader_ast_t vertex, shader::shader_ast_t fragment);

    program_t(const program_t&) = delete;
    program_t& operator=(const program_t&) = delete;
    program_t(program_t&&) = default;
    program_t& operator=(program_t&&) = default;

    const shader::shader_interface_t& vertex_interface() const;
    const shader::shader_interface_t& fragment_interface() const;

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
};

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::execution_context_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::program_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::execution_context_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::execution_context_t& execution_context, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "slot_capacity={} local_capacity={}", execution_context.slot_capacity(), execution_context.local_capacity());
        return out;
    }
};

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


} // namespace std

#endif // M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_SOFTWARE_SHADER_H
