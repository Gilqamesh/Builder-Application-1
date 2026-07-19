#ifndef M03GILSFSV3K34EJ14YTZ8A29K_MAP_H
# define M03GILSFSV3K34EJ14YTZ8A29K_MAP_H

# include "tile.h"

# include <vector>
# include <unordered_map>
# include <utility>
# include <functional>

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03gintxczohr63y44o77b4pyj_hyperrectangle/api.h>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

class map_t {
public:
    void set_tile(m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> logical_position, tile_t type);
    tile_t get_tile(m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> logical_position) const;

    void set_tile_size(m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> size);
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> get_tile_size() const;

    void for_each_tile(const std::function<void(m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>, tile_t)>& callback) const;

private:
    std::unordered_map<m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>, tile_t, std::hash<m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>>> m_tile_by_logical_position;
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> m_tile_size;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

#endif // M03GILSFSV3K34EJ14YTZ8A29K_MAP_H
