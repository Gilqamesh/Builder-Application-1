#ifndef M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_API_H
# define M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_API_H

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

/**
 * @brief Runs the tower-defense application's window and frame loop until closure.
 *
 * Call on the main thread with an OpenGL-capable display and no existing glfw_t:
 * this call owns GLFW initialization, the window, rendering resources and shutdown.
 * The current window requests desktop OpenGL 4.6 core. Run with this module's
 * assets directory available as ./assets; image paths are relative to the working directory.
 * The call blocks while polling events and rendering, releases its window before
 * terminating GLFW, and propagates exceptions from setup and the frame loop.
 * Frame timings and the final profiling report are written to stdout.
 *
 * game_t uses m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::software_renderer_t
 * and that module's camera, mesh, geometry, material and render_item_t contracts.
 * The application owns the CPU framebuffer and presents it through
 * m03gl22hn0dqmosreqjie9tg5m_opengl_renderer::opengl_renderer_t::present_rgba8().
 * renderer3_t, the inactive renderer_t path and standalone application-local
 * opengl_* wrappers are legacy or experimental, as specified in this module's AGENTS.md.
 *
 * @code{.cpp}
 * #include <m03gilsfsv3k34ej14ytz8a29k_tower_defense_game/api.h>
 *
 * #include <exception>
 * #include <iostream>
 *
 * int main() {
 *     try {
 *         m03gilsfsv3k34ej14ytz8a29k_tower_defense_game::run();
 *         return 0;
 *     } catch (const std::exception& exception) {
 *         std::cerr << exception.what() << '\n';
 *         return 1;
 *     }
 * }
 * @endcode
 */
void run();

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game

#endif // M03GILSFSV3K34EJ14YTZ8A29K_TOWER_DEFENSE_GAME_API_H
