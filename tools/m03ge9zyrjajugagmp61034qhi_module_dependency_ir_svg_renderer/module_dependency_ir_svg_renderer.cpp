#include <m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer/module_dependency_ir_svg_renderer.h>
#include <m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir/module_dependency_ir.h>
#include <m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer/module_dependency_ir_dot_renderer.h>
#include <m03gagbht6ja46uikb1ltan0x8_dot/dot.h>

#include <stdexcept>
#include <string_view>

namespace m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer {

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render(
    const m03ge9sciyp8y22mzr4nme82tm_module_dependency_ir::module_dependency_ir_t& dependency_ir,
    std::string_view target_module_name,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_svg_path
) {
    if (output_svg_path.extension() != ".svg") {
        throw std::runtime_error(std::format("m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer::render: output path '{}' must have .svg extension", output_svg_path));
    }

    // todo: create temporary file using m03gagbhsnusi43zogoacgj2ez_filesystem::create_temp_file
    const auto dot_path = output_svg_path + "_tmp.dot";
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(dot_path)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(dot_path);
    }

    try {
        const auto dot_output = m03gezzdungmcczpiwx8qi55c3_module_dependency_ir_dot_renderer::render(
            dependency_ir,
            target_module_name,
            dot_path
        );
        const auto result = m03gagbht6ja46uikb1ltan0x8_dot::render_svg(dot_output, output_svg_path);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(dot_path);
        return result;
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(dot_path);
        throw ;
    }
}

} // namespace m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer
