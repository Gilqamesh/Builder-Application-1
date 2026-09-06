#include "render_item.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

render_item_t::render_item_t():
    m_translation(0.0F),
    m_rotation(),
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

vector3f_t& render_item_t::translation() {
    return m_translation;
}

const vector3f_t& render_item_t::translation() const {
    return m_translation;
}

void render_item_t::rotation(const quaternion_t& rotation) {
    m_rotation = rotation.unit();
}

void render_item_t::rotation(const vector3f_t& euler_xyz) {
    m_rotation = quaternion_t::from_euler_xyz(euler_xyz);
}

const quaternion_t& render_item_t::rotation() const {
    return m_rotation;
}

vector3f_t& render_item_t::scale() {
    return m_scale;
}

const vector3f_t& render_item_t::scale() const {
    return m_scale;
}

matrix4f_t render_item_t::object_to_world() const {
    const auto finite = [](float component) { return std::isfinite(component); };
    if (!std::ranges::all_of(m_translation, finite) || !std::ranges::all_of(m_scale, finite)) {
        throw std::invalid_argument("render_item_t::object_to_world requires finite translation and scale");
    }
    const auto rotation = m_rotation.to_matrix();
    matrix4f_t matrix(0.0F);
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            matrix(row, column) = rotation(row, column) * m_scale[column];
        }
        matrix(row, 3) = m_translation[row];
    }
    matrix(3, 3) = 1;
    return matrix;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
