# `m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer`

## Purpose

Provide a headless CPU rendering pipeline and own the renderer-facing framebuffer, camera, mesh, geometry, material, and render-item model consumed by that pipeline.

Backend-independent shader construction and reflection belong to `m03gsy25j4v7nccgmsdov9ioft_shader`; individual CPU shader invocation belongs to `m03gt1djvvy5atia5evkbg6rqy_software_shader`; CPU texture storage and sampling belong to `m03gt0l0q3l4b1k27eab5k7py1_texture`.

The application owns scene organization, framebuffer allocation and resizing,
frame sequencing, windowing, and presentation. Points, lines, and triangles are
distinct rasterized primitive classes; strip, loop, and fan topologies define
assembly within those classes.

## Invariants

- Render items own object placement; cameras own pose, projection, and the
  destination rectangle. The renderer consumes these through their public
  interfaces. See [render items](render_item.h), [cameras](camera.h), and
  [drawing and clearing](software_renderer.h) for caller contracts.
- Quaternion mathematics and rotation conversions belong to
  [`m03gtgtrh2smvh28qlwgm7gdl4_quaternion`](../../ws1/m03gtgtrh2smvh28qlwgm7gdl4_quaternion/AGENTS.md).
  Render items and cameras enforce normalized rotation storage through their
  setters. Euler values are inputs only.
- Planar scenes use the same 3D model with an orthographic camera and explicit
  placement. Maintain one camera model and one public drawing API.

## Validation

Deterministic CPU pipeline behavior is validated headlessly within this module. Presentation and the tower-defense scene are integration checks owned by their respective consumers.

## Intended direction

Extend the existing 3D CPU rasterizer and planar rendering with depth/stencil
testing, face culling, blending, and mipmapped texture sampling.

Shader construction, shader execution, texture storage/sampling, and
application-owned scene organization and presentation retain their
existing ownership boundaries.

These are target capabilities, not claims of current implementation.

See the [feature milestones](docs/milestones.md) for scope and completion evidence.

## Open decisions

- Ownership of draw state and color/depth/stencil attachments.
- Color-space and alpha conventions; fragment discard and attachment-write
  ordering.
- Completion scope: interpolation modes, instancing, multiple color
  targets, multisampling, and advanced texture sampling.
