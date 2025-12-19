# Builder-Application-1

A long-lived application workspace built on **Builder** to validate Builder as a proof of concept and to evolve modules as I learn new concepts, languages, tools, and practices. Modules stay focused with explicit dependencies. Builder itself is consumed as a module under [`modules/builder`](https://github.com/Gilqamesh/Builder); it is not implemented here.

## Repository structure
```
.
├─ modules/               # builder/ (submodule) + application/library modules
├─ artifacts/             # versioned build outputs
├─ run_latest_driver.cpp  # bootstrap driver (C++)
├─ driver.sh              # helper: build (if needed) + run a target module
├─ binary.sh              # helper: run the latest-built binary for a module
└─ LICENSE
```
- Each directory under `modules/` is a module.
- Each module defines its build logic in `builder_plugin.cpp`.
- Outputs are written to `artifacts/`, versioned per build.

## Usage

### Clone and init
```bash
git clone https://github.com/Gilqamesh/Builder-Application-1.git
cd Builder-Application-1
git submodule update --init --recursive
```

### driver.sh — build and run a target module
- Builds `builder_driver` if missing, then runs the requested module.
- `-g` runs under `gdb`, attached to the latest `builder_driver` invocation for the target.
```bash
./driver.sh <target_module>
./driver.sh <target_module> -g
```

### binary.sh — run the latest-built binary for a module
- Locates the newest artifact for `<target_module>` and executes `<binary_name>`.
```bash
./binary.sh <target_module> <binary_name>
```

### run_latest_driver.cpp — bootstrap driver (C++)
- Same purpose as `driver.sh`, implemented in C++.
```bash
clang++ -std=c++23 run_latest_driver.cpp -o run_latest_driver
./run_latest_driver <target_module>
```

## Module dependencies
Each module declares dependencies in `deps.json`:
```json
{
  "builder_deps": ["builder"],
  "module_deps": ["builder"]
}
```
- `builder_deps`: modules required to build the module (build-time).
- `module_deps`: modules required by produced artifacts (link-time / runtime).
Builder validates and resolves dependencies.

## License
MIT. See `LICENSE`.