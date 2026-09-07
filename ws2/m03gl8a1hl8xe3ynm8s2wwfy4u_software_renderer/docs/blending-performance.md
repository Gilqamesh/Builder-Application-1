# Milestone 3 blending measurements — 2026-09-07

The four original workloads compare the default linear replacement path before
and after milestone 3. The two new workloads measure four translucent layers with
linear and sRGB attachments. No optimization claim is based on this feature delivery.

[Raw samples and metadata](blending-performance.json) preserve both complete runs,
including all per-run medians, p95, maxima, profiling comparisons, and loaded artifacts.
The reviewed base is `0d1db912038f0d15f76d256b3db55b04858a87fd`; implementation
is in the working tree. The record includes hashes of the renderer C++ sources/headers.

## Conditions

Both runs used the current native Builder configuration: Clang 17.0.6, C++23,
`-g`, no optimization option, assertions enabled, and shared dependency libraries.
The compiler options were checked in the native build logs. Loaded dependency
paths are retained in metadata. The historical GNU `-O2 -DNDEBUG` baseline remains
unchanged; these measurements are not a like-for-like comparison with that build.

CPU: AMD Ryzen 9 7945HX. Resolution: 128×128. Each workload runs in a fresh process,
with five runs of three warm-up frames and 20 paired samples per run. Enabled and
disabled profiling order alternates. Every pair produced identical color and depth.
No outliers were removed. CPU affinity and external system load were not controlled.

Elapsed time covers the application frame, full color/depth clears, and draw sequence;
setup, output comparisons and reporting are excluded. Peak RSS is process `VmHWM`,
including setup, warm-up, both configurations and text reporting, not per-draw memory.

## Existing workload comparison

Times below are milliseconds. Brackets show the range of the five run medians.
Changes compare aggregate medians; interpret them alongside run-to-run variation.

### Profiling disabled

| Workload | Before median [range] | After median [range] | Change | Before p95 | After p95 |
|---|---:|---:|---:|---:|---:|
| textured_fill | 106.295 [101.423–115.691] | 101.800 [101.086–102.031] | -4.23% | 119.287 | 103.622 |
| depth_overdraw | 410.218 [403.602–412.662] | 407.204 [405.639–423.231] | -0.73% | 418.811 | 425.857 |
| many_draws | 114.459 [114.135–114.636] | 114.440 [114.054–115.424] | -0.02% | 115.873 | 116.285 |
| clipping | 30.051 [29.792–30.409] | 26.568 [25.661–26.869] | -11.59% | 31.195 | 26.941 |

### Profiling enabled

| Workload | Before median [range] | After median [range] | Change | Before p95 | After p95 |
|---|---:|---:|---:|---:|---:|
| textured_fill | 106.954 [102.225–116.716] | 102.491 [101.908–102.584] | -4.17% | 118.862 | 103.741 |
| depth_overdraw | 412.551 [406.963–415.244] | 409.742 [407.966–422.665] | -0.68% | 419.809 | 426.175 |
| many_draws | 115.283 [115.066–115.536] | 115.272 [114.907–116.473] | -0.01% | 116.413 | 117.239 |
| clipping | 30.308 [29.907–30.735] | 26.774 [25.828–27.098] | -11.66% | 31.535 | 27.134 |

Default-path medians did not increase in this comparison. Depth-overdraw and
many-draws p95 rose slightly (1.68% and 0.36% with profiling disabled), while their
medians stayed within the observed run ranges. This does not establish a uniform
tail-latency improvement or an algorithmic speedup.

## New feature costs

The original workload definitions remain unchanged (version 1). Workload-set version 2
adds `translucent_linear` and `translucent_srgb`, each with four back-to-front
textured layers, source-over RGB/alpha equations, depth testing enabled and depth
writes disabled. These costs establish a new feature baseline.

| Workload | Disabled median | Disabled p95 | Enabled median | Enabled p95 | RSS MiB |
|---|---:|---:|---:|---:|---:|
| translucent_linear | 434.710 | 445.192 | 436.525 | 447.071 | 10.75 |
| translucent_srgb | 443.795 | 488.874 | 448.287 | 491.263 | 10.75 |

Original-workload peak RSS (MiB):

| Workload | Before | After |
|---|---:|---:|
| clipping | 10.75 | 10.75 |
| depth_overdraw | 10.75 | 10.50 |
| many_draws | 10.75 | 10.75 |
| textured_fill | 10.75 | 10.75 |

## Reproduction and artifacts

From Builder-Layout, using a new absolute output directory for each invocation:

```sh
./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark --output /home/gilqamesh/Projects/Builder-Layout/artifacts/renderer-runs/m3-before-001 --size 128 --warmup 3 --samples 20 --runs 5
./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark --output /home/gilqamesh/Projects/Builder-Layout/artifacts/renderer-runs/m3-after-001 --size 128 --warmup 3 --samples 20 --runs 5
```

The first command ran before implementation and the second afterward. Original
results and reports remain in those directories. Build and execution logs are
`/tmp/renderer-m3-implementation/before-build.log`, `native-renderer.log`,
`native-demo.log`, and `after-build.log`. The JSON record retains the exact binary
and loaded-library paths for each run.
