# Renderer direction

Continue the general-purpose 3D CPU rasterizer, using the same camera and drawing
model for planar scenes. Select optimizations from current workload measurements.

## Outstanding scope decisions

Instancing, multiple color targets, multisampling, advanced texture sampling,
configurable point coverage, and line coverage changes require a concrete use case
and a reviewed contract before implementation.

Current drawing behavior is defined in [software_renderer.h](../software_renderer.h).
See [profiling](profiling.md) for the headless measurement command.

Depth convention, point coverage, and snapped-polygon contract changes are tracked
in [rasterization planning](rasterization-planning.md).
