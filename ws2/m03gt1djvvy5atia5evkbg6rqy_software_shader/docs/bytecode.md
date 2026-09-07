# Compiled shader execution

`program_t` compiles both typed ASTs at construction and retains compiled code,
constants and reflection. AST ownership ends after lowering. The public reuse and
failure guarantees are defined in [software_shader.h](../software_shader.h);
[invocation.h](../invocation.h) owns bindings and invocation IO.

```cpp
namespace cpu = m03gt1djvvy5atia5evkbg6rqy_software_shader;
cpu::execution_context_t execution_context;
program.run(bindings, vertex_io, execution_context);
program.run(bindings, fragment_io, execution_context);
```

Retain a context alongside invocation IO. The renderer keeps one in its scratch
state and uses it for sequential vertex and fragment calls. Existing two-argument
calls construct temporary execution storage. Prepared bindings remain deferred;
each call validates and observes its current inputs and bindings.

## Internal representation

[helpers.h](../helpers.h) declares the in-memory lowering and execution structures.
They define module implementation details, without a serialization or external
bytecode contract.

- `stage_code_t`: reflected interface, instruction sequence, operand pool,
  constants, per-slot types and local count; only const views after construction.
- `instruction_t`: opcode, optional destination slot, operand span, optional jump
  target and numeric typed-kernel index. `operand_t` holds a kind and numeric index.
- `value_t`: the single complete scalar/vector/matrix variant used by constants,
  invocation IO and context slots. Resource operands reference reflected bindings.
- `execution_context_t`: retained value slots and local initialization flags. Locals
  occupy a stable prefix; temporary slots are assigned per lowered computation site.

The compiler chooses typed kernels while lowering. Execution uses an ordinary
opcode switch plus the immutable kernel table; values are assigned into destination
variants, establishing their alternatives even when a slot previously held another
type. Temporary reads follow writes on every executed path. Local reads and
assignments additionally check runtime initialization.

A shared expression is lowered at each use; it does not memoize a value. Loop
conditions sit at the loop head, continue targets that head, and break targets the
nearest loop's exit. Iterations reuse existing slots. Short-circuit branches skip
unneeded operand code. Invocation-dependent failures remain runtime failures.

For example, the public validation shader uses a local and conditional assignment:

```text
local = 1.0
if enabled:
    local = input + 1.0
output[0] = local * 2.0
position = (0, 0, 0, 1)
```

Its lowering has the following shape (symbolic names replace numeric indices):

```text
constant       t0, 1.0
initialize     local0, t0
input_bool     t1, enabled
jump_if_false  t1, after_branch
check_local    local0
input_float    t2, input
constant       t3, 1.0
add_float      t4, t2, t3
assign_local   local0, t4
jump           after_branch
after_branch:
read_local     t5, local0
constant       t6, 2.0
multiply_float t7, t5, t6
output_float   output0, t7
constant       t8, (0, 0, 0, 1)
position       t8
finish
```

The branch chooses 8.0 for input 3.0 with `enabled=true`, and 2.0 otherwise. Each
invocation clears results first, prepares capacity, resets local validity, validates
current inputs/bindings, and executes; failure clears results again. Reserving every
reflected numbered output during preparation covers previously untaken branches.

[The milestone 6 report](../../m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/docs/milestone-6-performance.md)
records differential validation, allocation checks, structure sizes, executable and
context storage, construction cost, shader throughput and complete-frame timings.
