# Milestone 6 bytecode measurements — 2026-09-07

This delivery compares the AST interpreter at `cd6be21` with the compiled shader
implementation in the working tree. All thirteen existing workloads preserve their
settings and measurement scopes. [Raw samples, source and executable hashes, build
helper, and shader benchmark source](milestone-6-performance.json) accompany this report.

## Conditions

GNU C++23, `-O2 -g -Wall -Wextra`, assertions enabled, for shader, software_shader,
texture, software_renderer, profiling, byte_stream, structure_of_arrays and
type_erased_array. Math templates use the same options. Unchanged native filesystem,
process, signal_handler and test-framework libraries remain linked. This is the
same optimization scope as the [milestone 5 comparison](milestone-5-performance.md);
normal Builder targets retain their existing settings.

AMD Ryzen 9 7945HX; Linux x86-64; each renderer workload in a fresh process pinned
to logical CPU 8, 128×128, five runs, three warm-up frames and twenty measured
profiling-disabled/enabled pairs per run, alternating pair order. No compilation
or presentation checks overlapped performance capture. Other system activity,
frequency scaling and sibling-thread activity were not controlled. No outliers
were removed. The first baseline textured-fill run had a 6.563 ms median; the
remaining four were 4.088–4.108 ms. The ranges and p95 below retain that variation.

The earlier historical baseline used different code and build conditions. It
remains unchanged; this report makes speedup claims only against its freshly
measured, matching pre-bytecode build. Stage profiling still groups interpolation,
shader execution, sampling and attachment writes; the isolated shader batches
below provide the additional execution attribution.

## Complete frames

Milliseconds, profiling disabled. Brackets contain the five per-run medians' range.
Percentage change compares medians over all measured frames. RSS includes setup,
both profiling configurations and reporting, as defined by the benchmark metadata.

| Workload | AST median [range] | Bytecode median [range] | Change | AST p95 | Bytecode p95 | AST / bytecode peak RSS (MiB) |
|---|---:|---:|---:|---:|---:|---:|
| clipping | 1.067 [1.065–1.069] | 0.752 [0.750–0.753] | -29.56% | 1.096 | 0.776 | 5.50 / 5.75 |
| depth_overdraw | 15.391 [15.351–15.403] | 10.492 [10.472–10.596] | -31.83% | 15.822 | 11.042 | 5.25 / 5.50 |
| flat_fill | 3.846 [3.843–3.853] | 2.528 [2.522–2.539] | -34.28% | 3.907 | 2.618 | 5.25 / 5.50 |
| many_draws | 4.351 [4.344–4.376] | 2.968 [2.963–2.972] | -31.80% | 4.488 | 3.011 | 5.25 / 5.50 |
| mipmapped_fill | 6.432 [6.421–6.445] | 4.406 [4.396–4.418] | -31.51% | 6.564 | 4.606 | 5.25 / 5.50 |
| mipmapped_two_pass | 28.119 [28.078–28.170] | 18.619 [18.561–18.674] | -33.78% | 28.835 | 19.062 | 5.50 / 5.75 |
| noperspective_fill | 4.060 [4.038–4.097] | 2.853 [2.813–2.873] | -29.73% | 4.181 | 3.019 | 5.25 / 5.50 |
| stencil_mask | 6.241 [6.226–6.260] | 4.297 [4.287–4.303] | -31.16% | 6.438 | 4.453 | 5.25 / 5.50 |
| textured_fill | 4.104 [4.088–6.563] | 2.826 [2.821–2.830] | -31.14% | 6.907 | 2.872 | 5.25 / 5.50 |
| translucent_linear | 18.181 [18.156–18.225] | 12.644 [12.617–12.696] | -30.45% | 18.464 | 13.351 | 5.25 / 5.50 |
| translucent_srgb | 20.099 [20.064–20.122] | 14.359 [14.339–14.690] | -28.56% | 20.444 | 15.347 | 5.25 / 5.50 |
| two_pass_linear | 25.268 [25.215–25.373] | 16.525 [16.498–16.569] | -34.60% | 25.527 | 17.016 | 5.25 / 5.50 |
| two_pass_srgb | 26.948 [26.880–27.135] | 18.504 [18.487–18.554] | -31.33% | 27.381 | 18.845 | 5.50 / 5.75 |

The measured frame medians decrease by 28.6–34.6%. Both profiling configurations'
raw timings, counters, peak RSS and per-run summaries are retained in the JSON.
This delivery combines bytecode dispatch and inline reusable storage; these
measurements do not attribute the improvement to either change independently.

## Shader batches and construction

Each batch executes 1,000 successful invocations. Five runs each contain three
warm-up batches and twenty measured batches, on CPU 8. AST uses the existing
`run(bindings, io)`; bytecode uses `run(bindings, io, context)` with retained storage.
Inputs, bindings and IO are set up outside timing. The arithmetic workload executes
a matrix/vector product, four normalize/add/scale steps and a dot output; control
uses local assignments, a twelve-iteration loop, modulo and continue; sampling
executes bilinear explicit-LOD filtering over two mip levels.

| Batch | AST ns/invocation | Bytecode ns/invocation | Throughput ratio | Bytecode program construction median (µs) |
|---|---:|---:|---:|---:|
| arithmetic | 1630.8 | 377.0 | 4.33× | 11.16 |
| control | 5064.5 | 971.4 | 5.21× | 3.26 |
| sampling | 304.9 | 180.0 | 1.69× | 1.28 |

