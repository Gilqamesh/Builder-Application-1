# Milestone 4 measurements — 2026-09-07

The six existing workloads compare the default, stencil-disabled renderer before
and after adding stencil and shared pixel views. Three new workloads measure enabled
stencil masking and complete direct two-pass rendering. [Raw samples and source
hashes](milestone-4-performance.json) preserve both runs and their artifact identities.

## Conditions and limits

Reviewed base: `e5daeb1`. The implementation is in the working tree; no commit was
created. Both builds use native Clang 17 C++23, `-g`, no optimization option, assertions
enabled, and shared dependency libraries. Build commands are retained in
`/tmp/renderer-m4-implementation/native-before.log` and `native-renderer.log`.
After measurement began, benchmark help wording alone changed from six to nine
workloads; `native-help.log` records its rebuild. The raw report identifies the
actual measured executable and every loaded library, with hashes of measured source.

CPU: AMD Ryzen 9 7945HX. Resolution: 128×128. Each workload executes in a fresh
process, with five runs of three warm-up frames and 20 paired samples. Profiling
order alternates. Every pair produced identical color, depth, stencil and target
bytes where present. No outliers were removed. CPU affinity and external load were
uncontrolled; compilation and brief presentation checks overlapped portions of the
captures. Interpret differences alongside run-to-run variation, not as isolated
causal measurements of an individual branch or allocation change.

The initial preliminary run used an older installed artifact and is excluded.
The retained before run followed a rebuild against the current unchanged source.
The historical optimized baseline remains unchanged; these native unoptimized
measurements are not comparable to it as a performance regression test.

## Existing workloads

Times are milliseconds; brackets give the range of the five run medians.
Existing workload inputs and draw sequences are preserved. Differences include
both the disabled-stencil path and the shared storage implementation.

### Profiling disabled

| Workload | Before median [range] | After median [range] | Change | Before p95 | After p95 |
|---|---:|---:|---:|---:|---:|
| clipping | 26.944 [26.910–27.034] | 28.325 [28.147–28.425] | +5.13% | 27.104 | 28.862 |
| depth_overdraw | 417.314 [415.343–422.615] | 422.534 [409.554–426.141] | +1.25% | 425.159 | 434.552 |
| many_draws | 110.953 [109.774–116.636] | 115.807 [115.327–116.878] | +4.38% | 117.067 | 117.843 |
| textured_fill | 105.288 [104.961–105.588] | 109.442 [108.240–110.472] | +3.95% | 106.041 | 113.595 |
| translucent_linear | 441.744 [430.878–453.516] | 449.195 [423.516–452.669] | +1.69% | 457.730 | 463.669 |
| translucent_srgb | 448.811 [445.913–467.156] | 467.838 [442.122–479.588] | +4.24% | 474.046 | 484.747 |

### Profiling enabled

| Workload | Before median [range] | After median [range] | Change | Before p95 | After p95 |
|---|---:|---:|---:|---:|---:|
| clipping | 27.189 [27.150–27.278] | 28.535 [28.270–28.637] | +4.95% | 27.326 | 29.191 |
| depth_overdraw | 419.925 [417.850–424.264] | 424.452 [413.242–429.468] | +1.08% | 428.755 | 440.551 |
| many_draws | 111.589 [110.684–117.399] | 116.846 [116.021–117.608] | +4.71% | 118.109 | 118.568 |
| textured_fill | 106.065 [105.718–106.229] | 110.655 [109.322–111.633] | +4.33% | 107.229 | 114.783 |
| translucent_linear | 446.321 [433.495–454.598] | 451.079 [424.594–455.572] | +1.07% | 463.604 | 464.303 |
| translucent_srgb | 450.715 [448.234–469.749] | 471.068 [445.542–481.731] | +4.52% | 478.165 | 490.942 |

## New feature costs

`stencil_mask` clears stencil, renders a smaller mask quad with color/depth writes
disabled, then tests that mask during a textured draw. `two_pass_linear` and
`two_pass_srgb` render four source-over layers into a texture-owned attachment,
then sample and composite the premultiplied result onto a separate output.

Timed frames include mask drawing, stencil clearing, framebuffer selection, both
render passes and output clearing. Target allocation, validated view construction,
and resource binding happen at setup. No full-image copy occurs between passes.
These results establish feature baselines; they do not claim an optimization.

| Workload | Disabled median | Disabled p95 | Enabled median | Enabled p95 | RSS MiB |
|---|---:|---:|---:|---:|---:|
| stencil_mask | 158.693 | 164.979 | 159.553 | 165.871 | 11.00 |
| two_pass_linear | 643.082 | 672.042 | 648.963 | 674.911 | 11.00 |
| two_pass_srgb | 667.672 | 682.524 | 673.782 | 691.767 | 11.25 |

| Existing workload | Before RSS MiB | After RSS MiB |
|---|---:|---:|
| clipping | 11.00 | 11.00 |
| depth_overdraw | 10.75 | 10.75 |
| many_draws | 10.75 | 11.00 |
| textured_fill | 10.75 | 10.75 |
| translucent_linear | 10.75 | 10.75 |
| translucent_srgb | 10.75 | 11.00 |

Default-path median changes ranged from +1.25% to +5.13% in this capture. The tables retain tail latency and variation rather than interpreting a single aggregate as a uniform speedup or slowdown.

Peak RSS is process `VmHWM`, including setup, warm-up, both profiling configurations
and text reporting. Setup, output comparisons, reporting and summary construction
are outside frame timing. The normal workload metric scopes are unchanged; new
feature scopes include all additional per-frame work.

## Reproduction and validation

Commands run from Builder-Layout, before and after applying the production changes:

```sh
/tmp/renderer-m1-implementation/install_binary m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
artifacts/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/latest/binary/benchmark/install/benchmark --output /tmp/renderer-m4-implementation/before-current --size 128 --warmup 3 --samples 20 --runs 5
/tmp/renderer-m1-implementation/install_binary m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
artifacts/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/latest/binary/benchmark/install/benchmark --output /tmp/renderer-m4-implementation/after --size 128 --warmup 3 --samples 20 --runs 5
python3 /tmp/renderer-m4-implementation/record_results.py
```

All commands exited 0. The independent Python check verified all 100 sample pairs
per workload, aggregate medians, nearest-rank p95, maxima, per-run medians and
profiling-overhead percentages from the raw samples. The raw JSON here retains
source hashes, metadata, summaries and every pair. Per-workload profiler reports
remain beside the original results under `/tmp/renderer-m4-implementation`.
