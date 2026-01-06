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
   git clone https://github.com/Gilqamesh/Builder-Application-1.git
   cd Builder-Application-1
   git submodule update --init --recursive
   ```

2. **Compile cli.cpp**

   ```bash
   clang++ -std=c++23 cli.cpp -o cli
   ```

3. **Run cli on the target module with optional arguments pass-through**

   ```bash
   ./cli F # builds module F, i.e., runs its builder.cpp implementation
   ./cli F f_shared # also run the produced `f_shared` binary of the `F` module
   ```

## Contributing
Issues and pull requests are welcome. Please describe:
- What changed and why.
- How you tested (commands/output).
- Any user-facing impact.

## Requirements

- Requirements of [Builder](https://github.com/Gilqamesh/Builder)

## License
MIT - see `LICENSE`.
