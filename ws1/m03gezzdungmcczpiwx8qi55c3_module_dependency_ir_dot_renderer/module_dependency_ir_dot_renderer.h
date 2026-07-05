#ifndef M03GEZZDUNGMCCZPIWX8QI55C3_MODULE_DEPENDENCY_IR_DOT_RENDERER_MODULE_H
# define M03GEZZDUNGMCCZPIWX8QI55C3_MODULE_DEPENDENCY_IR_DOT_RENDERER_MODULE_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>

# include <string_view>

namespace m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer {

/**
 * Renders the dependency_ir in DOT format and returns output_dot_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render(
    const m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t& dependency_ir,
    std::string_view target_module_name,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_dot_path
);

} // namespace m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer

#endif // M03GEZZDUNGMCCZPIWX8QI55C3_MODULE_DEPENDENCY_IR_DOT_RENDERER_MODULE_H
