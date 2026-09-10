#ifndef M03GE9ZYRJAJUGAGMP61034QHI_MODULE_DEPENDENCY_IR_SVG_RENDERER_MODULE_DEPENDENCY_IR_SVG_RENDERER_H
# define M03GE9ZYRJAJUGAGMP61034QHI_MODULE_DEPENDENCY_IR_SVG_RENDERER_MODULE_DEPENDENCY_IR_SVG_RENDERER_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>

# include <string_view>

namespace m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer {

/**
 * @brief Writes a dependency IR graph as SVG through the DOT renderer and Graphviz.
 *
 * Uses m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer::render()
 * for IR validation, dependency-to-dependent arrows, workspace grouping and
 * edge styling. target_module_name is an exact name match highlighted with a
 * heavier dark border and pale blue fill; an unmatched name highlights nothing.
 * Empty IR is accepted. Inputs are borrowed only during this synchronous call.
 *
 * Requires a new .svg destination and the Graphviz dot executable configured by
 * the owning builder of m03gagbht6ja46uikb1ltan0x8_dot::render_svg(). Missing
 * parent directories are created. Returns a copy of output_svg_path; invalid
 * extensions, existing SVG output, invalid IR, filesystem and tool failures throw.
 *
 * The intermediate path appends "_tmp.dot" to the entire SVG path, for example
 * graphs/dependencies.svg_tmp.dot. Current implementation removes an existing
 * entry at that path before writing DOT, even if an existing SVG will later make
 * rendering fail. Reserve both paths for the call; do not run concurrent renders
 * to the same destination. The intermediate is removed on success and cleanup is
 * attempted on failure; failed Graphviz output is also removed. Cleanup can throw,
 * leaving files behind or replacing the original error. Inspect remaining files
 * before retrying. An existing SVG is preserved.
 *
 * Example with hand-authored display names; choose an unused SVG path and reserve
 * its appended temporary sibling:
 * @code{.cpp}
 * #include <m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer/module_dependency_ir_svg_renderer.h>
 * #include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * namespace svg_renderer = m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer;
 * namespace ir = m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir;
 * namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *
 * int main() {
 *     const ir::module_dependency_ir_t dependency_ir {
 *         .workspaces = { { .name = "ws1", .modules = {
 *             { .name = "support", .module_dependencies = {}, .builder_dependencies = {} },
 *             { .name = "app", .module_dependencies = { "support" }, .builder_dependencies = {} }
 *         } } }
 *     };
 *     const auto svg_path = svg_renderer::render(
 *         dependency_ir, "app", filesystem::path_t("graphs/dependencies.svg")
 *     );
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render(
    const m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t& dependency_ir,
    std::string_view target_module_name,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_svg_path
);

} // namespace m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer

#endif // M03GE9ZYRJAJUGAGMP61034QHI_MODULE_DEPENDENCY_IR_SVG_RENDERER_MODULE_DEPENDENCY_IR_SVG_RENDERER_H
