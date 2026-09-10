#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_VERTEX_ATTRIBUTE_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_VERTEX_ATTRIBUTE_H

# include <format>
# include <cstddef>
# include <stdexcept>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/** @brief Describes stored component width and kind: R floating point, I signed integer, U unsigned integer. */
enum class vertex_attribute_type_t {
    R32,
    R64,
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
};

/** @brief Returns component size in bytes, throwing std::runtime_error for an unknown enumerator. */
std::size_t vertex_attribute_type_size(vertex_attribute_type_t type);

/**
 * @brief Describes one vertex stream's component representation and positive component count.
 *
 * Defaults to one R32 component. Construction and component_count() reject zero
 * with std::runtime_error. Storage-type recognition and shader compatibility are
 * checked when building a mesh and drawing it, respectively; see mesh_t.
 */
class vertex_attribute_t {
public:
    vertex_attribute_t();
    vertex_attribute_t(vertex_attribute_type_t type, std::size_t component_count);

    void type(vertex_attribute_type_t type);
    vertex_attribute_type_t type() const;

    void component_count(std::size_t count);
    std::size_t component_count() const;

private:
    vertex_attribute_type_t m_type;
    std::size_t m_component_count;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid vertex_attribute_type_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t& type, auto& ctx) const {
        auto out = ctx.out();

        switch (type) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::R32: {
                out = std::format_to(out, "R32");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::R64: {
                out = std::format_to(out, "R64");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::I8: {
                out = std::format_to(out, "I8");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::I16: {
                out = std::format_to(out, "I16");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::I32: {
                out = std::format_to(out, "I32");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::I64: {
                out = std::format_to(out, "I64");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::U8: {
                out = std::format_to(out, "U8");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::U16: {
                out = std::format_to(out, "U16");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::U32: {
                out = std::format_to(out, "U32");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_type_t::U64: {
                out = std::format_to(out, "U64");
            } break;
            default: {
                throw std::runtime_error(std::format("formatter<vertex_attribute_type_t>::format: unknown vertex attribute type ({})", static_cast<int>(type)));
            }
        }

        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid vertex_attribute_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_attribute_t& attribute, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{}", attribute.component_count());
        out = std::format_to(out, " x {}", attribute.type());

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_VERTEX_ATTRIBUTE_H
