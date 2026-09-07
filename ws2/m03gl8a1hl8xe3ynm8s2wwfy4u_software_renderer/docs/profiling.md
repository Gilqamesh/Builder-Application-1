# Renderer profiling and benchmark

Each ordinary `software_renderer_t` owns a profiler that starts disabled.
Applications use `renderer.profiler().enabled() = true` to enable recording and
assign `false` to disable it. Rendering and clearing execute in either state.
Enablement is sampled when each metric is created; existing active metrics finish
normally after a change, and previously completed results remain available.

## Use

Producers own their metric data types, constructors, counter meanings, and
`std::formatter` specializations. Applications measure their own work using the
same profiler. See [the complete headless caller](../benchmark.cpp).

```cpp
namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;

// frame_metrics_t and its std::formatter are application-owned.
renderer::software_renderer_t software_renderer(framebuffer);
auto& profiler = software_renderer.profiler();
profiler.enabled() = true;

auto metric = profiler.metric<frame_metrics_t>();
software_renderer.clear_color({0, 0, 0, 255});
software_renderer.clear_depth(1);
software_renderer.draw(camera, item);
metric.update([](frame_metrics_t& metric) noexcept {
    metric.m_draws = 1;
});
metric.stop();

profiler.report(std::cout);

profiler.enabled() = false;
software_renderer.draw(camera, item); // Rendering still executes; saved results remain.
```

The five usual operations are `enabled()`, `metric<T>()`, `update()`, `stop()`,
and `report()`. Applications borrow the renderer's profiler for at most the
renderer's lifetime; other consumers may own or borrow a profiler independently.

## Data and timing

The profiler owns one data object per measured type. Starting a measurement
reconstructs that object, then starts its clock. `update(function)` invokes the
callable immediately with a borrowed reference to that data and propagates any
exception. Application work stays outside the callback. `stop()` ends timing;
destruction calls it automatically. Later updates and stops do nothing.

Default metrics and metrics created while disabled do nothing. They skip lookup,
allocation, data construction, and clock reads; `update()` skips its callable.
Arguments and lambda captures still evaluate before calls. Put expensive
metric-only computations inside the callback. A returned metric holds one pointer;
disabled use still has runtime checks.

Different metric types can overlap. Starting an already-active type is rejected.
Repeating a stopped type replaces its application data and adds another timing
observation. Reads and reports require all metrics stopped. Borrowed result
pointers expire at their type's next start or profiler destruction.
[The profiling public contract](../../../ws1/m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h)
owns storage, lifetime, type requirements, and failure guarantees.

[profiling_metrics.h](../profiling_metrics.h) owns renderer counters and their
formatters. Color/depth clears retain their latest write counts separately.
Vertex metrics count entered calls and expected selected indices. Raster metrics
count fragment invocations, discards, depth rejections, and color/depth writes.
Discard and depth-rejection percentages use fragment invocations as the denominator;
zero invocations reports `n/a`. Draw and preparation metrics contain only timing.

Durations include nested work, such as shaders inside draw. There is no retained
parent/child relationship. Exception exits retain partial counters and mark the
latest completion `unwinding`. Lookup and data construction/destruction precede
timing; stopping reads the clock before updating statistics. First-use allocation
for a nested type can contribute to an enclosing measurement's duration.

## Reporting and readback

Reports separate **latest application data** from **timing since profiler
construction**, including exception exits. Each type has count, last duration,
mean, maximum, total, and age since last completion. Disabling and reporting
preserve these observations. A stage skipped by a later draw retains its previous
data; age helps identify older measurements.

```text
Recording: disabled
Data: latest measurement; timing: since profiler construction; inclusive durations
renderer.draw
  count=120 last=6.66 ms mean=6.4 ms max=12.1 ms total=768 ms age=30 ms
```

Types appear in descending total duration, with registration order breaking ties.
Durations scale automatically from nanoseconds to exaseconds. Inclusive durations
can overlap, so adding them does not give elapsed frame time or CPU utilization.
There is no sample history, application-data accumulation callback, or reset.

```cpp
if (const auto* vertex_metrics = profiler.metrics<renderer::vertex_metrics_t>()) {
    std::cout << vertex_metrics->m_invocations << '\n';
    std::cout << profiler.elapsed<renderer::vertex_metrics_t>()->count() << '\n';
}
```

`metrics<T>()` returns the latest completed data or null. `elapsed<T>()` and
`unwinding<T>()` return optional observations, and `size()` counts stored types.

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
worker writes a JSON result and a text report with latest data and timing statistics.
The coordinator writes `metadata.json` before the workers run and `results.json`
after all workers succeed.
A failed run retains its completed workload files and does not produce a complete
aggregate result.

Result schema version 3 reports the number of stored metric types as `metrics`.
Each workload result includes every timing pair, median, nearest-rank p95, maximum,
per-run medians, the percentage difference between enabled/disabled medians, peak RSS, and
stored metric type count (`metrics`). No outliers are removed. Build metadata records the actual
versioned executable, loaded file paths, benchmark compiler version, and whether
optimization and assertions were enabled in the benchmark translation unit.
Dependency compile options are explicitly unknown in this runtime metadata;
preserve Builder build logs and referenced versioned artifacts when comparing runs.
No source scanning or alternative build system runs inside the benchmark.

Each sample measures an application frame, complete color/depth clears,
and a fixed draw sequence. Setup, correctness comparisons, and reporting are outside
that interval. The profiler persists across samples. The final report contains
the latest frame and individual draw counters, with timing statistics across all enabled
measurements, including warm-up. These totals cover more observations than the
sample-only benchmark summaries. Metadata records that distinction. Both
configurations receive warm-up before each run; their execution order alternates.
Every pair is checked for identical color and depth output.

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
