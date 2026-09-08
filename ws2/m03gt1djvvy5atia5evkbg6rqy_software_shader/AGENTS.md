# `m03gt1djvvy5atia5evkbg6rqy_software_shader`

## Purpose

Link and execute individual CPU shader invocations from backend-independent ASTs
and canonical CPU resources. Assembly, interpolation, rasterization, framebuffer
behavior, and renderer-facing resource lifetimes belong to the renderer.

## Execution

Programs own immutable compiled stages and reflection; AST borrowing ends after
lowering. Bytecode is an internal in-memory representation. Keep one `value_t`
definition for invocation IO, constants, and complete-value execution slots.

Expressions observe current local state at each execution. Contexts retain storage
for sequential reuse across programs and stages; simultaneous invocations require
independent context and IO. Preserve freshness, result invalidation on failure,
and warmed allocation guarantees defined in [software_shader.h](software_shader.h).
[invocation.h](invocation.h) owns binding lifetimes, stage IO, and reset behavior.

Prepared execution resolves uniform snapshots and borrowed resources in program
reflection order. Its indexed input contract and borrow lifetime are defined by
`prepared_program_t` in [software_shader.h](software_shader.h). Standalone runs
retain their location-based validation and borrowing contract.
