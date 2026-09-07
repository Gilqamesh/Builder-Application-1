# Renderer profiling and benchmark

The ordinary `software_renderer_t` starts with an unattached profiling context.
Applications attach a borrowed collector with `renderer.profiler(profiler)` and
can detach with `renderer.profiler(nullptr)`. Rendering and clearing retain their
existing behavior in either state. Attachment changes require both collectors to
have no active measurements; attachment itself may precede `start()`.

## Capture setup

Producers own their metrics payload types, constructors, counter meanings, and
`std::formatter` specializations. Applications add their own metrics to the same
collector without assembling a payload variant or registering regions. See
[the complete headless caller](../benchmark.cpp).

```cpp
namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;

// frame_metrics_t and its std::formatter are application-owned.
std::array<std::byte, 64 * 1024> storage;
profiling::profiler_t profiler(storage);
renderer::software_renderer_t software_renderer(framebuffer);
software_renderer.profiler(profiler);
profiler.start();
{
    auto metric = profiler.metric<frame_metrics_t>();
    software_renderer.clear_color({0, 0, 0, 255});
    software_renderer.clear_depth(1);
    software_renderer.draw(camera, item);
    if (metric) {
        ++metric->m_draws;
    }
}
profiler.report(std::cout);
profiler.reset(); // Destroys retained payloads; attachments remain valid.
```

The buffer size is this example's application choice, not a collector default.
The collector borrows byte storage and constructs heterogeneous payloads directly
in aligned retained storage. Storage outlives the collector, and the collector
outlives attached operations and active handles. Payloads never move. Measurement closure
retains them; reset and collector destruction destroy them in reverse opening order.
Borrowed data inside a payload remains valid through its deferred use in reporting.

`start()` begins the initial capture and may be called only once. Attached measurement
creation before start is a programming error. `reset()` requires a started,
quiescent collector and begins another capture. Capture is synchronous and confined
to one thread. Reading, reporting, resetting, and changing attachments require no
active measurements, including false handles that own overflow-suppression bookkeeping.

## Metrics and timing

`metric<T>()` returns a `profiling::metric_t<T>` handle for one measurement.
The handle closes its timing interval at scope exit or explicit `close()`; `T`
is the producer's metrics payload, retained after closure. Finalized observations
are read through `record_t`. See [the profiling public contract](../../../ws1/m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h).

Renderer stages distinguish handles by their measurement roles:

```cpp
auto draw_metric = m_context.metric<draw_metrics_t>();
auto preparation_metric = m_context.metric<preparation_metrics_t>();
```

[profiling_metrics.h](../profiling_metrics.h) owns the renderer metrics payloads
and formatters used by the profiling integration. Draw and
preparation have empty typed payloads. A clear payload explicitly identifies color
or depth, including when it writes no samples. Vertex metrics record selected-index
count as `m_expected` and calls entered as `m_invocations`. Raster metrics preserve
fragment invocation, discard, depth-rejection, and actual sample-write meanings.
Shader execution remains inside the vertex and rasterization stages.

Collection performs no profiler-owned allocation, formatting, I/O, or locking.
Payload construction from forwarded arguments and destruction must be nonthrowing;
producers also guarantee allocation-free payload operations and counter updates.
A usable formatter for the const payload is required at compilation, even for an
unattached context. Formatting runs only during reporting; formatting or output
errors propagate without discarding the retained capture.

An unattached context returns false handles without constructing payloads or
reading the clock. Caller argument expressions still evaluate before `metric()`.
Runtime pointer checks and conditional counter work replace compile-time policies.

Payload construction precedes measurement timing and record publication. Durations
are inclusive monotonic elapsed nanoseconds. Self time subtracts direct-child
elapsed time only when those timings are complete. Exception unwinding retains
partial counters and an unwinding flag. Explicit `close()` is idempotent and makes
a handle false; its destructor then does nothing. Payload mutation belongs to the
active handle, and retained reads use const views.

Exhaustion returns a false handle and suppresses its descendants, counting every
omission without reading a clock or constructing omitted payloads. A failed large
reservation consumes no storage, allowing a later sibling to fit. Reports mark the
capture incomplete and omit self time for parents missing direct-child timings.

## Reading retained records

`profiler.records()` returns a borrowed range in opening order. Each record exposes
its parent index, depth, start, inclusive duration, optional self time, and unwinding
status. Repeated and recursive uses of a type produce separate records.
`record.metrics<T>()` returns a const payload pointer or null for a different type.
For example:

```cpp
for (const auto record : profiler.records()) {
    if (const auto* vertex_metrics = record.metrics<renderer::vertex_metrics_t>()) {
        std::cout << vertex_metrics->m_invocations << '\n';
    }
}
```

Views expire on reset or collector destruction. Index lookup is linear; traversal
visits each record once. A range retains its original extent when later measurements
append. The collector's ordinary `report(std::ostream&)` uses standard payload
formatters; the former template report policy and clock policy are removed.

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
worker writes a JSON result and a hierarchical text report. The coordinator writes
`metadata.json` before the workers run and `results.json` after all workers succeed.
A failed run retains its completed workload files and does not produce a complete
aggregate result.

Each workload result includes every timing pair, median, nearest-rank p95, maximum,
per-run medians, the percentage difference between attachment-configuration medians, peak RSS, and
retained record count. No outliers are removed. Build metadata records the actual
versioned executable, loaded file paths, benchmark compiler version, and whether
optimization and assertions were enabled in the benchmark translation unit.
Dependency compile options are explicitly unknown in this runtime metadata;
preserve Builder build logs and referenced versioned artifacts when comparing runs.
No source scanning or alternative build system runs inside the benchmark.

Each sample measures an application frame scope, complete color/depth clears,
and a fixed draw sequence. Setup, capture reset, correctness comparisons, and
reporting are outside that interval. Both configurations receive warm-up before each
run; their execution order alternates. Every pair is checked for identical color
and depth output and complete capture.

Peak RSS uses Linux `/proc/self/status` `VmHWM` through workload capture and the
text report, including setup, warm-up, and both configurations. Summary construction
and JSON serialization follow the measurement. This is the process high-water
mark, not per-draw memory or a difference between configurations. Elapsed render durations
include scheduling delays and are not process CPU-time counters.

Use consistent workload and build settings for comparisons. The old baseline
used GNU `-O2 -DNDEBUG` and direct object linking; the Builder target uses its
configured compiler and shared dependency libraries. These are different build
conditions, so a migration measurement does not establish a renderer speedup or
regression against that baseline.
