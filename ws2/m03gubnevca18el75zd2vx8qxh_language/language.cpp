#include "language.h"

#include <m03gubnevc9z8dwzxigmj54y25_lisp_module_command/module_command.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <fstream>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#if defined(__unix__) || defined(__APPLE__)
# include <termios.h>
# include <unistd.h>
#endif

namespace m03gubnevca18el75zd2vx8qxh_language {

namespace {

using json = nlohmann::json;

bool is_blank(std::string_view value) {
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c);
    });
}

bool is_incomplete_read_error(std::string_view message) {
    return message.find("unterminated list") != std::string_view::npos
        || message.find("unterminated string literal") != std::string_view::npos
        || message.find("unterminated string escape") != std::string_view::npos;
}

std::string history_display(std::string_view command) {
    std::string result;
    for (const char c : command) {
        switch (c) {
            case '\n': result += "\\n"; break ;
            case '\t': result += "\\t"; break ;
            default: result.push_back(c); break ;
        }
    }

    return result;
}

std::string read_file(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) {
    std::ifstream ifs(path.string(), std::ios::binary);
    if (!ifs) {
        throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::load: failed to open '{}'", path));
    }

    std::string result;
    char buffer[4096];
    while (ifs.read(buffer, sizeof(buffer)) || ifs.gcount() != 0) {
        result.append(buffer, static_cast<std::size_t>(ifs.gcount()));
    }
    if (!ifs.eof()) {
        throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::load: failed to read '{}'", path));
    }

    return result;
}

void append_history(std::vector<std::string>& history, const std::string& command) {
    if (is_blank(command)) {
        return ;
    }
    if (!history.empty() && history.back() == command) {
        return ;
    }

    history.push_back(command);
}

std::vector<std::string> single_line_history(const std::vector<std::string>& history) {
    std::vector<std::string> result;
    for (const auto& command : history) {
        if (command.find('\n') == std::string::npos) {
            result.push_back(command);
        }
    }

    return result;
}

void sort_unique(std::vector<std::string>& values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

bool is_completion_delimiter(char c) {
    return std::isspace(static_cast<unsigned char>(c))
        || c == '('
        || c == ')'
        || c == '"';
}

std::size_t completion_start(const std::string& buffer, std::size_t cursor) {
    std::size_t start = cursor;
    while (0 < start && !is_completion_delimiter(buffer[start - 1])) {
        --start;
    }

    return start;
}

std::vector<std::string> matching_completion_candidates(
    std::string_view prefix,
    const std::vector<std::string>& candidates
) {
    std::vector<std::string> matches;
    for (const auto& candidate : candidates) {
        if (candidate.starts_with(prefix)) {
            matches.push_back(candidate);
        }
    }
    sort_unique(matches);

    return matches;
}

std::string common_completion_prefix(const std::vector<std::string>& matches) {
    if (matches.empty()) {
        return {};
    }

    std::string result = matches.front();
    for (std::size_t i = 1; i < matches.size(); ++i) {
        std::size_t prefix_size = 0;
        while (
            prefix_size < result.size()
            && prefix_size < matches[i].size()
            && result[prefix_size] == matches[i][prefix_size]
        ) {
            ++prefix_size;
        }
        result.resize(prefix_size);
    }

    return result;
}

std::optional<std::size_t> parse_history_index(std::string_view command) {
    if (command.size() < 2 || command[0] != ':') {
        return std::nullopt;
    }

    std::size_t result = 0;
    for (std::size_t i = 1; i < command.size(); ++i) {
        const char c = command[i];
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return std::nullopt;
        }
        result = (result * 10) + static_cast<std::size_t>(c - '0');
    }

    return result;
}

struct history_command_t {
    enum class kind_t {
        NONE,
        CONTINUE,
        EXPAND
    };

    kind_t kind;
    std::string command;
};

history_command_t resolve_history_command(
    std::string_view line,
    const std::vector<std::string>& history,
    std::ostream& output
) {
    if (line == ":history") {
        for (std::size_t i = 0; i < history.size(); ++i) {
            output << i + 1 << ' ' << history_display(history[i]) << '\n';
        }
        return history_command_t { .kind = history_command_t::kind_t::CONTINUE, .command = {} };
    }

    if (line == ":!!") {
        if (history.empty()) {
            output << "error: history is empty\n";
            return history_command_t { .kind = history_command_t::kind_t::CONTINUE, .command = {} };
        }
        return history_command_t { .kind = history_command_t::kind_t::EXPAND, .command = history.back() };
    }

    if (const auto history_index = parse_history_index(line); history_index.has_value()) {
        if (*history_index == 0 || history.size() < *history_index) {
            output << std::format("error: history entry {} is not available\n", *history_index);
            return history_command_t { .kind = history_command_t::kind_t::CONTINUE, .command = {} };
        }
        return history_command_t { .kind = history_command_t::kind_t::EXPAND, .command = history[*history_index - 1] };
    }

    return history_command_t { .kind = history_command_t::kind_t::NONE, .command = {} };
}

