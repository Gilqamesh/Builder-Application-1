# `m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer`

## Purpose

Own the headless CPU pipeline and its framebuffer, camera, mesh, geometry,
material, and render-item model. Applications own scene organization, attachment
allocation, resizing, pass sequencing, windowing, and presentation.

Shader construction/reflection belongs to
[`shader`](../m03gsy25j4v7nccgmsdov9ioft_shader/AGENTS.md), individual CPU invocations
to [`software_shader`](../m03gt1djvvy5atia5evkbg6rqy_software_shader/AGENTS.md), and
texture storage/sampling to
[`texture`](../../ws1/m03gt0l0q3l4b1k27eab5k7py1_texture/AGENTS.md).

## Boundaries

- [Materials](material.h) own shared draw state and sampled resources;
  [framebuffers](framebuffer.h) borrow attachments. Clearing is explicit.
- [Render items](render_item.h) own placement; [cameras](camera.h) own pose,
  projection, and destination rectangles. Planar scenes use this same model.
- Quaternion mathematics belongs to
  [`quaternion`](../../ws1/m03gtgtrh2smvh28qlwgm7gdl4_quaternion/AGENTS.md).
- Shader inputs declare interpolation. Primitive assembly, provoking-vertex
  selection, clipping, coverage, and interpolation execution belong here.
  [software_renderer.h](software_renderer.h) owns the drawing contract.
- Renderer metrics, counter meanings, and stage boundaries belong here;
  [`profiling`](../../ws1/m03gtjqkhqacstl3luv2ojsz3q_profiling/AGENTS.md) owns timing,
  storage, and reporting. Rendering executes outside metric update callbacks.

## Validation and direction

Validate deterministic pipeline behavior headlessly in this module. Presentation
and application scenes are consumer integration checks. Outstanding feature
choices are recorded in [milestones.md](docs/milestones.md).
