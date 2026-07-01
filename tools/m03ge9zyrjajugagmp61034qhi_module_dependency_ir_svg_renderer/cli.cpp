#include <m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer/module_dependency_ir_svg_renderer.h>
#include <m03gagbhtahg11wzn32idilzte_module_dependency_ir_from_workspace_graph/module_dependency_ir_from_workspace_graph.h>

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << std::format("usage: {} <target_module_name> <output_svg_path>", argv[0]) << std::endl;
        return 1;
    }

    const auto target_module_name = std::string_view(argv[1]);
    const auto output_svg_path = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(argv[2]);

    try {
        const auto invocation_context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(
            invocation_context.workspace_root,
            invocation_context.artifact_root
        );
        auto* target_module = workspace_graph.discover_module(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t(target_module_name));

        const auto dependency_ir = m03gagbhtahg11wzn32idilzte_module_dependency_ir_from_workspace_graph::from_workspace_graph(workspace_graph);
        const auto output = m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer::render(
            dependency_ir,
            target_module_name,
            output_svg_path
        );
        std::cout << output.string() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
