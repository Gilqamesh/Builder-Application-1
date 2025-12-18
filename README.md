# Builder-Application-1

This repository is an **application workspace built on top of Builder**.
It shows how Builder is used in practice to develop, compose, and run
modules as applications.

Builder itself is **not implemented here**. It is consumed as a module
under `modules/builder`.

---

## Repository structure

```text
.
├─ modules/
│  ├─ builder/          # Builder module (reference, self-hosting)
│  ├─ <module_a>/       # Application or library module
│  ├─ <module_b>/
│  └─ ...
├─ artifacts/           # All build outputs (versioned)
├─ *.sh                 # Helper scripts (build / run / debug)
├─ main.cpp             # Bootstrap entrypoint
└─ LICENSE              # MIT license
```

- Each directory under `modules/` is a **module**
- Each module defines its build logic in `builder_plugin.cpp`
- All outputs are written under `artifacts/`, versioned per build

---

## Module dependencies

Each module declares its dependencies explicitly in a `deps.json` file:

```json
{
    "builder_deps": [
        "builder"
    ],
    "module_deps": [
        "builder"
    ]
}
```

- `builder_deps`  
  Modules required to *build* this module (build-time dependencies)

- `module_deps`  
  Modules required by the produced artifacts (link-time / runtime dependencies)

Dependency resolution and validation are handled by Builder.

---

## Building modules

Modules are built by invoking the **latest available Builder artifact**.
There are two equivalent entrypoints.

### 1. Using helper scripts

Shell scripts locate the most recent Builder build under
`artifacts/builder/` and invoke it with the correct arguments.

Example:
```bash
./driver.sh <target_module>
```

Some scripts support optional execution under `gdb`.

---

### 2. Via the bootstrap program (`main.cpp`)

`main.cpp` implements the bootstrap logic explicitly.

#### Compile the bootstrap driver

```bash
clang++ -std=c++23 main.cpp -o driver
```

#### Run

```bash
./driver <target_module>
```

Behavior:

- If a Builder artifact already exists:
  - it is executed directly
- Otherwise:
  - a temporary Builder binary is compiled from `modules/builder`
  - run once to install itself into `artifacts/`
  - then removed

This guarantees that Builder can **self-install and self-upgrade** before
being reused.

The shell scripts mirror this logic for convenience.

---

## License

MIT. See the `LICENSE` file for details.
