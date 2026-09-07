# `m03gsy25j4v7nccgmsdov9ioft_shader`

## Purpose

Define and validate immutable, backend-independent shader ASTs and reflection.
Execution belongs to backends; primitive processing, framebuffer behavior, and
resource ownership belong to renderers.

## Stage interfaces

Vertex position and fragment color are `vector<float, 4>` special outputs,
separate from numbered outputs. Vertex transforms use homogeneous
`matrix<float, 4, 4>` built-ins. Fragment inputs own interpolation metadata;
numbered vertex outputs retain location/type compatibility. Standalone invocation
value support is independent of renderer interpolation eligibility.
