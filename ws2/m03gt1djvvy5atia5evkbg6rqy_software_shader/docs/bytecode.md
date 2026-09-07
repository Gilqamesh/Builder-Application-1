# Compiled shader execution

[program_t](../software_shader.h) compiles ASTs at construction and executes them
with caller-owned bindings and IO:

```cpp
namespace cpu = m03gt1djvvy5atia5evkbg6rqy_software_shader;
cpu::execution_context_t context;
program.run(bindings, vertex_io, context);
program.run(bindings, fragment_io, context);
```

Keep a context alongside reusable IO. The public headers define storage reuse,
result validity, failure recovery, and borrowing requirements.

## Lowering

A shared expression is lowered at each use, so it observes the current local state.
For example, an expression `doubled = local * 2` created before a conditional
assignment still reads the assigned local when subsequently written to an output.

Loop conditions execute at the loop head, `continue` targets that head, and `break`
targets the nearest loop exit. Short-circuit branches skip unused operand code.
Locals occupy a stable slot prefix with runtime initialization checks. Temporary
slots are assigned before reading on every executed path and reused by iterations.

[helpers.cpp](../helpers.cpp) owns lowering and execution. Instructions use numeric
references into stage storage and the typed kernel table; this representation has
no serialization or externally supplied bytecode contract.
