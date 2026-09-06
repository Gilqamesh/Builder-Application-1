#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_RENDER_ITEM_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_RENDER_ITEM_H

# include "geometry.h"
# include "material.h"

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

# include <format>
# include <memory>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/**
 * @brief Selects geometry and material with a 2D float transform for one draw.
 *
 * Rotation is counter-clockwise in radians about the local origin. The transform
 * uses column vectors and applies scale, then rotation, then translation.
 */
class render_item_t {
public:
    render_item_t();

    std::shared_ptr<geometry_t>& geometry();
    const std::shared_ptr<geometry_t>& geometry() const;

    std::shared_ptr<material_t>& material();
    const std::shared_ptr<material_t>& material() const;

    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>& translation();
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>& translation() const;

    float& rotation();
    const float& rotation() const;

    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>& scale();
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>& scale() const;

private:
    std::shared_ptr<geometry_t> m_geometry;
    std::shared_ptr<material_t> m_material;
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2> m_translation;
    float m_rotation;
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2> m_scale;
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
