#include "camera.h"

#include <stdexcept>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

camera_t::camera_t(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& world_rect, const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& view_rect):
    m_world_rect(world_rect),
    m_view_rect(view_rect)
{
}

m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& camera_t::world_rect() {
    return m_world_rect;
}

m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& camera_t::view_rect() {
    return m_view_rect;
}

m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> camera_t::world_to_view(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& world_rect) const {
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> result;
    for (size_t i = 0; i < 2; ++i) {
        const auto& world_interval = world_rect[i];
        const auto& camera_world_interval = m_world_rect[i];
        const auto& camera_view_interval = m_view_rect[i];
        const auto camera_view_interval_length = camera_view_interval.length();
        const auto camera_world_length = camera_world_interval.length();
        if (camera_world_length == 0) {
            throw std::runtime_error("Camera world rectangle has zero length in dimension " + std::to_string(i));
        }
        result[i] = {
            camera_view_interval[0] + (world_interval[0] - camera_world_interval[0]) * camera_view_interval_length / camera_world_length,
            camera_view_interval[1] + (world_interval[1] - camera_world_interval[1]) * camera_view_interval_length / camera_world_length
        };
    }
    return result;
}

m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> camera_t::view_to_world(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& view_rect) const {
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> result;
    for (size_t i = 0; i < 2; ++i) {
        const auto& view_interval = view_rect[i];
        const auto& camera_view_interval = m_view_rect[i];
        const auto& camera_world_interval = m_world_rect[i];
        const auto camera_world_interval_length = camera_world_interval.length();
        const auto camera_view_length = camera_view_interval.length();
        if (camera_view_length == 0) {
            throw std::runtime_error("Camera view rectangle has zero length in dimension " + std::to_string(i));
        }
        result[i] = {
            camera_world_interval[0] + (view_interval[0] - camera_view_interval[0]) * camera_world_interval_length / camera_view_length,
            camera_world_interval[1] + (view_interval[1] - camera_view_interval[1]) * camera_world_interval_length / camera_view_length
        };
    }
    return result;
}

// m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> camera_t::world_to_view(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& world_point) const {
//     return {
//         m_view_rect.top_left().x + (world_point.x - m_world_rect.top_left().x) * m_view_rect.size().x / m_world_rect.size().x,
//         m_view_rect.top_left().y + (world_point.y - m_world_rect.top_left().y) * m_view_rect.size().y / m_world_rect.size().y
//     };
// }

// m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> camera_t::view_to_world(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& view_point) const {
//     return {
//         m_world_rect.top_left().x + (view_point.x - m_view_rect.top_left().x) * m_world_rect.size().x / m_view_rect.size().x,
//         m_world_rect.top_left().y + (view_point.y - m_view_rect.top_left().y) * m_world_rect.size().y / m_view_rect.size().y
//     };
// }

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