#if defined(__unix__) || defined(__APPLE__)
class terminal_raw_mode_t {
public:
    terminal_raw_mode_t():
        m_active(false),
        m_original()
    {
        if (tcgetattr(STDIN_FILENO, &m_original) != 0) {
            throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::repl: failed to read terminal attributes, errno {}", errno));
        }

        termios raw = m_original;
        raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON));
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
            throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::repl: failed to set terminal attributes, errno {}", errno));
        }
        m_active = true;
    }

    terminal_raw_mode_t(const terminal_raw_mode_t&) = delete;
    terminal_raw_mode_t& operator=(const terminal_raw_mode_t&) = delete;

    ~terminal_raw_mode_t() {
        if (m_active) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_original);
        }
    }

private:
    bool m_active;
    termios m_original;
};

std::optional<char> read_terminal_char() {
    char c = 0;
    while (true) {
        const ssize_t result = ::read(STDIN_FILENO, &c, 1);
        if (result == 1) {
            return c;
        }
        if (result == 0) {
            return std::nullopt;
        }
        if (errno != EINTR) {
            throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::repl: failed to read terminal input, errno {}", errno));
        }
    }
}

void redraw_terminal_line(
    std::ostream& output,
    std::string_view prompt,
    const std::string& buffer,
    std::size_t cursor
) {
    output << "\r\x1b[2K" << prompt << buffer;
    const auto move_left = buffer.size() - cursor;
    if (0 < move_left) {
        output << "\x1b[" << move_left << 'D';
    }
    output.flush();
}

struct completion_state_t {
    bool pending_list = false;
    std::string prefix;
    std::size_t start = 0;

    void reset() {
        pending_list = false;
        prefix.clear();
        start = 0;
    }
};

void display_completion_matches(
    std::ostream& output,
    std::string_view prompt,
    const std::string& buffer,
    std::size_t cursor,
    const std::vector<std::string>& matches
) {
    output << '\n';
    for (const auto& match : matches) {
        output << match << '\n';
    }
    redraw_terminal_line(output, prompt, buffer, cursor);
}

void replace_completion_prefix(
    std::string& buffer,
    std::size_t& cursor,
    std::size_t start,
    std::string_view replacement
) {
    buffer.replace(start, cursor - start, replacement);
    cursor = start + replacement.size();
}

void complete_terminal_word(
    std::ostream& output,
    std::string_view prompt,
    std::string& buffer,
    std::size_t& cursor,
    const std::vector<std::string>& candidates,
    completion_state_t& state
) {
    const auto start = completion_start(buffer, cursor);
    const auto prefix = buffer.substr(start, cursor - start);
    const auto matches = matching_completion_candidates(prefix, candidates);

    if (matches.empty()) {
        output << '\a';
        output.flush();
        state.reset();
        return ;
    }

    if (matches.size() == 1) {
        replace_completion_prefix(buffer, cursor, start, matches.front());
        redraw_terminal_line(output, prompt, buffer, cursor);
        state.reset();
        return ;
    }

    const auto common_prefix = common_completion_prefix(matches);
    if (prefix.size() < common_prefix.size()) {
        replace_completion_prefix(buffer, cursor, start, common_prefix);
        redraw_terminal_line(output, prompt, buffer, cursor);
        state.pending_list = true;
        state.prefix = common_prefix;
        state.start = start;
        return ;
    }

    if (state.pending_list && state.prefix == prefix && state.start == start) {
        display_completion_matches(output, prompt, buffer, cursor, matches);
        state.reset();
        return ;
    }

    state.pending_list = true;
    state.prefix = prefix;
    state.start = start;
}

std::optional<std::string> read_terminal_line(
    std::ostream& output,
    std::string_view prompt,
    const std::vector<std::string>& history,
    const std::vector<std::string>& completion_candidates
) {
    terminal_raw_mode_t raw_mode;
    std::string buffer;
    std::string draft;
    std::size_t cursor = 0;
    std::size_t history_cursor = history.size();
    completion_state_t completion_state;

    output << prompt;
    output.flush();

    while (true) {
        const auto maybe_c = read_terminal_char();
        if (!maybe_c.has_value()) {
            return std::nullopt;
        }

        const char c = *maybe_c;
        if (c == '\r' || c == '\n') {
            output << '\n';
            return buffer;
        }
        if (c == 4) {
            if (buffer.empty()) {
                output << '\n';
                return std::nullopt;
            }
            completion_state.reset();
            continue ;
        }
        if (c == 3) {
            output << "^C\n";
            return std::string();
        }
        if (c == 127 || c == 8) {
            if (0 < cursor) {
                buffer.erase(cursor - 1, 1);
                --cursor;
                redraw_terminal_line(output, prompt, buffer, cursor);
            }
            completion_state.reset();
            continue ;
        }
        if (c == 27) {
            const auto first = read_terminal_char();
            const auto second = read_terminal_char();
            if (!first.has_value() || !second.has_value() || *first != '[') {
                continue ;
            }

            switch (*second) {
                case 'A':
                    if (!history.empty() && 0 < history_cursor) {
                        if (history_cursor == history.size()) {
                            draft = buffer;
                        }
                        --history_cursor;
                        buffer = history[history_cursor];
                        cursor = buffer.size();
                        redraw_terminal_line(output, prompt, buffer, cursor);
                        completion_state.reset();
                    }
                    break ;
                case 'B':
                    if (history_cursor < history.size()) {
                        ++history_cursor;
                        if (history_cursor == history.size()) {
                            buffer = draft;
                        } else {
                            buffer = history[history_cursor];
                        }
                        cursor = buffer.size();
                        redraw_terminal_line(output, prompt, buffer, cursor);
                        completion_state.reset();
                    }
                    break ;
                case 'C':
                    if (cursor < buffer.size()) {
                        ++cursor;
                        redraw_terminal_line(output, prompt, buffer, cursor);
                        completion_state.reset();
                    }
                    break ;
                case 'D':
                    if (0 < cursor) {
                        --cursor;
                        redraw_terminal_line(output, prompt, buffer, cursor);
                        completion_state.reset();
                    }
                    break ;
                default:
                    break ;
            }
            continue ;
        }
        if (c == '\t') {
            complete_terminal_word(
                output,
                prompt,
                buffer,
                cursor,
                completion_candidates,
                completion_state
            );
            continue ;
        }
        if (std::isprint(static_cast<unsigned char>(c))) {
            buffer.insert(cursor, 1, c);
            ++cursor;
            redraw_terminal_line(output, prompt, buffer, cursor);
            completion_state.reset();
        }
    }
}
#endif

