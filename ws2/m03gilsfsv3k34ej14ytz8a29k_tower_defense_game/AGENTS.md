# `m03gilsfsv3k34ej14ytz8a29k_tower_defense_game`

## Purpose

Implement the tower-defense application and exercise `m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer` as its active rendering path.

The software-renderer module owns the camera, mesh, geometry, material, render-item, and CPU-pipeline model. The application constructs and consumes those abstractions without redefining their semantics.

## Public model

Use the software-renderer module's `render_item_t` and related resource types directly. `renderer3_t`, the inactive `renderer_t` path, standalone application-local `opengl_*` wrappers, and remaining `entity` terminology are legacy or experimental evidence rather than co-equal architecture.

## Invariants

- Rendering resources are constructed according to the software-renderer public contract rather than application-local representations.
- The application owns its profiler and passes a frame metric through rendering
  to the software renderer.
- The application owns the window and CPU framebuffer allocation, gives borrowed framebuffer storage to the software renderer, and presents that storage through the OpenGL renderer.
- Shared ownership is used only where the software-renderer resource model shares lifetime.

## Validation

Rendering integration requires the software-renderer validation suite and a visible tower-defense smoke test with an OpenGL-capable display.
