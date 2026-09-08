#include "profiling_metrics.h"

#include <algorithm>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

vertex_metrics_t::vertex_metrics_t(std::size_t expected) noexcept:
    expected(expected)
{
}

void vertex_metrics_t::accumulate(const vertex_metrics_t& counters) noexcept {
    expected += counters.expected;
    invocations += counters.invocations;
    reuses += counters.reuses;
    lookup_bytes = std::max(lookup_bytes, counters.lookup_bytes);
    touched_bytes = std::max(touched_bytes, counters.touched_bytes);
    result_bytes = std::max(result_bytes, counters.result_bytes);
    varying_bytes = std::max(varying_bytes, counters.varying_bytes);
    flat_bytes = std::max(flat_bytes, counters.flat_bytes);
}

void raster_metrics_t::accumulate(const raster_metrics_t& counters) noexcept {
    invocations += counters.invocations;
    discards += counters.discards;
    stencil_rejections += counters.stencil_rejections;
    stencil_writes += counters.stencil_writes;
    depth_rejections += counters.depth_rejections;
    color_writes += counters.color_writes;
    depth_writes += counters.depth_writes;
    points += counters.points;
    lines += counters.lines;
    triangles += counters.triangles;
    clipped_out += counters.clipped_out;
    clipping_intersections += counters.clipping_intersections;
    degenerate_triangles += counters.degenerate_triangles;
    front_triangles += counters.front_triangles;
    back_triangles += counters.back_triangles;
    culled_triangles += counters.culled_triangles;
    rasterized_polygons += counters.rasterized_polygons;
    triangulated_polygons += counters.triangulated_polygons;
    winding_polygons += counters.winding_polygons;
    generated_triangles += counters.generated_triangles;
    triangle_candidates += counters.triangle_candidates;
    winding_scanlines += counters.winding_scanlines;
    winding_events += counters.winding_events;
    winding_spans += counters.winding_spans;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
