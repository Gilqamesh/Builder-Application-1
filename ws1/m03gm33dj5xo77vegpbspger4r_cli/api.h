#ifndef M03GM33DJ5XO77VEGPBSPGER4R_CLI_API_H
# define M03GM33DJ5XO77VEGPBSPGER4R_CLI_API_H

# include <algorithm>
# include <charconv>
# include <cctype>
# include <cmath>
# include <concepts>
# include <cstddef>
# include <filesystem>
# include <format>
# include <functional>
# include <initializer_list>
# include <iosfwd>
# include <map>
# include <span>
# include <stdexcept>
# include <string>
# include <string_view>
# include <system_error>
# include <type_traits>
# include <utility>
# include <vector>

namespace m03gm33dj5xo77vegpbspger4r_cli {

/**
 * @brief Converts one command token to T for arguments_t::pop().
 *
 * Specializations provide static T parse(std::string_view token,
 * std::string_view name); name labels diagnostics. Built-in numeric parsers
 * require the whole token to fit T (decimal integers, finite floating-point
 * values) and throw std::invalid_argument on failure. Booleans accept lowercase
 * true/false, on/off, yes/no, and 1/0. String and path conversions do not validate
 * content or filesystem existence; std::string_view borrows the token.
 */
template <typename T>
struct argument_parser_t;

template <>
struct argument_parser_t<std::string_view>;

template <>
struct argument_parser_t<std::string>;

template <>
struct argument_parser_t<std::filesystem::path>;

template <>
struct argument_parser_t<bool>;

template <std::integral T>
requires (!std::same_as<T, bool>)
struct argument_parser_t<T>;

template <std::floating_point T>
struct argument_parser_t<T>;

/**
 * @brief Consumes typed tokens from a borrowed sequence of command arguments.
 *
 * The source strings and their storage must outlive this cursor and any returned
 * spans or string views. Copying a cursor copies its position, not the strings.
 */
class arguments_t {
public:
    /**
     * @brief Constructs an argument cursor over values.
     */
    explicit arguments_t(std::span<const std::string> values);

    /**
     * @brief Returns true if no arguments remain.
     */
    bool empty() const;

    /**
     * @brief Returns the number of remaining arguments.
     */
    std::size_t size() const;

    /**
     * @brief Returns all remaining arguments.
     */
    std::span<const std::string> remaining() const;

    /**
     * @brief Consumes the next token and converts it to T using argument_parser_t.
     *
     * Throws std::invalid_argument when no token remains. Conversion exceptions
     * propagate after consuming the token. The default std::string_view result
     * borrows the source string; name is used only for the call's diagnostics.
     */
    template <typename T = std::string_view>
    T pop(std::string_view name);

    /**
     * @brief Throws if arguments remain.
     */
    void expect_end(std::string_view usage) const;

private:
    std::string_view pop_token(std::string_view name);

    std::span<const std::string> m_values;
    std::size_t m_index;
};

/**
 * @brief Defines a positional argument's validation, completion, and usage text.
 *
 * Owns its strings and callbacks; references captured by callbacks remain the
 * caller's responsibility. Optional arguments follow required ones, and only
 * the last argument may be variadic; application_t::add() checks this ordering.
 */
class argument_t {
public:
    /**
     * @brief Constructs a plain required token argument.
     */
    argument_t();

    /**
     * @brief Marks the argument optional.
     */
    argument_t optional() &&;

    /**
     * @brief Marks the argument variadic.
     */
    argument_t variadic() &&;

    /**
     * @brief Returns the argument name.
     */
    std::string_view name() const;

    /**
     * @brief Returns the argument usage override.
     */
    std::string_view usage() const;

    /**
     * @brief Returns true if the argument is optional.
     */
    bool is_optional() const;

    /**
     * @brief Returns true if the argument is variadic.
     */
    bool is_variadic() const;

    /**
     * @brief Completes a partial value.
     */
    std::vector<std::string> complete(std::span<const std::string> arguments, std::string_view partial) const;

    /**
     * @brief Validates a value.
     */
    void validate(std::string_view value) const;

    /**
     * @brief Creates an unconstrained token argument.
     */
    static argument_t token(std::string name);

    /**
     * @brief Creates a signed integer argument.
     */
    static argument_t integer(std::string name);

    /**
     * @brief Creates an unsigned integer argument.
     */
    static argument_t unsigned_integer(std::string name);

    /**
     * @brief Creates a finite number argument.
     */
    static argument_t number(std::string name);

    /**
     * @brief Creates a boolean argument.
     */
    static argument_t boolean(std::string name);

