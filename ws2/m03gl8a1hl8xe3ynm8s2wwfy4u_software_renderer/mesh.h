#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MESH_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MESH_H

# include "vertex_attribute.h"

# include <m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays/api.h>

# include <cstddef>
# include <format>
# include <limits>
# include <span>
# include <stdexcept>
# include <utility>
# include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/**
 * @brief Owns vertex streams and matching storage descriptions indexed by shader input location.
 *
 * Stream i and attribute i describe vertex input location i, in the source
 * structure_of_arrays_t template-argument order. Each row is one vertex. Storage
 * is owned, and copies are independent; pass std::move(streams) to consume source
 * columns. Accessors borrow this mesh; keep it alive and reacquire views after
 * assigning or moving it. There are no per-stream mutation operations on a mesh.
 *
 * Construction checks one attribute per stream, equal row counts and exact element
 * byte widths (component_count * vertex_attribute_type_size), including overflow.
 * It does not infer a shader type from the original C++ type. Supply representations
 * matching the attribute, such as std::array<float, 2> for R32 with two components.
 * The erased storage's accepted C++ types are defined by
 * m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::erased_structure_of_arrays_t.
 *
 * Draw checks consumed locations against shader inputs: R32/I32/U32 map to
 * float/int32_t/uint32_t scalars or vectors of 2..4 components with no conversion
 * or normalization. Additional unconsumed streams are allowed. Other declared
 * storage widths can form a mesh but cannot supply these shader inputs.
 * Geometry owns index selection; software_renderer.h owns draw validation.
 * See [headless triangle](docs/headless-triangle.md) for complete setup and drawing.
 */
class mesh_t {
public:
    /** @brief Constructs an empty mesh with no streams, attributes or vertices. */
    mesh_t();

    /** @brief Owns and validates the supplied streams and their ordered attribute descriptions. */
    template <typename... Ts>
    mesh_t(m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::structure_of_arrays_t<Ts...> vertex_streams, std::vector<vertex_attribute_t> vertex_attributes);

    const m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::erased_structure_of_arrays_t& vertex_streams() const&;
    std::span<const vertex_attribute_t> vertex_attributes() const&;

    /** @brief Returns the shared row count, or zero when there are no streams. */
    std::size_t number_of_vertices() const;

private:
    m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::erased_structure_of_arrays_t m_vertex_streams;
    std::vector<vertex_attribute_t> m_vertex_attributes;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::mesh_t>;

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

template <typename... Ts>
mesh_t::mesh_t(m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::structure_of_arrays_t<Ts...> vertex_streams, std::vector<vertex_attribute_t> vertex_attributes):
    m_vertex_streams(std::move(vertex_streams)),
    m_vertex_attributes(std::move(vertex_attributes))
{
    const auto vertex_stream_count = m_vertex_streams.size();
    const auto vertex_attribute_count = m_vertex_attributes.size();
    if (vertex_stream_count != vertex_attribute_count) {
        throw std::runtime_error(std::format("mesh_t::mesh_t: number of vertex streams ({}) does not match number of vertex attributes ({})", vertex_stream_count, vertex_attribute_count));
    }

    std::size_t element_count = 0;
    for (std::size_t i = 0; i < vertex_stream_count; ++i) {
        if (i == 0) {
            element_count = m_vertex_streams[i].element_count();
        } else if (m_vertex_streams[i].element_count() != element_count) {
            throw std::logic_error(std::format("mesh_t::mesh_t: vertex stream {} element count ({}) does not match vertex stream 0 element count ({})", i, m_vertex_streams[i].element_count(), element_count));
        }

        const auto& attribute = m_vertex_attributes[i];
        const auto& stream = m_vertex_streams[i];
        const auto stream_element_size = stream.element_size();
        const auto attribute_type_size = vertex_attribute_type_size(attribute.type());
        if (std::numeric_limits<std::size_t>::max() / attribute_type_size < attribute.component_count()) {
            throw std::length_error(std::format("mesh_t::mesh_t: vertex attribute {} element size is not representable", i));
        }
        const auto attribute_element_size = attribute_type_size * attribute.component_count();
        if (stream_element_size != attribute_element_size) {
            throw std::runtime_error(std::format("mesh_t::mesh_t: vertex stream {} element size ({}) does not match vertex attribute element size ({})", i, stream_element_size, attribute_element_size));
        }
    }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::mesh_t> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::mesh_t& mesh, auto& ctx) const {
        auto out = ctx.out();

        out = format_to(out, "vertex_attributes: [ ");
        for (std::size_t i = 0; i < mesh.vertex_attributes().size(); ++i) {
            if (0 < i) {
                out = format_to(out, ",");
            }
            out = format_to(out, "{} ", mesh.vertex_attributes()[i]);
        }
        out = format_to(out, "], vertex_streams: {}", mesh.vertex_streams());

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_MESH_H
