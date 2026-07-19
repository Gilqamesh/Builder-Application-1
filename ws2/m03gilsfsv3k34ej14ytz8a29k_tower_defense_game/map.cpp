#include "map.h"

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

void map_t::set_tile(m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> logical_position, tile_t type) {
    m_tile_by_logical_position[logical_position] = type;
}

tile_t map_t::get_tile(m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> logical_position) const {
    auto it = m_tile_by_logical_position.find(logical_position);
    if (it != m_tile_by_logical_position.end()) {
        return it->second;
    }

    return tile_t::empty;
}

void map_t::set_tile_size(m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> size) {
    m_tile_size = size;
}

m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> map_t::get_tile_size() const {
    return m_tile_size;
}

void map_t::for_each_tile(const std::function<void(m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>, tile_t)>& callback) const {
    for (const auto& [logical_position, type] : m_tile_by_logical_position) {
        const auto top_left = logical_position * m_tile_size;
        const auto bottom_right = top_left + m_tile_size;
        m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> tile_rect(top_left, bottom_right);
        callback(tile_rect, type);
    }
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
