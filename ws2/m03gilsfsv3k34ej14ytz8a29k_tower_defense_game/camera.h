#ifndef M03GILSFSV3K34EJ14YTZ8A29K_CAMERA_H
# define M03GILSFSV3K34EJ14YTZ8A29K_CAMERA_H

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03gintxczohr63y44o77b4pyj_hyperrectangle/api.h>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

class camera_t {
public:
    camera_t(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& world_rect, const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& view_rect);

    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& world_rect();
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& view_rect();

    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> world_to_view(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& world_rect) const;
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> view_to_world(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& view_rect) const;

private:
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> m_world_rect;
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2> m_view_rect;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

#endif // M03GILSFSV3K34EJ14YTZ8A29K_CAMERA_H
