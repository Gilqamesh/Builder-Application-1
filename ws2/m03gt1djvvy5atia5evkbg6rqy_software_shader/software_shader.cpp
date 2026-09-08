#include "software_shader.h"

#include <algorithm>
#include <utility>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

program_t::program_t(shader::shader_ast_t vertex, shader::shader_ast_t fragment):
    m_vertex((validate_program_link(vertex, fragment), vertex)),
    m_fragment(fragment)
{
    for (const auto& input : fragment_interface().inputs()) {
        const auto outputs = vertex_interface().outputs();
        const auto output = std::ranges::find(outputs, input.index, &shader::shader_interface_element_t::index);
        m_fragment_sources.push_back(static_cast<std::size_t>(output - outputs.begin()));
    }
}

const shader::shader_interface_t& program_t::vertex_interface() const { return m_vertex.interface(); }

const shader::shader_interface_t& program_t::fragment_interface() const { return m_fragment.interface(); }

std::span<const std::size_t> program_t::fragment_sources() const { return m_fragment_sources; }

void program_t::validate_bindings(const bindings_t& bindings) const {
    validate_interface_bindings(m_vertex.interface(), bindings);
    validate_interface_bindings(m_fragment.interface(), bindings);
}

void program_t::run(const bindings_t& bindings, vertex_io_t& io) const {
    execution_context_t context;
    run(bindings, io, context);
}

void program_t::run(const bindings_t& bindings, fragment_io_t& io) const {
    execution_context_t context;
    run(bindings, io, context);
}

void program_t::run(const bindings_t& bindings, vertex_io_t& io, execution_context_t& context) const {
    context.run(*this, bindings, io);
}

void program_t::run(const bindings_t& bindings, fragment_io_t& io, execution_context_t& context) const {
    context.run(*this, bindings, io);
}

program_t::execution_context_t::execution_context_t() = default;

std::size_t program_t::execution_context_t::slot_capacity() const { return m_slots.capacity(); }

std::size_t program_t::execution_context_t::local_capacity() const { return m_local_initialized.capacity(); }

void program_t::execution_context_t::run(const program_t& program, const bindings_t& bindings, vertex_io_t& io) {
    execute_stage(program.m_vertex, &bindings, io, m_slots, m_local_initialized, nullptr);
}

void program_t::execution_context_t::run(const program_t& program, const bindings_t& bindings, fragment_io_t& io) {
    execute_stage(program.m_fragment, &bindings, io, m_slots, m_local_initialized, nullptr);
}

program_t::execution_context_t::prepared_t::prepared_t() = default;

program_t::execution_context_t::prepared_t& program_t::execution_context_t::prepared_t::operator=(const prepared_t& other) {
    if (this != &other) {
        prepared_t copy(other);
        *this = std::move(copy);
    }
    return *this;
}

program_t::execution_context_t::prepared_t::prepared_t(prepared_t&& other) noexcept:
    m_program(std::exchange(other.m_program, nullptr)),
    m_vertex_bindings(std::move(other.m_vertex_bindings)),
    m_fragment_bindings(std::move(other.m_fragment_bindings))
{
}

program_t::execution_context_t::prepared_t& program_t::execution_context_t::prepared_t::operator=(prepared_t&& other) noexcept {
    if (this != &other) {
        m_program = std::exchange(other.m_program, nullptr);
        m_vertex_bindings = std::move(other.m_vertex_bindings);
        m_fragment_bindings = std::move(other.m_fragment_bindings);
    }
    return *this;
}

void program_t::execution_context_t::prepared_t::reset() {
    m_program = nullptr;
    m_vertex_bindings.clear();
    m_fragment_bindings.clear();
}

void program_t::execution_context_t::prepared_t::run(std::span<const value_t> inputs, std::span<std::optional<value_t>> outputs, vertex_io_t& io, execution_context_t& context) const {
    if (!m_program) {
        io.clear_results();
        std::ranges::fill(outputs, std::nullopt);
        throw std::logic_error("prepared shader run requires successful preparation");
    }
    const prepared_invocation_t invocation {m_vertex_bindings, inputs, outputs};
    execute_stage(m_program->m_vertex, nullptr, io, context.m_slots, context.m_local_initialized, &invocation);
}

void program_t::execution_context_t::prepared_t::run(std::span<const value_t> inputs, std::span<std::optional<value_t>> outputs, fragment_io_t& io, execution_context_t& context) const {
    if (!m_program) {
        io.clear_results();
        std::ranges::fill(outputs, std::nullopt);
        throw std::logic_error("prepared shader run requires successful preparation");
    }
    const prepared_invocation_t invocation {m_fragment_bindings, inputs, outputs};
    execute_stage(m_program->m_fragment, nullptr, io, context.m_slots, context.m_local_initialized, &invocation);
}

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader
