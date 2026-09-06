# Profiling benchmark migration — 2026-09-06

Reviewed base: Builder-Modules `1aee3733`. This migration registers the existing
benchmark with Builder, preserves the profiler template architecture, and leaves
general build optimization support deferred.

## Result

- `builder.cpp` registers both the existing `cli` demo and the `benchmark` target.
  The default library source selection and profiling module producer are retained.
- `cli/benchmark.cpp` owns worker execution, raw observations, statistics, runtime
  build identity, and reporting. It uses the repository's process, filesystem, and
  JSON interfaces. Builder owns dependency discovery, source publication, compiler
  invocation, linking, public validation, and versioned build artifacts.
- `profiling/api.h` exposes storage-only and explicit-report constructors. The
  storage-only overload requires a default-constructible report policy; a move-only
  report with explicit state is covered by public validation.
- Descriptive template parameters in profiling and renderer integration use
  `*_type_t` names. Shared C++ instructions record that naming convention and the
  preference for overloads over default function arguments.
- Explanatory documentation and baseline evidence moved from `performance/` into
  `docs/`. The Python build driver was removed. New measurement output belongs in
  a new directory under `artifacts`, outside completed build installations.

The [historical raw baseline](profiling-baseline.json) is byte-for-byte unchanged:
SHA-256 `f2948e2d1f25258ed457182087ec6bdbdc446f3bf0e54d50115b47054af6790d`.
Its [report](profiling-baseline.md) retains the original measurements and commands,
with links to the retired driver at the reviewed commit.

## Validation obtained

Commands were run from Builder-Layout. Logs, compiler probes, check scripts, and
window captures remain in `/tmp/renderer-profiling-migration`.

The registered target built and ran through the normal Builder path:

```sh
./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark --help
./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark \
    --output /home/gilqamesh/Projects/Builder-Layout/artifacts/renderer-runs/profiling-migration-final \
    --size 32 --warmup 1 --samples 20 --runs 2
python3 /tmp/renderer-profiling-migration/verify_run.py \
    /home/gilqamesh/Projects/Builder-Layout/artifacts/renderer-runs/profiling-migration-final
python3 /tmp/renderer-profiling-migration/check_arguments.py
```

All passed. Native Clang builds ran profiler, renderer, and JSON public validation.
The normal Builder download path fetched and verified JSON's pinned header after
the initial sandboxed attempt could not resolve its host.

All four workloads retained 40 timing pairs each and complete captures with
identical color/depth output across policies. Independent recalculation confirmed
medians, nearest-rank p95, maxima, per-run medians, and policy percentage differences.
The final console log contains each workload summary once. A first execution
exposed inherited buffered stdout during worker launches; the coordinator now
flushes before starting each worker.

A second run used the installed benchmark directly with
`--size 1 --warmup 0 --samples 3 --runs 1`, writing
`artifacts/renderer-runs/profiling-migration-small`. The same verification command
passed for that directory, covering odd sample counts and zero warm-up.
Invalid arguments were rejected before output creation; an existing completed run
was preserved. Repeating `:benchmark --help` reused the completed target without
compiler invocations (`cache-reuse.log`).

Additional compiler checks passed:

```sh
/usr/bin/g++ -std=c++23 -O2 -Wall -Wextra \
    -I/tmp/renderer-profiling-migration/ws1 \
    /tmp/renderer-profiling-migration/ws1/m03gtjqkhqacstl3luv2ojsz3q_profiling/test/public_api.cpp \
    -o /tmp/renderer-profiling-migration/profiling-tests
/tmp/renderer-profiling-migration/profiling-tests
python3 /tmp/renderer-profiling-migration/check_disabled.py
```

The disabled draw probe uses native installed interfaces and GNU `-O2 -DNDEBUG`;
its emitted calls contain no profiling clock, exception-state, or recording calls.
`disabled-command.json` preserves the exact compiler arguments. This is a compiler
probe, not an optimized dependency build or a new optimized benchmark baseline.

Compiling `missing-formatter.cpp` with GNU C++23, `-fsyntax-only`, and
`-I/home/gilqamesh/Projects/Builder-Layout/ws1` failed as expected with
`enabled profiling requires a payload formatter`. The diagnostic is preserved in
`missing-formatter.log`.

Both graphical targets were built and checked:

```sh
/tmp/renderer-m1-implementation/install_binary m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
/tmp/renderer-m1-implementation/install_binary m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
python3 /tmp/renderer-profiling-migration/smoke.py
python3 /tmp/renderer-profiling-migration/smoke.py renderer
git -C /home/gilqamesh/Projects/Builder-Modules diff --check
git -C /home/gilqamesh/Projects/Builder diff --check
```

The consumer presented its existing 400×200 camera region in a 1600×1200 window.
The renderer presented its textured scene at 960×540. Both closed normally with
exit 0. The first renderer capture after two seconds was black; the repeated
capture waited ten seconds and showed the rendered scene. Its frame log records
approximately five seconds for the initial frame interval in this default build.

## Measurement limits

These runs validate the migration under current Builder settings. Runtime metadata
reports an unoptimized benchmark translation unit with assertions enabled, its
compiler, the actual executable artifact, and loaded file paths. Dependency compile
options are explicitly unknown in that runtime metadata; build logs retain the
invocations. No general optimization configuration or new optimized baseline was
introduced, and no performance comparison with the historical GNU direct-link
baseline is claimed. Profiling remains synchronous per collector and the benchmark
currently uses Linux process information.
