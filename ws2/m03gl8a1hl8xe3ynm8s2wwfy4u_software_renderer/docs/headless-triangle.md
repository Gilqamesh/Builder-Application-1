# Draw a headless triangle

[mesh.h](../mesh.h) owns stream-to-location mapping and storage validation;
[geometry.h](../geometry.h) owns index ranges and primitive validation.
[software_renderer.h](../software_renderer.h) defines draw behavior and the remaining
shader/resource checks. This example renders opaque red into application-owned
RGBA8 memory with a default inactive parent metric. It requires no window or profiler.

```cpp
#include <m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays/api.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/camera.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/framebuffer.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/geometry.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/index_buffer.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/material.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/mesh.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/render_item.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/software_renderer.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/vertex_attribute.h>
#include <m03gsy25j4v7nccgmsdov9ioft_shader/shader_builder.h>
#include <m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h>
#include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

#include <array>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

int main() {
    namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
    namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
    namespace cpu = m03gt1djvvy5atia5evkbg6rqy_software_shader;
    namespace soa = m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays;
    using position_t = std::array<float, 2>;
    using vector2f_t = shader::vector_t<float, 2>;
    using vector4f_t = shader::vector_t<float, 4>;

    soa::structure_of_arrays_t<position_t> vertex_streams;
    vertex_streams.push_back(position_t{-0.75F, -0.75F});
    vertex_streams.push_back(position_t{0.75F, -0.75F});
    vertex_streams.push_back(position_t{0.0F, 0.75F});
    auto mesh = std::make_shared<renderer::mesh_t>(std::move(vertex_streams),
        std::vector<renderer::vertex_attribute_t> {{renderer::vertex_attribute_type_t::R32, 2}});
    auto index_buffer = std::make_shared<renderer::index_buffer_t>();
    index_buffer->indices() = {0, 1, 2};
    auto geometry = std::make_shared<renderer::geometry_t>(index_buffer,
        renderer::index_range_t{0, 3}); // Three indices form one triangle by default.
    geometry->mesh() = mesh;
    geometry->validate(); // Construction did not require the mesh to be set.

    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector2f_t>(0); // Stream 0: two R32 components.
    const auto local_position = vertex.construct<vector4f_t>(position, -1.0F, 1.0F);
    vertex.position(vertex.world_to_clip() * vertex.object_to_world() * local_position);
    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(vector4f_t{1, 0, 0, 1});
    auto program = std::make_shared<const cpu::program_t>(
        std::move(vertex).finalize(), std::move(fragment).finalize());
    auto material = std::make_shared<renderer::material_t>(program);
    renderer::render_item_t render_item;
    render_item.geometry() = geometry;
    render_item.material() = material; // Default identity placement.

    std::vector<renderer::rgba8_t> pixels(64 * 64);
    renderer::software_renderer_t software_renderer(renderer::framebuffer_t(pixels, 64, 64));
    const renderer::camera_t camera({{0, 64}, {0, 64}},
        renderer::orthographic_t({{-1.0F, 1.0F}, {-1.0F, 1.0F}}, 0.0F, 2.0F));
    m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t parent_metric; // Inactive.
    software_renderer.clear_color({0, 0, 0, 255}, parent_metric);
    software_renderer.draw(camera, render_item, parent_metric);
    const auto center = pixels[32 * 64 + 32];
    assert(center.red == 255 && center.green == 0 && center.blue == 0 && center.alpha == 255);
    assert(pixels[0].red == 0); // Outside the triangle remains clear.
}
```

The camera looks along -Z, so the shader places the triangle at Z = -1, between
its near and far planes. The framebuffer has a top-left origin. Default material
state needs no depth/stencil attachments, disables culling and blending, and writes
all color channels.

The render item shares geometry and material. Geometry shares the mesh and index
buffer; material shares the program (and any textures/samplers you later bind).
Framebuffer storage is borrowed: keep `pixels` alive without reallocation during
renderer use. Resource changes become visible on later draws, which revalidate the
current selection. Empty framebuffer/camera intersections return before draw
resource validation, as specified by `software_renderer_t::draw()`.
