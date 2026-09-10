#ifndef M03GKCDY62BNZ808PMK4UZKJRA_GLFW_WINDOW_H
# define M03GKCDY62BNZ808PMK4UZKJRA_GLFW_WINDOW_H

# include "glfw_external.h"
# include "monitor.h"
# include "window_creation_settings.h"
# include "input.h"

# include <string>
# include <format>
# include <span>
# include <functional>
# include <memory>
# include <optional>
# include <unordered_map>
# include <vector>

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03gli1rb5p56mncplipxpf3he_ring_buffer/api.h>

namespace m03gkcdy62bnz808pmk4uzkjra_glfw {

/**
 * @brief Returns the address of the specified OpenGL or OpenGL ES function for the current context.
 *
 * An OpenGL or OpenGL ES context must be current on the calling thread.
 */
GLFWglproc get_proc_address(const char* proc_name);

/**
 * @brief Describes a non-owning, top-left-origin RGBA8 image.
 *
 * data must point to width * height * 4 bytes until the consuming function returns.
 */
struct image_t {
    /**
     * The image data is little-endian, non-premultiplied RGBA with eight bits per channel and the red channel first.
     */
    unsigned char* data;
    int width;
    int height;
};

/**
 * @brief Owns one GLFW window and its custom cursor.
 *
 * glfw_t must outlive the window, and the window must be used and destroyed on the main thread.
 */
class window_t {
public:
    using input_states_t = m03gli1rb5p56mncplipxpf3he_ring_buffer::ring_buffer_t<
        input_state_t,
        m03gli1rb5p56mncplipxpf3he_ring_buffer::staging_policy_t::dedicated,
        m03gli1rb5p56mncplipxpf3he_ring_buffer::commit_policy_t::copy_with_advance
    >;

public:
    /**
     * @brief Creates a windowed window after applying the specified settings.
     *
     * @param title The UTF-8 window title.
     * @param rect The initial { x, y, width, height } in screen coordinates.
     * @param settings The complete window creation settings.
     * @returns The new window, or nullptr if creation fails.
     */
    static std::shared_ptr<window_t> create(const std::string& title, const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 4>& rect, const window_creation_settings_t& settings);

    /**
     * @brief Creates a full-screen window after applying the specified settings.
     *
     * @param title The UTF-8 window title.
     * @param monitor The connected monitor used for full-screen mode.
     * @param video_mode The requested video mode whose RGB bit depths override settings.
     * @param settings The complete window creation settings.
     * @returns The new window, or nullptr if creation fails.
     */
    static std::shared_ptr<window_t> create(const std::string& title, const monitor_t& monitor, const video_mode_t& video_mode, const window_creation_settings_t& settings);

    ~window_t();

    window_t(const window_t&) = delete;
    window_t& operator=(const window_t&) = delete;
    window_t(window_t&&) = delete;
    window_t& operator=(window_t&&) = delete;

    /**
     * @brief Returns the owned GLFW handle; the caller must not destroy it or replace its user pointer or callbacks.
     */
    GLFWwindow* handle() const;

    /**
     * @brief Returns the cumulative input history whose staging snapshot is updated by callbacks.
     *
     * The returned buffer is borrowed from this window. A new window has empty history
     * and an initialized stage: the queried cursor position, zero scroll and counters,
     * and all buttons up. Commit that initial stage before the first event poll.
     * Then poll events, commit, and compare history(1) with history(0).
     *
     * Dedicated staging does not alias committed history. The copy_with_advance policy
     * already seeds the next stage from the snapshot just committed; no manual copy
     * is needed. Preserve cumulative counters between polls. History offsets and slot
     * reuse follow m03gli1rb5p56mncplipxpf3he_ring_buffer::ring_buffer_t.
     *
     * input_state_change_t borrows both snapshots: keep them alive and unchanged while
     * using the view. Finish comparisons before another commit can reuse their slots,
     * or keep independent snapshot copies for a longer-lived comparison.
     *
     * @code{.cpp}
     * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/glfw.h>
     * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/input.h>
     * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/window.h>
     * #include <m03gkcdy62bnz808pmk4uzkjra_glfw/window_creation_settings.h>
     *
     * int main() {
     *     using namespace m03gkcdy62bnz808pmk4uzkjra_glfw;
     *     glfw_t glfw; // Main thread; destroyed after the window.
     *     window_creation_settings_t window_creation_settings;
     *     auto window = window_t::create("Input history", {100, 100, 640, 480}, window_creation_settings);
     *     if (!window) {
     *         return 1;
     *     }
     *     auto& input_states = window->input_states();
     *     input_states.commit(); // Publish the initial state and seed the next stage.
     *     while (!window->should_close()) {
     *         poll_events();
     *         input_states.commit();
     *         const auto& previous_input_state = input_states.history(1);
     *         const auto& current_input_state = input_states.history(0);
     *         const input_state_change_t input_state_change(previous_input_state, current_input_state);
     *         if (input_state_change.was_pressed(button_t::button_escape)) {
     *             window->should_close(true);
     *         }
     *     } // The view ends before the next poll and commit.
     * }
     * @endcode
     */
    input_states_t& input_states();
    const input_states_t& input_states() const;

