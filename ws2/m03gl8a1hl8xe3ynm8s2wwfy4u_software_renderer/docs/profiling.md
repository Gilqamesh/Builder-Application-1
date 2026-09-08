# Renderer profiling and benchmark

Applications pass a parent metric to drawing and clearing. Each operation creates
its child metric; drawing measures preparation, vertex execution, and rasterization.
[profiling_metrics.h](../profiling_metrics.h) defines counters. The
[profiler API](../../../ws1/m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h) defines metric
lifetimes, enablement, accumulated timing, and reporting.

```cpp
namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;

// frame_metrics_t and its std::formatter belong to the application.
renderer::software_renderer_t software_renderer(framebuffer);
profiling::profiler_t profiler;
{
    auto frame_metric = profiler.metric<frame_metrics_t>();
    software_renderer.clear_color({0, 0, 0, 255}, frame_metric);
    software_renderer.clear_depth(1, frame_metric);
    software_renderer.draw(camera, item, frame_metric);
    frame_metric.update<frame_metrics_t>([](auto& metrics) noexcept {
        ++metrics.m_draws;
    });
}
profiler.report(std::cout);

// An inactive parent records nothing; rendering still executes.
profiling::metric_t inactive_metric;
software_renderer.draw(camera, item, inactive_metric);
```

## Feature counters and pass attribution

Draw data counts empty camera intersections and successfully prepared draws by
original topology. Preparation reports reflected resource entries, interpolation
components, and selected color paths. Rasterization reports submitted primitives,
clipping intersections/rejections, snapped degeneracy, facing/culling, and coverage
algorithm work alongside fragment outcomes. The exact accumulation and exception
rules are defined in [profiling_metrics.h](../profiling_metrics.h).

Vertex and raster counters accumulate in local storage and publish once per stage,
including exception exits. Inactive stages skip counter initialization and updates.
Rectangular clears publish one total after filling. No profiler calls or clock reads
occur per vertex, primitive, fragment, or scanline. Conditional local scalar increments
record hot-loop outcomes; coverage work is counted per triangle or winding scanline/span.
Rendering always executes outside metric-update callbacks.

The benchmark groups drawing under `application.mask`, `application.scene`, and
`application.composition`, with `application.mip_generation` measuring regenerated
levels separately. The interactive example also measures event polling, resize,
idle waiting, and the CPU presentation call. It stops frame measurement before
terminal output. Presentation timing measures the call, including any waiting;
it does not measure GPU execution separately.

Applications supply these parents using distinct metric types, each with its own
formatter. The same renderer metric type has independent data beneath each parent:

```cpp
// scene_metrics_t and composition_metrics_t are application-defined metric types.
{
    auto frame = profiler.metric<frame_metrics_t>();
    {
        auto scene = frame.metric<scene_metrics_t>();
        renderer.draw(camera, scene_item, scene);
    }
    {
        auto composition = frame.metric<composition_metrics_t>();
        renderer.draw(camera, composition_item, composition);
    }
}
```

Counters expose features without isolating their execution time. Clipping,
interpolation, shading, sampling, and attachment processing remain included in
rasterization. Candidate counts represent planned visits and may overlap; they are
not unique pixels. First-use storage allocation and warm steady-state performance
should be evaluated separately. Compare disabled absolute timings before/after
instrumentation as well as enabled/disabled differences, since both configurations
execute the same instrumented binary.

## Headless benchmark

From Builder-Layout:

```sh
./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark \
    --output "$PWD/artifacts/renderer-runs/run-001" \
    --size 128 --warmup 3 --samples 20 --runs 5 --report
```

