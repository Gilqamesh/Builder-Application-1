# Milestone 5 optimized measurements — 2026-09-07

The nine existing workloads compare revision `2924350` with the milestone 5
implementation. Four new workloads establish interpolation and mipmap feature
baselines. [Raw samples, source hashes and artifact hashes](milestone-5-performance.json)
retain both pre-change captures and the final changed-source capture.

## Conditions

The measurement binaries and all performance-relevant C++ dependencies were built
consistently with GNU C++23, `-O2 -g -Wall -Wextra`, assertions enabled. This includes
shader, software_shader, texture, software_renderer, profiling, byte_stream,
structure_of_arrays and type_erased_array; math templates compile at the same
optimization level. Unchanged native filesystem, process, signal_handler and test
framework libraries remain linked. The temporary build helper and logs are in
`/tmp/renderer-m5-implementation`. The normal Builder targets remain unchanged.
Raw benchmark metadata describes its normal Builder integration; the wrapper's
`actual_build` field and recorded commands identify this optimized staging build.

Hardware: AMD Ryzen 9 7945HX. Resolution: 128x128. Each workload runs in a fresh
process pinned to logical CPU 8, with five runs, three warm-up frames per run,
and 20 measured profiling-disabled/enabled pairs per run. Pair order alternates.
No compilation or presentation checks overlapped these captures. External load,
frequency scaling and sibling-thread activity were not controlled; no outliers
were removed. Reported ranges contain the five run medians.

An initial optimized pre-change capture establishes the milestone 5 baseline.
The unchanged binary was measured again immediately before the final changed
binary; the tables use that repeat for the before/after comparison. Both pre-change
captures are preserved. The historical optimized milestone 2 and unoptimized
milestone 3/4 records remain unchanged and are not treated as comparable regression
measurements across different build configurations or profiling contracts.

## Existing workloads

Milliseconds; brackets give the range of per-run medians. Positive changes mean
increased elapsed time. These are feature-delivery costs, not an optimization claim.

### Profiling disabled

| Workload | Before median [range] | After median [range] | Change | Before p95 | After p95 |
|---|---:|---:|---:|---:|---:|
| clipping | 1.039 [1.034–1.043] | 1.066 [1.065–1.067] | +2.58% | 1.086 | 1.122 |
| depth_overdraw | 14.555 [14.541–14.607] | 16.095 [16.077–16.203] | +10.58% | 14.994 | 16.434 |
| many_draws | 4.317 [4.309–4.340] | 4.332 [4.327–4.338] | +0.34% | 4.491 | 4.570 |
| stencil_mask | 5.890 [5.866–5.922] | 6.168 [6.155–6.192] | +4.73% | 6.108 | 6.433 |
| textured_fill | 3.893 [3.886–4.656] | 4.095 [4.089–4.098] | +5.19% | 6.492 | 4.163 |
| translucent_linear | 17.552 [17.530–17.576] | 18.488 [18.443–18.581] | +5.33% | 17.809 | 18.780 |
| translucent_srgb | 19.174 [19.128–19.220] | 20.235 [20.183–20.289] | +5.53% | 19.598 | 20.594 |
| two_pass_linear | 24.120 [24.020–24.206] | 27.122 [27.053–27.249] | +12.44% | 24.691 | 27.891 |
| two_pass_srgb | 26.050 [26.029–26.084] | 27.298 [27.113–28.303] | +4.79% | 26.650 | 32.200 |

### Profiling enabled

| Workload | Before median [range] | After median [range] | Change | Before p95 | After p95 |
|---|---:|---:|---:|---:|---:|
| clipping | 1.044 [1.042–1.047] | 1.075 [1.074–1.077] | +2.99% | 1.085 | 1.132 |
| depth_overdraw | 14.592 [14.570–14.601] | 16.115 [16.087–16.138] | +10.43% | 14.688 | 16.447 |
| many_draws | 4.347 [4.337–4.367] | 4.367 [4.363–4.371] | +0.46% | 4.430 | 4.434 |
| stencil_mask | 5.905 [5.880–5.956] | 6.181 [6.169–6.204] | +4.68% | 6.071 | 6.443 |
| textured_fill | 3.909 [3.896–4.686] | 4.133 [4.130–4.137] | +5.73% | 6.518 | 4.186 |
| translucent_linear | 17.612 [17.591–17.638] | 18.574 [18.524–18.642] | +5.46% | 17.846 | 18.885 |
| translucent_srgb | 19.160 [19.080–19.188] | 20.296 [20.255–20.328] | +5.93% | 19.509 | 20.894 |
| two_pass_linear | 24.167 [24.065–24.242] | 26.673 [26.622–26.783] | +10.37% | 24.396 | 27.287 |
| two_pass_srgb | 26.111 [26.089–26.147] | 27.441 [27.248–28.337] | +5.09% | 26.674 | 31.863 |

