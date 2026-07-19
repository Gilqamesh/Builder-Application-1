#ifndef M03GILSFSV3K34EJ14YTZ8A29K_CAMERA_H
# define M03GILSFSV3K34EJ14YTZ8A29K_CAMERA_H

# include "rect.h"
# include "vec2.h"

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

class camera_t {
public:
    camera_t(const rect_t& world_rect, const rect_t& view_rect);

    const rect_t& world_rect() const;
    const rect_t& view_rect() const;

    void world_rect(const rect_t& world_rect);
    void view_rect(const rect_t& view_rect);

    vec2_t world_to_view(const vec2_t& world_point) const;
    vec2_t view_to_world(const vec2_t& view_point) const;

    void move_world(const vec2_t& delta);
    void move_view(const vec2_t& delta);

private:
    void world_rect_internal(const rect_t& world_rect);
    void view_rect_internal(const rect_t& view_rect);

private:
    rect_t m_world_rect;
    rect_t m_view_rect;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

#endif // M03GILSFSV3K34EJ14YTZ8A29K_CAMERA_H
