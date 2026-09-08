# Software renderer completion proposal

Status: proposed scope for review. Approval of this documentation work does not
accept the feature set or change current rendering behavior. After scope review,
record accepted or amended dispositions here; design each accepted milestone
before changing its public contract.

## Target and baseline

Provide a conventional programmable CPU rasterizer capable of forward rendering,
shadow maps, HDR post-processing, deferred shading, instancing, and basic MSAA.
Use the same camera and drawing model for planar scenes. Each capability should
have a small example useful for study and measured costs on representative scenes.

Baseline: [Builder-Modules at d23a422](https://github.com/Gilqamesh/Builder-Modules/tree/d23a422e147eeda74918a6357c849843327a287d).
The cleanup introduced prepared shader execution and draw preparation, corrected
strip winding, and retained exceptional coverage guarantees. This proposal uses
source and test-source inspection; it adds no build or benchmark results.

Public headers own current behavior. Existing ownership remains as described in
[AGENTS.md](../AGENTS.md): applications and shaders implement lighting models,
shadow-pass organization, tone mapping, and scene management.

## Capability scope

Required means required for this recommended completion target; every disposition
below remains proposed. Complexity estimates describe additional implementation
burden, not schedules. Deferred capabilities can be reviewed separately.

| Capability | Current support | Recommended disposition | Concrete completion example | Complexity |
|---|---|---|---|---|
| Transforms, clipping, seven topologies, interpolation, depth/stencil, blending, sRGB, render-to-texture, explicit mip sampling | Implemented, including focused coverage regressions | Retain | Preserve conventional forward rendering and existing regressions | Existing |
| Vulkan depth convention, independent viewport/scissor, configurable viewport depth range | Z clips to [-W,W]; one camera rectangle; fixed depth mapping | Required | Scissor a perspective scene without changing its projection; demonstrate reversed depth mapping | Moderate |
| Non-indexed drawing | Geometry selects an index-buffer range | Required | Draw a vertex range without constructing an identity index buffer | Moderate |
| Instancing with per-instance data | Shader instance index exists; renderer supplies zero; no instance-rate input | Required | Draw repeated meshes with distinct transforms and colors in one draw | Moderate |
| Shader-readable indexed arrays or buffers | Individually declared typed uniforms; no general indexed collection | Required, separately from instancing | Read a light list or bone-matrix palette without a uniform per element | Substantial |
| Colorless rendering, sampleable float depth, comparison sampling, constant/slope depth bias | Borrowed float depth span alongside mandatory color; no native depth sampler or bias | Required | Render a shadow map and sample it in a later lighting pass | Substantial |
| Floating-point color attachments and format-appropriate blending | Float textures exist; renderer accepts only RGBA8 linear/sRGB targets | Required | Preserve values above one through rendering, then tone-map to RGBA8 | Moderate |
| Multiple color attachments with per-attachment blending/write masks | One color attachment and one color blend/write configuration | Required | Write albedo and normals together, then run a lighting pass | Substantial |
| Fragment derivatives, implicit LOD, explicit-gradient sampling | Individual shader invocations; ordinary sample uses level zero; explicit LOD exists | Required | A receding surface selects mips automatically, including shader-modified UVs | High |
| Border/mirrored addressing, LOD bias/clamps, exact texel fetch | Repeat/clamp-to-edge, nearest/linear filtering and explicit LOD clamped to allocated levels | Required | Handle shadow-map borders and make exact image-processing reads | Moderate |
| Square points with shader point size/coordinates; one selected Vulkan-compatible line rule | Fixed radius-three point disk; single-pixel lines include both floored endpoints | Required | Render textured point sprites and connected lines with predictable coverage | Moderate |
| Basic MSAA: 1x/4x, per-sample attachments/tests, centroid interpolation, sample masks, color resolve | One sample per pixel | Required | Resolve partially covered triangle edges with depth/stencil enabled | High |
| Cubemaps, texture arrays/3D textures, anisotropy, compressed/packed formats | 2D RGBA textures without these extensions | Deferred | Broader environment/volume rendering and storage efficiency | Separate extensions |
| Wide/stippled lines, wireframe, fragment-depth output, depth clamp/bounds, alpha-to-coverage, sample-rate shading, primitive restart | Not exposed | Deferred | Specialized rasterization and shading controls | Mixed |
| Compute, geometry/tessellation/mesh shaders, ray tracing, writable shader storage/atomics, Vulkan device/queue/memory emulation | Outside the current vertex/fragment CPU pipeline | Outside this completion target | Different execution systems beyond the selected rasterizer | High |

Current evidence: [drawing](../software_renderer.h), [geometry](../geometry.h),
[attachments](../framebuffer.h), [material state](../material.h),
[shader construction](../../m03gsy25j4v7nccgmsdov9ioft_shader/shader_builder.h),
[CPU execution](../../m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h),
[texture formats](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/pixel_view.h),
and [sampling](../../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/sampler.h).

## Decisions needed before implementation

These are recommendations for review, not settled API designs.

| Decision | Preferred direction | Consequence |
|---|---|---|
| Semantic reference | Use Vulkan 1.4 core behavior for supported features; identify optional features/extensions explicitly in each milestone. Keep the camera/item/material API. | The target is a documented subset, with intentional deviations identified; it is not a full Vulkan implementation. |
| Coordinates and coverage | Adopt [the rasterization recommendations](rasterization-planning.md): coordinated depth migration, independent viewport/scissor, retained screen orientation and exceptional polygon guarantees. Add compatibility modes only for demonstrated caller needs. | Projections, clipping, direct clip-coordinate callers, and consumers migrate together; broad coverage simplification is not a prerequisite. |
| Attachment model | Extend the existing texture/view model for typed depth and color storage; retain borrowed framebuffer attachments and explicit pass sequencing. Make framebuffer extent independent of a mandatory color attachment. | Shadows, HDR, and MRT share attachment validation/lifetime decisions, while remaining separate capabilities. Detailed layouts and sample storage belong to their milestone designs. |
| Per-instance data | Prefer instance-rate vertex attributes for the initial repeated-transform/color example. Review shader-indexed resources as an alternative if a concrete caller benefits. | Instancing requires a selected data mechanism; general arrays/buffers are not automatically a prerequisite. |
| General indexed shader data | Start with typed, read-only indexed resources for light lists and matrix palettes; keep shader writes/atomics outside scope. | Design bounds failures, borrowing, mutation visibility, and preparation/refresh behavior together before exposing this separate capability. |
| Derivative execution | Prefer coordinated 2x2 fragment groups with helper invocations and documented control-flow restrictions. Preserve the independent invocation path for shaders that do not need derivatives. | Shader construction, CPU execution, rasterization, and sampling need a shared execution contract; interpolant gradients alone cannot cover arbitrary shader-modified UVs. |
| Initial MSAA subset | Start with 1x/4x storage and per-sample tests, centroid interpolation and sample masks, followed by explicit color resolve. Defer sample-rate shading and depth/stencil resolve. | Coverage, shading frequency, attachment layout, and resolve semantics must be designed together; 1x behavior must remain covered by regression tests. |

The derivative design should use the
[Vulkan derivative rules](https://docs.vulkan.org/spec/latest/chapters/shaders.html#shaders-derivative-operations)
to identify helper participation and control-flow constraints. Detailed API design
and remaining numeric limits belong to each accepted milestone.

## Proposed milestones and dependencies

Suggested priority reflects practical value and study progression:

1. Coordinate/scissor semantics and viewport depth range.
2. Depth targets and shadows, including comparison and border sampling and bias.
3. Floating-point color attachments and blending.
4. Multiple color attachments and independent blend/write state.
5. Non-indexed drawing, instancing, and general indexed shader data as separately
   reviewable deliveries.
6. Derivative execution, implicit LOD, explicit gradients, and remaining sampler features.
7. Basic MSAA and color resolve.

Point/line changes form an independent bounded milestone. Priority is not a
dependency chain: shadows are not required for HDR, and HDR is not required for
MRT. MRT can start with existing formats.

Actual dependencies:

- The shadow example needs colorless/sampleable depth, comparison and border
  sampling, and bias. It does not need derivatives or general indexed shader data.
- HDR requires floating-point attachment writes and format-appropriate blending.
  MRT requires routing shader outputs to attachments and independent blend/write state.
  Both depend on reviewed attachment rules, not on each other.
- Instancing requires instance iteration/index semantics and the selected
  per-instance data mechanism. If instance-rate attributes are selected, it can
  precede general arrays/buffers; non-indexed drawing is independent.
- Implicit LOD for shader-computed coordinates requires the coordinated derivative
  execution design. Explicit-gradient sampling and other sampler additions can be
  delivered independently where their own contracts suffice.
- MSAA requires sample storage, coverage, tests, interpolation and resolve rules.
  It does not inherently depend on HDR, MRT, instancing, or derivatives.

## Completion evidence

For each accepted milestone, provide one small public-API example, focused
contract and negative-case validation, relevant direct-consumer checks, and
comparable measurements. Extend existing feature guides alongside the implementation.
Keep executable evidence in tests and measurement results in their reports.

Preserve the difficult clipping/shared-edge fixtures unless a separate contract
review explicitly accepts different coverage. Trace one ordinary triangle and an
exceptional case when evaluating readability; choose simplification only after
identifying a concrete explanation gap or measured cost.

Retain the small benchmarks. Rectangular benchmark support and fresh optimized
measurements at proposed sizes 640x360 and 1280x720 are subsequent work; the current
driver accepts a square --size. Select representative many-draw, indexed-mesh,
overdraw, filtering, and multipass scenes, recording revision, hardware, compiler
configuration, median/p95 times, and stage costs. See [profiling](profiling.md)
for current commands and measurement boundaries. Set numerical performance targets
after selecting workloads and hardware; earlier timings do not measure this cleanup.

Vertex reuse and early rejection remain measured candidates, with the
[fragment-test ordering constraints](rasterization-planning.md#fragment-test-ordering)
preserved or explicitly reviewed. Completion requires the accepted scope's examples
and validation evidence; this proposal alone establishes neither implementation
completion nor representative performance.
