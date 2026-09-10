# Location-based and prepared execution

[program_t and prepared_program_t](../software_shader.h) own the execution contract;
[invocation.h](../invocation.h) owns binding and IO lifetimes. This complete caller
runs a linked pair through both interfaces. Location-based calls use declared
location numbers. Prepared calls use dense spans in each stage's reflection order,
including optional slots for numbered outputs that might not be written. They use
the IO object only for built-ins and special results.

The program, texture and sampler stay alive at stable addresses while prepared
execution borrows them. Uniforms are copied into bindings and then copied again at
preparation; texture contents remain live. Replacing a binding requires preparation
again. Sequential runs reuse the context and IO; simultaneous runs need independent
context, IO and output storage.

```cpp
#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>
#include <m03gsy25j4v7nccgmsdov9ioft_shader/shader_builder.h>
#include <m03gt0l0q3l4b1k27eab5k7py1_texture/texture.h>
#include <m03gt0l0q3l4b1k27eab5k7py1_texture/sampler.h>
#include <m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

int main() {
    namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
    namespace texture_api = m03gt0l0q3l4b1k27eab5k7py1_texture;
    namespace cpu = m03gt1djvvy5atia5evkbg6rqy_software_shader;
    using byte_stream_t = m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t;
    using vector2f_t = shader::vector_t<float, 2>;
    using vector4f_t = shader::vector_t<float, 4>;

    shader::vertex_shader_ast_builder_t vertex;
    vertex.position(vertex.input<vector4f_t>(7));
    vertex.output(9, vertex.uniform<float>(3));
    shader::fragment_shader_ast_builder_t fragment;
    const auto image = fragment.resource<shader::shader_texture_2d_t>(0);
    const auto filtering = fragment.resource<shader::shader_sampler_t>(0);
    fragment.color(shader::sample(image, filtering, vector2f_t{0.5F, 0.5F})
        * fragment.input<float>(9));
    cpu::program_t program(std::move(vertex).finalize(), std::move(fragment).finalize());

    texture_api::texture_t texture(texture_api::format_t::rgba8_unorm, 1, 1,
        byte_stream_t(std::vector<std::byte> {
            std::byte{255}, std::byte{0}, std::byte{0}, std::byte{255}}));
    const texture_api::sampler_t sampler(texture_api::sampler_description_t {});
    cpu::bindings_t bindings;
    float intensity = 0.25F;
    bindings.uniform(3, intensity); // Copies intensity.
    intensity = 1.0F; // Does not change the stored uniform.
    bindings.texture(0, texture); // Borrows; sampler index 0 is a separate namespace.
    bindings.sampler(0, sampler);

    cpu::execution_context_t context;
    cpu::vertex_io_t vertex_io(0, 0);
    cpu::fragment_io_t fragment_io(vector4f_t{0.5F, 0.5F, 0.5F, 1.0F}, true);
    const vector4f_t position {0, 0, 0, 1};

    // Location-based execution: inputs/outputs are addressed by declared location.
    vertex_io.input(7, position);
    program.run(bindings, vertex_io, context);
    assert(vertex_io.position() == position);
    const auto output = vertex_io.output<float>(9);
    assert(output && *output == 0.25F);
    fragment_io.input(9, *output); // The caller supplies the fragment invocation.
    program.run(bindings, fragment_io, context);
    assert((fragment_io.color() == vector4f_t{0.25F, 0, 0, 0.25F}));
    vertex_io.reset(1, 0); // Clears inputs/results; matrices would be preserved.
    vertex_io.input(7, position);
    program.run(bindings, vertex_io, context); // Reuses context and IO capacity.

    // Prepared execution: array index 0 maps to location 7, then output location 9.
    assert(program.vertex_interface().inputs()[0].index == 7);
    assert(program.vertex_interface().outputs()[0].index == 9);
    assert(program.fragment_interface().inputs()[0].index == 9);
    cpu::prepared_program_t prepared_program;
    prepared_program.prepare(program, bindings); // Validates bindings for both stages.
    std::array<cpu::value_t, 1> vertex_inputs {position};
    std::array<std::optional<cpu::value_t>, 1> vertex_outputs;
    std::array<cpu::value_t, 1> fragment_inputs;
    std::array<std::optional<cpu::value_t>, 0> fragment_outputs; // Special color uses IO.
    prepared_program.run(vertex_inputs, vertex_outputs, vertex_io, context);
    assert(vertex_outputs[0] && std::get<float>(*vertex_outputs[0]) == 0.25F);
    assert(!vertex_io.output<float>(9)); // Indexed execution does not populate this.

    bindings.uniform(3, 0.5F);
    prepared_program.run(vertex_inputs, vertex_outputs, vertex_io, context);
    assert(std::get<float>(*vertex_outputs[0]) == 0.25F); // Old uniform snapshot.
    prepared_program.prepare(program, bindings);
    prepared_program.run(vertex_inputs, vertex_outputs, vertex_io, context);
    assert(std::get<float>(*vertex_outputs[0]) == 0.5F);
    fragment_inputs[0] = *vertex_outputs[program.fragment_sources()[0]];
    fragment_io.reset(vector4f_t{0.5F, 0.5F, 0.5F, 1.0F}, true);
    prepared_program.run(fragment_inputs, fragment_outputs, fragment_io, context);
    assert((fragment_io.color() == vector4f_t{0.5F, 0, 0, 0.5F}));

    const auto pixels = texture.view();
    pixels.bytes()[0] = std::byte{0};
    pixels.bytes()[1] = std::byte{255}; // Same borrowed texture is now green.
    prepared_program.run(fragment_inputs, fragment_outputs, fragment_io, context);
    assert((fragment_io.color() == vector4f_t{0, 0.5F, 0, 0.5F}));
    prepared_program.reset(); // Releases program/resource borrows, retains capacity.
}
```

These standalone stage calls do not rasterize or interpolate. A renderer supplies
those operations. Handle absent numbered results before forwarding them, and check
`fragment_io.discarded()`/`fragment_io.color()` when the shader can discard or omit
color. Failed runs clear results and leave the context reusable. Failed preparation
leaves the prepared program unprepared; correct bindings and prepare again before
running it. Prepared callers must supply each declared input's exact `value_t`
alternative; span-size checks do not provide full input type validation.
