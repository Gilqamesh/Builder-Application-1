#ifndef M03GL8FFB2E842EZG4FBOSLGQU_GLFW_WINDOW_RENDERER_GLFW_WINDOW_RENDERER_H
# define M03GL8FFB2E842EZG4FBOSLGQU_GLFW_WINDOW_RENDERER_GLFW_WINDOW_RENDERER_H

# include <m03gkcdy62bnz808pmk4uzkjra_glfw/window.h>

namespace m03gl8ffb2e842ezg4fboslgqu_glfw_window_renderer {

/**
 * @brief Keeps a GLFW window alive as the shared target of a derived renderer.
 *
 * This base owns a share of the window, but does not initialize GLFW or draw.
 * The external m03gkcdy62bnz808pmk4uzkjra_glfw::glfw_t must outlive the renderer
 * and every remaining window owner. Destroy derived rendering resources before
 * releasing the window, and release all window owners before destroying glfw_t.
 * Follow the GLFW module's main-thread event-loop contract for window operations
 * and destruction; shared ownership does not relax its thread constraints.
 *
 * @code{.cpp}
 * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/glfw.h>
 * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/window.h>
 * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/window_creation_settings.h>
 * #include <m03gl8ffb2e842ezg4fboslgqu_glfw_window_renderer/glfw_window_renderer.h>
 *
 * int main() {
 *     using namespace m03gkcdy62bnz808pmk4uzkjra_glfw;
 *     glfw_t glfw;
 *     window_creation_settings_t window_creation_settings;
 *     auto window = window_t::create("Renderer target", {100, 100, 640, 480}, window_creation_settings);
 *     if (!window) {
 *         return 1;
 *     }
 *     {
 *         m03gl8ffb2e842ezg4fboslgqu_glfw_window_renderer::glfw_window_renderer_t glfw_window_renderer(window);
 *         window = glfw_window_renderer.window(); // Shares the same mutable window.
 *     } // The renderer releases its share; window still owns the window.
 *     window.reset(); // Last owner releases the window before glfw is destroyed.
 * }
 * @endcode
 */
class glfw_window_renderer_t {
public:
    /**
     * @brief Retains shared ownership of a non-null render window.
     *
     * @throws std::invalid_argument If window is null.
     */
    explicit glfw_window_renderer_t(std::shared_ptr<m03gkcdy62bnz808pmk4uzkjra_glfw::window_t> window);
    virtual ~glfw_window_renderer_t() = default;

    /**
     * @brief Returns another shared owner of the renderer's mutable window.
     *
     * The result is non-null and may outlive this renderer, but must be released
     * before the external glfw_t is destroyed. It refers to the same window;
     * mutations through it are visible to every owner.
     */
    std::shared_ptr<m03gkcdy62bnz808pmk4uzkjra_glfw::window_t> window() const noexcept;

    glfw_window_renderer_t(const glfw_window_renderer_t&) = delete;
    glfw_window_renderer_t& operator=(const glfw_window_renderer_t&) = delete;
    glfw_window_renderer_t(glfw_window_renderer_t&&) = delete;
    glfw_window_renderer_t& operator=(glfw_window_renderer_t&&) = delete;

private:
    std::shared_ptr<m03gkcdy62bnz808pmk4uzkjra_glfw::window_t> m_window;
};

} // namespace m03gl8ffb2e842ezg4fboslgqu_glfw_window_renderer

#endif // M03GL8FFB2E842EZG4FBOSLGQU_GLFW_WINDOW_RENDERER_GLFW_WINDOW_RENDERER_H
