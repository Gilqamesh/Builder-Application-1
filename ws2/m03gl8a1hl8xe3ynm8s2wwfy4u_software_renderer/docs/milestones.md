# Software renderer milestones

Status: milestones 0 and 1 are implemented and validated. Later milestones remain proposed and unstarted. Public behavior is owned by the module headers.

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

- Outcome: perspective and orthographic scenes use `draw(camera, render_item)`.
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
software_renderer.clear(camera, {0, 0, 0, 255});
software_renderer.draw(camera, item);
```

## 2. Depth testing and face culling

Status: unstarted.

- Outcome: correct opaque visibility independent of submission order for unequal depths, plus configurable winding/cull state.
- Open decisions: depth attachment lifetime/format, clear value, comparison function, equal-depth tie behavior, independent depth writes, and the point at which fragment discard prevents writes. Settle ownership and public interfaces for this milestone's state and operations.
- Acceptance criteria: overlapping and intersecting surfaces render correctly in either order for unequal depths, with equal-depth samples following the chosen tie policy; depth-test and depth-write controls behave independently; discarded fragments do not occlude later geometry; culling follows the chosen convention.

## 3. Blending and color writes

Status: unstarted.

- Outcome: transparent overlays compose correctly with opaque geometry.
- Open decisions: blend factors/equations, color masks, straight versus premultiplied alpha, linear versus sRGB attachments, and encoding boundaries. Build on the ownership decisions settled in milestone 2.
- Acceptance criteria: known source/destination colors produce expected RGB and alpha, masks preserve disabled channels, sRGB conversion occurs at the agreed boundary, and shared edges have no blending seams. The application supplies transparent draw order.

## 4. Stencil and render-to-texture

Status: unstarted.

- Outcome: stencil masking and a clear path from rendering a pass to sampling its result. Basic offscreen color rendering already exists.
- Open decisions: color/depth/stencil attachment views and lifetimes, stencil comparisons/operations/masks, depth-only or unwritten-color behavior, clear semantics, and render-target/texture interoperability. Define or reject simultaneous sampling and writing of the same storage.
- Acceptance criteria: a stencil mask limits a draw correctly and one rendered pass is sampled by a later pass with matching orientation and color semantics.

## 5. Interpolation modes and mipmapped sampling

Status: unstarted.

- Outcome: flat varyings, including integer values; noperspective interpolation; mipmapped textures and explicit LOD sampling.
- Open decisions: interpolation metadata and linking, provoking-vertex rules through clipping/topology assembly, mip storage/generation, and LOD filtering. Keep reflection changes in shader, execution in software_shader, interpolation in the renderer, and sampling in texture.
- Acceptance criteria: flat values remain constant across clipped primitives, screen-linear and perspective interpolation visibly differ as intended, and explicit LOD selects/blends validated mip levels. Automatic derivative-based LOD is a separate later decision.

## 6. Measurement and incremental optimization

Status: unstarted.

- Outcome: each optimization delivery demonstrates its effect through repeatable measurements while preserving rendering correctness.
- Measurement module: introduce a separate module responsible for collecting measurements, summarizing repeated runs, and comparing deliveries. Renderer-specific workloads and correctness expectations remain with the renderer.
- Metrics: track median and high-percentile CPU render time, plus peak memory use, across a small, stable set of representative headless workloads. Keep the measured scope consistent across deliveries.
- Comparisons: record workload, resolution, rendering settings, hardware, build configuration, and source revision. Report absolute results, percentage changes, and run-to-run variation against both the previous delivery and the established baseline. New feature workloads establish their own baselines.
- Delivery process: use measured results to select each bounded optimization. Every delivery includes the same comparison report and correctness evidence, making improvements, regressions, and tradeoffs visible. Specific optimization techniques remain undecided until measurements justify them.
- Acceptance criteria: repeated runs establish measurement variability, and each optimization delivery has comparable before/after results with passing correctness validation.

Establish the measurement baseline before the first delivery to be compared. Comparisons covering milestones 2–5 require the measurement foundation before those deliveries; the dedicated optimization phase remains at the end.

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
