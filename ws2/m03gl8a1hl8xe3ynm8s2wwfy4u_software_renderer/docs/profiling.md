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

## Headless benchmark

From Builder-Layout:

```sh
./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark \
    --output "$PWD/artifacts/renderer-runs/run-001" \
    --size 128 --warmup 3 --samples 20 --runs 5
```

Use a new absolute output directory outside installed build artifacts.
[benchmark.cpp](../benchmark.cpp) defines workloads and timing boundaries. Samples
include clears and draws; two-pass samples include mask drawing and composition,
and `mipmapped_two_pass` includes mip generation. Setup, result comparisons, and
reporting are outside frame timing. Each profiling-enabled/disabled pair must
produce identical color, depth, stencil, and texture levels. Stage reports include
warm-up; sample summaries exclude it.

Use matching compiler options for performance-relevant dependencies when comparing
changes. Builder's default build is not optimized; the benchmark's compiler macros
describe only its own translation unit. Keep the actual build command with local
results. A suitable compilation command for each renderer/shader dependency source is:

```sh
g++ -std=c++23 -O2 -g -Wall -Wextra "${include_flags[@]}" -c source.cpp -o source.o
```

Apply the same options to shader, software_shader, texture, software_renderer,
profiling, byte_stream, structure_of_arrays, and type_erased_array. Link their objects
with the existing platform dependencies. Run correctness suites with the same build.
