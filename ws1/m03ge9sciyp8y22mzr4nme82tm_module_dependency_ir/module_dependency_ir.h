#ifndef M03GE9SCIYP8Y22MZR4NME82TM_MODULE_DEPENDENCY_IR_MODULE_DEPENDENCY_IR_H
# define M03GE9SCIYP8Y22MZR4NME82TM_MODULE_DEPENDENCY_IR_MODULE_DEPENDENCY_IR_H

# include <vector>
# include <string>

namespace m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir {

/**
 * @brief Owns one module's name and its direct module and builder dependency names.
 *
 * A dependency entry means this module depends on the named module: the vectors
 * belong to the dependent, not the dependency. Module dependencies describe its
 * library source set; builder dependencies describe its builder.cpp producer.
 * Source selection and eligibility belong to
 * m03gagbhsujjf63n0w3r2w4q6h_build_phases::discover_module_dependencies() and
 * m03gn8rf3pe86v64vphnaam6rl_source_dependencies::scan_sources().
 *
 * Workspace-derived names use the complete directory identity
 * `m<25-character-base36-encoded-UUIDv7>_<friendly_name>`, as validated by
 * m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t. These mutable
 * aggregates also accept arbitrary strings; construction performs no validation.
 */
struct module_t {
    /// @brief Identifies this module across all workspaces in the IR.
    std::string name;
    /// @brief Names the modules needed by this module's library sources.
    std::vector<std::string> module_dependencies;
    /// @brief Names the modules needed by this module's builder.cpp.
    std::vector<std::string> builder_dependencies;
};

/**
 * @brief Groups owned module records under a workspace label.
 *
 * Workspace-derived labels are `ws<N>`, with N a non-negative decimal workspace
 * position; m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_name_t owns
 * validation and ordering. This aggregate neither parses nor sorts the label.
 */
struct workspace_t {
    /// @brief Labels this workspace's group in rendered output.
    std::string name;
    /// @brief Stores modules in caller-supplied order, including an empty group.
    std::vector<module_t> modules;
};

/**
 * @brief Owns a mutable dependency graph grouped into ordered workspace records.
 *
 * All strings and vectors are owned values; copies and edits are independent of
 * a source workspace graph. Default construction produces an empty IR. No name,
 * uniqueness, dependency-resolution, cycle, or workspace-eligibility checks run
 * here. Vector order is preserved, not made topological; normal string/vector
 * reference-invalidation rules apply to edits.
 *
 * The DOT renderer (also used by the SVG renderer) checks globally unique module
 * names and requires every dependency name to occur in the IR. It compares names
 * exactly and does not validate module/workspace spelling or dependency eligibility.
 * Its arrows point from dependency to dependent; see
 * m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer::render().
 *
 * A two-module graph with the same earlier-workspace dependency used by both
 * the library and builder:
 * @code{.cpp}
 * #include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>
 *
 * #include <string>
 *
 * namespace ir = m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir;
 *
 * int main() {
 *     const std::string dependency_name = "m03gagbhsnusi43zogoacgj2ez_filesystem";
 *     const ir::module_dependency_ir_t dependency_ir {
 *         .workspaces = {
 *             { .name = "ws0", .modules = {
 *                 { .name = dependency_name, .module_dependencies = {}, .builder_dependencies = {} }
 *             } },
 *             { .name = "ws1", .modules = {
 *                 { .name = "m03gagbhtahg11wzn32idilzte_module_graph",
 *                   .module_dependencies = { dependency_name },
 *                   .builder_dependencies = { dependency_name } }
 *             } }
 *         }
 *     };
 * }
 * @endcode
 */
struct module_dependency_ir_t {
    /// @brief Stores workspace groups in caller-supplied order.
    std::vector<workspace_t> workspaces;
};

} // namespace m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir

#endif // M03GE9SCIYP8Y22MZR4NME82TM_MODULE_DEPENDENCY_IR_MODULE_DEPENDENCY_IR_H
