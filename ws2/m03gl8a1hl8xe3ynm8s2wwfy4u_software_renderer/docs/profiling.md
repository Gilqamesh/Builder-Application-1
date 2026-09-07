# Renderer profiling and benchmark

Applications own a `profiling::profiler_t` and pass a borrowed `metric_t&` into
drawing and clearing. Both types are non-template types. The renderer opens its
operation metric beneath that parent, and draw opens its stage metrics beneath
itself. Rendering executes with active or inactive metrics.

## Use

Producers own metric data types, constructors, counter meanings, and
`std::formatter` specializations. See [the complete headless caller](../benchmark.cpp).

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

Inside draw, the renderer composes measurements directly:

```cpp
auto draw_metric = parent_metric.metric<draw_metrics_t>();
auto preparation_metric = draw_metric.metric<preparation_metrics_t>();
// Prepare.
preparation_metric.stop();
auto vertex_metric = draw_metric.metric<vertex_metrics_t>();
// Process vertices.
vertex_metric.stop();
auto raster_metric = draw_metric.metric<raster_metrics_t>();
// Rasterize.
```

The receiver determines the parent. Metrics must finish in reverse start order;
starting another sibling or stopping a parent while a child is active is rejected.
Destruction with active children terminates. Normal exception unwinding closes
children before parents. A stopped recorded metric cannot open more children.
`update<T>()` checks the stored data type, invokes the callback immediately, and
propagates its exceptions. Inactive and stopped metrics skip updates. Keep
application work outside callbacks; arguments and captures still evaluate normally.

## Data and timing

Each `(parent node, metric type)` owns one persistent data object and timing record.
Repeated calls on the same path reuse it. The same type beneath different parents
has independent counters and timings; recursive types create distinct deeper nodes.
Constructor arguments initialize data only on that path's first use. Updates
explicitly accumulate or replace data. New paths may allocate; reusing established
paths and valid stopping do not allocate, format, perform I/O, or lock.

The profiler starts enabled. Assign `profiler.enabled() = false` to disable
subsequent root measurements and `true` to resume. A root samples recording once;
all descendants inherit its decision. Disabled metrics and their descendants skip
lookup, allocation, construction, clock reads, and update callbacks.

[profiling_metrics.h](../profiling_metrics.h) owns renderer counters and formatters.
Counters accumulate per metric path, including partial work before exceptions.
Vertex counters count entered calls and expected selected indices. Raster counters
count fragment invocations, discards, stencil/depth rejections, and actual
color/depth/stencil writes. Stencil keep and a zero write mask count no writes;
other operations count assignments even when the stored byte is unchanged. Stencil
clears have a separate metric type. Color writes count once per sample when at least one channel is assigned,
even if bytes are unchanged; an all-disabled mask counts zero. Both rejection percentages use fragment invocations as the denominator;
zero invocations reports `n/a`. Draw and preparation contain timing only.

Durations include nested work. Lookup and first data construction precede that
node's timer, and stopping reads the clock before updating statistics. Nested
first-use allocation can contribute to an enclosing measurement's duration.
Exception exits retain updates and mark the latest completion `unwinding`.

## Reporting and readback

Reports distinguish current data per path from timing across all completions since
profiler construction. Nodes precede their children, with roots and siblings in
first-use order. Repeated calls aggregate: `A, B, A` reports A with count 2 followed
by B with count 1. The report is a summary tree, not an event history.

Each node occupies one line: a tree label, count, last/min/mean/max/total duration,
age since its last completion, and application data. Durations choose SI units
from nanoseconds through exaseconds. Columns expand to fit their contents;
long rows may wrap in the terminal. There is no prologue.

The first word produced by the metric's formatter labels the row; the remaining
text goes in `Data`. Line breaks and tabs become spaces. Formatters should use
single-column characters in labels for alignment. A latest completion during
exception unwinding appends `[unwinding]` to the data. Empty reports print
`No measurements.` or `Profiling disabled.` according to recording state.

Illustrative report for two draws per frame:

