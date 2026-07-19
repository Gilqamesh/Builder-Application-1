#ifndef M03GILSFSV3K34EJ14YTZ8A29K_GAME_H
# define M03GILSFSV3K34EJ14YTZ8A29K_GAME_H

# include "camera.h"
# include "map.h"

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

class game_t {
public:
    game_t();
    ~game_t();

    void run();

private:
    void update(float dt);
    void render();

private:
    camera_t m_camera;
    map_t m_map;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

#endif // M03GILSFSV3K34EJ14YTZ8A29K_GAME_H
