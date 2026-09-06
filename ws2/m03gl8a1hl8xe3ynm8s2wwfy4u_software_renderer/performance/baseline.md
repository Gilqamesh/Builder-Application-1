# Initial profiling baseline — 2026-09-06

This establishes the optimized renderer baseline and the cost of enabling the
new profiling policy. Renderer algorithms are unchanged. It is not a comparison
with an earlier algorithmic optimization delivery.

[Raw observations and metadata](baseline.json) preserve every sample, per-run
medians, nearest-rank p95, maxima, and module revisions. The [driver](run.py) and
[usage](README.md) reproduce the build and measurement procedure.

## Build and measurement scope

- Compiler: `g++ (GCC) 13.3.1 20240913 (Red Hat 13.3.1-3)` via `/usr/bin/g++`.
- Flags on all 21 translation units, including renderer, software shader,
  texture/sampler, shader construction, and supporting implementations:
  `-std=c++23 -O2 -g -DNDEBUG -Wall -Wextra`.
- CPU: AMD Ryzen 9 7945HX with Radeon Graphics.
- Platform: `Linux-6.8.10-700.rog.fc39.x86_64-x86_64-with-glibc2.38`.
- Source base: Builder-Modules `5c7ca542d64efa8fb2112670f058e4349864c539`,
  plus the profiling implementation in the working tree; Builder
  `3ac66aca3f00e68bc32a7a54363ce826d3d0092e` is unchanged.
- Compiled source/configuration SHA-256:
  `b8f1be0fb8503893e4ea1f937bd07e53395df5aab6bb5fb3042b73bb62d39812`.
- Workload version 1; 128×128 framebuffer; five runs, each with three warm-up
  frames per policy and 20 paired samples. Execution order alternates.
- Measured elapsed time includes the application frame scope, complete color and
  depth clears, and the workload's draw sequence. Setup, reset, result comparison,
  and reporting are excluded. Scheduling delays are included.

The source snapshot, compile commands, executable binaries, disabled assembly,
compile-failure diagnostic, and four example reports remain in
`/tmp/renderer-profiling-baseline`. Snapshot contents and compile flags determine
the digest; subsequent documentation changes do not change the compiled source.

## Results

Times are milliseconds. Difference compares the two aggregate medians.

| Workload | Disabled median | Enabled median | Disabled p95 | Enabled p95 | Difference | Peak RSS MiB |
|---|---:|---:|---:|---:|---:|---:|
| textured_fill | 3.760 | 3.766 | 3.775 | 3.799 | +0.17% | 3.75 |
| depth_overdraw | 15.358 | 15.555 | 15.584 | 15.746 | +1.28% | 3.75 |
| many_draws | 4.358 | 4.369 | 4.389 | 4.403 | +0.25% | 4.00 |
| clipping | 1.079 | 1.068 | 1.094 | 1.083 | -0.97% | 3.75 |

Peak RSS uses Linux `/proc/self/status` `VmHWM` for each fresh benchmark address
space. It includes setup, warm-up, reporting, and both policies. It is not a
per-policy memory comparison. This avoids the pre-exec launcher high-water mark
observed with `getrusage` in this environment.

Ranges of the five per-run medians:

| Workload | Disabled range, ms | Enabled range, ms |
|---|---:|---:|
| textured_fill | 3.727–3.766 | 3.709–3.775 |
| depth_overdraw | 15.321–15.543 | 15.511–15.681 |
| many_draws | 4.327–4.368 | 4.336–4.384 |
| clipping | 1.050–1.087 | 1.043–1.080 |

These measurements characterize this workload and machine. No samples were
removed; small policy differences should be interpreted alongside run variation.
CPU affinity and system load were not controlled. There is no general overhead
or speedup guarantee.

The final textured-fill capture demonstrates the shared application/renderer
hierarchy and counter meanings; it is one frame, not an aggregate statistic:

```text
application.frame inclusive=3694479 ns self=101 ns draws=1
  renderer.clear_color inclusive=1663 ns self=1663 ns color_writes=16384, depth_writes=0
  renderer.clear_depth inclusive=3286 ns self=3286 ns color_writes=0, depth_writes=16384
  renderer.draw inclusive=3689429 ns self=90 ns
    renderer.preparation inclusive=180 ns self=180 ns
    renderer.vertices inclusive=2625 ns self=2625 ns vertex_invocations=6
    renderer.rasterization inclusive=3686534 ns self=3686534 ns fragment_invocations=16384, discards=0, depth_rejections=0, color_writes=16384, depth_writes=16384
capture: 7 records, 0 omitted, complete
```

## Validation obtained

Commands below passed. A separate
`./cli m03gilsfsv3k34ej14ytz8a29k_tower_defense_game` invocation rebuilt the
consumer successfully, then its sandboxed launch could not connect to the
desktop. The X11 smoke check below passed.

From Builder-Layout:

```sh
/tmp/renderer-m0-implementation/install_library m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
/tmp/renderer-m0-implementation/install_library m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
artifacts/m03gtjqkhqacstl3luv2ojsz3q_profiling/latest/library/build/validation/public_api/runner
artifacts/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/latest/library/build/validation/public_api/runner
python3 ws2/m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/performance/run.py \
    --workspace-root /home/gilqamesh/Projects/Builder-Layout \
    --output /tmp/renderer-profiling-baseline \
    --compiler /usr/bin/g++ --size 128 --warmup 3 --samples 20 --runs 5
python3 /tmp/builder-profiling/smoke.py
git -C /home/gilqamesh/Projects/Builder-Modules diff --check
```

Native Clang validation and optimized GNU validation passed. Profiler tests cover
controlled-clock nesting, inclusive/self durations, owned registration metadata,
reset, explicit closure, unwinding, overflow and omitted subtrees, zero-capacity
captures, disabled unconstructible payloads, independent collectors, deferred
formatting, and allocation-free recording.

The full renderer regression suite passes alongside new enabled/disabled
comparisons, exact counters, mixed application and renderer payloads, exceptional
exit, clipping, all seven primitive topologies, and exhausted capture storage.
Every benchmark sample also produced identical color and depth with both policies.
The optimized driver verified the missing-formatter compile failure and inspected
emitted call/jump instructions for profiling clocks and recording in disabled draw.

The tower-defense binary was rebuilt using the native Builder path and launched
from its installed asset directory with X11 selected. The inspected 1600×1200
window displayed its existing 400×200 camera region and closed normally, exit 0.
The temporary smoke helper and `tower-smoke.png` remain in
`/tmp/builder-profiling`; native and optimized build logs are there as well.

No profiling design decision remains unresolved. Profiling is synchronous and
confined to one thread per collector; the benchmark currently targets Linux.
Algorithmic renderer optimization remains subsequent work.
