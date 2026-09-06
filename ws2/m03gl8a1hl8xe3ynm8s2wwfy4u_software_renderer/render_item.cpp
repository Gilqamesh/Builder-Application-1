#include "render_item.h"

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

render_item_t::render_item_t():
    m_translation(0.0F),
    m_rotation(0.0F),
    m_scale(1.0F)
{
}

std::shared_ptr<geometry_t>& render_item_t::geometry() {
    return m_geometry;
}

const std::shared_ptr<geometry_t>& render_item_t::geometry() const {
    return m_geometry;
}

std::shared_ptr<material_t>& render_item_t::material() {
    return m_material;
}

const std::shared_ptr<material_t>& render_item_t::material() const {
    return m_material;
}

m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>& render_item_t::translation() {
    return m_translation;
}

const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>& render_item_t::translation() const {
    return m_translation;
}

float& render_item_t::rotation() {
    return m_rotation;
}

const float& render_item_t::rotation() const {
    return m_rotation;
}

m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>& render_item_t::scale() {
    return m_scale;
}

const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>& render_item_t::scale() const {
    return m_scale;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