```text
Metric                        Count    Last     Min    Mean     Max   Total     Age  Data
application.frame                 1    1 ms    1 ms    1 ms    1 ms    1 ms    0 ns
├─ renderer.clear_color           1   10 us   10 us   10 us   10 us   10 us  980 us  color_writes=16
├─ renderer.clear_depth           1   10 us   10 us   10 us   10 us   10 us  960 us  depth_writes=16
└─ renderer.draw                  2  400 us  300 us  350 us  400 us  700 us   10 us
   ├─ renderer.preparation        2   20 us   10 us   15 us   20 us   30 us  380 us
   ├─ renderer.vertices           2   50 us   30 us   40 us   50 us   80 us  320 us  vertex_invocations=12, expected=12
   └─ renderer.rasterization      2  300 us  250 us  275 us  300 us  550 us   15 us  fragment_invocations=20, discards=0, stencil_rejections=0, stencil_writes=0, depth_rejections=0, color_writes=20, depth_writes=20, discarded=0.0%, depth_rejected=0.0%
```

Inclusive totals already contain child durations. A skipped stage retains its
previous data and timing; age identifies older measurements. Reporting and
recording changes preserve results, including when reporting fails.

Readback uses a complete root-to-leaf type path:

```cpp
if (const auto* metrics = profiler.metrics<frame_metrics_t, renderer::draw_metrics_t, renderer::vertex_metrics_t>()) {
    std::cout << metrics->m_invocations << '\n';
}
```

`metrics<T, Path...>()` returns the leaf data or null when absent. `elapsed` and
`unwinding` accept the same paths and return optional observations. `size()` counts
all stored nodes. Reads and reports require all metrics stopped. Borrowed data
pointers remain valid until profiler destruction; access requires an idle profiler.
[The profiling public contract](../../../ws1/m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h)
owns complete lifetime, construction, enablement, and failure guarantees.

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
measurement executable. Builder owns source publication, dependency discovery, compilation,
linking, public validation, and versioned artifacts. The C++ benchmark owns workload
execution, measurement, summaries, and reports.

The benchmark uses Builder's current compilation settings. The current default
build supplies C++23 and `-g`, without an optimization option or `NDEBUG`. Registering
a target does not optimize its dependency libraries. General optimization support
remains deferred. Separately captured [milestone 5](milestone-5-performance.md) and
[milestone 6](milestone-6-performance.md) comparisons use consistent optimization
across performance-relevant dependencies. The [historical optimized baseline](profiling-baseline.md)
remains available with its [unchanged raw data](profiling-baseline.json).

The coordinator starts a fresh copy of its installed binary for each workload.
Workload version 4 preserves the nine workloads from versions 1–3, including
`stencil_mask` and `two_pass_linear`/`two_pass_srgb`. It adds `flat_fill`,
`noperspective_fill`, `mipmapped_fill` and `mipmapped_two_pass`. The last regenerates
mips between the offscreen and sampled passes. The mask draw populates stencil
inside a smaller quad, then scene draws test that mask. Two-pass workloads render
four translucent layers into a texture-owned target and composite its premultiplied
result onto a separate output. Allocation, view validation, and bindings occur at
setup; timed frames include stencil clears/mask draws, target selection, both passes,
and output clearing. New feature workloads establish their own baselines. Each
worker writes a JSON result and a text report with current data and timing statistics.
The coordinator writes `metadata.json` before the workers run and `results.json`
after all workers succeed.
A failed run retains its completed workload files and does not produce a complete
aggregate result.

Result schema version 4 reports the number of stored metric nodes as `metric_nodes`.
Each workload result includes every timing pair, median, nearest-rank p95, maximum,
per-run medians, the percentage difference between enabled/disabled medians, peak RSS, and
stored metric node count (`metric_nodes`). No outliers are removed. Build metadata records the actual
versioned executable, loaded file paths, benchmark compiler version, and whether
optimization and assertions were enabled in the benchmark translation unit.
Dependency compile options are explicitly unknown in this runtime metadata;
preserve Builder build logs and referenced versioned artifacts when comparing runs.
No source scanning or alternative build system runs inside the benchmark.

Each sample measures an application frame, complete color/depth clears,
and a fixed draw sequence. Setup, correctness comparisons, and reporting are outside
that interval. The profiler persists across samples. The final report contains
cumulative frame and draw counters with timing statistics across all enabled
measurements, including warm-up. These totals cover more observations than the
sample-only benchmark summaries. Metadata records that distinction. Both
configurations receive warm-up before each run; their execution order alternates.
Every pair is checked for identical color, depth, stencil, and offscreen texture bytes
where present.

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

Milestone 3 has a [matching before/after comparison](blending-performance.md) with
[raw data](blending-performance.json), including the new translucent workloads.

Milestone 4 has [matching before/after measurements](milestone-4-performance.md)
and [raw evidence](milestone-4-performance.json) for stencil and direct two-pass rendering.
