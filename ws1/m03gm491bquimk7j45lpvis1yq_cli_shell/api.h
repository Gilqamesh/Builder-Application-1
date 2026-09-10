#ifndef M03GM491BQUIMK7J45LPVIS1YQ_CLI_SHELL_API_H
# define M03GM491BQUIMK7J45LPVIS1YQ_CLI_SHELL_API_H

# include <m03gm33dj5xo77vegpbspger4r_cli/api.h>

# include <chrono>
# include <cstddef>
# include <filesystem>
# include <functional>
# include <iosfwd>
# include <optional>
# include <string>

namespace m03gm491bquimk7j45lpvis1yq_cli_shell {

/**
 * @brief Reads interactive commands with Readline and dispatches them to a borrowed application.
 *
 * The application must outlive the shell. Configure the shell before run(); input,
 * dispatch, and idle callbacks execute synchronously on the calling thread.
 * Readline state and history are process-wide: serialize shell use and other
 * Readline access. Only one active shell is supported; nested runs are rejected
 * with std::logic_error.
 *
 * @code{.cpp}
 * #include <m03gm33dj5xo77vegpbspger4r_cli/api.h>
 * #include <m03gm491bquimk7j45lpvis1yq_cli_shell/api.h>
 *
 * #include <chrono>
 * #include <exception>
 * #include <iostream>
 *
 * int main() {
 *     m03gm33dj5xo77vegpbspger4r_cli::application_t application;
 *     m03gm491bquimk7j45lpvis1yq_cli_shell::shell_t shell(application, "> ");
 *     shell.idle(std::chrono::milliseconds(100), [&application] {
 *         application.stop(); // Stop on the first input timeout, on this thread.
 *     });
 *     try {
 *         return shell.run(); // Blocks until EOF or the application stops.
 *     } catch (const std::exception& exception) {
 *         std::cerr << exception.what() << '\n';
 *         return 1;
 *     }
 * } // Shell is destroyed before the application borrowed by its callback.
 * @endcode
 */
class shell_t {
public:
    /**
     * @brief Constructs a shell over an application.
     *
     * The application is borrowed and must outlive the shell.
     */
    shell_t(m03gm33dj5xo77vegpbspger4r_cli::application_t& application, std::string prompt);

    /**
     * @brief Sets the maximum history entry count.
     *
     * Applied at the next run() to process-wide Readline history. Zero retains no
     * entries. Values exceeding the maximum int are rejected by run() with
     * std::invalid_argument; this setter only stores the value.
     */
    void history_size(std::size_t size);

    /**
     * @brief Sets the persistent history file.
     *
     * Stores an owned path. run() reads an existing file before input begins;
     * a missing file is skipped. After normal EOF or application stop it writes
     * history, creating parent directories as needed. Exceptional exit from the
     * input loop skips the write. History and filesystem errors propagate from
     * run(); destruction does not write history.
     */
    void history_file(std::filesystem::path path);

    /**
     * @brief Installs a callback invoked after an input wait times out.
     *
     * The interval is a poll timeout in milliseconds, not a periodic deadline:
     * input readiness takes precedence, and continuous input can delay callbacks.
     * Callbacks run synchronously between input reads, never during a command
     * handler. Zero polls without waiting and may invoke the callback repeatedly.
     * The callback may stop the borrowed application. Captured references must
     * remain valid through run(); callback exceptions propagate out of run().
     * Throws std::invalid_argument for an empty callback or an interval outside
     * [0, maximum int] milliseconds. A later call replaces the stored callback.
     */
    void idle(std::chrono::milliseconds interval, std::function<void()> callback);

    /**
     * @brief Runs the shell with standard streams.
     *
     * Equivalent to run(std::cout, std::cerr); input remains blocking.
     */
    int run();

    /**
     * @brief Blocks on standard input until EOF or application stop, returning 0 on normal exit.
     *
     * The streams are borrowed for command output and diagnostics; they do not
     * redirect Readline's input, prompt, or editing display. EOF ends this run
     * without changing the application's running state. A stopped application
     * skips input, but configured history is still read and written.
     *
     * Nonempty input lines enter history before dispatch. Command failures derived
     * from std::invalid_argument print "error: " to err; other std::exception
     * failures print "exception: ". The loop continues while the application is
     * running, so a 0 result does not imply every command succeeded. History,
     * filesystem, polling, and idle callback errors propagate; stream failures
     * follow the supplied streams' exception settings.
     * @see m03gm33dj5xo77vegpbspger4r_cli::application_t::run_command() for dispatch behavior.
     */
    int run(std::ostream& out, std::ostream& err);

private:
    m03gm33dj5xo77vegpbspger4r_cli::application_t& m_application;
    std::string m_prompt;
    std::optional<std::size_t> m_history_size;
    std::optional<std::filesystem::path> m_history_file;
    std::optional<std::chrono::milliseconds> m_idle_interval;
    std::function<void()> m_idle_callback;
};

} // namespace m03gm491bquimk7j45lpvis1yq_cli_shell

#endif // M03GM491BQUIMK7J45LPVIS1YQ_CLI_SHELL_API_H
