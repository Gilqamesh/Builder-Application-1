#ifndef M03GF09LA5RVBH6KK4VVT1QAWV_MODULE_SHELL_MODULE_SHELL_H
# define M03GF09LA5RVBH6KK4VVT1QAWV_MODULE_SHELL_MODULE_SHELL_H

namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell {

/**
 * @brief Blocks in an interactive shell that builds and runs selected module targets.
 *
 * Takes a snapshot of module names from the invocation-context workspace and
 * artifact roots at startup; modules added later require a new run(). To select
 * roots explicitly, set BUILDER_WORKSPACE_ROOT and BUILDER_ARTIFACT_ROOT before
 * calling. Root defaults, validation, and process-environment updates belong to
 * m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context().
 *
 * Reads standard input with the prompt `module_shell> `, sends command output to
 * standard output and diagnostics to standard error, and returns on EOF (normally
 * Ctrl-D at an empty prompt). Each module invocation builds its selected binary
 * target and waits for the child process to finish before accepting another
 * command. A normal return does not imply that every command succeeded.
 *
 * Commands use `<module>[:target] [arguments...]`; omitting `:target` selects
 * `cli`. Both sides of an explicit colon must be non-empty. Target names and
 * build/launch behavior follow
 * m03gagbhst621faiop1rztfkqp_builder_cli::create_and_wait_checked().
 * `help` shows shell help; `ls` lists friendly names, including duplicates.
 *
 * Every complete module directory name in the snapshot is accepted as a selector.
 * A friendly name is accepted only if it identifies exactly one module and is
 * not reserved by `help`, `ls`, or any complete module name in the snapshot.
 * For duplicate or reserved friendly names, use the complete name, with an
 * optional `:target`. These complete names remain usable even when omitted
 * from command help and completion; obtain them from the workspace directories.
 *
 * Tokenization follows
 * m03gm33dj5xo77vegpbspger4r_cli::application_t::run_command(). A trailing `help`
 * token requests shell-side help instead of invoking the module. Before launch,
 * each argument token exactly matching a friendly name is replaced by a complete
 * module name; other tokens are forwarded unchanged. Argument rewriting currently
 * does not apply the selector's ambiguity or reserved-name checks: even a quoted
 * token or a complete name is rewritten if it matches a friendly name. Do not
 * rely on which module an ambiguous argument selects.
 *
 * Standard exceptions from command parsing, lookup, building, or child execution
 * are reported and the input loop continues. Startup discovery and shell setup
 * failures propagate to the caller. Shell resources are local to this call;
 * coordinate calls with process-environment mutation and other Readline use.
 * The process-wide terminal/history constraints belong to
 * m03gm491bquimk7j45lpvis1yq_cli_shell::shell_t.
 *
 * For a caller built as `module-shell-example`, select the combined workspace
 * and a writable artifact directory in its launch environment:
 * @code{.sh}
 * BUILDER_WORKSPACE_ROOT=/path/to/workspace \
 * BUILDER_ARTIFACT_ROOT=/path/to/artifacts ./module-shell-example
 * @endcode
 * @code{.cpp}
 * #include <m03gf09la5rvbh6kk4vvt1qawv_module_shell/module_shell.h>
 *
 * #include <exception>
 * #include <iostream>
 *
 * int main() {
 *     try {
 *         m03gf09la5rvbh6kk4vvt1qawv_module_shell::run();
 *         return 0; // EOF ended the session; commands may have reported errors.
 *     } catch (const std::exception& exception) {
 *         std::cerr << exception.what() << '\n';
 *         return 1;
 *     }
 * }
 * @endcode
 */
void run();

} // namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell

#endif // M03GF09LA5RVBH6KK4VVT1QAWV_MODULE_SHELL_MODULE_SHELL_H
