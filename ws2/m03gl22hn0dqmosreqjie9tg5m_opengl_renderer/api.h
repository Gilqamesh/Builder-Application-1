#ifndef M03GL22HN0DQMOSREQJIE9TG5M_OPENGL_RENDERER_API_H
# define M03GL22HN0DQMOSREQJIE9TG5M_OPENGL_RENDERER_API_H

# include "opengl_renderer_external.h"

# include <m03gkcdy62bnz808pmk4uzkjra_glfw/window.h>
# include <m03gl8ffb2e842ezg4fboslgqu_glfw_window_renderer/glfw_window_renderer.h>

# include <cstddef>
# include <memory>
# include <span>

namespace m03gl22hn0dqmosreqjie9tg5m_opengl_renderer {

/**
 * @brief Owns OpenGL entry points and presentation resources for a shared GLFW window.
 *
 * Use and destroy the renderer on the GLFW main thread. Its base retains the
 * window; the external m03gkcdy62bnz808pmk4uzkjra_glfw::glfw_t must outlive the
 * renderer and all window owners. Destruction makes the window's context current
 * to release presentation resources and does not restore the previous context.
 */
class opengl_renderer_t : public m03gl8ffb2e842ezg4fboslgqu_glfw_window_renderer::glfw_window_renderer_t {
public:
    /**
     * @brief Makes a desktop OpenGL window's context current and loads its entry points.
     *
     * Construction alone does not validate the capabilities needed by present_rgba8().
     * @throws std::invalid_argument If window is null.
     * @throws std::runtime_error If the client API is not desktop OpenGL, the context
     * cannot be made current, or loading its entry points fails.
     */
    explicit opengl_renderer_t(std::shared_ptr<m03gkcdy62bnz808pmk4uzkjra_glfw::window_t> window);
    ~opengl_renderer_t();

    /**
     * @brief Borrows the OpenGL entry-point table loaded for this renderer's window.
     *
     * The table remains alive with this renderer. This accessor does not make a
     * context current; make the window's context current before calling GL functions.
     */
    const GladGLContext& get_gl() const;

    /**
     * @brief Uploads a CPU RGBA8 image, draws it to the window, and swaps buffers.
     *
     * This temporary convenience operation requires desktop OpenGL supporting the
     * GLSL 3.30 presentation shaders and vertex arrays; request OpenGL 3.3 or later.
     * pixels contains exactly width * height * 4 bytes, with no row padding: rows
     * run top to bottom, pixels left to right, and each pixel stores red, green,
     * blue, alpha as four unsigned 8-bit channels with non-premultiplied alpha.
     * Storage is borrowed only for this call: keep it alive and unchanged until
     * return, then it may be reused.
     *
     * width and height must be positive pixel dimensions and are expected to match
     * the current GLFW framebuffer size. Skip presentation for a zero-sized framebuffer.
     *
     * Makes the window's context current and leaves it current. Changes GL state,
     * including the viewport, depth/cull/blend enables, unpack alignment, active
     * texture, texture binding, program and vertex-array binding; previous state
     * is not restored. Blending is disabled, so alpha does not blend with prior contents.
     *
     * @throws std::invalid_argument If either dimension is non-positive or the byte count differs.
     * @throws std::length_error If the required pixel or byte count overflows std::size_t.
     * Input validation happens before any context or GL state change.
     * @throws std::runtime_error If making the context current or checked presentation-resource
     * setup fails, including shader compilation/linking. Other OpenGL errors are not
     * comprehensively checked or translated to exceptions.
     *
     * @code{.cpp}
     * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/glfw.h>
     * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/window.h>
     * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/window_creation_settings.h>
     * #include <m03gl22hn0dqmosreqjie9tg5m_opengl_renderer/api.h>
     *
     * #include <cstddef>
     * #include <vector>
     *
     * int main() {
     *     using namespace m03gkcdy62bnz808pmk4uzkjra_glfw;
     *     glfw_t glfw;
     *     window_creation_settings_t window_creation_settings;
     *     window_creation_settings.opengl(3, 3, opengl_profile_t::core);
     *     auto window = window_t::create("CPU image", {100, 100, 640, 480}, window_creation_settings);
     *     if (!window) {
     *         return 1;
     *     }
     *     m03gl22hn0dqmosreqjie9tg5m_opengl_renderer::opengl_renderer_t opengl_renderer(window);
     *     window->swap_interval(1);
     *     std::vector<std::byte> pixels;
     *     while (!window->should_close()) {
     *         poll_events();
     *         const auto framebuffer_size = window->framebuffer_size();
     *         if (framebuffer_size[0] <= 0 || framebuffer_size[1] <= 0) {
     *             continue;
     *         }
     *         const auto byte_count = static_cast<std::size_t>(framebuffer_size[0]) *
     *             static_cast<std::size_t>(framebuffer_size[1]) * 4;
     *         pixels.assign(byte_count, std::byte{255}); // Opaque white RGBA pixels.
     *         opengl_renderer.present_rgba8(pixels, framebuffer_size[0], framebuffer_size[1]);
     *     }
     * } // Renderer resources, then window, then GLFW are destroyed.
     * @endcode
     */
    void present_rgba8(std::span<const std::byte> pixels, int width, int height);

private:
    void create_present_resources();
    void destroy_present_resources() noexcept;
    void resize_present_texture(int width, int height);

private:
    GladGLContext m_gl;
    GLuint m_present_program;
    GLuint m_present_texture;
    GLuint m_present_vertex_array;
    int m_present_texture_width;
    int m_present_texture_height;
};

} // namespace m03gl22hn0dqmosreqjie9tg5m_opengl_renderer

#endif // M03GL22HN0DQMOSREQJIE9TG5M_OPENGL_RENDERER_API_H
