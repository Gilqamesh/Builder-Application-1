#ifndef M03GE9IJ4LBNS2MQ6722CD8654_FUNCTION_VISUALIZER_FUNCTION_VISUALIZER_H
# define M03GE9IJ4LBNS2MQ6722CD8654_FUNCTION_VISUALIZER_FUNCTION_VISUALIZER_H

namespace m03ge9ij4lbns2mq6722cd8654_function_visualizer {

/**
 * @brief Runs the blocking graphical application for viewing and editing nested function rectangles.
 *
 * Creates a fresh canvas and a 1600 by 900 Raylib window, initializes its text
 * editor's rlImGui context, and handles input/drawing until WindowShouldClose().
 * Normal return shuts down the editor before closing the window. Call on the
 * application's main thread with a working graphical session and OpenGL support;
 * the caller does not create a window or ImGui context first. The implementation
 * uses process-global canvas, repository and editor state, so concurrent or nested
 * calls cannot provide independent sessions.
 *
 * Persistence uses a function_ir_file_repository_t with the relative path
 * `functions`. Its namespace-scope construction may create that directory during
 * module initialization, before exec() is called. Later filesystem operations
 * resolve it against the then-current working directory; keep that directory
 * stable for the session. Ctrl+S passes the root function's stored IR to both the
 * in-memory repository and its ID-derived file. Existing files at that path are
 * overwritten. Startup does not load saved functions, and closing does not save
 * automatically. ImGui may also read/write `imgui.ini` in the working directory.
 * See m03ge9ij4fjvyaf48asgpm6fdt_function_ir_file_repository::function_ir_file_repository_t
 * for filename, encoding and write-failure limitations, and
 * m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository::function_repository_t for the
 * in-memory duplicate-ID behavior.
 *
 * @warning Saving currently does not reconstruct the root IR from the live
 * runtime tree. Creating, naming or moving a child updates runtime nodes without
 * adding those edits to the root's stored IR; Ctrl+S therefore does not persist
 * the edited canvas. See
 * m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t::function_ir()
 * and m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t::children()
 * for the distinction between stored IR and runtime children.
 *
 * Allocation, encoding and repository failures during exec() propagate to the
 * caller; the loop has no error dialog or recovery handler. A file save failure
 * does not roll back the preceding in-memory save. Exceptions from global
 * repository construction occur outside this call and cannot be caught around
 * exec().
 * @warning The current implementation performs window/editor shutdown only on
 * normal exit; an exception from the loop bypasses that cleanup. Do not assume
 * catching an exception makes the graphical session reusable.
 *
 * @code{.cpp}
 * #include <m03ge9ij4lbns2mq6722cd8654_function_visualizer/function_visualizer.h>
 *
 * int main() {
 *     // Launch from the directory where functions/ and imgui.ini should live.
 *     m03ge9ij4lbns2mq6722cd8654_function_visualizer::exec();
 * }
 * @endcode
 */
void exec();

} // namespace m03ge9ij4lbns2mq6722cd8654_function_visualizer

#endif // M03GE9IJ4LBNS2MQ6722CD8654_FUNCTION_VISUALIZER_FUNCTION_VISUALIZER_H