Use a new absolute output directory outside installed build artifacts.
All workloads run sequentially and write one `results.json`; `--report` adds a text
stage report for each workload. Schema version 7 includes raw timing pairs,
summaries, profiling overhead, metric-node counts, first-frame timings, scene vertex
counts, and retained vertex-storage capacities. The first-frame pair includes initial
storage growth; it is also part of warm-up (or the sample set when warm-up is zero).
Scene counts include every measured frame, including warm-up. Capacity fields are
per-field maxima in bytes, measured after successful vertex stages; they exclude
other renderer storage and allocator overhead. Maxima from different draws need
not occur simultaneously.

Workload version 6 retains the seventeen existing workloads and adds
`indexed_varyings`, `unshared_varyings`, and `sparse_varyings`. All three render the
same grid with perspective, noperspective, and flat inputs consumed by the fragment
shader. The unshared mesh expands every index occurrence into a separate vertex.
The sparse mesh selects 1,089 vertices at the end of a 1,048,576-vertex allocation.
Compare these workloads to measure reuse, payload storage, and the dense lookup's
cost when a draw selects a small part of a large mesh.
Metric-node counts must remain stable after the first frame; disabled runs store no nodes.
Peak RSS is not measured.
[benchmark.cpp](../benchmark.cpp) defines workloads and timing boundaries. Samples
include clears and draws; two-pass samples include mask drawing and composition,
and `mipmapped_two_pass` includes mip generation. Setup, result comparisons, and
reporting are outside frame timing. Each profiling-enabled/disabled pair must
produce identical color, depth, stencil, and texture levels. Each workload also
writes `<workload>.attachments` outside timing: tightly packed final color bytes,
then float depth bytes, stencil bytes, and any offscreen texture levels in increasing
level order. These native-format files can be compared byte-for-byte between builds
on the same platform with identical workloads and dimensions. Compare the three
varying-grid files with one another as an additional equivalence check; matching
profiling-enabled/disabled execution alone cannot detect a shared rendering defect.
Stage reports include warm-up; sample summaries exclude it.

## Reproducible optimized build

From Builder-Layout:

```sh
BUILDER_BUILD_MODE=optimized ./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark \
    --output "$PWD/artifacts/renderer-runs/optimized-001" \
    --size 128 --warmup 3 --samples 20 --runs 5 --report
```

Builder owns compiler selection, options, dependency discovery, library validation,
and linking. Its optimized mode applies `-O2 -g` to C and C++ sources throughout
that build, including dependency libraries and public API validation. The toolchain
uses C++23. Unset or `debug` mode retains the default `-g` compilation; unsupported
mode names fail. Configuration affects compiled artifact keys, so switching back
can reuse the earlier mode's completed artifacts.

Keep compiler output alongside benchmark results when comparing builds. Use
identical compiler configurations, workload arguments, and machine conditions.
Give separate workspaces separate `BUILDER_ARTIFACT_ROOT` directories. Builder's
`docs/repository-model.md` owns configuration and
artifact semantics; the renderer adds no independent dependency list or flags.
Full-frame timings include preparation and resource resolution; shader invocation
improvements alone do not establish a full-frame speedup.

## Vertex result reuse

The renderer reuses completed vertex results within each draw. `expected` continues
to count selected index occurrences; `vertex_invocations` counts actual attempted
shader calls and `reuses` counts completed cache hits. The drawing contract permits
reuse without fixing invocation frequency or order. The current implementation
walks distinct source indices in first-occurrence order and preserves the original
index sequence for assembly. The 32×32 indexed grid therefore reports 6,144
occurrences, 1,089 invocations, and 5,055 reuses per successful draw.

The lookup and touched-index list retain their capacities between draws but reset
all entries at draw completion, including exceptions. Result records reserve at
most `min(vertex_count, index_count)` entries on each reservation request; existing
larger capacities are retained. Payload vectors grow only for executed vertices.
Capacity metrics include the lookup, touched indices, result records, interpolated
payloads, and flat payloads. The dense lookup costs one `std::size_t` per mesh vertex
at its retained high-water capacity, even for a sparse selection; the benchmark
makes this memory and first-use cost visible alongside steady-state timing.
