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

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader
