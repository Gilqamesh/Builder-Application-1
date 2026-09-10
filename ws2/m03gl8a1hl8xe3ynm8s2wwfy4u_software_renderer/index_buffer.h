#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_INDEX_BUFFER_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_INDEX_BUFFER_H

# include <cstddef>
# include <cstdint>
# include <format>
# include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/**
 * @brief Owns a mutable sequence of unsigned 32-bit mesh vertex indices.
 *
 * Starts empty. Vector access and indexing do not validate geometry; operator[]
 * requires an in-range element index. Vector mutation follows std::vector reference
 * and iterator invalidation rules. geometry_t selects and validates a range at use.
 */
class index_buffer_t {
public:
    using index_t = std::uint32_t;
    using indices_t = std::vector<index_t>;

public:
    indices_t& indices();
    const indices_t& indices() const;

    indices_t::iterator begin();
    indices_t::const_iterator begin() const;

    indices_t::iterator end();
    indices_t::const_iterator end() const;

    index_t& operator[](std::size_t index);
    const index_t& operator[](std::size_t index) const;

private:
    indices_t m_indices;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::index_buffer_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::index_buffer_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid index_buffer_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::index_buffer_t& index_buffer, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");

        const auto& indices = index_buffer.indices();
        out = std::format_to(out, "indices: [");
        for (std::size_t i = 0; i < indices.size(); ++i) {
            if (0 < i) {
                out = std::format_to(out, ", ");
            }
            out = std::format_to(out, "{}", indices[i]);
        }
        out = std::format_to(out, "]");

        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_INDEX_BUFFER_H
