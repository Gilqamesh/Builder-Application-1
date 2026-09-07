# Stencil and direct render targets

Textures own allocations. Framebuffers borrow writable pixel views, and materials
retain sampled textures through their existing bindings. Applications keep borrowed
storage alive and sequence passes. No additional owning render-target type or
implicit image transfer is involved.

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

The owning texture constructor requires nonzero extents. Pixel views and framebuffers
permit zero extents with empty storage. Allocate or replace targets at resize boundaries;
after replacing texture storage, rebind framebuffer views before using them. Repeated
draws into the same allocation need no rebinding of the sampled texture. Two targets
can alternate roles across successive effects. Copying a texture explicitly preserves
an independent snapshot when needed.

## Storage and interpretation

The renderer accepts tightly packed `rgba8_unorm` and `rgba8_srgb` views. Row zero
is the framebuffer's top row; the texture performs no vertical flip. Sampling at
`((x + 0.5) / width, (y + 0.5) / height)` addresses the corresponding texel center.

Encoding is stored only in the framebuffer's pixel view. `encoding(srgb)` changes
`pixels().format()` to `rgba8_srgb` while preserving bytes and the owning texture's
format. Copies have independent view metadata. Explicit reinterpretation can make
rendering and sampling interpretations differ; the caller coordinates that choice.
Constructing from `target->view()` aligns them initially.

Sampling decodes RGB to linear values and filters components independently without
changing alpha association. Shader output and blend equations determine association.
For example, source-over rendering of straight red `(1,0,0,0.5)` over transparent
black produces premultiplied `(0.5,0,0,0.5)` before storage quantization. To composite
that sampled result, use RGB factors `one` and `one_minus_src_alpha`, avoiding another
multiplication of RGB by alpha. Alpha can use the same factors.

## Accessor migration

`framebuffer_t(span<rgba8_t>, width, height)` remains supported. `pixels()` returns
`texture::pixel_view_t` by value, replacing the old `span<rgba8_t>` result. Use its
`bytes()` for byte access and its dimensions/format for layout. Existing applications
can continue accessing their original `rgba8_t` arrays directly. Byte-backed textures
do not expose fabricated `rgba8_t` objects.

A const texture returns `const_pixel_view_t`; a const framebuffer returns a writable
borrowed view, matching its previous access semantics. Mutable views convert to
read-only views. View copies do not revalidate storage or acquire ownership.

## Feedback and validation

A nonempty draw rejects overlap between any reflected texture byte range in either
shader stage and any attached color, depth or stencil byte range. This deliberately
includes disabled writes and partial overlap. Unused extra material bindings are
accepted. Validation happens before vertex execution; no checks enter the fragment
loop. Empty framebuffer/camera intersections retain their early return behavior.

The renderer demo combines a shader-discarded stencil mask, intersecting 3D geometry,
a translucent layer, and a sampled composite. Public tests provide deterministic
headless evidence; the benchmark includes separate mask and complete two-pass costs.
