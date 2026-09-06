# Renderer profiling and optimized baseline

`software_renderer_t<Profiler>` borrows the application's profiler. The default
`software_renderer_t<>` disables profiling. Both variants can coexist, with
`draw(camera, item)` retaining its rendering contract.

## Capture setup

Producers define their metric types and `std::formatter` specializations before
instantiating the profiler. An application can combine its own metrics with the
renderer's `clear_metrics_t`, `vertex_metrics_t`, and `raster_metrics_t` in one
variant. See [the complete headless caller](../cli/benchmark.cpp).

```cpp
namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;

// frame_metrics_t and its formatter are application-owned.
using payload_t = std::variant<frame_metrics_t,
    renderer::clear_metrics_t, renderer::vertex_metrics_t, renderer::raster_metrics_t>;
using profiler_t = profiling::profiler_t<payload_t>;
std::vector<profiler_t::record_type_t> storage(1024);
profiler_t profiler(storage);
renderer::regions_t regions(profiler); // Explicit producer registration.
auto frame_region = profiler.register_region("application.frame");
renderer::software_renderer_t<profiler_t> measured(framebuffer, profiler, regions);
profiler.start(); // Freezes all registered names, then starts capture.
{
    auto frame = profiler.scope<frame_metrics_t>(frame_region);
    measured.clear_color({0, 0, 0, 255});
    measured.clear_depth(1);
    measured.draw(camera, item);
    ++frame.metrics().m_draws;
}
profiler.report(std::cout); // Deferred formatting and output.
profiler.reset();          // Discards records, preserving region IDs and names.
```

The profiler copies region names during registration. Frozen metadata remains
owned by that profiler until destruction; registration after `start()` fails.
The caller's record storage must outlive the profiler, and both must remain valid
through reporting. Reads and resets require no active scopes. Scopes are confined
to one thread and close in reverse opening order, including during unwinding.

Draw and preparation scopes carry timing only. Vertex and rasterization scopes
accumulate local counters, with no clocks or reports per fragment. The producer
counter definitions are in [metrics.h](../metrics.h). Renderer algorithms and
individual shader execution remain unchanged. Disabled policies create no metric
payloads or records and perform no profiling clock reads or counter updates.

Timings include child scopes. Self time subtracts direct-child durations only
when all those timings were retained. Exhaustion omits the new scope and its
whole nested subtree; reports identify omissions. Inclusive timings still include
omitted work. Partial counters and an unwinding flag survive producer exceptions.

## Reproducible optimized build

From `Builder-Layout` on Linux:

```sh
python3 ws2/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/performance/run.py \
    --workspace-root "$PWD" \
    --output /tmp/renderer-profiling-baseline \
    --compiler /usr/bin/g++ --size 128 --warmup 3 --samples 20 --runs 5
```

This dedicated benchmark path snapshots the current headless C++ source closure
and compiles it with `-std=c++23 -O2 -g -DNDEBUG -Wall -Wextra`. It compiles the
renderer, `software_shader`, shader construction, texture storage and sampling,
and their supporting implementations. Header-defined matrix, vector, and
quaternion operations receive the same flags where instantiated. It links those
objects directly and does not use prebuilt Builder libraries. The normal Builder
build and configuration are unchanged.

The path supports this workload's ordinary module sources and literal includes;
it is not a replacement for Builder's module producers. `cli/benchmark.cpp` is
an executable source, excluded from the default module library by Builder's
existing `cli/` convention.

Outputs include:

- `compile_commands.json`, `build.log`, and a content-identified source snapshot;
- optimized profiler and full renderer validation executables;
- `disabled.s`, checked for calls to profiling clocks and recording;
- per-workload hierarchical reports and `results.json` with every timing sample,
  medians, nearest-rank p95, maxima, per-run medians, and profiling overhead;
- compiler, source digest, module revisions/modification state, workload settings,
  processor, and operating-system metadata.

Each sample measures an application frame scope, complete color/depth clears,
and a fixed draw sequence. Setup, capture reset, correctness comparisons, and
reporting are outside that interval. Both policies receive warm-up before each
run; their execution order alternates to reduce order bias. Every pair is checked
for identical color and depth output and complete capture. No outliers are removed.

Workloads cover textured fill, depth overdraw, many small draws, and clipping.
Each runs in a fresh process. Peak RSS uses Linux `/proc/self/status` `VmHWM`
for the benchmark address space, including setup, warm-up, reporting, and **both**
policies; it is not per-draw memory or a measurement of the difference between
policies. Elapsed render durations
include scheduling delays and are not process CPU-time counters.

Use the same settings and environment for subsequent deliveries. Preserve raw
results and source snapshots; interpret small differences in light of the
per-run variation. Algorithmic renderer optimization is subsequent work.

The [initial measured baseline](baseline.md) records obtained validation and
results; [baseline.json](baseline.json) preserves its raw observations.
