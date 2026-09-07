# `m03gt1djvvy5atia5evkbg6rqy_software_shader`

## Purpose

Execute individual vertex and fragment shader invocations on the CPU from backend-independent shader ASTs and canonical CPU resources.

This module owns program-link compatibility and per-invocation execution. Vertex assembly, interpolation, rasterization, framebuffer state, presentation, and renderer-facing resource lifetimes remain renderer or pipeline responsibilities.

## Invariants

- Every `run()` starts with fresh locals and invalidates prior stage results before execution.
- Clearing execution results preserves caller-supplied inputs and vertex matrices. Reusable invocation state is reset explicitly by its caller.
- Program linking rejects incompatible stages, interfaces, and resource bindings before execution.
- Vertex invocation state supplies identity object-to-world and world-to-clip matrices by default and permits explicit overrides.
- Expressions are evaluated where they execute and observe the current local state rather than cached expression values.
- A normally completed vertex invocation has written position.
- Fragment color is an optional special result distinct from numbered outputs. Fragment discard terminates the invocation and invalidates color and all numbered outputs.
- Programs own immutable compiled stages and reflection; compiler AST references end
  after lowering. Bytecode is an internal in-memory representation with numeric
  references, without a serialized format or externally supplied bytecode interface.
- `value_t` has one definition shared by invocation IO, constants, and complete-value
  execution slots. Writes establish their active variant alternative; typed reads
  follow compilation guarantees or local initialization checks.
- Callers retain `execution_context_t` for sequential reuse across stages and programs.
  Each simultaneous invocation requires independent context and IO. Context storage
  grows as required and is retained; logical freshness does not require clearing all
  temporary memory. Failure leaves the context reusable.
- Observable results are invalidated before any potentially failing preparation and
  again on failure. Successful invocations allocate no execution or IO storage once
  both have sufficient capacity; all reflected numbered outputs are reserved.
  Exception construction and resource operations are outside that storage guarantee.
- Execution representation and strategy remain private to permit later alternatives
  without changing the public contract. Prepared bindings remain deferred.
