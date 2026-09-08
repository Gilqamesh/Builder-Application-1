# Rasterization contract decisions

The [completion proposal](milestones.md) recommends a scope for review. The
directions below remain proposed; current public contracts stay authoritative
until a replacement is reviewed and implemented. This document owns detailed
rasterization comparisons and recommendations, not additional current guarantees.

## Depth convention

Confirmed difference: current behavior clips Z to [-W,W]; camera near/far map to
NDC -1/+1 and fragment depth maps through (Z/W+1)/2. Vulkan defaults to [0,W].

Recommended direction: adopt [0,W] consistently across perspective and orthographic
projection, depth clipping, camera world-to-framebuffer results, and fragment depth
mapping. Migrate examples, fixtures, and direct consumers together, including
callers that supply clip coordinates directly. Introduce a migration period or
legacy convention only when a concrete caller requires it.

Consequence: this changes accepted clip positions and projection results, so
changing a depth clip plane alone is insufficient. Preserve X/Y screen orientation
and independent depth comparison/write rules.

Validation: near/far boundaries, just-outside positions, varying positive W,
clipped lines and triangles, perspective/noperspective interpolation, equal-depth
comparison, clears, stencil/depth ordering, and render-to-texture consumers.

Reference: [Vulkan primitive clipping](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-clipping).

## Viewport, scissor, and depth range

Confirmed difference: the [camera rectangle](../camera.h) currently supplies both
viewport mapping and write bounds, with a fixed depth mapping into [0,1].
Framebuffer intersection limits writes without changing projection aspect.
Vulkan specifies viewport mapping/depth range separately from scissor bounds.

Recommended direction: retain the camera's projection/viewport role, introduce an
independent scissor selection for drawing, and support viewport depth endpoints
including reversed order. Preserve full-viewport scissoring and depth range [0,1]
as defaults. Keep clearing explicit and decide its region independently of draw
scissoring; retain existing camera-based clear regions.

Consequence: narrowing the scissor must not change projection or aspect. Reversed
depth mapping requires a coordinated clear value and comparison selected by the
application; it must not silently change material state. Settle public state
placement in this milestone's API design.

Validation: a small scissor inside an unchanged perspective view, partial
framebuffer overlap, empty intersections, near/far depth endpoints in both orders,
and explicit clears independent of material state.

References: [Vulkan viewport](https://docs.vulkan.org/refpages/latest/refpages/source/VkViewport.html),
[scissor test](https://docs.vulkan.org/spec/latest/chapters/fragops.html#fragops-scissor).

## Y mapping and facing

Unresolved comparison: the current mapping flips Y into a top-left framebuffer.
The header describes CCW NDC facing; the implementation derives facing from snapped
framebuffer geometry. For ordinary triangles at the same framebuffer positions,
its sign test agrees with Vulkan's signed-area rule. That wording difference alone
does not establish a facing defect. Exceptional snapped boundaries require separate
comparison.

Recommended direction: preserve the current screen orientation, validate facing
with projection and viewport Y mapping together, and describe the selected
framebuffer-area convention explicitly. Retain exceptional-boundary behavior as
described below. Vulkan permits negative viewport height, so a Y flip alone does
not establish incompatibility.

Consequence: avoid changing culling merely to replace NDC terminology. Validate
the complete transform and actual sample/facing outcomes before any semantic change.

Validation: CW/CCW triangles, negative object scale, strip/list equivalents,
clipping, and front/back stencil behavior under the selected viewport mapping.

References: [Vulkan polygon facing](https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-polygons-basic),
[viewport mapping](https://docs.vulkan.org/refpages/latest/refpages/source/VkViewport.html).

## Point coverage

Confirmed difference: current points cover a fixed disk of integer offsets
dx*dx+dy*dy <= 9 around the floored projected position. Vulkan uses a square
controlled by shader point size.

Recommended direction: support square points with shader-produced size and point
coordinates for textured sprites, defaulting to a one-pixel size when omitted.
Review supported size limits, invalid-value behavior, and clipping in that
milestone; prefer explicit validation to undocumented clamping.

Consequence: coordinate shader outputs/built-ins, projection, sample-center
coverage, and viewport bounds. Preserve the front-facing and fragment/depth/stencil
rules. The old disk footprint changes; add compatibility only for a demonstrated
caller requirement.

Validation: sizes around pixel boundaries, fractional centers, sprite-coordinate
orientation, viewport edges, clipped centers, invalid sizes, and point attributes.

Reference: [Vulkan point rasterization](https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-points).

## Line coverage

Current lines include both floored projected endpoints. Vulkan has multiple line
rasterization modes, so compatibility needs an explicitly selected rule.

Recommended direction: select width-one Bresenham line rasterization using the
specified diamond-exit convention, including its endpoint handling. Retain line
loops as assembly convenience and defer width/stipple controls.

Consequence: connected lines may change coverage from the current inclusive-endpoint
rule. Document deterministic boundary choices and interpolation within the selected
rule; do not claim current coverage already matches it.

Validation: horizontal, vertical and diagonal segments, fractional endpoints,
connected strips/loops, reversed traversal, clipping, zero-length segments, and
interactions with blending and stencil at shared endpoints.

Reference: [Vulkan Bresenham line rasterization](https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-lines-bresenham).

## Fragment-test ordering

Current covered samples run the fragment shader before stencil and depth tests.
Discard preserves all attachments. Vulkan distinguishes ordinary fragment
operations from explicitly requested early-test behavior; always running late
does not by itself demonstrate incorrect attachment results.

Recommended direction: retain the current observable ordering as the baseline.
Review early rejection only when measurements justify it and its eligible cases
preserve the contract, or when an explicit contract change is accepted.

Consequence: account for shader errors, discard, stencil fail/depth-fail/pass
updates, and profiling counters, not just final color/depth. Derivative helpers
must not write attachments; the derivative and MSAA designs must define their
interaction with tests before execution is reordered.

Validation: discarded and failing shaders behind occluders, all stencil outcomes,
absent color, disabled writes, and profiling-enabled/disabled equivalence.

Reference: [Vulkan fragment operations](https://docs.vulkan.org/spec/latest/chapters/fragops.html).

## Snapped-polygon guarantees

Current triangles are clipped, projected, and snapped to a 1/256-pixel grid.
The renderer supports nonzero-winding coverage, deterministic interpolation and
facing on degenerate or self-crossing snapped boundaries, and at most one shaded
sample per original triangle. Raster fixtures exercise these guarantees.

Recommended direction: retain this contract during feature work. Simplification
is not a prerequisite. Before proposing triangle-oriented coverage, specify
shared-edge ownership, snapping/clipping order, degenerate-boundary behavior,
facing, interpolation on overlapping generated triangles, and preservation of
the original provoking vertex. A clipped-polygon triangle fan is not an
equivalent replacement.

Consequence: any replacement first needs a small reference implementation compared
with the existing fixture corpus. Classify every changed sample and attribute as
an intended semantic change or defect before deleting boundary classification,
ear clipping, rational scans, or payload ordering.

Validation: shared boundaries under reversed traversal, all clip planes, coincident
vertices with distinct payloads, self-crossing and degenerate boundaries, extreme
coordinates, all interpolation modes, no duplicate shading, culling and stencil
faces. Record accepted differences explicitly and update public documentation
and fixtures together. Measure complexity and performance after agreement on
those differences.
