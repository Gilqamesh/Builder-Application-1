#ifndef M03GILSFSV3K34EJ14YTZ8A29K_MAP_H
# define M03GILSFSV3K34EJ14YTZ8A29K_MAP_H

# include <vector>
# include <unordered_map>
# include <utility>
# include <functional>

# include "rect.h"
# include "vec2.h"
# include "tile.h"

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

class map_t {
public:
    void set_tile(vec2_t logical_position, tile_t type);
    tile_t get_tile(vec2_t logical_position) const;

    void set_tile_size(vec2_t size);
    vec2_t get_tile_size() const;

    void for_each_tile(const std::function<void(rect_t, tile_t)>& callback) const;

private:
    std::unordered_map<vec2_t, tile_t, std::hash<vec2_t>> m_tiles;
    vec2_t m_tile_size;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

#endif // M03GILSFSV3K34EJ14YTZ8A29K_MAP_H
