#ifndef M03GUBNEVCA18EL75ZD2VX8QXH_LANGUAGE_LANGUAGE_H
# define M03GUBNEVCA18EL75ZD2VX8QXH_LANGUAGE_LANGUAGE_H

# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
# include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

# include <m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h>

# include <iosfwd>
# include <string>
# include <string_view>
# include <vector>

namespace m03gubnevca18el75zd2vx8qxh_language {

/** @brief A parsed Lisp symbol, string or list. */
struct syntax_t {
    enum class kind_t {
        symbol,
        string,
        list
    };

    kind_t kind;
    std::string text;
    std::vector<syntax_t> elements;
};

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t type_module();

/** @brief Parses Lisp forms, rejecting malformed lists, strings and escapes. */
std::vector<syntax_t> read(std::string_view input);
m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t syntax_value(const std::vector<syntax_t>& forms);
bool is_syntax_value(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value);
std::vector<syntax_t> syntax_from_value(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value);

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module_procedure_value(const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& module_name);
bool is_module_procedure_value(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value);
m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t module_procedure_name(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value);
m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t as_module_name(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value);

/** @brief Evaluates forms with lexical scope and native module capabilities. */
m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t evaluate(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const std::vector<syntax_t>& forms
);
m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t evaluate(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    std::string_view input
);
m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t load(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path
);

std::string print_syntax(const std::vector<syntax_t>& forms);
std::string print(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value);

/** @brief Evaluates commands in a persistent environment with multiline input, completion and history. */
void repl(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    std::istream& input,
    std::ostream& output
);

} // namespace m03gubnevca18el75zd2vx8qxh_language

namespace std {

template <> struct formatter<m03gubnevca18el75zd2vx8qxh_language::syntax_t>;
template <> struct formatter<m03gubnevca18el75zd2vx8qxh_language::syntax_t::kind_t>;

} // namespace std

namespace std {

template <> struct formatter<m03gubnevca18el75zd2vx8qxh_language::syntax_t> : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};
template <> struct formatter<m03gubnevca18el75zd2vx8qxh_language::syntax_t::kind_t> : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

} // namespace std

#endif // M03GUBNEVCA18EL75ZD2VX8QXH_LANGUAGE_LANGUAGE_H