    /**
     * @brief Creates a token argument with filesystem path completion.
     *
     * Does not require the supplied path to exist or validate its type.
     */
    static argument_t file(std::string name);

    /**
     * @brief Creates an argument constrained to a fixed set of values.
     */
    static argument_t choice(std::string name, std::initializer_list<std::string_view> values);

    /**
     * @brief Creates an argument constrained to a fixed set of values.
     */
    static argument_t choice(std::string name, std::vector<std::string> values);

    /**
     * @brief Creates an argument from custom completion and validation callbacks.
     *
     * Completion receives previously supplied argument tokens and the partial
     * token; validation receives the token and its argument name. Inputs are
     * borrowed for each synchronous call. Empty callbacks disable that operation;
     * validation signals failure by throwing. Callback exceptions propagate.
     */
    static argument_t custom(
        std::string name,
        std::function<std::vector<std::string>(std::span<const std::string>, std::string_view)> complete = {},
        std::function<void(std::string_view, std::string_view)> validate = {}
    );

private:
    std::string m_name;
    std::string m_usage;
    std::function<std::vector<std::string>(std::span<const std::string>, std::string_view)> m_complete;
    std::function<void(std::string_view, std::string_view)> m_validate;
    bool m_is_optional;
    bool m_is_variadic;
};

/**
 * @brief Borrows command arguments and streams and carries a handler's stop request.
 *
 * A dispatched context is valid only during the handler call. Copy tokens that
 * must survive dispatch; do not retain the context or views into its arguments.
 */
class context_t {
public:
    /**
     * @brief Constructs a command context.
     */
    context_t(arguments_t arguments, std::ostream& out, std::ostream& err);

    /**
     * @brief The command arguments.
     */
    arguments_t arguments;

    /**
     * @brief The standard output stream.
     */
    std::ostream& out;

    /**
     * @brief The standard error stream.
     */
    std::ostream& err;

    /**
     * @brief Requests application shutdown after the handler returns normally.
     */
    void stop();

    /**
     * @brief Returns true if the command requested shutdown.
     */
    bool stop_requested() const;

private:
    bool m_stop_requested;
};

/**
 * @brief Owns a command's dispatch paths, argument schema, help text, and handler.
 *
 * Construction stores the description; application_t::add() checks registration
 * invariants. Callback captures must remain valid for their later invocations.
 */
struct command_t {
    /**
     * @brief Constructs an empty command.
     */
    command_t();

    /**
     * @brief Constructs a command with generated usage.
     */
    command_t(std::vector<std::string> path, std::string description, std::function<void(context_t&)> handler);

    /**
     * @brief Constructs a command with generated usage.
     */
    command_t(std::vector<std::string> path, std::string description, std::vector<argument_t> arguments, std::function<void(context_t&)> handler);

    /**
     * @brief Constructs a command with explicit usage.
     */
    command_t(std::vector<std::string> path, std::string usage, std::string description, std::function<void(context_t&)> handler);

    /**
     * @brief Constructs a command with explicit usage.
     */
    command_t(std::vector<std::string> path, std::string usage, std::string description, std::vector<argument_t> arguments, std::function<void(context_t&)> handler);

    /**
     * @brief The token path used for dispatch.
     */
    std::vector<std::string> path;

    /**
     * @brief Additional dispatch paths hidden from help and completion.
     */
    std::vector<std::vector<std::string>> aliases;

    /**
     * @brief Excludes the command from help and command completion.
     */
    bool hidden;

    /**
     * @brief Short help text.
     */
    std::string description;

    /**
     * @brief Positional arguments consumed after the path.
     */
    std::vector<argument_t> arguments;

    /**
     * @brief Handler called after lookup and validation.
     */
    std::function<void(context_t&)> handler;

