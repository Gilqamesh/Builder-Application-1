#include <m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer/module_dependency_ir_dot_renderer.h>

#include <fstream>
#include <format>
#include <stdexcept>
#include <unordered_map>

namespace m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer {

static std::string dot_string(std::string_view value) {
    std::string result = "\"";

    for (const char c : value) {
        if (c == '\n') {
            result += "\\n";
            continue ;
        }
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        }
        result.push_back(c);
    }

    result.push_back('"');
    return result;
}

static std::unordered_map<std::string, std::string> module_node_ids(
    const m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t& dependency_ir
) {
    std::unordered_map<std::string, std::string> result;
    std::size_t index = 0;

    for (const auto& workspace : dependency_ir.workspaces) {
        for (const auto& module : workspace.modules) {
            const auto [_, inserted] = result.emplace(module.name, std::format("m{}", index));
            if (!inserted) {
                throw std::runtime_error(std::format(
                    "m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer::module_node_ids: duplicate module '{}'",
                    module.name
                ));
            }
            ++index;
        }
    }

    return result;
}

static const std::string& dependency_node_id(
    const std::unordered_map<std::string, std::string>& node_id_by_module,
    const std::string& dependency_name
) {
    const auto it = node_id_by_module.find(dependency_name);
    if (it == node_id_by_module.end()) {
        throw std::runtime_error(std::format(
            "m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer::dependency_node_id: dependency '{}' is not present in the dependency IR",
            dependency_name
        ));
    }

    return it->second;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render(
    const m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t& dependency_ir,
    std::string_view target_module_name,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_dot_path
) {
    if (output_dot_path.extension() != ".dot") {
        throw std::runtime_error(std::format("m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer::render: output path '{}' must have .dot extension", output_dot_path));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_dot_path)) {
        throw std::runtime_error(std::format("m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer::render: output path '{}' already exists", output_dot_path));
    }

    const auto output_parent = output_dot_path.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_parent)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(output_parent);
    }

    const auto node_id_by_module = module_node_ids(dependency_ir);

    std::ofstream ofs(output_dot_path.string(), std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error(std::format("m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer::render: failed to open output file '{}'", output_dot_path));
    }

    ofs << "digraph BuilderModuleDependencyGraph {\n";
    ofs << "  graph [rankdir=LR, splines=true, nodesep=0.6, ranksep=1.2, bgcolor=\"white\"];\n";
    ofs << "  node [shape=box, style=\"rounded,filled\", fillcolor=\"#F5F7FB\", color=\"#9AA4B2\", fontname=\"Inter,Helvetica,Arial\", fontsize=11];\n";
    ofs << "  edge [fontname=\"Inter,Helvetica,Arial\", fontsize=9, arrowsize=0.75];\n\n";

    for (std::size_t workspace_index = 0; workspace_index < dependency_ir.workspaces.size(); ++workspace_index) {
        const auto& workspace = dependency_ir.workspaces[workspace_index];

        ofs << std::format("  subgraph cluster_workspace_{} {{\n", workspace_index);
        ofs << std::format("    label={};\n", dot_string(workspace.name));
        ofs << "    labelloc=\"t\";\n";
        ofs << "    fontsize=12;\n";
        ofs << "    color=\"#CBD5E1\";\n";
        ofs << "    style=\"rounded\";\n\n";

        for (const auto& module : workspace.modules) {
            const auto target_style = module.name == target_module_name ? ", penwidth=2.2, color=\"#111827\", fillcolor=\"#E0F2FE\"" : "";
            ofs << std::format(
                "    {} [label={}{}];\n",
                node_id_by_module.at(module.name),
                dot_string(module.name),
                target_style
            );
        }

        ofs << "  }\n\n";
    }

    for (const auto& workspace : dependency_ir.workspaces) {
        for (const auto& module : workspace.modules) {
            const auto& module_node = node_id_by_module.at(module.name);

            for (const auto& dependency : module.module_dependencies) {
                ofs << std::format(
                    "  {} -> {} [label=\"module\", color=\"#2563EB\", fontcolor=\"#1D4ED8\"];\n",
                    dependency_node_id(node_id_by_module, dependency),
                    module_node
                );
            }

            for (const auto& dependency : module.builder_dependencies) {
                ofs << std::format(
                    "  {} -> {} [label=\"builder\", color=\"#D97706\", fontcolor=\"#B45309\", style=\"dashed\"];\n",
                    dependency_node_id(node_id_by_module, dependency),
                    module_node
                );
            }
        }
    }

    ofs << "}\n";

    if (!ofs) {
        throw std::runtime_error(std::format("m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer::render: failed while writing output file '{}'", output_dot_path));
    }

    return output_dot_path;
}

} // namespace m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer
