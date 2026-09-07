# Interpolation and explicit LOD

Fragment declarations select interpolation. The
[renderer contract](../software_renderer.h) defines supported varying types and
coverage; [materials](../material.h) select the provoking vertex.

```cpp
namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
using vector2f_t = shader::vector_t<float, 2>;

shader::fragment_shader_ast_builder_t fragment;
auto surface_uv = fragment.input<vector2f_t>(0); // Perspective by default.
auto screen_uv = fragment.input<vector2f_t>(1, shader::interpolation_t::noperspective);
auto face_id = fragment.input<std::uint32_t>(2, shader::interpolation_t::flat);
```

## Render, generate, sample

Texture [storage](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/texture.h) and
[sampling](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/sampler.h) define mip
construction, filtering, and borrowed-view lifetimes.

```cpp
namespace texture = m03gt0l0q3l4b1k27eab5k7py1_texture;
namespace bytes = m03gagbht2l61mj6qitacwbmea_byte_stream;

auto target = std::make_shared<texture::texture_t>(
    texture::texture_description_t {texture::format_t::rgba8_srgb, 7, 3, 3},
    bytes::byte_stream_t(std::vector<std::byte>(7 * 3 * 4))
);
renderer::framebuffer_t offscreen(target->view());
auto lower_level = target->view(1); // Writable 3x1 view.

// Render or edit level zero, then regenerate before sampling lower levels.
target->generate_mipmaps();
// offscreen and lower_level remain bound to the same addresses.
```

Select a different framebuffer before sampling the rendered texture. An explicit
`shader::sample_lod(texture, sampler, coordinates, lod)` reads the selected levels;
rendering does not automatically regenerate them or compute derivatives.
