# `m03gubnevca0u4aqlfbtuz06ix_lisp_runtime`

Preserve the Lisp runtime value and JSON serialization contract: a value contains `type_module` and `data`; lists and records recursively contain serialized values. Capability values identify a module and a native capability by name. Closures belong to the evaluator and stay outside this serialized value boundary.
