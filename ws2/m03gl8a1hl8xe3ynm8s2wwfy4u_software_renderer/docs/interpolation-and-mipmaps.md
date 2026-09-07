# Interpolation and explicit LOD

Fragment inputs select interpolation in the shader AST. The [shader interface](../../m03gsy25j4v7nccgmsdov9ioft_shader/shader_builder.h)
records the selection; the [CPU program](../../m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h)
continues linking numbered inputs and outputs by location and type. The
[renderer](../software_renderer.h) enforces its rasterized input types.

```cpp
namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
using vector2f_t = shader::vector_t<float, 2>;

shader::fragment_shader_ast_builder_t fragment;
auto surface_uv = fragment.input<vector2f_t>(0); // Perspective by default.
auto screen_uv = fragment.input<vector2f_t>(1, shader::interpolation_t::noperspective);
auto face_id = fragment.input<std::uint32_t>(2, shader::interpolation_t::flat);
```

Float scalars and vectors support all three modes. Signed and unsigned 32-bit
scalars and vectors require flat interpolation when rasterized. Boolean and matrix
values retain their standalone shader-invocation support, but are not renderer
varyings. Conflicting consumed declarations at one location are rejected. As with
existing reflection, unreachable expressions do not introduce interface requirements.

Perspective interpolation accounts for reciprocal W. Noperspective interpolation
is linear in screen space, using the renderer's existing snapped triangle positions.
Clipping preserves this distinction: internally, noperspective payloads carry
`W * attribute` in double precision until projection. This is equivalent to adjusting
the clipping interpolation parameter in framebuffer space, while avoiding division
by zero at intermediate clip vertices. Coverage, facing, depth, reciprocal W and
supported clip coordinates retain their existing contracts.

[Materials](../material.h) own `provoking_vertex(first/last)`, defaulting to first.
Flat inputs borrow the selected original vertex's payload once per primitive;
clipping and triangulation cannot replace it. With zero-based assembled indices:

| Primitive | First | Last |
|---|---|---|
| Point `(vi)` | `vi` | `vi` |
| Line `(vi, vj)` | `vi` | `vj` |
| Closing line-loop segment `(v_last, v0)` | `v_last` | `v0` |
| Triangle list `(vi, vi+1, vi+2)` | `vi` | `vi+2` |
| Triangle strip primitive starting at `vi` | `vi` | `vi+2` |
| Triangle fan `(v0, vi, vi+1)` | `vi` | `vi+1` |

Selection precedes winding adjustment and follows selected index-buffer order.
An indexed vertex's numeric index does not determine whether it is first or last.

## Mip ownership and generation

The texture module owns [mip storage](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/texture.h)
and [sampling](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/sampler.h).
The old texture constructor creates one level. The typed constructor accepts a
nonempty contiguous prefix of the full chain, with a common format and dimensions
`max(1, previous / 2)`; for example, `7x3 -> 3x1 -> 1x1`.
The supplied bytes initialize level zero exactly; lower levels start zero-filled.
Existing `view()`, `bytes()`, `width()` and `height()` still refer to level zero.

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

Generation averages each preceding level with an area-weighted box, including odd
rows and columns. It supports all four texture formats. sRGB RGB is decoded before
averaging and encoded afterward; alpha is averaged linearly and independently.
Float formats retain HDR values, with binary16 rounding to nearest, ties to even.
UNORM quantization rounds halfway upward. No alpha association conversion occurs.
Floating-point special values follow IEEE arithmetic.

Generation changes contents without allocating or rebinding mip storage. Ordinary
pixel writes and rendering do not trigger generation: lower levels retain their
previous contents until explicitly written or regenerated. Assignment, moving
from the texture and destruction keep their existing borrowed-view invalidation
rules. Applications continue owning pass sequencing and borrowed lifetimes.

## Sampling

```cpp
texture::sampler_t sampler(texture::sampler_description_t {
    .magnification_filter = texture::filter_t::nearest,
    .minification_filter = texture::filter_t::linear,
    .mipmap_filter = texture::filter_t::linear,
    .address_u = texture::address_mode_t::repeat,
    .address_v = texture::address_mode_t::clamp_to_edge
});
auto color = texture::sample_lod(*target, sampler, vector2f_t({0.25F, 0.5F}), 1.5F);

// The same operation is available in either shader stage.
fragment.color(shader::sample_lod(
    fragment.resource<shader::shader_texture_2d_t>(0),
    fragment.resource<shader::shader_sampler_t>(0),
    surface_uv,
    1.5F
));
```

Nonpositive LOD selects magnification filtering; positive LOD selects minification
filtering, even when storage has only one level. Level selection clamps to the
allocated range. Nearest mip filtering chooses the lower level at exact halfway
ties. Linear mip filtering blends adjacent filtered levels in linear RGBA.
Coordinates and LOD must be finite; empty textures cannot be sampled.
The old sampler constructor sets both within-level filters to its supplied filter
and mip filtering to nearest. The legacy `filter()` accessor returns the
magnification filter. `sample()` always samples level zero with that filter.

Mipmaps affect explicit-LOD calls only. Derivatives, automatic LOD, anisotropic
filtering and specialized normal/coverage-preserving mip generation remain later
scope. There is no claim of complete Vulkan pipeline compatibility; existing
renderer clip-depth and coverage conventions remain authoritative.

Draw preparation conservatively checks every allocated level of each reflected
texture against every attachment, in both shader stages. Sampling level zero does
not permit attaching another mip of that texture during the same draw. Disabled
writes and partial byte overlap retain the same rejection rules. Render into one
framebuffer, generate, then select a different framebuffer before sampling.

## Demo and measurements

The demo uses perspective interpolation on the red surface, noperspective on the
blue surface, and flat interpolation on the green translucent surface. The final
composite samples level zero on the left and explicit LOD 1.5 on the right. It
regenerates the offscreen texture's mip chain between passes.

The benchmark preserves its nine existing workloads and adds `flat_fill`,
`noperspective_fill`, `mipmapped_fill`, and `mipmapped_two_pass`. Static texture
mip generation is setup work in `mipmapped_fill`; the two-pass workload includes
regeneration in frame timing and compares every generated level between profiling
configurations. See the [milestone 5 performance record](milestone-5-performance.md) for measured
build settings and results.

These selections follow Vulkan's [flat-shading and clipping rules](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html),
[primitive conventions](https://docs.vulkan.org/spec/latest/chapters/primsrast.html),
and [sampling model](https://docs.vulkan.org/spec/latest/chapters/textures.html), within the module's documented supported scope.
