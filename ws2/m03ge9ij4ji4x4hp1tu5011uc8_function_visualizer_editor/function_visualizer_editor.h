#ifndef M03GE9IJ4JI4X4HP1TU5011UC8_FUNCTION_VISUALIZER_EDITOR_FUNCTION_VISUALIZER_EDITOR_H
# define M03GE9IJ4JI4X4HP1TU5011UC8_FUNCTION_VISUALIZER_EDITOR_FUNCTION_VISUALIZER_EDITOR_H

# include <functional>
# include <string>

namespace m03ge9ij4ji4x4hp1tu5011uc8_function_visualizer_editor {

/**
 * @brief Presents one single-line text request and delivers submitted text to a stored callback.
 *
 * Construction starts closed and does not initialize graphics. After creating a
 * Raylib window, call init() once; draw() belongs inside the caller's paired
 * rlImGuiBegin()/rlImGuiEnd() frame. Call deinit() before CloseWindow(). Destruction
 * releases the stored callback but does not call deinit(). These operations use
 * rlImGui's shared context, not an independent backend per editor; use one lifecycle
 * owner on the application's main thread and keep it alive during drawing.
 *
 * Text is UTF-8 with at most 127 bytes, excluding the terminating null byte; this
 * is a byte limit, not a character count. Each accepted request starts empty.
 * Closing the editor window without submission invokes no callback. The callable
 * and its captures remain stored after cancellation and deinit(), until another
 * accepted request replaces them or this editor is destroyed. Borrowed captures
 * must remain valid whenever the callback can run.
 *
 * @code{.cpp}
 * #include <m03ge9ij4ji4x4hp1tu5011uc8_function_visualizer_editor/function_visualizer_editor.h>
 * #include <m03gagbht17w4tser1fescqxye_raylib/raylib.h>
 * #include <m03ge9ij4hzaaehqgc59cj8f9q_rl_imgui/rlImGui.h>
 *
 * #include <string>
 * #include <utility>
 *
 * int main() {
 *     InitWindow(800, 450, "Text editor");
 *     if (!IsWindowReady()) {
 *         return 1;
 *     }
 *     std::string submitted_text;
 *     m03ge9ij4ji4x4hp1tu5011uc8_function_visualizer_editor::function_visualizer_editor_t editor;
 *     editor.init();
 *     editor.create_text_editor([&submitted_text](std::string text) noexcept {
 *         submitted_text = std::move(text);
 *     });
 *     SetTargetFPS(60);
 *     while (!WindowShouldClose()) {
 *         BeginDrawing();
 *         ClearBackground(RAYWHITE);
 *         rlImGuiBegin();
 *         editor.draw();
 *         rlImGuiEnd();
 *         EndDrawing();
 *     }
 *     editor.deinit();
 *     CloseWindow();
 * }
 * @endcode
 */
class function_visualizer_editor_t {
public:
    /// @brief Creates a closed editor with no pending callback or initialized graphics backend.
    function_visualizer_editor_t();

    /**
     * @brief Sets up the shared rlImGui context with the dark theme.
     * Requires an initialized Raylib window. This owns the high-level rlImGui
     * lifecycle; do not also call rlImGuiSetup() for the same session.
     */
    void init();
    /**
     * @brief Shuts down the shared rlImGui backend and destroys its context.
     * Call after the last completed frame and before destroying the Raylib window.
     * Does not submit, cancel or reset the pending request; do not draw after this
     * call. Context-dependent ImGui references become invalid.
     */
    void deinit();

    /**
     * @brief Opens an empty text request, or leaves the existing request unchanged if already open.
     * @param on_text_complete Nonempty callable stored by value for submission.
     * The next visible draw focuses the input. Enter in that input or the Done
     * button invokes the callback synchronously inside draw(), with an owned
     * string copy (which may be empty). On normal callback return, draw() releases
     * the callback and closes the request. Keep the editor and active ImGui frame
     * alive throughout the callback; defer opening another request until draw()
     * returns, because the current request is not yet closed during submission.
     */
    void create_text_editor(std::function<void(std::string)> on_text_complete);
    /**
     * @brief Draws and processes the pending text request, doing nothing when closed.
     * Requires init() and an active rlImGui frame when open; it neither begins nor
     * ends that frame. It must not be called concurrently or reentered.
     * @warning String-allocation and callback exceptions propagate. The current
     * implementation then skips callback clearing, request closure and ImGui::End();
     * it provides no automatic frame recovery. Use callbacks that return normally.
     */
    void draw();

    /// @brief Reports whether a text request is open, independently of backend initialization.
    bool open();
    /**
     * @brief Reports ImGui's current WantCaptureMouse flag only while this editor is open.
     * Requires a current ImGui context when open. This is a context-wide input
     * routing hint, not a hit test for the editor; no input is consumed by this query.
     */
    bool is_captured_mouse();
    /**
     * @brief Reports ImGui's current WantCaptureKeyboard flag only while this editor is open.
     * Requires a current ImGui context when open. The flag reflects ImGui's most
     * recent frame processing; no input is consumed by this query.
     */
    bool is_captured_keyboard();

private:
    char m_buffer[128];
    bool m_open;
    bool m_just_opened;
    std::function<void(std::string)> m_on_complete;
};

} // namespace m03ge9ij4ji4x4hp1tu5011uc8_function_visualizer_editor

#endif // M03GE9IJ4JI4X4HP1TU5011UC8_FUNCTION_VISUALIZER_EDITOR_FUNCTION_VISUALIZER_EDITOR_H
