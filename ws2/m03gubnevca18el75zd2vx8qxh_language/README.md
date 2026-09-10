# Lisp

Evaluate Lisp forms and invoke module capabilities in the current Builder workspace.

```bash
./cli m03gubnevc9uwrppfipf0i15zk_builder_repl
./cli m03gubnevc9vfwh0ka3iz3mjuf_builder_eval '(let ((fs (filesystem))) ((get fs "exists") "/tmp"))'
./cli m03gubnevc9vfwh0ka3iz3mjuf_builder_eval --file ws2/m03gubnevc9vfwh0ka3iz3mjuf_builder_eval/examples/programs.builder
```

The language supports strings, booleans, lists, records, lexical functions, `define`, `lambda`, `let`, `if`, `begin`, `load`, `get`, `record?`, `capability?` and `equal?`. A module name evaluates to a callable value; `(filesystem)` returns a record of named capabilities. Capabilities survive JSON serialization. Closures remain inside evaluation.

The REPL retains definitions and supports multiline input, tab completion, `:history`, `:!!` and `:<index>`.

Native bindings export `module__apply` and `module__capability__<name>` from an ordinary module library, accepting and returning Lisp runtime values. `lisp_<name>` modules adapt existing modules for Lisp; complete module identities select a specific binding. Builder's build introspection reports the current source, interface, library and binary phases and shared-library output.

Run `test/integration.sh` and `test/file_entry.sh` in the `builder_eval` module with `BUILDER_WORKSPACE_ROOT` set to the combined workspace. Builder also runs the language and runtime public API tests during library validation.