Construction is timed separately for twenty programs per workload. AST construction
is excluded; linking, lowering both stages and destroying the consumed ASTs are
included. Program destruction is excluded. Raw construction samples for both
implementations are retained. These small synthetic workloads establish a
compilation baseline; they do not bound larger programs' compilation cost.

## Representation and allocation

On this build `sizeof(value_t) = 68`, `sizeof(instruction_t) = 64`, and
`sizeof(operand_t) = 16` bytes. The immutable typed-kernel table has 648 entries,
25,920 bytes shared by all programs, excluding machine code. A kernel instruction
stores a numeric table index resolved during lowering, so arithmetic does not
repeat the AST's type dispatch. The switch loop invokes that selected typed kernel.

| Workload / stage | Instructions | Operands | Constants | Slots (including locals) | Locals | Stage storage bytes |
|---|---:|---:|---:|---:|---:|---:|
| arithmetic / vertex | 51 | 70 | 8 | 48 | 0 | 7668 |
| arithmetic / fragment | 3 | 2 | 1 | 1 | 0 | 552 |
| control / vertex | 34 | 37 | 7 | 22 | 2 | 6272 |
| control / fragment | 3 | 2 | 1 | 1 | 0 | 552 |
| sampling / vertex | 5 | 5 | 2 | 2 | 0 | 972 |
| sampling / fragment | 5 | 7 | 1 | 3 | 0 | 1064 |

Retained context storage is 3,312 bytes for the arithmetic batch, 1,546 for control,
and 252 for sampling. These include the context object, slot capacity and local
validity capacity, excluding allocator bookkeeping and IO. Each batch invokes
only its relevant stage. Stage totals include the stage object, instruction,
operand, constant and slot-type capacities plus logical reflection elements;
allocator bookkeeping and spare reflection capacity are excluded.

The allocation probe observes zero allocations during twenty warmed vertex/fragment
pairs, including newly taken branches with numbered outputs. It intercepts ordinary
and aligned allocation and injects failures into slot, local-validity and output
preparation for both IO types. All six failure cases clear old results and permit
successful context reuse. The guarantee concerns execution and IO storage for
successful invocations; exception construction and resource operations are outside
its scope. Operand-pool access, whole-value slot size and unused temporary lifetimes
remain measured candidates for a later optimization.

## Validation and reproduction

- Optimized shader, software_shader, texture and renderer public API suites pass.
  Software-shader cases cover all 25 value types, changing slot alternatives,
  branches, short-circuit traps, nested loops/break/continue, discard, current
  bindings, preparation failure and warmed storage reuse.
- A development-only copy of the old evaluator compares 10,240 typed arithmetic,
  constructor, swizzle and matrix cases with bytecode. All pass, including signed
  overflow, division errors, signed zeros, NaNs and infinities. Finite results and
  signed zeros are compared by bits; NaNs by classification; failures by exception
  type. Non-language kernel-table combinations are skipped. The reference stays
  outside production; its source remains available at the base revision.
- All thirteen renderer workloads match across implementations: 49 color, depth,
  stencil and mip buffers compare byte for byte. Their sizes and SHA-256 hashes
  are recorded. Profiling-enabled/disabled regression assertions also pass.
- Native Builder installs and public validation pass for the renderer and its
  tower-defense consumer. Both desktop demos produced visible frames and closed
  normally; this is a smoke check, not exhaustive interactive validation.

Commands were run from `Builder-Layout`; staging sources, drivers and logs are in
`/tmp/renderer-m6-implementation`:

```sh
python3 /tmp/renderer-m6-implementation/build.py before shader software_shader texture renderer benchmark
python3 /tmp/renderer-m6-implementation/build.py after shader software_shader texture renderer
python3 /tmp/renderer-m6-implementation/build.py after software_shader shader_benchmark
python3 /tmp/renderer-m6-implementation/build.py after differential
python3 /tmp/renderer-m6-implementation/build.py after software_shader

taskset -c 8 /tmp/renderer-m6-implementation/build-before/benchmark --output /tmp/renderer-m6-before --size 128 --warmup 3 --samples 20 --runs 5
taskset -c 8 /tmp/renderer-m6-implementation/build-after/benchmark --output /tmp/renderer-m6-after --size 128 --warmup 3 --samples 20 --runs 5

CCACHE_DIR=/tmp/renderer-m6-implementation/ccache /tmp/renderer-m0-implementation/install_library m03gt1djvvy5atia5evkbg6rqy_software_shader
CCACHE_DIR=/tmp/renderer-m6-implementation/ccache /tmp/renderer-m1-implementation/install_binary m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
CCACHE_DIR=/tmp/renderer-m6-implementation/ccache /tmp/renderer-m1-implementation/install_binary m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
```

The retained build helper also builds `shader_benchmark` and the temporary `frame`
capture target for either snapshot. Each shader binary was invoked for `arithmetic`,
`control` and `sampling` using `taskset -c 8`. Each frame capture used
`--size 128 --warmup 1 --samples 1 --runs 1`; comparisons run outside timed frames.
Screenshots are `/tmp/renderer-m6-implementation/renderer-desktop-smoke-0.png`
and `tower-defense-smoke-0.png`; each launch exited with status 0.
No sanitizer or concurrent stress test was run. Prepared bindings, SIMD, JIT,
serialization and early depth/stencil testing remain deferred.
