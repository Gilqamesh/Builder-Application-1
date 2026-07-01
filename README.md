# Builder-Application

Long-lived application workspace of **Builder**.

## Contents

- [Quick start](#quick-start)
- [Notable modules](#notable-modules)
- [Requirements](#requirements)
- [License](#license)

## Quick start

1. **Clone the workspace**

   ```bash
   git clone <repo-url> Builder-Application-1
   cd Builder-Application-1
   ```

2. **Bootstrap the Builder CLI**

   ```bash
   make -f foundation/m03gagbhst621faiop1rztfkqp_builder_cli/bootstrap.mk
   ```

3. **Run a module with optional arguments pass-through**

   ```bash
   ./cli <module-name> [<args>...]
   ```
   ```bash
   ./cli m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer m03ge9ij4lbns2mq6722cd8654_function_visualizer out.svg
   ```
   ```bash
   ./cli m03gagbht5685jfnokvj7crv2c_create_module tools some_module_name
   ```

## Notable modules

- `m03gagbht5685jfnokvj7crv2c_create_module` - creates a new module in a workspace.
- `m03ge9zyrjajugagmp61034qhi_module_dependency_ir_svg_renderer` - renders a module dependency graph to SVG.
- `m03ge9ij4lbns2mq6722cd8654_function_visualizer` - infinitely zoomable prototype to visualize m03ge9ij46lc986vpdamnc2fka_function_ir.

## Requirements

- Requirements of [Builder](https://github.com/Gilqamesh/Builder)

## License
MIT - see `LICENSE`.