class repl_line_reader_t {
public:
    repl_line_reader_t(std::istream& input, std::ostream& output):
        m_input(input),
        m_output(output),
        m_interactive(is_interactive(input, output))
    {
    }

    std::optional<std::string> read_line(
        std::string_view prompt,
        const std::vector<std::string>& history,
        const std::vector<std::string>& completion_candidates
    ) {
        if (m_interactive) {
#if defined(__unix__) || defined(__APPLE__)
            return read_terminal_line(
                m_output,
                prompt,
                single_line_history(history),
                completion_candidates
            );
#endif
        }

        m_output << prompt;
        m_output.flush();

        std::string line;
        if (!std::getline(m_input, line)) {
            return std::nullopt;
        }

        return line;
    }

private:
    static bool is_interactive(const std::istream& input, const std::ostream& output) {
#if defined(__unix__) || defined(__APPLE__)
        return &input == &std::cin
            && &output == &std::cout
            && isatty(STDIN_FILENO)
            && isatty(STDOUT_FILENO);
#else
        (void)input;
        (void)output;
        return false;
#endif
    }

private:
    std::istream& m_input;
    std::ostream& m_output;
    bool m_interactive;
};

class reader_t {
public:
    explicit reader_t(std::string_view input):
        m_input(input),
        m_position(0)
    {
    }

    std::vector<syntax_t> read_program() {
        std::vector<syntax_t> forms;
        skip_whitespace();
        while (!eof()) {
            if (peek() == ')') {
                throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::read: unexpected ')'");
            }
            forms.push_back(read_form());
            skip_whitespace();
        }

        return forms;
    }

private:
    bool eof() const {
        return m_position == m_input.size();
    }

    char peek() const {
        return m_input[m_position];
    }

    char get() {
        return m_input[m_position++];
    }

    void skip_whitespace() {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
            ++m_position;
        }
    }

    syntax_t read_form() {
        skip_whitespace();
        if (eof()) {
            throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::read: expected expression");
        }

        if (peek() == '(') {
            return read_list();
        }
        if (peek() == '"') {
            return syntax_t {
                .kind = syntax_t::kind_t::string,
                .text = read_string(),
                .elements = {}
            };
        }

        return syntax_t {
            .kind = syntax_t::kind_t::symbol,
            .text = read_symbol(),
            .elements = {}
        };
    }

    syntax_t read_list() {
        get();
        std::vector<syntax_t> elements;
        skip_whitespace();
        while (!eof() && peek() != ')') {
            elements.push_back(read_form());
            skip_whitespace();
        }
        if (eof()) {
            throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::read: unterminated list");
        }
        get();

        return syntax_t {
            .kind = syntax_t::kind_t::list,
            .text = {},
            .elements = std::move(elements)
        };
    }

    std::string read_string() {
        get();
        std::string result;
        while (!eof()) {
            const char c = get();
            if (c == '"') {
                return result;
            }
            if (c == '\\') {
                if (eof()) {
                    throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::read: unterminated string escape");
                }
                const char escaped = get();
                switch (escaped) {
                    case 'n': result.push_back('\n'); break ;
                    case 't': result.push_back('\t'); break ;
                    case '"': result.push_back('"'); break ;
                    case '\\': result.push_back('\\'); break ;
                    default:
                        throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::read: unsupported string escape '\\{}'", escaped));
                }
                continue ;
            }
            result.push_back(c);
        }

        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::read: unterminated string literal");
    }

    std::string read_symbol() {
        std::string result;
        while (!eof()) {
            const char c = peek();
            if (std::isspace(static_cast<unsigned char>(c)) || c == '(' || c == ')' || c == '"') {
                break ;
            }
            result.push_back(get());
        }
        if (result.empty()) {
            throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::read: expected symbol");
        }

        return result;
    }