    /**
     * @brief Replaces the custom cursor image using a hotspot inside the image.
     */
    void cursor_image(const image_t& image, const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& hotspot);

    /**
     * @brief Restores the platform-default cursor image.
     */
    void reset_cursor_image();

    /**
     * @brief Selects the cursor's visible and locked state.
     */
    void cursor_mode(bool visible, bool locked);

    /**
     * @brief Returns whether the cursor is visible.
     */
    bool cursor_visible() const;

    /**
     * @brief Returns whether the cursor is locked to the window.
     */
    bool cursor_locked() const;

    /**
     * @brief Enables or disables raw mouse motion for a hidden and locked cursor.
     *
     * @returns false if raw mouse motion is unsupported, otherwise true after applying the requested state.
     */
    bool cursor_raw_motion(bool value);

    /**
     * @brief Returns whether raw mouse motion is enabled.
     */
    bool cursor_raw_motion();

    /**
     * @brief Returns whether the cursor is inside the window content area.
     */
    bool cursor_is_in_content_area() const;

    /**
     * @brief Returns the client API associated with the window.
     */
    client_api_t client_api() const;

    /**
     * @brief Returns whether the window has been requested to close.
     */
    bool should_close() const;

    /**
     * @brief Sets whether the window should close.
     */
    void should_close(bool value) const;

    /**
     * @brief Returns the UTF-8 window title.
     */
    std::string title() const;

    /**
     * @brief Sets the UTF-8 window title.
     */
    void title(const std::string& title);

    /**
     * @brief Makes the window full-screen on the connected monitor using its current video mode.
     */
    void fullscreen(const monitor_t& monitor);

    /**
     * @brief Returns whether the window is full-screen.
     */
    bool fullscreen() const;

    /**
     * @brief Makes the window windowed at the specified { x, y, width, height } screen-coordinate rectangle.
     */
    void windowed(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 4>& work_area);

    /**
     * @brief Returns whether the window is windowed.
     */
    bool windowed() const;

    /**
     * @brief Maximizes the window.
     */
    void maximize();

    /**
     * @brief Returns whether the window is maximized.
     */
    bool maximized() const;

    /**
     * @brief Minimizes the window.
     */
    void minimize();

    /**
     * @brief Returns whether the window is minimized.
     */
    bool minimized() const;

    /**
     * @brief Restores the window to its previous state before it was minimized or maximized.
     */
    void restore();

    /**
     * @brief Sets the window size in screen coordinates.
     */
    void size(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& size);

    /**
     * @brief Returns the window size in screen coordinates.
     */
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> size() const;
    
    /**
     * @brief Returns the framebuffer size in pixels.
     */
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2> framebuffer_size() const;

    /**
     * @brief Sets optional windowed-mode size limits, where an empty value removes that limit.
     */
    void size_limits(
        std::optional<int> min_width,
        std::optional<int> min_height,
        std::optional<int> max_width,
        std::optional<int> max_height
    );

    /**
     * @brief Constrains the window content area to the specified positive width-to-height ratio.
     */
    void aspect_ratio(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& ratio);

    /**
     * @brief Removes the window aspect-ratio constraint.
     */
    void clear_aspect_ratio();

