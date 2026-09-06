# Renderer profiling and benchmark

`software_renderer_t<profiler_type_t>` borrows the application's profiler. The default
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
measurement executable. `cli/benchmark.cpp` is excluded from the default library
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
worker writes a JSON result and a hierarchical text report. The coordinator writes
`metadata.json` before the workers run and `results.json` after all workers succeed.
A failed run retains its completed workload files and does not produce a complete
aggregate result.

Each workload result includes every timing pair, median, nearest-rank p95, maximum,
per-run medians, the percentage difference between policy medians, peak RSS, and
retained record count. No outliers are removed. Build metadata records the actual
versioned executable, loaded file paths, benchmark compiler version, and whether
optimization and assertions were enabled in the benchmark translation unit.
Dependency compile options are explicitly unknown in this runtime metadata;
preserve Builder build logs and referenced versioned artifacts when comparing runs.
No source scanning or alternative build system runs inside the benchmark.

Each sample measures an application frame scope, complete color/depth clears,
and a fixed draw sequence. Setup, capture reset, correctness comparisons, and
reporting are outside that interval. Both policies receive warm-up before each
run; their execution order alternates. Every pair is checked for identical color
and depth output and complete capture.

Peak RSS uses Linux `/proc/self/status` `VmHWM` through workload capture and the
text report, including setup, warm-up, and both policies. Summary construction
and JSON serialization follow the measurement. This is the process high-water
mark, not per-draw memory or a difference between policies. Elapsed render durations
include scheduling delays and are not process CPU-time counters.

Use consistent workload and build settings for comparisons. The old baseline
used GNU `-O2 -DNDEBUG` and direct object linking; the Builder target uses its
configured compiler and shared dependency libraries. These are different build
conditions, so a migration measurement does not establish a renderer speedup or
regression against that baseline.

## Report policy construction

`profiler_t profiler(storage)` default-constructs its report policy. To supply a
stateful or non-default-constructible policy, use `profiler_t profiler(storage,
report)` with the chosen concrete policy type in the profiler template. The
storage-only overload is available only for default-constructible report policies.
Both overloads retain the same storage, metadata, and deferred-reporting contract.

The [migration validation record](profiling-migration.md) lists the commands and
obtained evidence for the Builder target and constructor changes.
