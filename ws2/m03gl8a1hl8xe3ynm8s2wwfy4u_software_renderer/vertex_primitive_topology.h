#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_VERTEX_PRIMITIVE_TOPOLOGY_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_VERTEX_PRIMITIVE_TOPOLOGY_H

# include <format>
# include <stdexcept>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/** @brief Defines how the selected index sequence assembles points, lines or triangles. */
enum class vertex_primitive_topology_t {
                        /* Vertex index interpretation of 6 vertices        */
    point,              /* {0}, {1}, {2}, {3}, {4}, {5}                     */
    line,               /* {0, 1}, {2, 3}, {4, 5}                           */
    line_strip,         /* {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}           */
    line_loop,          /* {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 0}   */
    triangle,           /* {0, 1, 2}, {3, 4, 5}                             */
    triangle_strip,     /* {0, 1, 2}, {2, 1, 3}, {2, 3, 4}, {4, 3, 5}       */ // For n vertices, n-2 triangles: zero-based i even -> (i, i + 1, i + 2), i odd -> (i + 1, i, i + 2).
    triangle_fan,       /* {0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 5}       */
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid vertex_primitive_topology_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t& topology, auto& ctx) const {
        auto out = ctx.out();

        switch (topology) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t::point: {
                out = std::format_to(out, "point");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t::line: {
                out = std::format_to(out, "line");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t::line_strip: {
                out = std::format_to(out, "line_strip");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t::line_loop: {
                out = std::format_to(out, "line_loop");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t::triangle: {
                out = std::format_to(out, "triangle");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t::triangle_strip: {
                out = std::format_to(out, "triangle_strip");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_primitive_topology_t::triangle_fan: {
                out = std::format_to(out, "triangle_fan");
            } break;
            default: {
                throw std::runtime_error("formatter<vertex_primitive_topology_t>::format: unknown vertex primitive topology");
            } break;
        }

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_VERTEX_PRIMITIVE_TOPOLOGY_H