    /**
     * @brief Shows or hides the window.
     */
    void visible(bool value);

    /**
     * @brief Returns whether the window is visible.
     */
    bool visible() const;

    /**
     * @brief Sets whether showing the window also gives it input focus.
     */
    void focus_on_visible(bool value);

    /**
     * @brief Returns whether showing the window gives it input focus.
     */
    bool focus_on_visible() const;

    /**
     * @brief Requests input focus for the window.
     */
    void focus();

    /**
     * @brief Returns whether the window has input focus.
     */
    bool focused() const;

    /**
     * @brief Sets the window opacity from zero to one.
     */
    void opacity(float value);

    /**
     * @brief Returns the window opacity from zero to one.
     */
    float opacity() const;

    /**
     * @brief Requests user attention for the window.
     */
    void request_attention();

    /**
     * @brief Sets the window icons, or restores the default icon when images is empty.
     *
     * GLFW copies the images during the call and selects or rescales the most suitable sizes.
     */
    void icon(std::span<const image_t> images);

    /**
     * @brief Returns whether the window has a transparent framebuffer.
     */
    bool transparent_framebuffer() const;

    /**
     * @brief Makes this window's context current or detaches it from the calling thread, and does nothing if the window has no context.
     *
     * @returns whether this window's context is current on the calling thread after the call.
     */
    bool context_current(bool value) const;

    /**
     * @brief Returns whether this window's context is current on the calling thread.
     */
    bool context_current() const;

    /**
     * @brief Makes this window's context current and sets its swap interval, or does nothing if the window has no context.
     *
     * An interval of zero disables vertical synchronization.
     */
    void swap_interval(int interval);

    /**
     * @brief Makes this window's context current and swaps its front and back buffers, or does nothing if it has no context.
     */
    void swap_buffers();

    /**
     * @brief Makes this window's context current and returns whether it supports the specified extension, or false if it has no context.
     */
    bool extension_supported(const std::string& extension_name) const;

    /**
     * Callback setters replace the previous callback, and an empty function clears it.
     *
     * Callbacks run synchronously during event processing and must not throw or destroy GLFW resources before returning.
     */

    /**
     * @brief Sets the callback invoked when the window position changes.
     */
    void position_callback(std::function<void(window_t*, int, int)> callback);
    const std::function<void(window_t*, int, int)>& position_callback() const;

    /**
     * @brief Sets the callback invoked when the window size changes.
     */
    void size_callback(std::function<void(window_t*, int, int)> callback);
    const std::function<void(window_t*, int, int)>& size_callback() const;

    /**
     * @brief Sets the callback invoked when the window is requested to close.
     */
    void close_callback(std::function<void(window_t*)> callback);
    const std::function<void(window_t*)>& close_callback() const;

    /**
     * @brief Sets the callback invoked when the window needs to be refreshed.
     * 
     * On many modern systems, this may rarely or never be called.
     */
    void refresh_callback(std::function<void(window_t*)> callback);
    const std::function<void(window_t*)>& refresh_callback() const;

    /**
     * @brief Sets the callback invoked when the window gains or loses focus.
     */
    void focus_callback(std::function<void(window_t*, bool)> callback);
    const std::function<void(window_t*, bool)>& focus_callback() const;

    /**
     * @brief Sets the callback invoked when the window is minimized or restored.
     */
    void iconify_callback(std::function<void(window_t*, bool)> callback);
    const std::function<void(window_t*, bool)>& iconify_callback() const;

    /**
     * @brief Sets the callback invoked when the window is maximized or restored.
     */
    void maximize_callback(std::function<void(window_t*, bool)> callback);
    const std::function<void(window_t*, bool)>& maximize_callback() const;

    /**
     * @brief Sets the callback invoked when the framebuffer size changes.
     */
    void framebuffer_size_callback(std::function<void(window_t*, int, int)> callback);
    const std::function<void(window_t*, int, int)>& framebuffer_size_callback() const;

    /**
     * @brief Sets the callback invoked when the content scale changes.
     */
    void content_scale_callback(std::function<void(window_t*, float, float)> callback);
    const std::function<void(window_t*, float, float)>& content_scale_callback() const;

