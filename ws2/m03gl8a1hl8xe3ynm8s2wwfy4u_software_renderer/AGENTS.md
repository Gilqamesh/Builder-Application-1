# `m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer`

## Purpose

Provide a headless CPU rendering pipeline and own the renderer-facing framebuffer, camera, mesh, geometry, material, and render-item model consumed by that pipeline.

Backend-independent shader construction and reflection belong to `m03gsy25j4v7nccgmsdov9ioft_shader`; individual CPU shader invocation belongs to `m03gt1djvvy5atia5evkbg6rqy_software_shader`; CPU texture storage and sampling belong to `m03gt0l0q3l4b1k27eab5k7py1_texture`.

The application owns scene organization, framebuffer allocation and resizing,
frame sequencing, windowing, and presentation. Points, lines, and triangles are
distinct rasterized primitive classes; strip, loop, and fan topologies define
assembly within those classes.

## Invariants

- Materials own draw state alongside their program and bindings. Items sharing
  a material share its settings. Applications select materials, organize passes,
  and order draws; the renderer executes the selected material through
  `draw(camera, render_item, parent_metric)`. See [materials](material.h).
- Framebuffers borrow application-owned color and optional depth storage.
  Clearing is explicit and independent of material draw state. See
  [attachments](framebuffer.h) and [drawing and clearing](software_renderer.h).
- Materials own independent RGB/alpha blend equations, constants, and channel-write
  masks. Framebuffers own color-encoding metadata. Storage prescribes no alpha
  association; shader output and blend equations determine it. Texture sampling
  retains its own contract. See [materials](material.h), [attachments](framebuffer.h),
  and [fragment processing](software_renderer.h).
- Texture-owned render targets use the texture module's pixel views. Framebuffers
  borrow writable storage and materials retain sampled owners; applications sequence
  passes and maintain borrowed lifetimes. Feedback eligibility belongs to the
  renderer's [draw contract](software_renderer.h). See [render targets](docs/render-targets.md).
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

- Applications own profilers and pass a borrowed parent metric to drawing and
  clearing. Each operation creates its own child metric; draw stages create
  children beneath the draw metric. A default inactive metric disables recording.
  Renderer metrics, formatters, counter meanings, and stage boundaries remain
  owned here; tree storage, timing, and reporting belong to
  [`profiling`](../../ws1/m03gtjqkhqacstl3luv2ojsz3q_profiling/AGENTS.md).
  Counter updates run through metric.update<T>(); rendering executes outside
  those callbacks. Counters accumulate per metric path, including partial work
  before an exception. Color, depth and stencil clears have distinct counter types.

- Materials select first/last provoking vertices; first is the default. Selection
  follows original topology before clipping or winding adjustment. Shader inputs
  own interpolation modes; rasterization and supported varying types belong here.

## Validation

Deterministic CPU pipeline behavior is validated headlessly within this module. Presentation and the tower-defense scene are integration checks owned by their respective consumers.

## Intended direction

Continue the general-purpose 3D CPU rasterizer and planar rendering through
measured incremental optimization and the remaining feature milestones.

Shader construction, shader execution, texture storage/sampling, and
application-owned scene organization and presentation retain their
existing ownership boundaries.

These are target capabilities, not claims of current implementation.

See the [feature milestones](docs/milestones.md) for scope and completion evidence.

## Open decisions

- Completion scope: instancing, multiple color
  targets, multisampling, and advanced texture sampling.
