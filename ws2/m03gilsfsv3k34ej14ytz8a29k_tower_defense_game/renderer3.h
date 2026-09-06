#ifndef M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_RENDERER3_H
# define M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_RENDERER3_H

# include <m03gkcdy62bnz808pmk4uzkjra_glfw/window.h>
# include <m03gl22hn0dqmosreqjie9tg5m_opengl_renderer/api.h>
# include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/camera.h>
# include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/render_item.h>

# include <cstdint>
# include <format>
# include <memory>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

struct renderer3_color_t {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};

class renderer3_t {
public:
    explicit renderer3_t(std::shared_ptr<m03gkcdy62bnz808pmk4uzkjra_glfw::window_t> window);
    ~renderer3_t();

    renderer3_t(const renderer3_t&) = delete;
    renderer3_t& operator=(const renderer3_t&) = delete;
    renderer3_t(renderer3_t&&) = delete;
    renderer3_t& operator=(renderer3_t&&) = delete;

    bool begin_frame();
    bool begin_frame(renderer3_color_t clear_color);
    void present();

    int width() const noexcept;
    int height() const noexcept;

    void draw(
        const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::camera_t& camera,
        const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::render_item_t& render_item
    );

private:
    void create_resources();
    void destroy_resources() noexcept;

private:
    m03gl22hn0dqmosreqjie9tg5m_opengl_renderer::opengl_renderer_t m_opengl_renderer;
    int m_width;
    int m_height;
    GLuint m_program;
    GLuint m_vertex_array;
    GLuint m_vertex_buffer;
    GLuint m_index_buffer;
    GLint m_viewport_size_location;
    GLint m_entity_color_location;
    GLint m_point_shape_location;
    bool m_frame_active;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

namespace std {

template <>
struct formatter<m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::renderer3_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::renderer3_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid renderer3_t format specifier");
        }
        return it;
    }

    auto format(const m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::renderer3_t& renderer, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ width: {}, height: {} }}", renderer.width(), renderer.height());

        return out;
    }
};

} // namespace std

#endif // M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_RENDERER3_H
