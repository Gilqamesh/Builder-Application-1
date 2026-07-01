#ifndef M03GE9ZYRJAJUGAGMP61034QHI_MODULE_DEPENDENCY_IR_SVG_RENDERER_MODULE_H
# define M03GE9ZYRJAJUGAGMP61034QHI_MODULE_DEPENDENCY_IR_SVG_RENDERER_MODULE_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>

# include <string_view>

namespace m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer {

/**
 * Renders the dependency_ir in SVG format and returns output_svg_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render(
    const m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t& dependency_ir,
    std::string_view target_module_name,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_svg_path
);

} // namespace m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer

#endif // M03GE9ZYRJAJUGAGMP61034QHI_MODULE_DEPENDENCY_IR_SVG_RENDERER_MODULE_H
