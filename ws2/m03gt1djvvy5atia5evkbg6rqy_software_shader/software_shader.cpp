#include "software_shader.h"

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

execution_context_t::execution_context_t() = default;

std::size_t execution_context_t::slot_capacity() const { return m_slots.capacity(); }
std::size_t execution_context_t::local_capacity() const { return m_local_initialized.capacity(); }

void execution_context_t::execute(const stage_code_t& code, const bindings_t& bindings, vertex_io_t& io) {
    execute_stage(code, bindings, io, m_slots, m_local_initialized);
}

void execution_context_t::execute(const stage_code_t& code, const bindings_t& bindings, fragment_io_t& io) {
    execute_stage(code, bindings, io, m_slots, m_local_initialized);
}

program_t::program_t(shader::shader_ast_t vertex, shader::shader_ast_t fragment):
    m_vertex((validate_program_link(vertex, fragment), vertex)),
    m_fragment(fragment)
{
}

const shader::shader_interface_t& program_t::vertex_interface() const { return m_vertex.interface(); }
const shader::shader_interface_t& program_t::fragment_interface() const { return m_fragment.interface(); }

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
    context.execute(m_vertex, bindings, io);
}

void program_t::run(const bindings_t& bindings, fragment_io_t& io, execution_context_t& context) const {
    context.execute(m_fragment, bindings, io);
}

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader
