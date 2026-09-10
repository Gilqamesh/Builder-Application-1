#include <m03gubnevca18el75zd2vx8qxh_language/language.h>
#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevca18el75zd2vx8qxh_language {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gubnevca18el75zd2vx8qxh_language::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t language_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("language"),
        name
    );
}

m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph() {
    const auto invocation_context = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::invocation_context();
    return m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t(
        invocation_context.workspace_root,
        invocation_context.artifact_root
    );
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "eval", language_capability("eval") },
        { "print", language_capability("print") },
        { "read", language_capability("read") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__eval(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "eval");
    auto graph = workspace_graph();
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(args[0])) {
        return m03gubnevca18el75zd2vx8qxh_language::evaluate(graph, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0]));
    }
    if (m03gubnevca18el75zd2vx8qxh_language::is_syntax_value(args[0])) {
        return m03gubnevca18el75zd2vx8qxh_language::evaluate(graph, m03gubnevca18el75zd2vx8qxh_language::syntax_from_value(args[0]));
    }

    throw std::runtime_error(std::format("m03gubnevca18el75zd2vx8qxh_language::eval: unsupported argument type '{}'", args[0].type_module));
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__print(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "print");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(m03gubnevca18el75zd2vx8qxh_language::print(args[0]));
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__read(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "read");
    return m03gubnevca18el75zd2vx8qxh_language::syntax_value(m03gubnevca18el75zd2vx8qxh_language::read(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0])));
}

} // namespace m03gubnevca18el75zd2vx8qxh_language
