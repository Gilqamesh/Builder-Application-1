#include "map.h"

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

void map_t::set_tile(vec2_t logical_position, tile_t type) {
    m_tiles[logical_position] = type;
}

tile_t map_t::get_tile(vec2_t logical_position) const {
    auto it = m_tiles.find(logical_position);
    if (it != m_tiles.end()) {
        return it->second;
    }

    return tile_t::empty;
}

void map_t::for_each_tile(const std::function<void(rect_t, tile_t)>& callback) const {
    for (const auto& [logical_position, type] : m_tiles) {
        const auto tile_top_left = logical_position * m_tile_size;
        const auto tile_bottom_right = tile_top_left + m_tile_size;
        rect_t tile_rect(tile_top_left, tile_bottom_right);
        callback(tile_rect, type);
    }
}

void map_t::set_tile_size(vec2_t size) {
    m_tile_size = size;
}

vec2_t map_t::get_tile_size() const {
    return m_tile_size;
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