The disabled-path increases range from 0.34% to 12.44% in this capture. This
milestone prepares interpolation lists before rasterization, retains scratch
capacity, borrows flat values once per primitive and keeps ordinary sampling on a
direct level-zero path. Broader rasterizer/interpreter optimization remains a
measurement-driven milestone 6 task. Variation and uncontrolled machine state
limit attribution of individual timing differences to particular code changes.

## Feature baselines

`flat_fill` and `noperspective_fill` exercise those varying modes over a filled
quad. `mipmapped_fill` samples a static mipmapped texture at fractional LOD with
trilinear filtering; generation is excluded as setup. `mipmapped_two_pass` renders
the existing masked translucent scene, regenerates the full target chain in place,
and composites it at LOD 1.5. Its frame time includes regeneration.

| Workload | Disabled median [range] | Disabled p95 | Enabled median | Enabled p95 | RSS MiB |
|---|---:|---:|---:|---:|---:|
| flat_fill | 3.821 [3.783–3.846] | 3.880 | 3.823 | 3.876 | 5.25 |
| noperspective_fill | 4.059 [4.045–4.067] | 4.134 | 4.062 | 4.110 | 5.25 |
| mipmapped_fill | 7.162 [7.157–7.166] | 7.271 | 7.143 | 7.318 | 5.25 |
| mipmapped_two_pass | 28.090 [28.039–28.274] | 28.771 | 28.347 | 28.978 | 5.50 |

| Existing workload | Before RSS MiB | After RSS MiB |
|---|---:|---:|
| clipping | 5.50 | 5.50 |
| depth_overdraw | 5.25 | 5.25 |
| many_draws | 5.25 | 5.25 |
| stencil_mask | 5.25 | 5.25 |
| textured_fill | 5.25 | 5.25 |
| translucent_linear | 5.25 | 5.25 |
| translucent_srgb | 5.50 | 5.50 |
| two_pass_linear | 5.25 | 5.25 |
| two_pass_srgb | 5.25 | 5.50 |

Peak RSS is process VmHWM through capture and text reporting, including both
profiling configurations, allocation, setup and warm-up. Setup, byte comparisons,
reporting and summary calculation remain outside frame timing. Every measured
pair matched color/depth/stencil bytes and target bytes where present; the new
mipmapped two-pass workload also compares every lower level.

## Reproduction and validation

Commands run from Builder-Layout, all exit 0:

```sh
python3 /tmp/renderer-m5-implementation/build.py before benchmark
taskset -c 8 /tmp/renderer-m5-implementation/build-before/benchmark --output /tmp/renderer-m5-before --size 128 --warmup 3 --samples 20 --runs 5
python3 /tmp/renderer-m5-implementation/build.py after shader software_shader texture renderer benchmark
taskset -c 8 /tmp/renderer-m5-implementation/build-before/benchmark --output /tmp/renderer-m5-before-repeat --size 128 --warmup 3 --samples 20 --runs 5
taskset -c 8 /tmp/renderer-m5-implementation/build-after/benchmark --output /tmp/renderer-m5-final --size 128 --warmup 3 --samples 20 --runs 5
python3 /tmp/renderer-m5-implementation/record_performance.py
```

The independent Python check verified sample counts and identities, medians,
nearest-rank p95, maxima, per-run medians and profiling overhead from every raw
sample. An initial feature capture routed ordinary sampling through explicit LOD;
the retained final implementation restores its direct base-level path. That
preliminary feature capture remains in `/tmp/renderer-m5-after` and is excluded
from these tables. No before/after equality of entire frames across separate
binaries is claimed; correctness is supplied by the public regression suites.