    /**
     * @brief Sets the callback invoked when a mouse button is pressed or released.
     */
    void mouse_button_callback(std::function<void(window_t*, int, int, int)> callback);
    const std::function<void(window_t*, int, int, int)>& mouse_button_callback() const;

    /**
     * @brief Sets the callback invoked when the cursor moves.
     */
    void cursor_position_callback(std::function<void(window_t*, double, double)> callback);
    const std::function<void(window_t*, double, double)>& cursor_position_callback() const;

    /**
     * @brief Sets the callback invoked when the cursor enters or leaves the content area.
     */
    void cursor_enter_callback(std::function<void(window_t*, bool)> callback);
    const std::function<void(window_t*, bool)>& cursor_enter_callback() const;

    /**
     * @brief Sets the callback invoked when scrolling occurs.
     */
    void scroll_callback(std::function<void(window_t*, double, double)> callback);
    const std::function<void(window_t*, double, double)>& scroll_callback() const;

    /**
     * @brief Sets the callback invoked when a key is pressed, repeated or released.
     */
    void key_callback(std::function<void(window_t*, int, int, int, int)> callback);
    const std::function<void(window_t*, int, int, int, int)>& key_callback() const;

    /**
     * @brief Sets the callback invoked when a Unicode character is input.
     * 
     * The character is encoded as a native endian UTF-32 code point.
     */
    void char_callback(std::function<void(window_t*, std::uint32_t)> callback);
    const std::function<void(window_t*, std::uint32_t)>& char_callback() const;

    /**
     * @brief Sets the callback invoked when UTF-8 paths are dropped onto the window.
     *
     * The path vector is valid only until the callback returns.
     */
    void drop_callback(std::function<void(window_t*, const std::vector<std::string>&)> callback);
    const std::function<void(window_t*, const std::vector<std::string>&)>& drop_callback() const;

private:
    explicit window_t(GLFWwindow* handle);
    static std::shared_ptr<window_t> create_internal(GLFWwindow* handle);
    bool context_current_internal(bool value) const;

private:
    GLFWwindow* m_handle;
    input_states_t m_input_states;
    GLFWcursor* m_cursor_image;

    std::function<void(window_t*, int, int)> m_position_callback;
    std::function<void(window_t*, int, int)> m_size_callback;
    std::function<void(window_t*)> m_close_callback;
    std::function<void(window_t*)> m_refresh_callback;
    std::function<void(window_t*, bool)> m_focus_callback;
    std::function<void(window_t*, bool)> m_iconify_callback;
    std::function<void(window_t*, bool)> m_maximize_callback;
    std::function<void(window_t*, int, int)> m_framebuffer_size_callback;
    std::function<void(window_t*, float, float)> m_content_scale_callback;

    std::function<void(window_t*, int, int, int)> m_mouse_button_callback;
    std::function<void(window_t*, double, double)> m_cursor_position_callback;
    std::function<void(window_t*, bool)> m_cursor_enter_callback;
    std::function<void(window_t*, double, double)> m_scroll_callback;
    std::function<void(window_t*, int, int, int, int)> m_key_callback;
    std::function<void(window_t*, std::uint32_t)> m_char_callback;
    std::function<void(window_t*, const std::vector<std::string>&)> m_drop_callback;
};

} // namespace m03gkcdy62bnz808pmk4uzkjra_glfw

namespace std {

template <>
struct formatter<m03gkcdy62bnz808pmk4uzkjra_glfw::window_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gkcdy62bnz808pmk4uzkjra_glfw::window_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gkcdy62bnz808pmk4uzkjra_glfw::window_t& window, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");

        out = std::format_to(out, "should_close: {}, ", window.should_close());

        out = std::format_to(out, "title: {}, ", window.title());

        out = std::format_to(out, "size [screen coordinates]: {}, ", window.size());

        out = std::format_to(out, "visible: {}, ", window.visible());

        out = std::format_to(out, "focus_on_visible: {}", window.focus_on_visible());

        out = std::format_to(out, " }}");

        return out;
    };
};

} // namespace std

#endif // M03GKCDY62BNZ808PMK4UZKJRA_GLFW_WINDOW_H
