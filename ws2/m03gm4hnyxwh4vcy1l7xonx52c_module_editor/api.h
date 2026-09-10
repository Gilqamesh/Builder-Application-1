#ifndef M03GM4HNYXWH4VCY1L7XONX52C_MODULE_EDITOR_API_H
# define M03GM4HNYXWH4VCY1L7XONX52C_MODULE_EDITOR_API_H

namespace m03gm4hnyxwh4vcy1l7xonx52c_module_editor {

/**
 * @brief Runs the graphical module editor until its window is requested to close.
 *
 * Call on the main thread with a display supporting desktop OpenGL 2.1
 * compatibility functionality and no existing glfw_t. The call owns GLFW
 * initialization, window creation, event polling, drawing and shutdown; it blocks
 * until closure or failure. Rendering resources and the window are released
 * before GLFW terminates, including when a standard exception unwinds the loop.
 *
 * @returns 0 on normal window closure; 1 after a caught std::exception, with
 * "module_editor: ", exception.what() and a newline written to stderr.
 * Non-standard exceptions are not caught by this entry point.
 *
 * @code{.cpp}
 * #include <m03gm4hnyxwh4vcy1l7xonx52c_module_editor/api.h>
 *
 * int main() {
 *     return m03gm4hnyxwh4vcy1l7xonx52c_module_editor::run();
 * }
 * @endcode
 */
int run();

} // namespace m03gm4hnyxwh4vcy1l7xonx52c_module_editor

#endif // M03GM4HNYXWH4VCY1L7XONX52C_MODULE_EDITOR_API_H
