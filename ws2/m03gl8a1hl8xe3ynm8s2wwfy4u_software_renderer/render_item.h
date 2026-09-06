#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_RENDER_ITEM_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_RENDER_ITEM_H

# include "geometry.h"
# include "material.h"
# include "types.h"

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

# include <format>
# include <memory>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/**
 * @brief Selects geometry and material with a 3D float transform for one draw.
 *
 * The object-to-world matrix uses column vectors and applies scale, then
 * quaternion rotation, then translation. Zero and negative scale components are valid.
 * Translation and scale must be finite when deriving the matrix.
 * Rotation setters store a normalized copy, rejecting zero or non-finite
 * quaternions. Euler input uses finite radians about fixed X, then Y, then Z
 * axes. Failed rotation updates preserve the previous orientation.
 */
class render_item_t {
public:
    render_item_t();

    std::shared_ptr<geometry_t>& geometry();
    const std::shared_ptr<geometry_t>& geometry() const;

    std::shared_ptr<material_t>& material();
    const std::shared_ptr<material_t>& material() const;

    vector3f_t& translation();
    const vector3f_t& translation() const;

    void rotation(const quaternion_t& rotation);
    void rotation(const vector3f_t& euler_xyz);
    const quaternion_t& rotation() const;

    vector3f_t& scale();
    const vector3f_t& scale() const;

    matrix4f_t object_to_world() const;

private:
    std::shared_ptr<geometry_t> m_geometry;
    std::shared_ptr<material_t> m_material;
    vector3f_t m_translation;
    quaternion_t m_rotation;
    vector3f_t m_scale;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::render_item_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::render_item_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid render_item_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::render_item_t& render_item, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");

        const auto& geometry = render_item.geometry();
        if (geometry) {
            out = std::format_to(out, "geometry: {}, ", *geometry);
        } else {
            out = std::format_to(out, "geometry: -, ");
        }

        const auto& material = render_item.material();
        if (material) {
            out = std::format_to(out, "material: {}, ", *material);
        } else {
            out = std::format_to(out, "material: -, ");
        }

        out = std::format_to(out, "translation: {}, ", render_item.translation());

        out = std::format_to(out, "rotation: {}, ", render_item.rotation());

        out = std::format_to(out, "scale: {}", render_item.scale());

        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_RENDER_ITEM_H
