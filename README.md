# Builder-Application-1

Long-lived PoC workspace of **Builder**.

## Contents

- [Quick start](#quick-start)
- [Contributing](#contributing)
- [Requirements](#requirements)
- [License](#license)

## Quick start

1. **Clone and initialize submodules**

   ```bash
   git clone https://github.com/Gilqamesh/Builder-Example.git
   cd Builder-Example
   git submodule update --init --recursive
   ```

2. **Compile cli.cpp**

   ```bash
   clang++ -std=c++23 cli.cpp -o cli
   ```

3. **Run cli on the target module, optionally running the module's produced binary as a post-step**

   ```bash
   ./cli F # builds module F, i.e., runs its builder_plugin.cpp implementation
   ./cli F f_static # also run 'f_static' under latest installed binaries directory 
   ```

## Contributing
Issues and pull requests are welcome. Please describe:
- What changed and why.
- How you tested (commands/output).
- Any user-facing impact.

## Requirements

- C++23 compiler
- Unix environment
- Requirements of [Builder](https://github.com/Gilqamesh/Builder)

## License
MIT - see `LICENSE`.
