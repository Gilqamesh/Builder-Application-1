#ifndef M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_RENDERER_H
# define M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_RENDERER_H

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03gagbht17w4tser1fescqxye_raylib/raylib.h>
# include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/camera.h>
# include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/render_item.h>

# include <format>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

/**
 * @brief Owns a Raylib window for the inactive legacy rendering path.
 *
 * This is legacy/experimental code, as specified in AGENTS.md. The active application
 * uses the software renderer through game_t; see the entry point in api.h.
 *
 * todo: remove window dependency from renderer_t, instead pass in a drawable surface to draw to, which can be a window or something else, like a texture.
 */
class renderer_t {
public:
    renderer_t();
    explicit renderer_t(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& window_bounds);
    ~renderer_t();

    void window_bounds(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& bounds);
    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& window_bounds() const;

    void draw(
        const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::camera_t& camera,
        const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::render_item_t& render_item
    );

private:
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> m_window_bounds;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

namespace std {

template <>
struct formatter<m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::renderer_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::renderer_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid renderer_t format specifier");
        }
        return it;
    }

    auto format(const m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::renderer_t& renderer, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ window_bounds: {} }}", renderer.window_bounds());

        return out;
    }
};

} // namespace std

#endif // M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_RENDERER_H
