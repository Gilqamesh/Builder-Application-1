#include "builder_repl.h"

#include <m03gubnevca18el75zd2vx8qxh_language/language.h>

#include <format>
#include <iostream>
#include <stdexcept>

namespace m03gubnevc9uwrppfipf0i15zk_builder_repl {

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (!args.empty()) {
        throw std::runtime_error(std::format("m03gubnevc9uwrppfipf0i15zk_builder_repl::apply: expected 0 arguments, got {}", args.size()));
    }

    const auto invocation_context = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::invocation_context();
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(
        invocation_context.workspace_root,
        invocation_context.artifact_root
    );
    m03gubnevca18el75zd2vx8qxh_language::repl(workspace_graph, std::cin, std::cout);

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::unit_value();
}

} // namespace m03gubnevc9uwrppfipf0i15zk_builder_repl
