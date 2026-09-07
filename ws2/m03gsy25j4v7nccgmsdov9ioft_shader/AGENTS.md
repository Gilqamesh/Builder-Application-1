# `m03gsy25j4v7nccgmsdov9ioft_shader`

## Purpose

Define and validate immutable, backend-independent shader ASTs and their reflected interfaces.

Shader execution belongs to backend modules. Primitive processing, framebuffer behavior, and resource ownership belong to renderers.

## Invariants

- Vertex position and fragment color are stage-specific special outputs and do not appear among reflected numbered outputs.
- Vertex position and fragment color have type `vector<float, 4>`.
- Generic numbered outputs remain available independently of special outputs.
- Reflection records the typed inputs, numbered outputs, and bindings required by an AST.
- Vertex shaders can read backend-supplied `object_to_world` and `world_to_clip` semantics as homogeneous `matrix<float, 4, 4>` values.

- Fragment inputs own perspective, noperspective, or flat interpolation metadata.
  Numbered vertex outputs retain location/type compatibility; standalone invocation
  value support remains independent of renderer interpolation eligibility.