private:
    std::string_view m_input;
    std::size_t m_position;
};

json syntax_to_json(const syntax_t& syntax) {
    switch (syntax.kind) {
        case syntax_t::kind_t::symbol:
            return json {
                { "kind", "symbol" },
                { "text", syntax.text }
            };
        case syntax_t::kind_t::string:
            return json {
                { "kind", "string" },
                { "text", syntax.text }
            };
        case syntax_t::kind_t::list: {
            json elements = nlohmann::json::array();
            for (const auto& element : syntax.elements) {
                elements.push_back(syntax_to_json(element));
            }
            return json {
                { "kind", "list" },
                { "elements", elements }
            };
        }
    }

    throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::syntax_to_json: unknown syntax kind");
}

syntax_t syntax_from_json(const json& value) {
    if (!value.is_object() || !value.contains("kind") || !value.at("kind").is_string()) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::syntax_from_json: syntax JSON must contain string field 'kind'");
    }

    const auto kind = value.at("kind").get<std::string>();
    if (kind == "symbol" || kind == "string") {
        if (!value.contains("text") || !value.at("text").is_string()) {
            throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::syntax_from_json: symbol/string syntax JSON must contain string field 'text'");
        }

        return syntax_t {
            .kind = kind == "symbol" ? syntax_t::kind_t::symbol : syntax_t::kind_t::string,
            .text = value.at("text").get<std::string>(),
            .elements = {}
        };
    }
    if (kind == "list") {
        if (!value.contains("elements") || !value.at("elements").is_array()) {
            throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::syntax_from_json: list syntax JSON must contain array field 'elements'");
        }

        std::vector<syntax_t> elements;
        for (const auto& element : value.at("elements")) {
            elements.push_back(syntax_from_json(element));
        }

        return syntax_t {
            .kind = syntax_t::kind_t::list,
            .text = {},
            .elements = std::move(elements)
        };
    }

    throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::syntax_from_json: unknown syntax kind '{}'", kind));
}

json syntax_forms_to_json(const std::vector<syntax_t>& forms) {
    json result = nlohmann::json::array();
    for (const auto& form : forms) {
        result.push_back(syntax_to_json(form));
    }

    return result;
}

std::vector<syntax_t> syntax_forms_from_json(const json& value) {
    if (!value.is_array()) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::syntax_forms_from_json: forms must be a JSON array");
    }

    std::vector<syntax_t> result;
    for (const auto& form : value) {
        result.push_back(syntax_from_json(form));
    }

    return result;
}

std::string print_syntax(const syntax_t& syntax);

std::string print_string_literal(std::string_view value) {
    std::string result("\"");
    for (const char c : value) {
        switch (c) {
            case '\n': result += "\\n"; break ;
            case '\t': result += "\\t"; break ;
            case '"': result += "\\\""; break ;
            case '\\': result += "\\\\"; break ;
            default: result.push_back(c); break ;
        }
    }
    result.push_back('"');
    return result;
}

std::string print_syntax(const syntax_t& syntax) {
    switch (syntax.kind) {
        case syntax_t::kind_t::symbol:
            return syntax.text;
        case syntax_t::kind_t::string:
            return print_string_literal(syntax.text);
        case syntax_t::kind_t::list: {
            std::string result("(");
            for (std::size_t i = 0; i < syntax.elements.size(); ++i) {
                if (i != 0) {
                    result.push_back(' ');
                }
                result += print_syntax(syntax.elements[i]);
            }
            result.push_back(')');
            return result;
        }
    }

    throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::print_syntax: unknown syntax kind");
}

struct closure_t;
class environment_t;

using environment_ptr_t = std::shared_ptr<environment_t>;

struct closure_t {
    std::vector<std::string> parameters;
    std::vector<syntax_t> body;
    environment_ptr_t environment;
};

using evaluation_value_t = std::variant<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t, closure_t>;

class environment_t {
public:
    explicit environment_t(environment_ptr_t parent = nullptr):
        m_parent(std::move(parent))
    {
    }

    void define(const std::string& name, evaluation_value_t value) {
        m_values.insert_or_assign(name, std::move(value));
    }

    std::optional<evaluation_value_t> find(const std::string& name) const {
        if (auto it = m_values.find(name); it != m_values.end()) {
            return it->second;
        }
        if (m_parent != nullptr) {
            return m_parent->find(name);
        }

        return std::nullopt;
    }

    std::vector<std::string> names() const {
        std::vector<std::string> result;
        append_names(result);
        sort_unique(result);

        return result;
    }

private:
    void append_names(std::vector<std::string>& result) const {
        if (m_parent != nullptr) {
            m_parent->append_names(result);
        }
        for (const auto& [name, _] : m_values) {
            result.push_back(name);
        }
    }

private:
    environment_ptr_t m_parent;
    std::unordered_map<std::string, evaluation_value_t> m_values;
};

std::vector<std::string> repl_completion_candidates(
    const std::vector<std::string>& module_names,
    const environment_t& environment,
    const std::vector<std::string>& history
) {
    std::vector<std::string> result = {
        "begin",
        "bool?",
        "callable?",
        "capability?",
        "define",
        "equal?",
        "false",
        "get",
        "if",
        "lambda",
        "let",
        "list",
        "list?",
        "load",
        "module?",
        "path?",
        "record",
        "record?",
        "string?",
        "true",
        "unit?",
        ":history",
        ":!!"
    };

    const auto environment_names = environment.names();
    result.insert(result.end(), environment_names.begin(), environment_names.end());

    result.insert(result.end(), module_names.begin(), module_names.end());

    for (std::size_t i = 0; i < history.size(); ++i) {
        result.push_back(std::format(":{}", i + 1));
    }

    sort_unique(result);
    return result;
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t export_runtime_value(const evaluation_value_t& value) {
    if (const auto* runtime_value = std::get_if<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>(&value)) {
        return *runtime_value;
    }

    throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: closures cannot cross the runtime value boundary");
}

const syntax_t& require_list_element(
    const syntax_t& syntax,
    std::size_t index,
    std::string_view form_name
) {
    if (syntax.kind != syntax_t::kind_t::list || syntax.elements.size() <= index) {
        throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::evaluate: malformed {} form", form_name));
    }

    return syntax.elements[index];
}

std::string require_symbol(const syntax_t& syntax, std::string_view context) {
    if (syntax.kind != syntax_t::kind_t::symbol) {
        throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::evaluate: {} must be a symbol", context));
    }

    return syntax.text;
}

void require_expression_size(
    const syntax_t& expression,
    std::size_t size,
    std::string_view form_name
) {
    if (expression.elements.size() != size) {
        throw std::runtime_error(std::format(
            "m03gubnevca18el75zd2vx8qxh_language::evaluate: {} requires {} argument{}",
            form_name,
            size - 1,
            size == 2 ? "" : "s"
        ));
    }
}

std::vector<std::string> require_parameter_list(const syntax_t& syntax) {
    if (syntax.kind != syntax_t::kind_t::list) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: lambda parameters must be a list");
    }

    std::vector<std::string> parameters;
    for (const auto& parameter : syntax.elements) {
        parameters.push_back(require_symbol(parameter, "lambda parameter"));
    }

    return parameters;
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t require_runtime_value(
    const evaluation_value_t& value,
    std::string_view context
) {
    if (const auto* runtime_value = std::get_if<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>(&value)) {
        return *runtime_value;
    }

    throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::evaluate: {} cannot be a closure", context));
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_value_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value, std::string_view context) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_path(value)) {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
    }

    throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::evaluate: {} must be a string or path value", context));
}

evaluation_value_t evaluate_expression(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
);

evaluation_value_t evaluate_program(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const std::vector<syntax_t>& forms,
    const environment_ptr_t& environment
) {
    evaluation_value_t result = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::unit_value();
    for (const auto& form : forms) {
        result = evaluate_expression(workspace_graph, form, environment);
    }

    return result;
}

evaluation_value_t evaluate_loaded_file(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path,
    const environment_ptr_t& environment
) {
    return evaluate_program(workspace_graph, read(read_file(path)), environment);
}

evaluation_value_t evaluate_define(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    if (expression.elements.size() < 3) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: define requires a name and value");
    }

    const auto& target = require_list_element(expression, 1, "define");
    if (target.kind == syntax_t::kind_t::symbol) {
        if (expression.elements.size() != 3) {
            throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: value define requires exactly one expression");
        }
        environment->define(target.text, evaluate_expression(workspace_graph, expression.elements[2], environment));
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::unit_value();
    }

    if (target.kind == syntax_t::kind_t::list && !target.elements.empty()) {
        const auto name = require_symbol(target.elements[0], "function define name");
        std::vector<syntax_t> lambda_body;
        lambda_body.insert(lambda_body.end(), expression.elements.begin() + 2, expression.elements.end());
        std::vector<std::string> parameters;
        for (std::size_t i = 1; i < target.elements.size(); ++i) {
            parameters.push_back(require_symbol(target.elements[i], "function define parameter"));
        }

        environment->define(name, closure_t {
            .parameters = std::move(parameters),
            .body = std::move(lambda_body),
            .environment = environment
        });
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::unit_value();
    }

    throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: define target must be a symbol or function signature");
}

evaluation_value_t evaluate_lambda(const syntax_t& expression, const environment_ptr_t& environment) {
    if (expression.elements.size() < 3) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: lambda requires parameters and body");
    }

    std::vector<syntax_t> body;
    body.insert(body.end(), expression.elements.begin() + 2, expression.elements.end());

    return closure_t {
        .parameters = require_parameter_list(expression.elements[1]),
        .body = std::move(body),
        .environment = environment
    };
}

evaluation_value_t evaluate_if(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    require_expression_size(expression, 4, "if");

    const auto condition = require_runtime_value(
        evaluate_expression(workspace_graph, expression.elements[1], environment),
        "if condition"
    );
    if (!m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_bool(condition)) {
        throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::evaluate: if condition must be '{}'", m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_type_module()));
    }

    return evaluate_expression(
        workspace_graph,
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_bool(condition) ? expression.elements[2] : expression.elements[3],
        environment
    );
}

evaluation_value_t evaluate_begin(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    std::vector<syntax_t> body;
    body.insert(body.end(), expression.elements.begin() + 1, expression.elements.end());
    return evaluate_program(workspace_graph, body, environment);
}

evaluation_value_t evaluate_let(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    if (expression.elements.size() < 3) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: let requires bindings and body");
    }
    if (expression.elements[1].kind != syntax_t::kind_t::list) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: let bindings must be a list");
    }

    auto local_environment = std::make_shared<environment_t>(environment);
    for (const auto& binding : expression.elements[1].elements) {
        if (binding.kind != syntax_t::kind_t::list || binding.elements.size() != 2) {
            throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: let binding must be a two-element list");
        }
        local_environment->define(
            require_symbol(binding.elements[0], "let binding name"),
            evaluate_expression(workspace_graph, binding.elements[1], environment)
        );
    }

    std::vector<syntax_t> body;
    body.insert(body.end(), expression.elements.begin() + 2, expression.elements.end());
    return evaluate_program(workspace_graph, body, local_environment);
}

evaluation_value_t evaluate_list(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t> values;
    for (std::size_t i = 1; i < expression.elements.size(); ++i) {
        values.push_back(require_runtime_value(
            evaluate_expression(workspace_graph, expression.elements[i], environment),
            "list element"
        ));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::list_value(values);
}

evaluation_value_t evaluate_record(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    if ((expression.elements.size() - 1) % 2 != 0) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: record requires field/value pairs");
    }

    std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_field_t> fields;
    for (std::size_t i = 1; i < expression.elements.size(); i += 2) {
        const auto field = require_runtime_value(
            evaluate_expression(workspace_graph, expression.elements[i], environment),
            "record field name"
        );
        if (!m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(field)) {
            throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: record field name must evaluate to a string");
        }
        fields.push_back({
            m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(field),
            require_runtime_value(
                evaluate_expression(workspace_graph, expression.elements[i + 1], environment),
                "record field value"
            )
        });
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value(fields);
}

evaluation_value_t evaluate_get(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    require_expression_size(expression, 3, "get");

    const auto record = require_runtime_value(
        evaluate_expression(workspace_graph, expression.elements[1], environment),
        "get record"
    );
    const auto field = require_runtime_value(
        evaluate_expression(workspace_graph, expression.elements[2], environment),
        "get field"
    );
    if (!m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(field)) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: get field must evaluate to a string");
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_field(record, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(field));
}

evaluation_value_t evaluate_load(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    require_expression_size(expression, 2, "load");

    const auto path = path_value_arg(
        require_runtime_value(
            evaluate_expression(workspace_graph, expression.elements[1], environment),
            "load path"
        ),
        "load path"
    );
    return evaluate_loaded_file(workspace_graph, path, environment);
}

evaluation_value_t evaluate_equal(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    require_expression_size(expression, 3, "equal?");

    const auto lhs = require_runtime_value(
        evaluate_expression(workspace_graph, expression.elements[1], environment),
        "equal? left operand"
    );
    const auto rhs = require_runtime_value(
        evaluate_expression(workspace_graph, expression.elements[2], environment),
        "equal? right operand"
    );

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(lhs.type_module == rhs.type_module && lhs.data == rhs.data);
}

evaluation_value_t evaluate_predicate(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment,
    std::string_view predicate
) {
    require_expression_size(expression, 2, predicate);

    const auto value = evaluate_expression(workspace_graph, expression.elements[1], environment);
    if (predicate == "callable?") {
        if (std::holds_alternative<closure_t>(value)) {
            return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(true);
        }
        const auto runtime_value = require_runtime_value(value, "callable? value");
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(
            is_module_procedure_value(runtime_value)
            || m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_capability(runtime_value)
        );
    }
    if (std::holds_alternative<closure_t>(value)) {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(false);
    }

    const auto runtime_value = require_runtime_value(value, std::format("{} value", predicate));
    if (predicate == "string?") {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(runtime_value));
    }
    if (predicate == "path?") {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_path(runtime_value));
    }
    if (predicate == "unit?") {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_unit(runtime_value));
    }
    if (predicate == "bool?") {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_bool(runtime_value));
    }
    if (predicate == "list?") {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_list(runtime_value));
    }
    if (predicate == "record?") {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_record(runtime_value));
    }
    if (predicate == "capability?") {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_capability(runtime_value));
    }
    if (predicate == "module?") {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(is_module_procedure_value(runtime_value));
    }

    throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::evaluate: unknown predicate '{}'", predicate));
}

evaluation_value_t apply_closure(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const closure_t& closure,
    const std::vector<evaluation_value_t>& args
) {
    if (closure.parameters.size() != args.size()) {
        throw std::runtime_error(std::format(
            "m03gubnevca18el75zd2vx8qxh_language::evaluate: closure expected {} arguments, got {}",
            closure.parameters.size(),
            args.size()
        ));
    }

    auto local_environment = std::make_shared<environment_t>(closure.environment);
    for (std::size_t i = 0; i < closure.parameters.size(); ++i) {
        local_environment->define(closure.parameters[i], args[i]);
    }

    return evaluate_program(workspace_graph, closure.body, local_environment);
}

evaluation_value_t apply_runtime_value(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& procedure,
    const std::vector<evaluation_value_t>& args
) {
    if (!is_module_procedure_value(procedure) && !m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_capability(procedure)) {
        throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::evaluate: value of type '{}' is not applicable", procedure.type_module));
    }

    std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t> runtime_args;
    runtime_args.reserve(args.size());
    for (const auto& arg : args) {
        runtime_args.push_back(export_runtime_value(arg));
    }

    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_capability(procedure)) {
        return m03gubnevc9z8dwzxigmj54y25_lisp_module_command::apply_capability(
            workspace_graph,
            m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_module(procedure),
            m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_name(procedure),
            runtime_args
        );
    }

    return m03gubnevc9z8dwzxigmj54y25_lisp_module_command::apply(
        workspace_graph,
        module_procedure_name(procedure),
        runtime_args
    );
}

evaluation_value_t evaluate_application(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    if (expression.elements.empty()) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: cannot evaluate empty application");
    }

    const auto procedure = evaluate_expression(workspace_graph, expression.elements[0], environment);
    std::vector<evaluation_value_t> args;
    for (std::size_t i = 1; i < expression.elements.size(); ++i) {
        args.push_back(evaluate_expression(workspace_graph, expression.elements[i], environment));
    }

    if (const auto* closure = std::get_if<closure_t>(&procedure)) {
        return apply_closure(workspace_graph, *closure, args);
    }
    if (const auto* runtime_value = std::get_if<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>(&procedure)) {
        return apply_runtime_value(workspace_graph, *runtime_value, args);
    }

    throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: unsupported procedure value");
}

evaluation_value_t evaluate_expression(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const syntax_t& expression,
    const environment_ptr_t& environment
) {
    switch (expression.kind) {
        case syntax_t::kind_t::string:
            return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(expression.text);
        case syntax_t::kind_t::symbol:
            if (expression.text == "true") {
                return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(true);
            }
            if (expression.text == "false") {
                return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(false);
            }
            if (auto value = environment->find(expression.text); value.has_value()) {
                return value.value();
            }

            m03gubnevc9z8dwzxigmj54y25_lisp_module_command::resolve_module(workspace_graph, m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(expression.text));
            return module_procedure_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(expression.text));
        case syntax_t::kind_t::list:
            if (!expression.elements.empty() && expression.elements[0].kind == syntax_t::kind_t::symbol) {
                if (expression.elements[0].text == "define") {
                    return evaluate_define(workspace_graph, expression, environment);
                }
                if (expression.elements[0].text == "lambda") {
                    return evaluate_lambda(expression, environment);
                }
                if (expression.elements[0].text == "if") {
                    return evaluate_if(workspace_graph, expression, environment);
                }
                if (expression.elements[0].text == "begin") {
                    return evaluate_begin(workspace_graph, expression, environment);
                }
                if (expression.elements[0].text == "let") {
                    return evaluate_let(workspace_graph, expression, environment);
                }
                if (expression.elements[0].text == "list") {
                    return evaluate_list(workspace_graph, expression, environment);
                }
                if (expression.elements[0].text == "record") {
                    return evaluate_record(workspace_graph, expression, environment);
                }
                if (expression.elements[0].text == "get") {
                    return evaluate_get(workspace_graph, expression, environment);
                }
                if (expression.elements[0].text == "load") {
                    return evaluate_load(workspace_graph, expression, environment);
                }
                if (expression.elements[0].text == "equal?") {
                    return evaluate_equal(workspace_graph, expression, environment);
                }
                if (
                    expression.elements[0].text == "string?"
                    || expression.elements[0].text == "path?"
                    || expression.elements[0].text == "unit?"
                    || expression.elements[0].text == "bool?"
                    || expression.elements[0].text == "list?"
                    || expression.elements[0].text == "record?"
                    || expression.elements[0].text == "capability?"
                    || expression.elements[0].text == "module?"
                    || expression.elements[0].text == "callable?"
                ) {
                    return evaluate_predicate(
                        workspace_graph,
                        expression,
                        environment,
                        expression.elements[0].text
                    );
                }
            }

            return evaluate_application(workspace_graph, expression, environment);
    }

    throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::evaluate: unknown syntax kind");
}

} // namespace

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t type_module() {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("language");
}

std::vector<syntax_t> read(std::string_view input) {
    return reader_t(input).read_program();
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t syntax_value(const std::vector<syntax_t>& forms) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t {
        .type_module = type_module(),
        .data = json {
            { "kind", "syntax" },
            { "forms", syntax_forms_to_json(forms) }
        }
    };
}

bool is_syntax_value(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    return value.type_module == type_module()
        && value.data.is_object()
        && value.data.contains("kind")
        && value.data.at("kind") == "syntax";
}

std::vector<syntax_t> syntax_from_value(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (!is_syntax_value(value)) {
        throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::syntax_from_value: value type '{}' is not syntax", value.type_module));
    }
    if (!value.data.contains("forms")) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::syntax_from_value: syntax value is missing forms");
    }

    return syntax_forms_from_json(value.data.at("forms"));
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module_procedure_value(const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& module_name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t {
        .type_module = type_module(),
        .data = json {
            { "kind", "module_procedure" },
            { "module", module_name.string() }
        }
    };
}

bool is_module_procedure_value(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    return value.type_module == type_module()
        && value.data.is_object()
        && value.data.contains("kind")
        && value.data.at("kind") == "module_procedure";
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t module_procedure_name(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (!is_module_procedure_value(value)) {
        throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::module_procedure_name: value type '{}' is not a module procedure", value.type_module));
    }
    if (!value.data.contains("module") || !value.data.at("module").is_string()) {
        throw std::runtime_error("m03gubnevca18el75zd2vx8qxh_language::module_procedure_name: module procedure value must contain string field 'module'");
    }

    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(value.data.at("module").get<std::string>());
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t as_module_name(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }
    if (is_module_procedure_value(value)) {
        return module_procedure_name(value);
    }

    throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::as_module_name: value type '{}' is not a module name string or module procedure", value.type_module));
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t evaluate(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const std::vector<syntax_t>& forms
) {
    return export_runtime_value(evaluate_program(
        workspace_graph,
        forms,
        std::make_shared<environment_t>()
    ));
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t evaluate(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    std::string_view input
) {
    return evaluate(workspace_graph, read(input));
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t load(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path
) {
    return export_runtime_value(evaluate_loaded_file(
        workspace_graph,
        path,
        std::make_shared<environment_t>()
    ));
}

std::string print_syntax(const std::vector<syntax_t>& forms) {
    std::string result;
    for (std::size_t i = 0; i < forms.size(); ++i) {
        if (i != 0) {
            result.push_back(' ');
        }
        result += print_syntax(forms[i]);
    }

    return result;
}

std::string print_list(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    const auto elements = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_list(value);
    std::string result("[");
    for (std::size_t i = 0; i < elements.size(); ++i) {
        if (i != 0) {
            result += " ";
        }
        result += print(elements[i]);
    }
    result.push_back(']');

    return result;
}

std::string print_record(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    const auto fields = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_record(value);
    std::string result("{");
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i != 0) {
            result += " ";
        }
        result += print_string_literal(fields[i].first);
        result += ": ";
        result += print(fields[i].second);
    }
    result.push_back('}');

    return result;
}

std::string print(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value);
    }
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_path(value)) {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value).string();
    }
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_unit(value)) {
        return "unit";
    }
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_bool(value)) {
        return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_bool(value) ? "true" : "false";
    }
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_list(value)) {
        return print_list(value);
    }
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_record(value)) {
        return print_record(value);
    }
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_capability(value)) {
        return std::format(
            "<capability {}.{}>",
            m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_module(value),
            m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_name(value)
        );
    }
    if (is_syntax_value(value)) {
        return print_syntax(syntax_from_value(value));
    }
    if (is_module_procedure_value(value)) {
        return module_procedure_name(value).string();
    }

    return value.data.dump();
}

void repl(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    std::istream& input,
    std::ostream& output
) {
    auto environment = std::make_shared<environment_t>();
    repl_line_reader_t line_reader(input, output);
    const auto module_names = m03gubnevc9z8dwzxigmj54y25_lisp_module_command::completion_names(workspace_graph);
    std::vector<std::string> history;
    std::string pending;

    while (true) {
        const auto completions = repl_completion_candidates(module_names, *environment, history);
        const auto line = line_reader.read_line(pending.empty() ? "> " : "| ", history, completions);
        if (!line.has_value()) {
            if (!pending.empty()) {
                output << "error: incomplete input\n";
            }
            break ;
        }

        if (pending.empty() && is_blank(*line)) {
            continue ;
        }

        if (pending.empty()) {
            const auto history_command = resolve_history_command(*line, history, output);
            if (history_command.kind == history_command_t::kind_t::CONTINUE) {
                continue ;
            }
            if (history_command.kind == history_command_t::kind_t::EXPAND) {
                pending = history_command.command;
            } else {
                pending = *line;
            }
        } else {
            pending.push_back('\n');
            pending += *line;
        }

        try {
            const auto forms = read(pending);
            append_history(history, pending);
            const auto result = export_runtime_value(evaluate_program(
                workspace_graph,
                forms,
                environment
            ));
            output << print(result) << '\n';
        } catch (const std::exception& e) {
            if (is_incomplete_read_error(e.what())) {
                continue ;
            }
            append_history(history, pending);
            output << "error: " << e.what() << '\n';
        }

        pending.clear();
    }
}

} // namespace m03gubnevca18el75zd2vx8qxh_language
