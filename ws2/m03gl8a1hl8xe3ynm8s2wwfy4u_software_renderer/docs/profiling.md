# Renderer profiling and benchmark

The ordinary `software_renderer_t` starts without an attached profiler.
Applications attach a borrowed profiler with `renderer.profiler(profiler)` and
can detach with `renderer.profiler(nullptr)`. Rendering and clearing execute in
either state. Attachment changes require both profilers to have no active metrics.

## Use

Producers own their metric data types, constructors, counter meanings, and
`std::formatter` specializations. Applications measure their own work using the
same profiler. See [the complete headless caller](../benchmark.cpp).

```cpp
namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;

// frame_metrics_t and its std::formatter are application-owned.
profiling::profiler_t profiler;
renderer::software_renderer_t software_renderer(framebuffer);
software_renderer.profiler(profiler);

auto metric = profiler.metric<frame_metrics_t>();
software_renderer.clear_color({0, 0, 0, 255});
software_renderer.clear_depth(1);
software_renderer.draw(camera, item);
metric->m_draws = 1;
metric.stop();

profiler.report(std::cout);
```

The profiler owns its growing storage. Each active metric holds its current data
and starting timestamp; stopping replaces that type's stored data and duration.
Repeating the example keeps one metric per type. Reporting again without measuring
a type again preserves its previous observation. The profiler outlives attached
uses and active metrics. It is ready immediately after construction and requires
no start or reset operation.

## Metrics and timing

`metric<T>()` returns a `profiling::metric_t<T>`. Destruction stops it automatically;
explicit `stop()` ends the timing interval earlier and is idempotent. After stopping,
the metric is inactive and its application data can no longer be mutated through it.
Default metrics are inactive and construct no data or clock observation. Renderer
operations use them when no profiler is attached; rendering always executes.

[profiling_metrics.h](../profiling_metrics.h) owns the renderer metric data and
formatters. Color and depth clears have separate types, preserving the latest
observation of each even for empty clear regions. Draw and preparation metrics
have empty data. Vertex metrics retain selected-index count as `m_expected` and
calls entered as `m_invocations`. Raster metrics retain fragment invocations,
discards, depth rejections, and actual color/depth writes for the latest raster
measurement. Repeated draws replace those stage metrics independently.

Lookup and application-data construction precede the starting timestamp. `stop()`
reads the ending timestamp before replacement. Durations are inclusive monotonic
elapsed nanoseconds, including shader execution within the vertex and raster
measurements. Timing boundaries are independent; the profiler preserves no
parent/child relationships. For overlapping uses of the same type, the last
completion wins. Exception unwinding replaces the metric with partial counters
and an unwinding flag. Allocation during first use of a nested type can contribute
to an enclosing measurement's time.

Storage, lifetime, nonthrowing move requirements, and failure guarantees are owned
by [the profiling public contract](../../../ws1/m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h).
Formatting runs during reporting. Reads and reports require no active metrics.

## Reading metrics

```cpp
if (const auto* vertex_metrics = profiler.metrics<renderer::vertex_metrics_t>()) {
    std::cout << vertex_metrics->m_invocations << '\n';
    std::cout << profiler.elapsed<renderer::vertex_metrics_t>()->count() << '\n';
}
```

`metrics<T>()` returns the latest completed data or null. `elapsed<T>()` and
`unwinding<T>()` return optional observations, and `size()` counts stored types.
Borrowed pointers expire on replacement of their type or profiler destruction.
The ordinary `report(std::ostream&)` formats each metric in first-registration
order. A stage not entered during the most recent draw still describes the last
draw that entered it.

## Builder benchmark

From `Builder-Layout` on Linux:

```sh
./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark \
    --output "$PWD/artifacts/renderer-runs/run-001" \
    --size 128 --warmup 3 --samples 20 --runs 5
```

The output must name a new absolute directory. Builder launches binaries from
an installed directory, so an absolute output avoids ambiguity and keeps generated
runs outside completed build installations. Use a different directory for each
run. `--help` prints the options; omitting the numeric options uses the values above.

The module registers `cli` for the existing demo and `benchmark` for the headless
measurement executable. `benchmark.cpp` is excluded from the default library
sources. Builder owns source publication, dependency discovery, compilation,
linking, public validation, and versioned artifacts. The C++ benchmark owns workload
execution, measurement, summaries, and reports.

The benchmark uses Builder's current compilation settings. The current default
build supplies C++23 and `-g`, without an optimization option or `NDEBUG`. Registering
a target does not optimize its dependency libraries. General optimization support
and a new optimized baseline are deferred. The [historical optimized baseline](profiling-baseline.md)
remains available with its [unchanged raw data](profiling-baseline.json).

The coordinator starts a fresh copy of its installed binary for each of the four
workloads: textured fill, depth overdraw, many small draws, and clipping. Each
worker writes a JSON result and a flat text report of the latest completed metric of each type. The coordinator writes
`metadata.json` before the workers run and `results.json` after all workers succeed.
A failed run retains its completed workload files and does not produce a complete
aggregate result.

Result schema version 3 reports the number of stored metric types as `metrics`.
Each workload result includes every timing pair, median, nearest-rank p95, maximum,
per-run medians, the percentage difference between attachment-configuration medians, peak RSS, and
stored metric type count (`metrics`). No outliers are removed. Build metadata records the actual
versioned executable, loaded file paths, benchmark compiler version, and whether
optimization and assertions were enabled in the benchmark translation unit.
Dependency compile options are explicitly unknown in this runtime metadata;
preserve Builder build logs and referenced versioned artifacts when comparing runs.
No source scanning or alternative build system runs inside the benchmark.

Each sample measures an application frame, complete color/depth clears,
and a fixed draw sequence. Setup, correctness comparisons, and reporting are outside
that interval. The profiler persists across samples; each completion replaces the
previous metric of its type. The final report contains the latest frame and the
latest individual draw stages, rather than sums across draws or samples. Both configurations receive warm-up before each
run; their execution order alternates. Every pair is checked for identical color
and depth output.

Peak RSS uses Linux `/proc/self/status` `VmHWM` through workload measurement and the
text report, including setup, warm-up, and both configurations. Summary construction
and JSON serialization follow the measurement. This is the process high-water
mark, not per-draw memory or a difference between configurations. Elapsed render durations
include scheduling delays and are not process CPU-time counters.

Use consistent workload and build settings for comparisons. The old baseline
used GNU `-O2 -DNDEBUG` and direct object linking; the Builder target uses its
configured compiler and shared dependency libraries. These are different build
conditions, so a migration measurement does not establish a renderer speedup or
regression against that baseline.
