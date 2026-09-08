# Rasterization contract decisions

These three decisions remain open. The current contract in
[software_renderer.h](../software_renderer.h) remains authoritative until a
replacement is reviewed and implemented. This document describes proposed work,
not additional current guarantees.

## Depth convention

Current behavior clips Z to [-W,W]; camera near/far map to NDC -1/+1 and
fragment depth maps through (Z/W+1)/2. Vulkan defaults to [0,W].

Recommended direction: adopt [0,W] consistently if Vulkan's default convention
is the intended public contract. Decide whether existing callers need a migration
period or an explicitly selected legacy convention. Avoid an additional mode
without a concrete compatibility requirement.

Implementation must change perspective and orthographic projection, all depth
clip planes, camera world-to-framebuffer results, and fragment depth mapping
together. Update public headers, examples, fixtures, and direct consumers,
including callers that supply clip coordinates directly. Preserve X/Y viewport
mapping and the independent depth comparison/write rules.

Validation: near/far boundaries, just-outside positions, varying positive W,
clipped lines and triangles, perspective/noperspective interpolation, equal-depth
comparison, clears, stencil/depth ordering, and render-to-texture consumers.

Reference: [Vulkan primitive clipping](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-clipping).

## Point coverage

Current behavior is a fixed disk around the floored projected position, covering
integer offsets dx*dx+dy*dy <= 9. Vulkan uses square point coverage and point size.

Recommended direction: review square coverage when configurable points have a
concrete use case. Decide the owner and source of point size (draw state or shader
output), default size, supported range, invalid-value behavior, clipping, and
whether point sprite coordinates are in scope. These choices affect the shader
interface as well as the renderer when point size is shader-controlled.

Implementation must coordinate point-size production, projection, sample-center
coverage, viewport bounds, and any point-coordinate built-in. Preserve the
front-facing and fragment/depth/stencil behavior unless explicitly changed.

Validation: sizes around pixel boundaries, fractional centers, viewport edges,
clipped centers, non-finite/invalid sizes, and interpolation of point attributes.

Reference: [Vulkan point rasterization](https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-points).

## Snapped-polygon guarantees

Current triangles are clipped, projected, and snapped to a 1/256-pixel grid.
The renderer supports nonzero-winding coverage, deterministic interpolation and
facing on degenerate or self-crossing snapped boundaries, and at most one shaded
sample per original triangle. Raster fixtures exercise these guarantees.

Recommended direction: retain this contract during structural cleanup. Before
replacing it with triangle-oriented coverage, specify shared-edge ownership,
snapping and clipping order, degenerate-boundary behavior, facing, interpolation
on overlapping generated triangles, and preservation of the original provoking
vertex. A clipped-polygon triangle fan is not an equivalent replacement.

First build a small reference implementation of the proposed contract and compare
it with the existing fixture corpus. Classify every changed sample and attribute
as an intended semantic change or an implementation defect before deleting the
boundary classification, ear clipping, rational scans, or payload ordering.

Validation: shared boundaries under reversed traversal, all clip planes, coincident
vertices with distinct payloads, self-crossing and degenerate boundaries, extreme
coordinates, all interpolation modes, no duplicate shading, culling and stencil
faces. Record accepted differences explicitly and update public documentation
and fixtures together. Measure complexity and performance only after agreement
on those differences.
