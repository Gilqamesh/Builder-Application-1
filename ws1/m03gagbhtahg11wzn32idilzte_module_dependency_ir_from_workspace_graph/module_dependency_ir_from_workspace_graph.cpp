#include <m03gagbhtahg11wzn32idilzte_module_dependency_ir_from_workspace_graph/module_dependency_ir_from_workspace_graph.h>

#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>

#include <utility>

namespace m03gagbhtahg11wzn32idilzte_module_dependency_ir_from_workspace_graph {

m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t from_workspace_graph(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph
) {
    m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t result;
    const auto modules = workspace_graph.modules();

    for (const auto* workspace : workspace_graph.workspaces()) {
        m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::workspace_t ir_workspace {
            .name = workspace->name().relative_path().string(),
            .modules = {}
        };

        for (const auto* module : modules) {
            if (&module->workspace() != workspace) {
                continue ;
            }

            m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_t ir_module {
                .name = module->name().unique_name(),
                .module_dependencies = {},
                .builder_dependencies = {}
            };

            for (const auto* dependency : module->dependencies()) {
                ir_module.module_dependencies.push_back(dependency->name().unique_name());
            }

            for (const auto* dependency : module->builder_dependencies()) {
                ir_module.builder_dependencies.push_back(dependency->name().unique_name());
            }

            ir_workspace.modules.push_back(std::move(ir_module));
        }

        result.workspaces.push_back(std::move(ir_workspace));
    }

    return result;
}

} // namespace m03gagbhtahg11wzn32idilzte_module_dependency_ir_from_workspace_graph
