#ifndef M03GEZZDUNGMCCZPIWX8QI55C3_MODULE_DEPENDENCY_IR_DOT_RENDERER_MODULE_DEPENDENCY_IR_DOT_RENDERER_H
# define M03GEZZDUNGMCCZPIWX8QI55C3_MODULE_DEPENDENCY_IR_DOT_RENDERER_MODULE_DEPENDENCY_IR_DOT_RENDERER_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>

# include <string_view>

namespace m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer {

/**
 * @brief Writes a workspace-clustered DOT graph with arrows from dependency to dependent.
 *
 * Each module is a rounded box. Solid blue edges labelled "module" represent
 * module dependencies; dashed amber edges labelled "builder" represent builder
 * dependencies. An exact name match with target_module_name gives the module a
 * heavier dark border and pale blue fill. An unmatched target highlights nothing.
 * An empty target likewise highlights nothing unless a module has an empty name.
 * Input vector order determines cluster/node emission order; layout is left to
 * the DOT consumer. No Graphviz executable is needed to write this file.
 *
 * Names are escaped labels, not parsed module/workspace identities. Module names
 * must be globally unique and every dependency must name a module in the IR;
 * violations throw std::runtime_error. Empty IR is accepted. See
 * m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t for
 * dependency meaning and the distinction between storage and validation.
 *
 * output_dot_path must end in .dot and must not already exist; these checks and
 * detected open/write failures throw std::runtime_error. Missing parent
 * directories are created and filesystem failures propagate. Returns a copy of
 * output_dot_path. Duplicate names are rejected before opening the file; missing
 * dependencies or failures after opening can leave a partial file. Remove that
 * file before retrying. Inputs are borrowed only for this synchronous call and
 * are not modified or retained. Coordinate access to the inputs and destination;
 * the existence check is not an atomic reservation.
 *
 * Hand-authored IR may use display names, as in this tiny graph with both kinds:
 * @code{.cpp}
 * #include <m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer/module_dependency_ir_dot_renderer.h>
 * #include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * namespace dot_renderer = m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer;
 * namespace ir = m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir;
 * namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *
 * int main() {
 *     const ir::module_dependency_ir_t dependency_ir {
 *         .workspaces = {
 *             { .name = "ws0", .modules = {
 *                 { .name = "support", .module_dependencies = {}, .builder_dependencies = {} }
 *             } },
 *             { .name = "ws1", .modules = {
 *                 { .name = "app", .module_dependencies = { "support" },
 *                   .builder_dependencies = { "support" } }
 *             } }
 *         }
 *     };
 *     // Choose a destination that does not exist.
 *     const auto dot_path = dot_renderer::render(
 *         dependency_ir, "app", filesystem::path_t("graphs/dependencies.dot")
 *     );
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render(
    const m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t& dependency_ir,
    std::string_view target_module_name,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_dot_path
);

} // namespace m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer

#endif // M03GEZZDUNGMCCZPIWX8QI55C3_MODULE_DEPENDENCY_IR_DOT_RENDERER_MODULE_DEPENDENCY_IR_DOT_RENDERER_H