    /**
     * @brief Optional full usage string overriding generated usage.
     */
    std::string usage;
};

/**
 * @brief Owns registered commands and synchronously validates and dispatches input.
 *
 * Starts running. Dispatch borrows input and streams for the call; the stored
 * handlers and fallback may capture caller-owned state, which must remain valid
 * when invoked. Dispatch chooses the longest registered path before validating
 * positional arguments. A trailing help token requests help without invoking
 * the command handler. See run_arguments() for return and exception semantics.
 *
 * @code{.cpp}
 * #include <m03gm33dj5xo77vegpbspger4r_cli/api.h>
 *
 * #include <exception>
 * #include <iostream>
 * #include <string>
 * #include <vector>
 *
 * namespace cli = m03gm33dj5xo77vegpbspger4r_cli;
 *
 * int main() {
 *     cli::application_t application;
 *     application.add({
 *         {"show"}, "Print a count.", {cli::argument_t::integer("count")},
 *         [](cli::context_t& context) {
 *             const int count = context.arguments.pop<int>("count");
 *             context.out << count << '\n';
 *         }
 *     });
 *     application.add({{"quit"}, "Stop the application.", [](cli::context_t& context) {
 *         context.stop();
 *     }});
 *     try {
 *         application.run_command("show 3", std::cout, std::cerr); // Returns true.
 *         const std::vector<std::string> arguments{"quit"}; // No executable name.
 *         const bool running = application.run_arguments(arguments, std::cout, std::cerr);
 *         return running ? 1 : 0; // Successful quit returns false.
 *     } catch (const std::exception& exception) {
 *         std::cerr << exception.what() << '\n';
 *         return 1;
 *     }
 * }
 * @endcode
 */
class application_t {
public:
    /**
     * @brief Constructs an empty command application.
     */
    application_t();

    application_t(const application_t& other) = delete;
    application_t& operator=(const application_t& other) = delete;
    application_t(application_t&& other) = delete;
    application_t& operator=(application_t&& other) = delete;

    /**
     * @brief Takes ownership of a command after checking its registration invariants.
     *
     * Throws std::logic_error for an empty handler, empty path or path component,
     * duplicate path/alias, empty argument name, nonfinal variadic argument, or
     * required argument following an optional one. Allocation failures propagate.
     */
    void add(command_t command);

    /**
     * @brief Installs the built-in help command.
     */
    void install_help_command();

    /**
     * @brief Sets a handler for unknown command lines.
     *
     * The handler receives the original tokens as arguments and should return
     * true only when it handled the line. This handled result is separate from
     * the running state returned by dispatch. An empty handler clears fallback;
     * exceptions propagate. The context's borrowing rules apply.
     */
    void fallback(std::function<bool(context_t&)> handler);

    /**
     * @brief Returns true while the application should keep running.
     */
    bool running() const;

    /**
     * @brief Stops the application.
     */
    void stop();

    /**
     * @brief Tokenizes and runs one command line.
     *
     * Accepts whitespace-separated tokens with quotes, backslash escapes, and
     * comments starting with # at a token boundary. Unterminated quotes or an
     * incomplete final escape throw std::invalid_argument before dispatch.
     * Returns the running state and otherwise follows run_arguments().
     */
    bool run_command(std::string_view command, std::ostream& out, std::ostream& err);

    /**
     * @brief Dispatches already-tokenized input and returns the application's running state.
     *
     * Supply the command path and positional arguments without an executable name.
     * No tokenization is performed. Empty input only returns running(). The result
     * is true while running and false after a stop request; it is not a command
     * success flag. Callers control whether to dispatch again after stopping.
     *
     * Unknown commands not handled by fallback and built-in argument validation
     * failures throw std::invalid_argument. Custom validator, handler, and fallback
     * exceptions propagate unchanged. Dispatch does not print these exceptions to
     * err; the caller reports or handles them. Handler output and other effects
     * before an exception are not rolled back. context_t::stop() takes effect only
     * after the callback returns normally.
     */
    bool run_arguments(std::span<const std::string> arguments, std::ostream& out, std::ostream& err);

    /**
     * @brief Dispatches script lines until EOF or a stop request and returns the running state.
     *
     * Echoes nonempty tokenized lines with path and line number when requested.
     * Opening failure throws std::invalid_argument. A std::exception while
     * processing a line stops the script and is rethrown as std::invalid_argument
     * with path and line number; nonstandard exceptions propagate unchanged.
     * Read failure throws std::ios_base::failure. Prior command effects remain.
     */
    bool run_script(const std::filesystem::path& path, std::ostream& out, std::ostream& err, bool echo_commands = true);

    /**
     * @brief Completes a command line at cursor.
     */
    std::vector<std::string> complete_line(std::string_view line, std::size_t cursor = std::string_view::npos) const;

private:
    std::pair<const command_t*, std::size_t> find(std::span<const std::string> tokens) const;
    std::pair<const command_t*, std::size_t> find_completion_command(std::span<const std::string> prefix) const;
    bool has_help_topic(std::span<const std::string> prefix) const;
    std::vector<std::string> postfix_help_topic(std::span<const std::string> prefix) const;
    void validate(const command_t& command, std::span<const std::string> arguments) const;
    std::vector<std::string> complete(std::span<const std::string> prefix, std::string_view partial) const;
    std::vector<std::string> complete_topic(std::span<const std::string> prefix, std::string_view partial) const;
    void help(std::ostream& out, std::span<const std::string> prefix = {}) const;
    bool run_fallback(std::span<const std::string> tokens, std::ostream& out, std::ostream& err);
    bool run_tokens(std::span<const std::string> tokens, std::ostream& out, std::ostream& err);

private:
    bool m_running;
    std::vector<command_t> m_commands;
    std::map<std::vector<std::string>, std::size_t> m_index_by_path;
    std::size_t m_max_path_size;
    std::function<bool(context_t&)> m_fallback;
};

} // namespace m03gm33dj5xo77vegpbspger4r_cli

namespace std {

template <>
struct formatter<m03gm33dj5xo77vegpbspger4r_cli::argument_t>;

template <>
struct formatter<m03gm33dj5xo77vegpbspger4r_cli::command_t>;

} // namespace std

namespace m03gm33dj5xo77vegpbspger4r_cli {

template <>
struct argument_parser_t<std::string_view> {
    static std::string_view parse(std::string_view value, std::string_view) {
        return value;
    }
};

template <>
struct argument_parser_t<std::string> {
    static std::string parse(std::string_view value, std::string_view) {
        return std::string(value);
    }
};

template <>
struct argument_parser_t<std::filesystem::path> {
    static std::filesystem::path parse(std::string_view value, std::string_view) {
        return std::filesystem::path(std::string(value));
    }
};

template <>
struct argument_parser_t<bool> {
    static bool parse(std::string_view value, std::string_view name) {
        if (value == "true" || value == "on" || value == "yes" || value == "1") {
            return true;
        }
        if (value == "false" || value == "off" || value == "no" || value == "0") {
            return false;
        }
        throw std::invalid_argument(std::format("{} must be true/false, on/off, yes/no or 1/0, got '{}'", name, value));
    }
};

template <std::integral T>
requires (!std::same_as<T, bool>)
struct argument_parser_t<T> {
    static T parse(std::string_view value, std::string_view name) {
        T result{};
        const char* const begin = value.data();
        const char* const end = begin + value.size();
        const auto [position, error] = std::from_chars(begin, end, result);
        if (error != std::errc{} || position != end) {
            throw std::invalid_argument(std::format("{} must be an integer, got '{}'", name, value));
        }
        return result;
    }
};

template <std::floating_point T>
struct argument_parser_t<T> {
    static T parse(std::string_view value, std::string_view name) {
        T result{};
        const char* const begin = value.data();
        const char* const end = begin + value.size();
        const auto [position, error] = std::from_chars(begin, end, result);
        if (error != std::errc{} || position != end || !std::isfinite(result)) {
            throw std::invalid_argument(std::format("{} must be a finite number, got '{}'", name, value));
        }
        return result;
    }
};

template <typename T>
T arguments_t::pop(std::string_view name) {
    static_assert(std::same_as<T, std::remove_cvref_t<T>>, "arguments_t::pop<T>: T must not be cv-qualified or a reference.");
    return argument_parser_t<T>::parse(pop_token(name), name);
}

} // namespace m03gm33dj5xo77vegpbspger4r_cli

namespace std {

template <>
struct formatter<m03gm33dj5xo77vegpbspger4r_cli::argument_t> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gm33dj5xo77vegpbspger4r_cli::argument_t& argument, auto& ctx) const {
        auto out = ctx.out();

        if (argument.is_optional()) {
            out = format_to(out, "[");
        }

        if (!argument.usage().empty()) {
            out = format_to(out, "{}", argument.usage());
        } else {
            out = format_to(out, "<{}>", argument.name());
        }

        if (argument.is_variadic()) {
            out = format_to(out, "...");
        }
        
        if (argument.is_optional()) {
            out = format_to(out, "]");
        }

        return out;
    }
};

template <>
struct formatter<m03gm33dj5xo77vegpbspger4r_cli::command_t> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gm33dj5xo77vegpbspger4r_cli::command_t& command, auto& ctx) const {
        auto out = ctx.out();

        if (!command.usage.empty()) {
            out = format_to(out, "{}", command.usage);
        } else {
            bool first = true;
            for (const std::string& component : command.path) {
                if (!first) {
                    out = format_to(out, " ");
                }
                first = false;
                out = format_to(out, "{}", component);
            }

            for (const m03gm33dj5xo77vegpbspger4r_cli::argument_t& argument : command.arguments) {
                if (!first) {
                    out = format_to(out, " ");
                }
                first = false;
                out = format_to(out, "{}", argument);
            }
        }

        return out;
    }
};

} // namespace std

#endif // M03GM33DJ5XO77VEGPBSPGER4R_CLI_API_H
