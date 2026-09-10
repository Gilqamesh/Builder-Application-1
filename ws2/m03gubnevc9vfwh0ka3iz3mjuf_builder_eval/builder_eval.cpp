#include "builder_eval.h"

#include <m03gubnevca18el75zd2vx8qxh_language/language.h>

#include <format>
#include <stdexcept>

namespace m03gubnevc9vfwh0ka3iz3mjuf_builder_eval {

namespace {

m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t invocation_workspace_graph() {
    const auto invocation_context = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::invocation_context();
    return m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t(
        invocation_context.workspace_root,
        invocation_context.artifact_root
    );
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t evaluate_file_with_graph(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path
) {
    return m03gubnevca18el75zd2vx8qxh_language::load(workspace_graph, path);
}

} // namespace

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t evaluate_file(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) {
    auto workspace_graph = invocation_workspace_graph();
    return evaluate_file_with_graph(workspace_graph, path);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 1) {
        throw std::runtime_error(std::format("m03gubnevc9vfwh0ka3iz3mjuf_builder_eval::apply: expected 1 argument, got {}", args.size()));
    }

    auto workspace_graph = invocation_workspace_graph();
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_path(args[0])) {
        return evaluate_file_with_graph(workspace_graph, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(args[0]));
    }
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(args[0])) {
        return m03gubnevca18el75zd2vx8qxh_language::evaluate(workspace_graph, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0]));
    }
    if (m03gubnevca18el75zd2vx8qxh_language::is_syntax_value(args[0])) {
        return m03gubnevca18el75zd2vx8qxh_language::evaluate(workspace_graph, m03gubnevca18el75zd2vx8qxh_language::syntax_from_value(args[0]));
    }

    throw std::runtime_error(std::format("m03gubnevc9vfwh0ka3iz3mjuf_builder_eval::apply: unsupported argument type '{}'", args[0].type_module));
}

} // namespace m03gubnevc9vfwh0ka3iz3mjuf_builder_eval
