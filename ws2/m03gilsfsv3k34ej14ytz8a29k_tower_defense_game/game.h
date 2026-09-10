#ifndef M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_GAME_H
# define M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_GAME_H

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03gkcdy62bnz808pmk4uzkjra_glfw/glfw.h>
# include <m03gkcdy62bnz808pmk4uzkjra_glfw/window.h>
# include <m03gl22hn0dqmosreqjie9tg5m_opengl_renderer/api.h>
# include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/software_renderer.h>

# include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

# include <memory>
# include <vector>

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

/**
 * @brief Owns the application's GLFW lifetime, scene and CPU framebuffer for software rendering.
 *
 * Construct, run and destroy on the main thread with no other glfw_t alive.
 * Construction opens the window and loads ./assets using the environment described
 * by this module's free run() entry point in api.h. The software-renderer module
 * owns scene-resource semantics; the OpenGL renderer presents the resulting pixels.
 */
class game_t {
public:
    game_t();
    ~game_t();

    /**
     * @brief Polls input and renders frames until this game's window is requested to close.
     *
     * Blocks on the calling thread and propagates exceptions. Returning leaves
     * the window owned by this game; destruction releases the window and GLFW.
     */
    void run();

private:
    void update(float dt);
    void render(m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t& parent_metric);

private:
    m03gkcdy62bnz808pmk4uzkjra_glfw::glfw_t glfw;
    std::shared_ptr<m03gkcdy62bnz808pmk4uzkjra_glfw::window_t> m_window;
    std::vector<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::rgba8_t> m_pixels;
    m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::software_renderer_t m_software_renderer;
    m03gl22hn0dqmosreqjie9tg5m_opengl_renderer::opengl_renderer_t m_opengl_renderer;
    m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::camera_t m_camera;
    std::vector<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::render_item_t> m_render_items;
};

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

#endif // M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_GAME_H
