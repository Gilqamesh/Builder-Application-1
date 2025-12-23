# Builder-Application-1

Long-lived application workspace built with **Builder**. Use it to try Builder quickly, explore modules, and produce artifacts in a clean, versioned way.

## Contents

- [Quick start](#quick-start)
- [Contributing](#contributing)
- [Requirements](#requirements)
- [License](#license)

## Quick start
1) **Build a module** (outputs go to `artifacts/`)
```bash
./build.sh <module_name>
```

2) **Run the module's latest binary** (passes any extra args through)
```bash
./run.sh <module_name> <binary_name> [args...]
```

## Contributing
Issues and pull requests are welcome. Please describe:
- What changed and why.
- How you tested (commands/output).
- Any user-facing impact.

## Requirements
- POSIX-like environment with a C++23 `clang++` compiler.
- Standard Unix toolchain available in PATH.

## License
MIT — see `LICENSE`.
