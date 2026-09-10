#ifndef M03GAGBHTAHG11WZN32IDILZTE_MODULE_DEPENDENCY_IR_FROM_WORKSPACE_GRAPH_MODULE_DEPENDENCY_IR_FROM_WORKSPACE_GRAPH_H
# define M03GAGBHTAHG11WZN32IDILZTE_MODULE_DEPENDENCY_IR_FROM_WORKSPACE_GRAPH_MODULE_DEPENDENCY_IR_FROM_WORKSPACE_GRAPH_H

# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
# include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>

namespace m03gagbhtahg11wzn32idilzte_module_dependency_ir_from_workspace_graph {

/**
 * @brief Copies materialized modules into workspace-grouped IR with scanned direct dependencies.
 *
 * Captures workspace_graph.modules() at entry, then emits every workspace,
 * including empty ones, in graph order. Modules retain graph order within each
 * workspace. Indexed but unmaterialized modules are not included. Names are
 * copied as complete module identities and workspace relative-path labels.
 *
 * Each captured module is scanned through
 * m03gagbhsujjf63n0w3r2w4q6h_build_phases::discover_module_dependencies(): its
 * library source set supplies module dependencies and builder.cpp supplies
 * builder dependencies. That API and the source_dependencies module own source
 * selection, include handling, eligibility and dependency ordering.
 *
 * Scanning may materialize dependencies in the graph despite the const reference;
 * they are not added to this call's captured module list. The returned IR can
 * therefore name dependencies absent from its module records, which the IR
 * renderers reject. Materialize the desired modules and their dependencies before
 * calling; this operation does not compute a transitive closure.
 *
 * Keep the graph alive and serialize access while scanning. The returned IR owns
 * its strings/vectors and can outlive the graph. Discovery, source traversal,
 * unreadable/missing input (including builder.cpp), and ineligible-dependency
 * failures propagate; no partial IR is returned, but graph discovery is not
 * rolled back. Unrecognized or unindexed include prefixes follow the scanner's
 * local/external-include handling, rather than guaranteeing a missing-module error.
 *
 * For an existing workspace with readable library sources and builder.cpp files,
 * materialize all indexed modules before converting. Invocation roots follow
 * m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context().
 * @code{.cpp}
 * #include <m03gagbhtahg11wzn32idilzte_module_dependency_ir_from_workspace_graph/module_dependency_ir_from_workspace_graph.h>
 * #include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
 *
 * namespace graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
 * namespace conversion = m03gagbhtahg11wzn32idilzte_module_dependency_ir_from_workspace_graph;
 *
 * int main() {
 *     const auto invocation_context = graph::invocation_context();
 *     graph::workspace_graph_t workspace_graph(
 *         invocation_context.workspace_root, invocation_context.artifact_root
 *     );
 *     for (const auto& module_name : workspace_graph.module_names()) {
 *         workspace_graph.discover_module(module_name);
 *     }
 *     const auto dependency_ir = conversion::from_workspace_graph(workspace_graph);
 *     // dependency_ir owns its snapshot; later graph changes do not update it.
 * }
 * @endcode
 */
m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t from_workspace_graph(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph
);

} // namespace m03gagbhtahg11wzn32idilzte_module_dependency_ir_from_workspace_graph

#endif // M03GAGBHTAHG11WZN32IDILZTE_MODULE_DEPENDENCY_IR_FROM_WORKSPACE_GRAPH_MODULE_DEPENDENCY_IR_FROM_WORKSPACE_GRAPH_H
