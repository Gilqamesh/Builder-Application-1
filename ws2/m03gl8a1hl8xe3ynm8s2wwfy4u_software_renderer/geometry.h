#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_GEOMETRY_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_GEOMETRY_H

# include "index_buffer.h"
# include "mesh.h"
# include "vertex_primitive_topology.h"

# include <cstddef>
# include <format>
# include <memory>
# include <span>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/** @brief Selects [offset, offset + count) in index-buffer elements, not bytes or primitives. */
struct index_range_t {
    std::size_t offset;
    std::size_t count;
};

/**
 * @brief Shares a mesh and index buffer with a fixed index range and mutable primitive topology.
 *
 * Construction requires a non-null index buffer, defaults to triangle topology,
 * and leaves mesh() unset. Assign a mesh before validate() or draw(). The one-argument
 * constructor captures the buffer's current full range; later appends do not extend
 * that selection. The explicit-range constructor checks current bounds, permitting
 * an empty range at construction. Null buffers throw std::invalid_argument and
 * out-of-bounds ranges throw std::out_of_range.
 *
 * Copies share mesh/index-buffer resources but retain independent range, mesh
 * selection and topology. Shared index contents and a selected mesh can change;
 * successful validation is not a freeze. Sequence resource edits between draws.
 * See [headless triangle](docs/headless-triangle.md) for a complete caller.
 */
class geometry_t {
public:
    explicit geometry_t(std::shared_ptr<index_buffer_t> index_buffer);
    geometry_t(std::shared_ptr<index_buffer_t> index_buffer, index_range_t index_range);

    /**
     * @brief Validates the current mesh, selected index range, and primitive topology.
     *
     * Resources remain mutable after validation. Each draw validates their current state again.
     * Requires a mesh with streams and a nonempty selected range whose indices are
     * below number_of_vertices(). Points need at least one index; lines need pairs;
     * line strips/loops need at least two; triangles need triples; triangle strips/fans
     * need at least three. Invalid topology/counts/mesh indices throw std::runtime_error;
     * a range invalidated by buffer resizing throws std::out_of_range. Shader input
     * compatibility and signed 32-bit vertex-index eligibility are checked by draw().
     */
    void validate() const;

    std::shared_ptr<mesh_t>& mesh();
    std::shared_ptr<mesh_t> mesh() const;

    std::shared_ptr<index_buffer_t> index_buffer() const;

    index_range_t index_range() const;
    /**
     * @brief Borrows the selected indices after rechecking their range against the current buffer.
     *
     * Does not validate topology or vertex bounds. The span borrows the index
     * vector; keep the buffer alive and reacquire after resizing or replacement.
     */
    std::span<const index_buffer_t::index_t> indices() const;

    vertex_primitive_topology_t& primitive_topology();
    vertex_primitive_topology_t primitive_topology() const;

private:
    std::shared_ptr<mesh_t> m_mesh;
    std::shared_ptr<index_buffer_t> m_index_buffer;
    vertex_primitive_topology_t m_primitive_topology;
    index_range_t m_index_range;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::index_range_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::geometry_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::index_range_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid index_range_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::index_range_t& index_range, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");

        out = std::format_to(out, "offset: {}", index_range.offset);
        out = std::format_to(out, ", count: {}", index_range.count);

        out = std::format_to(out, " }}");

        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::geometry_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid geometry_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::geometry_t& geometry, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");

        const auto& mesh = geometry.mesh();
        if (mesh) {
            out = std::format_to(out, "mesh: {}", *mesh);
        } else {
            out = std::format_to(out, "mesh: -");
        }

        const auto& index_buffer = geometry.index_buffer();
        if (index_buffer) {
            out = std::format_to(out, ", index_buffer: {}", *index_buffer);
        } else {
            out = std::format_to(out, ", index_buffer: -");
        }

        out = std::format_to(out, ", index_range: {}", geometry.index_range());

        out = std::format_to(out, ", primitive_topology: {}", geometry.primitive_topology());

        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_GEOMETRY_H
