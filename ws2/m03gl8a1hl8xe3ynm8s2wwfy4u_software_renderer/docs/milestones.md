# Software renderer milestones

Status: milestone 0 is implemented; later milestones remain proposed and unstarted. Public behavior is owned by the module headers.

Baseline: [Builder-Modules at 540bbede71740d24292cc3b7cd9c8ed126eca0c3](https://github.com/Gilqamesh/Builder-Modules/tree/540bbede71740d24292cc3b7cd9c8ed126eca0c3).

Develop a general-purpose headless 3D CPU rasterizer while retaining the existing 2D path. Design and implementation follow the shared [agent workflow](../../../../Builder/docs/agent-workflow.md).

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

Status: unstarted.

- Outcome: perspective and orthographic scenes through the public API, with separate viewport mapping and scissor clipping.
- Open decisions: how callers supply object/view/projection transforms; which camera and transform helpers remain conveniences; how the 2D path maps into the same pipeline; viewport/scissor ownership and clear behavior.
- Acceptance criteria: a textured 3D object transforms and crosses the near plane correctly, perspective texture mapping is demonstrated, and a bounded view cannot overwrite pixels outside its scissor. Existing 2D scenes retain the agreed behavior.

## 2. Depth testing and face culling

Status: unstarted.

- Outcome: correct opaque visibility independent of submission order for unequal depths, plus configurable winding/cull state.
- Open decisions: depth attachment lifetime/format, clear value, comparison function, equal-depth tie behavior, independent depth writes, and the point at which fragment discard prevents writes. Decide draw-state ownership here before adding more state.
- Acceptance criteria: overlapping and intersecting surfaces render correctly in either order for unequal depths, with equal-depth samples following the chosen tie policy; depth-test and depth-write controls behave independently; discarded fragments do not occlude later geometry; culling follows the chosen convention.

## 3. Blending and color writes

Status: unstarted.

- Outcome: transparent overlays compose correctly with opaque geometry.
- Open decisions: blend factors/equations, color masks, straight versus premultiplied alpha, linear versus sRGB attachments, and encoding boundaries. Reuse the draw-state model from milestone 2.
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
