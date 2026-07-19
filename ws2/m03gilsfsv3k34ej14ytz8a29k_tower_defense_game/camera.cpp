#include "camera.h"

#include <stdexcept>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

camera_t::camera_t(const rect_t& world_rect, const rect_t& view_rect) {
    world_rect_internal(world_rect);
    view_rect_internal(view_rect);
}

const rect_t& camera_t::world_rect() const {
    return m_world_rect;
}

const rect_t& camera_t::view_rect() const {
    return m_view_rect;
}

void camera_t::world_rect(const rect_t& world_rect) {
    world_rect_internal(world_rect);
}

void camera_t::view_rect(const rect_t& view_rect) {
    view_rect_internal(view_rect);
}

vec2_t camera_t::world_to_view(const vec2_t& world_point) const {
    return {
        m_view_rect.top_left().x + (world_point.x - m_world_rect.top_left().x) * m_view_rect.size().x / m_world_rect.size().x,
        m_view_rect.top_left().y + (world_point.y - m_world_rect.top_left().y) * m_view_rect.size().y / m_world_rect.size().y
    };
}

vec2_t camera_t::view_to_world(const vec2_t& view_point) const {
    return {
        m_world_rect.top_left().x + (view_point.x - m_view_rect.top_left().x) * m_world_rect.size().x / m_view_rect.size().x,
        m_world_rect.top_left().y + (view_point.y - m_view_rect.top_left().y) * m_world_rect.size().y / m_view_rect.size().y
    };
}

void camera_t::move_world(const vec2_t& delta) {
    m_world_rect.top_left() += delta;
    m_world_rect.bottom_right() += delta;
}

void camera_t::move_view(const vec2_t& delta) {
    m_view_rect.top_left() += delta;
    m_view_rect.bottom_right() += delta;
}

void camera_t::world_rect_internal(const rect_t& world_rect) {
    if (world_rect.size().x <= 0 || world_rect.size().y <= 0) {
        throw std::invalid_argument("World rectangle must have positive width and height.");
    }

    m_world_rect = world_rect;
}

void camera_t::view_rect_internal(const rect_t& view_rect) {
    if (view_rect.size().x <= 0 || view_rect.size().y <= 0) {
        throw std::invalid_argument("View rectangle must have positive width and height.");
    }

    m_view_rect = view_rect;
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
