#include "geometry.h"

#include <format>
#include <stdexcept>
#include <utility>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

geometry_t::geometry_t(std::shared_ptr<index_buffer_t> index_buffer):
    m_index_buffer(std::move(index_buffer)),
    m_primitive_topology(vertex_primitive_topology_t::triangle)
{
    if (!m_index_buffer) {
        throw std::invalid_argument("geometry_t: index_buffer is not set");
    }

    m_index_range = {0, m_index_buffer->indices().size()};
}

geometry_t::geometry_t(std::shared_ptr<index_buffer_t> index_buffer, index_range_t index_range):
    m_index_buffer(std::move(index_buffer)),
    m_primitive_topology(vertex_primitive_topology_t::triangle),
    m_index_range(index_range)
{
    if (!m_index_buffer) {
        throw std::invalid_argument("geometry_t: index_buffer is not set");
    }

    const auto index_count = m_index_buffer->indices().size();
    if (index_count < m_index_range.offset || index_count - m_index_range.offset < m_index_range.count) {
        throw std::out_of_range(std::format("geometry_t: index_range ({}) is out of bounds for index_buffer indices size ({})", m_index_range, index_count));
    }
}

void geometry_t::validate() const {
    if (!m_mesh) {
        throw std::runtime_error("geometry_t::validate: mesh is not set");
    }

    const auto indices = this->indices();

    if (indices.empty()) {
        throw std::runtime_error("geometry_t::validate: does not support geometry with no indices");
    }

    std::size_t expected_index_count_divisor = 1;
    std::size_t expected_minimum_index_count = 0;
    switch (m_primitive_topology) {
        case vertex_primitive_topology_t::point: {
            expected_index_count_divisor = 1;
            expected_minimum_index_count = 1;
        } break;
        case vertex_primitive_topology_t::line: {
            expected_index_count_divisor = 2;
            expected_minimum_index_count = 2;
        } break;
        case vertex_primitive_topology_t::line_strip: {
            expected_index_count_divisor = 1;
            expected_minimum_index_count = 2;
        } break;
        case vertex_primitive_topology_t::line_loop: {
            expected_index_count_divisor = 1;
            expected_minimum_index_count = 2;
        } break;
        case vertex_primitive_topology_t::triangle: {
            expected_index_count_divisor = 3;
            expected_minimum_index_count = 3;
        } break;
        case vertex_primitive_topology_t::triangle_strip: {
            expected_index_count_divisor = 1;
            expected_minimum_index_count = 3;
        } break;
        case vertex_primitive_topology_t::triangle_fan: {
            expected_index_count_divisor = 1;
            expected_minimum_index_count = 3;
        } break;
        default: {
            throw std::runtime_error(std::format("geometry_t::validate: unknown vertex_primitive_topology_t: {}", m_primitive_topology));
        }
    }

    if (indices.size() < expected_minimum_index_count) {
        throw std::runtime_error(std::format("geometry_t::validate: index count ({}) is less than expected minimum index count ({}) for vertex_primitive_topology_t: {}", indices.size(), expected_minimum_index_count, m_primitive_topology));
    }

    if (indices.size() % expected_index_count_divisor != 0) {
        throw std::runtime_error(std::format("geometry_t::validate: index count ({}) is not divisible by expected index count divisor ({}) for vertex_primitive_topology_t: {}", indices.size(), expected_index_count_divisor, m_primitive_topology));
    }

    const auto& vertex_streams = m_mesh->vertex_streams();
    if (vertex_streams.size() == 0) {
        throw std::runtime_error("geometry_t::validate: does not support mesh with no vertex streams");
    }
    const auto vertex_count = m_mesh->number_of_vertices();
    for (auto index : indices) {
        if (vertex_count <= index) {
            throw std::runtime_error(std::format("geometry_t::validate: index ({}) is out of bounds for vertex count ({})", index, vertex_count));
        }
    }
}

std::shared_ptr<mesh_t>& geometry_t::mesh() {
    return m_mesh;
}

std::shared_ptr<mesh_t> geometry_t::mesh() const {
    return m_mesh;
}

std::shared_ptr<index_buffer_t> geometry_t::index_buffer() const {
    return m_index_buffer;
}

index_range_t geometry_t::index_range() const {
    return m_index_range;
}

std::span<const index_buffer_t::index_t> geometry_t::indices() const {
    const auto& indices = static_cast<const index_buffer_t&>(*m_index_buffer).indices();
    const auto index_count = indices.size();
    if (index_count < m_index_range.offset || index_count - m_index_range.offset < m_index_range.count) {
        throw std::out_of_range(std::format("geometry_t::indices: index_range ({}) is out of bounds for index_buffer indices size ({})", m_index_range, index_count));
    }
    return std::span<const index_buffer_t::index_t>(indices).subspan(m_index_range.offset, m_index_range.count);
}

vertex_primitive_topology_t& geometry_t::primitive_topology() {
    return m_primitive_topology;
}

vertex_primitive_topology_t geometry_t::primitive_topology() const {
    return m_primitive_topology;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
