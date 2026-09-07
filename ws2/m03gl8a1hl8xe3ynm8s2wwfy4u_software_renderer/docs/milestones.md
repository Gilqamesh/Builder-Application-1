# Software renderer milestones

Status: milestones 0–4 are implemented and validated. Milestone 6 has profiling and an initial optimized baseline; algorithmic optimization remains unstarted. Milestone 5 remains proposed and unstarted. Public behavior is owned by the module headers.

Baseline: [Builder-Modules at 540bbede71740d24292cc3b7cd9c8ed126eca0c3](https://github.com/Gilqamesh/Builder-Modules/tree/540bbede71740d24292cc3b7cd9c8ed126eca0c3).

Develop a general-purpose headless 3D CPU rasterizer with planar scenes using the same 3D model. Design and implementation follow the shared [agent workflow](../../../../Builder/docs/agent-workflow.md).

Keep later milestones at outcome level until their dependencies are settled. Existing tests inform validation; they do not determine the design. A milestone is complete when its behavior works through the public API, with focused evidence and updated contracts.

The current module boundaries remain the starting point: shader construction/reflection in shader, individual CPU invocation in software_shader, texture storage/sampling in texture, and rasterization/attachments in software_renderer. Applications own scene organization, allocation, frame sequencing, and presentation.

The baseline includes shared geometry/material resources, indexed ranges, all seven primitive topologies, homogeneous clipping, perspective-correct float varyings, texture sampling, fragment discard, and caller-owned offscreen RGBA8 storage. Public camera mapping and render-item transforms remain 2D. There is no depth/stencil testing, blending, or culling. These are source-review findings; tests were not run for this document.

## 0. Coordinates and shared-edge coverage

Status: implemented; automated validation passed. Consumer smoke validation is recorded below.

- Contract: [`software_renderer_t::draw()`](../software_renderer.h) defines clip/depth/sample conventions, supported dimensions and W, snapping, facing, interpolation and degenerate behavior. Existing 2D camera mapping and `T * R * S` are preserved.
- Implementation: canonical homogeneous clipping with double intermediates, one-time 1/256-pixel snapping, int64 edge tests, deterministic ears for simple polygons, and nonzero-winding rational scanline spans for crossed/touching/overlapping boundaries. Both paths emit pre-shading samples exactly once per original triangle. Whole-primitive discard is not a response to a snapped crossing.
- Ownership: matching shared boundaries with filled interiors on opposite sides have complementary top-left ownership. Independently overlapping snapped interiors retain separate primitive coverage.
- Validation: literal fractional and clipped adjacent-triangle regressions assert independent expected unions and disjoint masks; per-original-primitive hit counts cover every generated ear/span. Tests also cover the end-to-end crossed and concave fixtures, all six planes, multiple-plane intersections, winding/cyclic/submission reversals, topology assembly, unequal W, coincident payloads, collapse, facing, fragment coordinates, perspective varyings, depth, line tangencies, and integer limits.
- Numerical proof: grid coordinates are in `[0,2^31]`; individual determinants are bounded by `2^62`. Rational scanline comparisons use quotient/remainder operations instead of cross products. No int64 polygon-area sum or floating coverage epsilon is used. Interpolation uses exact residuals when nearby rational crossings coincide in double.

## 1. 3D transforms, cameras, viewport, and scissor

Status: implemented; automated and visible integration validation passed.

- Outcome: perspective and orthographic scenes use `draw(camera, render_item, parent_metric)`.
  A camera owns one rendering rectangle for viewport mapping and write bounds.
  See [camera.h](../camera.h) and [software_renderer.h](../software_renderer.h).
- Placement: [render_item_t](../render_item.h) owns 3D translation, rotation, and
  scale. Cameras and items share [quaternion_t<float>](../../../ws1/m03gtgtrh2smvh28qlwgm7gdl4_quaternion/api.h); both `rotation`
  overloads update the authoritative quaternion. There is no Euler getter.
- Mapping and clearing: partial overlap retains the original projection and
  viewport mapping; empty intersections perform no work. Explicit camera-based
  clearing uses only the rectangle.
- Callers: the renderer demo displays a textured, rotating 3D surface crossing
  the near plane. Tower-defense scenes use an orthographic camera and explicit
  planar placement, including their existing screen orientation.
- Acceptance evidence: public tests cover camera pose/inverse view, both
  projections, analytical texture sampling across the near plane, all primitive
  topologies in bounded rectangles, original aspect after intersection, explicit
  clearing, empty regions, and the previous planar rendering regressions.

Representative caller (with geometry and material already assigned):

```cpp
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
renderer::camera_t camera(
    {{0, width}, {0, height}},
    renderer::perspective_t(std::numbers::pi_v<float> / 3, 0.5F, 20.0F)
);
camera.look_at({0, 0, 4}, {0, 0, 0}, {0, 1, 0});
item.translation() = {0, 0, 0};
item.scale() = {1, 1, 1};
item.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> {0.25F, 0.5F, 0});
// Quaternion input uses the same setter:
item.rotation(m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>(1, 0, 0, 0));
renderer::profiling::metric_t inactive_metric;
software_renderer.clear_color(camera, {0, 0, 0, 255}, inactive_metric);
software_renderer.draw(camera, item, inactive_metric);
```

## 2. Depth testing and face culling

Status: implemented; automated and visible integration validation passed.

- Outcome: opaque visibility is independent of submission order for distinct
  stored float depths. Equal-depth ties follow the selected comparison.
- Ownership: [materials](../material.h) own the depth and culling properties.
  Applications select materials and draw order; `draw(camera, render_item, parent_metric)`
  consumes the selected material. Items sharing a material share its settings.
- Attachments and clearing: [framebuffers](../framebuffer.h) borrow optional
  float depth storage alongside color. [Depth clears](../software_renderer.h)
  explicitly fill the framebuffer or camera intersection, independently of draw
  state. Clear values, including infinities, are clamped to [0,1]; NaN is rejected.
- Processing: disabling depth testing bypasses comparison and depth writes.
  Enabled testing requires depth storage; the write flag controls passing samples.
  The initial state disables testing, enables writes, selects less, and disables
  culling. Late testing preserves depth and color on discard or comparison failure;
  an unwritten fragment color preserves color while permitting depth writes.
- Facing: configurable CW/CCW NDC front faces and none/front/back/both culling
  preserve existing coverage and per-original-triangle facing. Points and lines
  remain front-facing and are unaffected by culling.
- Acceptance evidence: tests cover all comparisons and controls, exact ties,
  overlapping and intersecting surfaces in either order, analytical depth through
  near-plane clipping in both projections, discard, absent color, all seven
  topologies, negative scale, pathological clipped boundaries, attachment rebinding,
  bounded depth writes and clears, and invalid resources and state.

Representative caller (with geometry and material already assigned):

```cpp
std::vector<float> depth_pixels(renderer::framebuffer_t::pixel_count(width, height));
renderer::framebuffer_t framebuffer(pixels, width, height);
framebuffer.depth(depth_pixels);
software_renderer.framebuffer() = framebuffer;
item.material()->depth_test(true);
item.material()->cull(renderer::cull_mode_t::back);
renderer::profiling::metric_t inactive_metric;
software_renderer.clear_color(camera, {0, 0, 0, 255}, inactive_metric);
software_renderer.clear_depth(camera, 1.0F, inactive_metric); // less rejects samples exactly at 1.
software_renderer.draw(camera, item, inactive_metric);
```

## 3. Blending and color writes

Status: implemented; automated, visible integration, and benchmark validation passed.

- Outcome: configurable single-source blending composes transparent geometry in application-supplied order.
- Materials own blend enablement, independent RGB/alpha equations, all 15 single-source factors, five basic operations, linear constants, and RGBA channel masks. Defaults preserve linear replacement output. See [material.h](../material.h).
- Framebuffer views own linear/sRGB encoding and prescribe no alpha association. Encoding changes reinterpret storage; byte clears remain literal fills. See [framebuffer.h](../framebuffer.h).
- Processing sanitizes and clamps source values, decodes destination RGB when needed, evaluates both equations from original inputs, clamps, encodes, quantizes, and writes enabled channels. See [software_renderer.h](../software_renderer.h). No implicit premultiplication or division by alpha is performed.
- Acceptance criteria: independent RGB/alpha expectations, factor and operation tables, exact masked-byte preservation, sRGB boundaries, repeated translucent composition, shared-edge coverage, unchanged late depth/discard behavior, and consistent profiling counters.
- Dual-source blending, advanced operations, logic operations, and additional color attachments remain deferred.

Representative caller (with geometry, program and bindings already assigned):

```cpp
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
auto& material = *transparent_item.material();
material.blend_color({
    .source = renderer::blend_factor_t::src_alpha,
    .destination = renderer::blend_factor_t::one_minus_src_alpha,
    .operation = renderer::blend_op_t::add
});
material.blend_alpha({
    .source = renderer::blend_factor_t::one,
    .destination = renderer::blend_factor_t::one_minus_src_alpha,
    .operation = renderer::blend_op_t::add
});
material.blend(true);
material.depth_test(true);
material.depth_write(false);
software_renderer.framebuffer().encoding(renderer::color_encoding_t::srgb);
renderer::profiling::metric_t inactive_metric;
software_renderer.draw(camera, opaque_item, inactive_metric);
software_renderer.draw(camera, transparent_item, inactive_metric);
```

This example emits straight shader color into a premultiplied destination convention.
For premultiplied shader output, use `one` as the RGB source factor. The application
initializes attachments before the pass and supplies transparent draw ordering.

## 4. Stencil and render-to-texture

Status: implemented; headless, native consumer, visible integration, and benchmark validation passed.

- Outcome: per-face stencil masking and direct rendering into texture-owned color storage, sampled by a later pass without an image copy.
- [Materials](../material.h) own stencil enablement and validated face descriptions. [Framebuffers](../framebuffer.h) borrow independent eight-bit stencil storage; [clears and drawing](../software_renderer.h) define late operation ordering and bounded writes.
- The texture module owns [validated mutable/read-only views](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/pixel_view.h). Framebuffers borrow writable views and materials retain sampled textures; applications maintain lifetimes and order passes.
- `pixels()` now returns a writable pixel view. Its format is the framebuffer's encoding source of truth. Explicit reinterpretation changes that view without changing owner metadata or bytes. See the [caller and migration guide](render-targets.md).
- [Sampling](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/sampler.h) returns linear RGBA without changing alpha association. Premultiplied intermediate results compose through the selected blend equations.
- Draw preparation rejects reflected texture storage overlapping any attached color/depth/stencil bytes, including partial overlap and disabled writes. Both shader stages are checked; unused extra bindings and empty-region returns retain their contracts.
- Acceptance evidence covers stencil operation/comparison tables, masks, discard/depth outcomes, face selection, shared/clipped edges, direct two-pass color/orientation, premultiplied composition, storage reuse, feedback rejection and profiling equivalence.
- Colorless framebuffers, floating-point color attachments, and early testing remain deferred. No milestone 4 semantic decision remains open.

## 5. Interpolation modes and mipmapped sampling

Status: unstarted.

- Outcome: flat varyings, including integer values; noperspective interpolation; mipmapped textures and explicit LOD sampling.
- Open decisions: interpolation metadata and linking, provoking-vertex rules through clipping/topology assembly, mip storage/generation, and LOD filtering. Keep reflection changes in shader, execution in software_shader, interpolation in the renderer, and sampling in texture.
- Acceptance criteria: flat values remain constant across clipped primitives, screen-linear and perspective interpolation visibly differ as intended, and explicit LOD selects/blends validated mip levels. Automatic derivative-based LOD is a separate later decision.

## 6. Measurement and incremental optimization

Status: profiling and an initial optimized baseline are implemented and validated;
algorithmic optimization is unstarted. See [the measurement path](profiling.md)
and [baseline evidence](profiling-baseline.md).

- Outcome: each optimization delivery demonstrates its effect through repeatable measurements while preserving rendering correctness.
- Measurement ownership: [`profiling`](../../../ws1/m03gtjqkhqacstl3luv2ojsz3q_profiling/AGENTS.md) owns timing statistics, persistent storage, and deferred reporting. The renderer owns its metric types and measurement boundaries. Applications own profilers and pass parent metrics into rendering. The benchmark driver owns workloads, repeated-run summaries, and comparisons.
- Metrics: track median and high-percentile elapsed render time and process peak RSS across stable headless workloads. The benchmark records precise timing and memory scopes. Keep these scopes consistent across deliveries. Current Builder runs use its default build; dependency-wide optimization and a new optimized baseline remain deferred.
- Comparisons: record workload, resolution, rendering settings, hardware, build configuration, and source revision. Report absolute results, percentage changes, and run-to-run variation against both the previous delivery and the established baseline. New feature workloads establish their own baselines.
- Delivery process: use measured results to select each bounded optimization. Every delivery includes the same comparison report and correctness evidence, making improvements, regressions, and tradeoffs visible. Specific optimization techniques remain undecided until measurements justify them.
- Acceptance criteria: repeated runs establish measurement variability, and each optimization delivery has comparable before/after results with passing correctness validation.

The initial baseline follows milestone 2 and precedes future feature and optimization comparisons. It does not provide historical before/after measurements for completed milestones. Preserve this baseline when comparing subsequent deliveries.

## Completion records

For each milestone, append its settled decisions, implementation commit, checks actually run, and remaining limitations. Preserve the original baseline and add subsequent review and completion references; do not count planned features as implemented.

### Milestone 0 implementation record — 2026-09-05

Reviewed base: `Builder-Modules` revision `392b85c8663267e5f8ed41ab78af58bbf6042182`.
Implementation is in the working tree; no commit was created by the implementation task.
Exact hexadecimal input fixtures are retained in [`test/raster_fixtures.h`](../test/raster_fixtures.h).
The crossed fixture yields grid points `(126,128),(132,127),(134,126),(127,129)` and expected sample `{(0,0)}`; its Y=128 interval is `[126,388/3)`. The concave fixture has the same one-sample mask. The fractional rectangle covers `4<=x<=28, 8<=y<=24`; the clipped rectangle covers all 32x32 samples.

Checks obtained during staging: current-source public validation built with GNU C++23,
`-Wall -Wextra -ftrapv`; independent pre-shading winding/mask oracle; seeded arbitrary
boundary walks and fraction comparisons; analytical depth/reciprocal-W/varying checks.
The existing matrix implementations were also compared byte-for-byte with the reviewed base.

The normal Builder library phase automatically compiles `helpers.cpp` and
runs `test/public_api.cpp`, including its private geometry checks. Re-run the installed
validation executable after building the module:

```sh
artifacts/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/latest/library/build/validation/public_api/runner
```

Staged build command used during implementation:
`python3 /tmp/renderer-m0-implementation/build.py`.
The helper installs the normal library phase without launching the graphical demo:
`/tmp/renderer-m0-implementation/install_library m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer`.
Both temporary helper sources and logs remain in `/tmp/renderer-m0-implementation`.
Checks after application: the native Builder library phase compiled the production
sources with Clang C++23 and ran `test/public_api.cpp` successfully. The tower-defense
consumer rebuilt successfully. Its visible OpenGL smoke test presented textured
geometry at 1600x1200 and closed normally (exit 0). The capture is
`/tmp/renderer-m0-implementation/tower-defense-smoke.png`; native build and consumer
logs are in the same directory. `git diff --check` passed.

For a normal rebuild from `Builder-Layout`, the following command builds the renderer,
runs its public validation and opens its existing demo (close the window to finish):

```sh
./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
```

The consumer was rebuilt with
`./cli m03gilsfsv3k34ej14ytz8a29k_tower_defense_game`. Its initial sandboxed launch
could not connect to Wayland; a desktop launch from the layout root then failed to
find relative texture assets. The successful smoke launch used the module directory
and explicit X11 selection, without changing consumer code:

```sh
renderer_workspace="$PWD"
(
    cd ws2/m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
    env -u WAYLAND_DISPLAY XDG_SESSION_TYPE=x11 \
        "$renderer_workspace/artifacts/m03gilsfsv3k34ej14ytz8a29k_tower_defense_game/latest/binary/cli/install/cli"
)
```

No milestone 0 semantic decision remains open. The numerical limits in the public
draw contract are intentional; performance optimization and later milestones remain
unimplemented. The smoke test establishes presentation and integration, not a
performance target or an exhaustive rendering proof.

### Milestone 1 implementation record — 2026-09-06

Reviewed base: `Builder-Modules` revision
`602dd360f4d2d26eadacffd46d4043209c01dd4d`. Implementation is in the working
tree; no commit was created by this task.

The settled camera and transform contracts are in [camera.h](../camera.h),
[the quaternion module](../../../ws1/m03gtgtrh2smvh28qlwgm7gdl4_quaternion/api.h), [render_item.h](../render_item.h), and
[software_renderer.h](../software_renderer.h). The old 2D camera template and
mutable scalar rotation were replaced throughout the renderer demo, tests, and
all tower-defense rendering paths. No `to_world` callers existed to migrate.

Rasterization now works in viewport-local coordinates and restricts traversal
and writes to the framebuffer intersection. Supporting the complete signed-int
rectangle endpoint range requires grid coordinates below `2^40` and determinants
that fit in 81 signed bits; coverage uses 128-bit integer intermediates. Line
traversal skips invisible major-axis steps while preserving its inclusive
Bresenham coverage. These changes retain milestone 0's shared-edge rules.

Automated checks passed:

- `python3 /tmp/renderer-m1-implementation/build.py public_api`: GNU C++23,
  `-Wall -Wextra -ftrapv`, all existing and new public pipeline tests.
- `/tmp/renderer-m0-implementation/install_library m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer`:
  native Clang C++23 library build and automatic public validation.
- `env -u WAYLAND_DISPLAY XDG_SESSION_TYPE=x11 ./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer`:
  rebuilt the final header change, reran automatic validation, and built the demo.
  Its sandboxed launch could not access the display; desktop validation followed.
- `artifacts/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/latest/library/build/validation/public_api/runner`:
  explicitly reran the installed validation executable, exit 0.
- `/tmp/renderer-m0-implementation/install_library m03gilsfsv3k34ej14ytz8a29k_tower_defense_game`
  and `/tmp/renderer-m1-implementation/install_binary m03gilsfsv3k34ej14ytz8a29k_tower_defense_game`:
  native consumer library and executable builds, including experimental rendering paths.
- Focused staging checks exercised quaternion extremes and composition, full-range
  viewport arithmetic, and 10,000 rational comparisons. An independent comparison
  checked direct line evaluation against the prior stepping algorithm for all
  4,225 endpoint deltas in `[-32,32]^2`.
- `git -C /home/gilqamesh/Projects/Builder-Modules diff --check`: passed.

The public suite includes an analytical ray/plane texture oracle for a tilted
surface crossing the near plane in both projections, zero Z scale, camera/item
translation equivalence, camera inverse pose and look-at, near/far depth, all
seven topologies in bounded views, original perspective aspect after intersection,
absolute fragment coordinates, pose-independent clearing, and empty-region no-ops.
Look-at regressions include exactly parallel diagonal up vectors; the cross
product precedes normalization to preserve their rejection.

Visible desktop checks used X11 and each module's working directory for assets:

```sh
python3 /tmp/renderer-m1-implementation/smoke.py m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer 'Software Renderer' renderer
python3 /tmp/renderer-m1-implementation/smoke.py m03gilsfsv3k34ej14ytz8a29k_tower_defense_game 'Tower Defense Game' tower-defense
```

Both applications presented textured geometry and closed normally with exit 0.
The renderer capture shows perspective texture mapping at 960x540; the
1600x1200 tower-defense capture retains the configured 400x200 camera region.
Captures are `/tmp/renderer-m1-implementation/renderer-smoke-0.png` and
`/tmp/renderer-m1-implementation/tower-defense-smoke-0.png`. Build, validation,
and smoke logs and the temporary helper sources are in the same directory.

No milestone 1 semantic decision remains open. The inactive tower-defense
rendering backends were migrated and compiled; only its active software-renderer
path was visually checked. The smoke checks establish presentation and integration,
not a performance target. Depth/stencil, culling, blending, and later milestones
remain unimplemented.

### Quaternion module integration — 2026-09-06

The renderer now consumes
[`m03gtgtrh2smvh28qlwgm7gdl4_quaternion`](../../../ws1/m03gtgtrh2smvh28qlwgm7gdl4_quaternion/AGENTS.md).
The local quaternion implementation was removed. Camera and render-item APIs
use the quaternion module type directly. Their setters normalize a copy
with `unit()`, while Euler and look-at conversions use `from_euler_xyz()` and
`from_matrix()`. The shared module supplies rotation matrices through `to_matrix()`.
The pose owners retain read-only rotation access and reject invalid assignments
without changing the previous orientation. No quaternion-module source changed.

Validation passed:

```sh
python3 /tmp/renderer-quaternion-reuse/build.py public_api
/tmp/renderer-m1-implementation/install_binary m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
/tmp/renderer-m1-implementation/install_binary m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
python3 /tmp/renderer-quaternion-reuse/smoke.py m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer 'Software Renderer' renderer
python3 /tmp/renderer-quaternion-reuse/smoke.py m03gilsfsv3k34ej14ytz8a29k_tower_defense_game 'Tower Defense Game' tower-defense
```

The GNU staging suite and native Clang renderer validation passed. Both native
application builds and both visible smoke checks exited 0. Renderer tests verify
type identity with the shared module, normalization at both pose setters, copying
of mutable inputs, invalid-update state preservation, and Euler input semantics.
`git diff --check` passed. Logs, helper scripts, and window captures are in
`/tmp/renderer-quaternion-reuse`. Existing milestone limitations remain unchanged.

### Milestone 2 implementation record — 2026-09-06

Reviewed base: `Builder-Modules` revision
`42a5572cdf3db385725919f8a2fb24d431900c1d`. Implementation is in the working
tree; no commit was created by this task.

The milestone 2 contracts were recorded in [draw_state.h](https://github.com/Gilqamesh/Builder-Modules/blob/47ecefce6aaf3e7f67137ab7f3c4523cd040b550/ws2/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/draw_state.h),
[material.h](../material.h), [framebuffer.h](../framebuffer.h), and
[software_renderer.h](../software_renderer.h). The renderer reads the material's
state for each draw. Its existing coverage and interpolation paths supply the
same clamped float window depth to the shader, depth comparison, and depth write.
Effective front-face selection is derived separately from geometric winding so
that culling cannot change triangulation or sample ownership. Depth comparisons
and attachment writes follow shader completion; discard suppresses both writes.

The demo now renders intersecting textured surfaces with depth testing and back-face
culling, alternates their submission order, and retains near-plane clipping.
The application resizes color and depth storage together and explicitly clears
both each frame. No consumer or dependency source changed.

Checks passed (exit 0):

```sh
python3 /tmp/renderer-m2-implementation/build.py public_api
/tmp/renderer-m1-implementation/install_binary m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
/tmp/renderer-m1-implementation/install_binary m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
artifacts/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/latest/library/build/validation/public_api/runner
python3 /tmp/renderer-m2-implementation/smoke.py m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer 'Software Renderer' renderer
python3 /tmp/renderer-m2-implementation/smoke.py m03gilsfsv3k34ej14ytz8a29k_tower_defense_game 'Tower Defense Game' tower-defense
git -C /home/gilqamesh/Projects/Builder-Modules diff --check
```

The staging build used GNU C++23 with `-Wall -Wextra -ftrapv`. Native Builder
used Clang C++23 and automatically ran the renderer public suite. Additional
GNU C++23 syntax checks covered the demo and the consumer's `game.cpp`,
`renderer.cpp`, and `renderer3.cpp`; existing consumer warnings remain.

Both final desktop smoke checks used X11, each module's asset working directory,
and exited normally. Captures were inspected: the renderer shows intersecting
textured surfaces at 960x540; tower-defense retains its existing 400x200 camera
region within the 1600x1200 framebuffer. The initial sandboxed renderer launch
could not open the display. An initial desktop capture preceded the first rendered
frame; the smoke helper now waits for visible content. An initial consumer capture
lost its window and timed out; its repeated check completed successfully.

Logs, staging helpers, and captures are in `/tmp/renderer-m2-implementation`,
including `public_api.log`, `native-renderer.log`, `native-tower-defense.log`,
`renderer-smoke-0.png`, and `tower-defense-smoke-0.png`.

No milestone 2 semantic decision remains open. Stencil, blending, color masks,
colorless framebuffers, and later milestones remain unimplemented. Inactive
consumer rendering paths were compiled; only the active software-renderer path
was visually checked. These checks establish correctness and integration;
no performance baseline or optimization comparison was produced.

### Material and attachment API migration — 2026-09-06

Reviewed base: `Builder-Modules` revision
`47ecefce6aaf3e7f67137ab7f3c4523cd040b550`. The changes are in the working tree.

[Material properties](../material.h) replace the public `draw_state_t` and own
immediate enum validation. The [framebuffer depth setter](../framebuffer.h)
validates attachment replacement. [Clearing](../software_renderer.h) now uses
`clear_color` and requires an explicit depth value. The draw signature, defaults,
depth and culling behavior, and clear-value rules are preserved.

Changes cover `material.h/.cpp`, `framebuffer.h/.cpp`,
`software_renderer.h/.cpp`, `helpers.h/.cpp`, `cli.cpp`, `test/public_api.cpp`,
`AGENTS.md`, and this roadmap; `draw_state.h` was removed. The tower-defense
consumer's `game.cpp` uses `clear_color`. Current examples use the new API;
earlier milestone records retain their historical evidence.

Checks passed (exit 0), from the Builder-Layout directory:

```sh
python3 /tmp/renderer-api-migration/build.py public_api > /tmp/renderer-api-migration/public_api.log 2>&1
/tmp/renderer-m1-implementation/install_binary m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer > /tmp/renderer-api-migration/native-renderer.log 2>&1
/tmp/renderer-m1-implementation/install_binary m03gilsfsv3k34ej14ytz8a29k_tower_defense_game > /tmp/renderer-api-migration/native-tower-defense.log 2>&1
artifacts/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/latest/library/build/validation/public_api/runner
python3 /tmp/renderer-api-migration/smoke.py m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer 'Software Renderer' renderer
python3 /tmp/renderer-api-migration/smoke.py m03gilsfsv3k34ej14ytz8a29k_tower_defense_game 'Tower Defense Game' tower-defense-retry
git -C /home/gilqamesh/Projects/Builder-Modules diff --check
```

The staged GNU C++23 suite used `-Wall -Wextra -ftrapv`. Native Clang builds
passed for both applications and ran the renderer suite. Added cases verify
immediate invalid-enum rejection with previous-value preservation, material
copying and sharing, attachment replacement and detachment, failed replacement,
and independent framebuffer views. Existing depth, culling, clipping,
interpolation, and clear regressions pass through the new API. The installed
interface no longer contains `draw_state.h`.

Both final X11 smoke checks presented frames and closed normally. Inspected
captures show the renderer's intersecting textured surfaces at 960x540 and the
consumer's existing 400x200 camera region within its 1600x1200 framebuffer.
The first consumer capture (the same command with label `tower-defense`) lost
its window and timed out; the repeated check above passed. Logs, helpers, and
captures are in `/tmp/renderer-api-migration`, including `renderer-smoke-0.png`
and `tower-defense-retry-smoke-0.png`.

No API-migration decision remains open. Only the active consumer rendering path
was visually checked; performance was not benchmarked.

### Profiling and initial optimized baseline — 2026-09-06

The initial `profiling` module and `software_renderer_t<Profiler>` integration implemented
the then-settled capture, registration, lifetime, overflow, deferred formatting, and
disabled-policy contracts. Application and renderer scopes share one application
profiler, with explicit region registration before capture. The tower-defense
consumer and renderer demo select the default disabled template policy.

The dedicated headless benchmark compiles the renderer and performance-relevant
dependencies, including `software_shader` and texture sampling, with `-O2`.
Native Clang and optimized GNU public validation passed, as did the mixed-payload,
disabled assembly, missing-formatter, and consumer presentation checks.
[The baseline report](profiling-baseline.md) records exact commands, source
identity, empirical results, and limitations; [raw samples](profiling-baseline.json)
are preserved. The profiling implementation was committed as `1aee3733`. No algorithmic
optimization delivery is claimed.

### Profiling benchmark migration — 2026-09-06

The benchmark is registered through the renderer's existing Builder producer API,
with the demo target preserved. C++ owns workload execution and statistics; Builder
owns its build. Documentation and the original baseline now reside in `docs/`, and
the standalone Python driver has been removed. Profiler constructor overloads and
descriptive template naming preserve the existing capture and renderer contracts.

Native public validation, independent summary checks, constructor/formatter/compiler
checks, target reuse, and both graphical smoke checks passed. See the
[migration validation record](profiling-migration.md) for exact commands and limits.
The original optimized raw baseline is unchanged. General dependency optimization
and a new optimized baseline remain deferred. These changes were committed as `13888dd`.

### Runtime-attached profiling — 2026-09-07

The ordinary renderer borrows an application-owned collector through an attachment
setter. Typed nullable scopes replace template policies and region registration.
Payloads remain in borrowed byte storage through deferred reporting; reset releases
them while preserving attachments. Renderer stage boundaries and counter meanings
are preserved, and the benchmark compares attached and unattached instances of the
same renderer class. See [the current profiling contract and caller](profiling.md).


### Milestone 3 implementation record — 2026-09-07

Reviewed base: `0d1db912038f0d15f76d256b3db55b04858a87fd`. Changes are in the
working tree; this task created no commit.

The settled contracts are in [material.h](../material.h), [framebuffer.h](../framebuffer.h),
and [software_renderer.h](../software_renderer.h). Materials have validated blend
properties and independent RGB/alpha equations; framebuffer color storage is neutral
about alpha association and supports explicit linear/sRGB interpretation. Both equations
use original inputs before any channel is stored. Byte clears and late depth behavior
are preserved. No milestone 3 semantic decision remains open.

Changed files: `material.h/.cpp`, `framebuffer.h/.cpp`, `helpers.h/.cpp`,
`software_renderer.h`, `profiling_metrics.h`, `test/public_api.cpp`, `benchmark.cpp`,
`cli.cpp`, `AGENTS.md`, `docs/milestones.md`, `docs/profiling.md`, and
`docs/blending-performance.md/.json`.

Checks obtained from changed source (all exit 0):

```sh
python3 /tmp/renderer-m3-implementation/build.py public_api
./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark --help
/tmp/renderer-m1-implementation/install_binary m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
/tmp/renderer-m1-implementation/install_binary m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
python3 /tmp/renderer-m3-implementation/smoke.py m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer 'Software Renderer' renderer
python3 /tmp/renderer-m3-implementation/smoke.py m03gilsfsv3k34ej14ytz8a29k_tower_defense_game 'Tower Defense Game' tower-defense
git -C /home/gilqamesh/Projects/Builder-Modules diff --check
```

Staged GNU C++23 validation used `-O0 -g -Wall -Wextra -ftrapv` against current
installed dependencies. Additional GNU `-std=c++23 -Wall -Wextra -fsyntax-only`
checks passed for the demo and benchmark. Native Builder rebuilt with Clang C++23
and automatically ran the expanded public suite. Native consumer compilation also
passed. The first new-test failures exposed an incorrect literal sRGB expectation
and a fixture missing a required shader AST output; these were corrected before
application and native validation.

Public checks cover all factors in both contribution roles and RGB/alpha channels,
all operations, separate equations, original-alpha dependence, constants, numeric
boundaries, all 16 masks in linear/sRGB storage, transfer boundaries and all 256
sRGB byte round trips, repeated composition, state validation/copying/sharing,
attachment interpretation, discard, depth failure, absent color, disabled masks,
profiling equivalence/counters, camera bounds, shared/clipped edges, crossed and
concave fixtures, and point/line/strip/loop/fan behavior.

Both X11 desktop checks displayed content and closed normally. Inspected captures
are `/tmp/renderer-m3-implementation/renderer-smoke-0.png` (960×540, translucent
overlay over intersecting opaque geometry) and `tower-defense-smoke-0.png`
(1600×1200, preserving the consumer's 400×200 camera region). The renderer demo
uses an opaque clear so alpha remains one; its sRGB bytes pass through the existing
RGBA8 presentation upload without texture sRGB decoding.

[Benchmark comparison and exact commands](blending-performance.md) and
[raw results](blending-performance.json) retain the four existing workloads and
new linear/sRGB translucent costs. These use the same native build settings before
and after, separately from the historical optimized baseline. Timing conclusions
are limited by measured variation and uncontrolled external load.

Logs, staged sources and helpers remain in `/tmp/renderer-m3-implementation`.
Only the active tower-defense software path was visually checked. Dual-source
blending, advanced operations, logic operations, extra attachments, stencil, and
later feature milestones remain deferred.

### Milestone 4 implementation record — 2026-09-07

Reviewed base: `e5daeb1`. Changes are in the working tree; no commit was created.
The settled public contracts are in [material.h](../material.h),
[framebuffer.h](../framebuffer.h), [software_renderer.h](../software_renderer.h),
the texture module's [pixel_view.h](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/pixel_view.h)
and [sampler.h](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/sampler.h).
The [migration and caller guide](render-targets.md) records the explicit `pixels()`
return-type change, mutable/read-only view access, encoding reinterpretation and
application-owned pass sequencing. No milestone 4 semantic decision remains open.

Changed texture files: `pixel_view.h/.cpp`, `texture.h/.cpp`, `sampler.h/.cpp`,
`test/public_api.cpp`, and `AGENTS.md`. Changed renderer files:
`framebuffer.h/.cpp`, `material.h/.cpp`, `helpers.h/.cpp`, `software_renderer.h/.cpp`,
`profiling_metrics.h`, `test/public_api.cpp`, `cli.cpp`, `benchmark.cpp`, `AGENTS.md`,
`docs/milestones.md`, `docs/profiling.md`, `docs/render-targets.md`, and
`docs/milestone-4-performance.md/.json`.

Checks obtained from changed source (exit 0):

```sh
python3 /tmp/renderer-m4-implementation/build_stencil.py
python3 /tmp/renderer-m4-implementation/build.py texture public_api
/tmp/renderer-m1-implementation/install_binary m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
/tmp/renderer-m1-implementation/install_binary m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
python3 /tmp/renderer-m4-implementation/smoke.py m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer 'Software Renderer' renderer-retry
python3 /tmp/renderer-m4-implementation/smoke.py m03gilsfsv3k34ej14ytz8a29k_tower_defense_game 'Tower Defense Game' tower-defense
artifacts/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/latest/binary/benchmark/install/benchmark --help
git -C /home/gilqamesh/Projects/Builder-Modules diff --check
```

Staged validation used GNU C++23 with `-O0 -g -Wall -Wextra -ftrapv`; stencil
validation passed before the pixel-access migration. The native Builder build used
Clang C++23 and automatically rebuilt and passed the texture, software-shader and
renderer public suites. The renderer demo, benchmark and all tower-defense source
paths rebuilt successfully. GNU syntax checks also passed for the demo and benchmark.
Source comparison verified installed texture/renderer C++ files against the working
source; measured hashes are preserved with the performance report.

Focused tests cover every stencil operation/comparison, masks and boundaries,
front/back selection, point/line behavior, discard, absent/masked color, depth outcomes,
clear bounds, attachment/state replacement and profiling equivalence. Shared and
clipped edges, including crossed/concave fixtures, preserve one increment per covered
original primitive. View tests cover zero extents, overflow, format eligibility,
constness, copying, movement, rebinding, shared storage identity and owner lifetime.
Two-pass tests cover orientation, linear/sRGB storage, premultiplied composition and
bilinear edges, reused allocation, explicit snapshots and conservative feedback
rejection before vertex execution in both stages, including partial overlap,
disabled writes, clipped geometry and unused extra bindings.

The final desktop checks displayed content and closed normally. Inspected captures
are `/tmp/renderer-m4-implementation/renderer-retry-smoke-0.png` (960×540, stencil
mask and sampled composite) and `tower-defense-smoke-0.png` (1600×1200, retaining
the consumer's 400×200 rendering region). The sandboxed renderer launch could not
open X11; desktop validation succeeded. The first desktop helper timed out waiting
30 seconds for closure after capturing a frame. The demo's unoptimized frames took
roughly 21–25 seconds; a repeated check with a longer close timeout passed.

[Performance results and exact commands](milestone-4-performance.md), with
[raw evidence](milestone-4-performance.json), compare the six existing workloads
and establish three feature baselines. This is a feature delivery without an
algorithmic optimization claim. Colorless framebuffers, floating-point color
attachments, early testing and milestone 5 remain deferred. Only the active
tower-defense presentation path was visually checked.

## Deferred scope

Defer instancing, multiple color targets, MSAA, advanced sampling, and performance architecture work until a concrete scene or measurement warrants them. Point size/shape and line coverage also remain explicit future scope; the baseline uses a fixed point radius and single-pixel lines.

## Documentation ownership

This module-local roadmap records proposed outcomes, open decisions, and completion evidence. Public headers own public contracts; each module's AGENTS.md owns its durable module-specific invariants. Record cross-module decisions here with links to their authoritative contracts, without duplicating those contracts or the shared workflow.

## Baseline references

- [Renderer contract and intended direction](https://github.com/Gilqamesh/Builder-Modules/blob/540bbede71740d24292cc3b7cd9c8ed126eca0c3/ws2/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/AGENTS.md)
- [Public renderer API](https://github.com/Gilqamesh/Builder-Modules/blob/540bbede71740d24292cc3b7cd9c8ed126eca0c3/ws2/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/software_renderer.h)
- [Pipeline implementation](https://github.com/Gilqamesh/Builder-Modules/blob/540bbede71740d24292cc3b7cd9c8ed126eca0c3/ws2/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/software_renderer.cpp)
- [Renderer tests](https://github.com/Gilqamesh/Builder-Modules/blob/540bbede71740d24292cc3b7cd9c8ed126eca0c3/ws2/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/test/public_api.cpp)
- [Texture sampling API](https://github.com/Gilqamesh/Builder-Modules/blob/540bbede71740d24292cc3b7cd9c8ed126eca0c3/ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/sampler.h)
