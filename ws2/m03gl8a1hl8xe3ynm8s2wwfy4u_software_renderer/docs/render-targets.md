# Render to a texture

Textures own storage, framebuffers borrow writable views, and materials retain
sampled textures. Applications keep views valid and sequence passes. Attachment
and feedback requirements are in [framebuffer.h](../framebuffer.h) and
[software_renderer.h](../software_renderer.h).

## Two-pass caller

With geometry, shader programs, camera and material already configured:

```cpp
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
namespace texture = m03gt0l0q3l4b1k27eab5k7py1_texture;
namespace bytes = m03gagbht2l61mj6qitacwbmea_byte_stream;

auto target = std::make_shared<texture::texture_t>(
    texture::format_t::rgba8_srgb, width, height,
    bytes::byte_stream_t(std::vector<std::byte>(renderer::framebuffer_t::pixel_count(width, height) * 4))
);
std::vector<std::uint8_t> stencil(renderer::framebuffer_t::pixel_count(width, height));
renderer::framebuffer_t offscreen(target->view());
offscreen.stencil(stencil);

software_renderer.framebuffer() = offscreen;
software_renderer.clear_color({0, 0, 0, 0}, metric);
software_renderer.clear_stencil(0, metric);
mask.material()->stencil_test(true);
mask.material()->stencil_front({.reference = 1, .pass = renderer::stencil_op_t::replace});
mask.material()->stencil_back(mask.material()->stencil_front());
mask.material()->color_write(renderer::color_mask_t::none);
software_renderer.draw(camera, mask, metric);

scene.material()->stencil_test(true);
scene.material()->stencil_front({.comparison = renderer::comparison_t::equal, .reference = 1});
scene.material()->stencil_back(scene.material()->stencil_front());
software_renderer.draw(camera, scene, metric);

software_renderer.framebuffer() = output;
postprocess.material()->texture(0, target);
software_renderer.draw(output_camera, postprocess, metric);
```

## Compositing the result

For example, source-over rendering of straight red `(1,0,0,0.5)` over transparent
black produces premultiplied `(0.5,0,0,0.5)` before storage quantization. To composite
that sampled result, use RGB factors `one` and `one_minus_src_alpha`, avoiding another
multiplication of RGB by alpha. Alpha can use the same factors.
