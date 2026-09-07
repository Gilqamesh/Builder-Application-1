#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>
#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gt1djvvy5atia5evkbg6rqy_software_shader/api.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace byte_stream = m03gagbht2l61mj6qitacwbmea_byte_stream;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;
namespace software_shader = m03gt1djvvy5atia5evkbg6rqy_software_shader;
namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
namespace texture = m03gt0l0q3l4b1k27eab5k7py1_texture;

namespace {

using vector2f_t = shader::vector_t<float, 2>;
using vector3f_t = shader::vector_t<float, 3>;
using vector4f_t = shader::vector_t<float, 4>;
using matrix2f_t = shader::matrix_t<float, 2, 2>;
using matrix2x3f_t = shader::matrix_t<float, 2, 3>;
using matrix3x2f_t = shader::matrix_t<float, 3, 2>;
using matrix4f_t = shader::matrix_t<float, 4, 4>;

template <typename T>
concept accepts_temporary_texture = requires(software_shader::bindings_t& bindings, T&& value) {
    bindings.texture(0, std::forward<T>(value));
};

template <typename T>
concept accepts_temporary_sampler = requires(software_shader::bindings_t& bindings, T&& value) {
    bindings.sampler(0, std::forward<T>(value));
};

void expect_near(float actual, float expected) {
    test::expect([](float lhs, float rhs) { return std::abs(lhs - rhs) < 0.00001F; }, actual, expected);
}

void expect_vector_near(const vector2f_t& actual, const vector2f_t& expected) {
    expect_near(actual[0], expected[0]);
    expect_near(actual[1], expected[1]);
}

shader::shader_ast_t trivial_fragment() {
    shader::fragment_shader_ast_builder_t fragment;
    fragment.output(0, vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
    return std::move(fragment).finalize();
}

texture::texture_t single_texel(std::array<std::uint8_t, 4> color) {
    std::vector<std::byte> bytes;
    bytes.reserve(color.size());
    for (const auto component : color) {
        bytes.push_back(static_cast<std::byte>(component));
    }
    return texture::texture_t(
        texture::format_t::rgba8_unorm,
        1,
        1,
        byte_stream::byte_stream_t(std::move(bytes))
    );
}


void test_explicit_lod_execution() {
    texture::texture_t target(texture::texture_description_t {texture::format_t::rgba8_unorm, 2, 1, 2},
        byte_stream::byte_stream_t(std::vector<std::byte>{std::byte{255},std::byte{0},std::byte{0},std::byte{255},std::byte{0},std::byte{0},std::byte{255},std::byte{255}}));
    target.generate_mipmaps();
    const texture::sampler_t sampler(texture::sampler_description_t {texture::filter_t::nearest, texture::filter_t::nearest, texture::filter_t::linear});
    shader::vertex_shader_ast_builder_t vertex;
    vertex.position(vector4f_t({0,0,0,1}));
    vertex.output(0, shader::sample_lod(vertex.resource<shader::shader_texture_2d_t>(0), vertex.resource<shader::shader_sampler_t>(0), vector2f_t({0.25F,0.5F}), vertex.uniform<float>(0)));
    shader::fragment_shader_ast_builder_t fragment;
    fragment.output(0, fragment.input<vector4f_t>(0));
    fragment.color(shader::sample_lod(fragment.resource<shader::shader_texture_2d_t>(0), fragment.resource<shader::shader_sampler_t>(0), vector2f_t({0.25F,0.5F}), fragment.uniform<float>(0)));
    software_shader::program_t program(std::move(vertex).finalize(), std::move(fragment).finalize());
    software_shader::bindings_t bindings;
    bindings.texture(0, target); bindings.sampler(0, sampler);
    software_shader::vertex_io_t vertex_io(0,0);
    software_shader::fragment_io_t fragment_io(vector4f_t(0), true);
    for (float lod : {0.0F, 0.5F, 1.0F, 9.0F}) {
        bindings.uniform(0, lod);
        program.run(bindings, vertex_io);
        const auto expected = texture::sample_lod(target, sampler, vector2f_t({0.25F,0.5F}), lod);
        fragment_io.input(0, *vertex_io.output<vector4f_t>(0));
        program.run(bindings, fragment_io);
        for (std::size_t component = 0; component < 4; ++component) {
            expect_near((*vertex_io.output<vector4f_t>(0))[component], expected[component]);
            expect_near((*fragment_io.color())[component], expected[component]);
        }
    }
    bindings.uniform(0, std::numeric_limits<float>::quiet_NaN());
    test::expect_throws([&] { program.run(bindings, vertex_io); });
    test::expect(std::identity(), !vertex_io.output<vector4f_t>(0));
    test::expect_throws([&] { program.run(bindings, fragment_io); });
    test::expect(std::identity(), !fragment_io.color());
}

void test_program_link_validation() {
    test::expect_throws<std::invalid_argument>([] {
        shader::fragment_shader_ast_builder_t first;
        first.output(0, 1.0F);
        shader::fragment_shader_ast_builder_t second;
        second.output(0, 1.0F);
        [[maybe_unused]] software_shader::program_t program(
            std::move(first).finalize(),
            std::move(second).finalize()
        );
    });

    test::expect_throws<std::invalid_argument>([] {
        shader::vertex_shader_ast_builder_t vertex;
        vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
        vertex.output(0, 1.0F);

        shader::fragment_shader_ast_builder_t fragment;
        fragment.output(0, fragment.input<std::int32_t>(0));

        [[maybe_unused]] software_shader::program_t program(
            std::move(vertex).finalize(),
            std::move(fragment).finalize()
        );
    });

    test::expect_throws<std::invalid_argument>([] {
        shader::vertex_shader_ast_builder_t vertex;
        vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
        vertex.output(0, vertex.uniform<float>(0));

        shader::fragment_shader_ast_builder_t fragment;
        fragment.output(0, fragment.uniform<std::int32_t>(0));

        [[maybe_unused]] software_shader::program_t program(
            std::move(vertex).finalize(),
            std::move(fragment).finalize()
        );
    });
}

void test_vertex_execution_is_fresh_and_reads_current_state() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector4f_t>(0);
    const auto conditional_output = vertex.input<bool>(1);
    const auto increment = vertex.input<std::int32_t>(2);
    const auto optional = vertex.input<float>(3);
    const auto local = vertex.local(std::int32_t(0));
    const auto current_sum = local + increment;

    vertex.position(position);
    vertex.loop(local < std::int32_t(5), [&] {
        vertex.assign(local, local + std::int32_t(1));
        vertex.branch(local == std::int32_t(2), [&] { vertex.continue_loop(); });
        vertex.branch(local == std::int32_t(4), [&] { vertex.break_loop(); });
    });
    vertex.output(0, conditional_output);
    vertex.output(1, current_sum);
    vertex.branch(conditional_output, [&] { vertex.output(2, optional); });

    shader::fragment_shader_ast_builder_t fragment;
    fragment.output(0, fragment.input<bool>(0));

    software_shader::program_t program(
        std::move(vertex).finalize(),
        std::move(fragment).finalize()
    );
    software_shader::bindings_t bindings;
    software_shader::vertex_io_t io(7, 3);
    io.input(0, vector4f_t({1.0F, 2.0F, 3.0F, 1.0F}));
    io.input(1, true);
    io.input(2, std::int32_t(6));
    io.input(3, 9.0F);

    program.run(bindings, io);
    test::expect(std::equal_to<>(), io.position(), vector4f_t({1.0F, 2.0F, 3.0F, 1.0F}));
    test::expect(std::identity(), io.output<bool>(0).has_value());
    test::expect(std::equal_to<>(), *io.output<std::int32_t>(1), std::int32_t(10));
    test::expect(std::equal_to<>(), *io.output<float>(2), 9.0F);

    io.input(1, false);
    io.input(2, std::int32_t(1));
    program.run(bindings, io);
    test::expect(std::equal_to<>(), *io.output<std::int32_t>(1), std::int32_t(5));
    test::expect(std::logical_not<>(), io.output<float>(2).has_value());
}

void test_vertex_transform_state_defaults_overrides_and_reset() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto local = vertex.input<vector4f_t>(0);
    vertex.position(vertex.world_to_clip() * vertex.object_to_world() * local);
    vertex.output(7, vertex.object_to_world());
    vertex.output(19, vertex.world_to_clip());

    software_shader::program_t program(std::move(vertex).finalize(), trivial_fragment());
    software_shader::bindings_t bindings;
    software_shader::vertex_io_t io(3, 2);
    io.input(0, vector4f_t({1.0F, 2.0F, 3.0F, 1.0F}));
    program.run(bindings, io);
    test::expect(std::equal_to<>(), io.position(), vector4f_t({1.0F, 2.0F, 3.0F, 1.0F}));

    const matrix4f_t object_to_world {
        1.0F, 0.0F, 0.0F, 2.0F,
        0.0F, 1.0F, 0.0F, 3.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };
    const matrix4f_t world_to_clip {
        2.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 3.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 4.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };
    io.object_to_world(object_to_world);
    io.world_to_clip(world_to_clip);
    io.input(0, vector4f_t({1.0F, 2.0F, 3.0F, 1.0F}));
    program.run(bindings, io);
    test::expect(std::equal_to<>(), io.position(), vector4f_t({6.0F, 15.0F, 12.0F, 1.0F}));
    test::expect(std::equal_to<>(), *io.output<matrix4f_t>(7), object_to_world);
    test::expect(std::equal_to<>(), *io.output<matrix4f_t>(19), world_to_clip);

    io.reset(11, 5);
    test::expect(std::equal_to<>(), io.object_to_world(), object_to_world);
    test::expect(std::equal_to<>(), io.world_to_clip(), world_to_clip);
    test::expect_throws<std::invalid_argument>([&] { program.run(bindings, io); });
    test::expect(std::logical_not<>(), io.output<matrix4f_t>(7).has_value());
}

void test_vertex_position_is_required_on_the_executed_path() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto writes_position = vertex.input<bool>(0);
    vertex.branch(writes_position, [&] {
        vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
    });
    vertex.output(0, writes_position);

    shader::fragment_shader_ast_builder_t fragment;
    fragment.output(0, fragment.input<bool>(0));

    software_shader::program_t program(
        std::move(vertex).finalize(),
        std::move(fragment).finalize()
    );
    software_shader::bindings_t bindings;
    software_shader::vertex_io_t io(0, 0);
    io.input(0, false);

    test::expect_throws<std::runtime_error>([&] { program.run(bindings, io); });
    test::expect_throws<std::logic_error>([&] { (void)io.position(); });
    test::expect(std::logical_not<>(), io.output<bool>(0).has_value());
}

void test_fragment_discard_terminates_and_invalidates_outputs() {
    shader::vertex_shader_ast_builder_t vertex;
    vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
    vertex.output(0, vertex.input<bool>(0));

    shader::fragment_shader_ast_builder_t fragment;
    const auto should_discard = fragment.input<bool>(0);
    fragment.color(vector4f_t({1.0F, 0.0F, 0.0F, 1.0F}));
    fragment.output(0, vector4f_t({1.0F, 0.0F, 0.0F, 1.0F}));
    fragment.branch(should_discard, [&] { fragment.discard(); });
    fragment.output(1, vector4f_t({0.0F, 1.0F, 0.0F, 1.0F}));
    fragment.output(2, fragment.fragment_coordinate());
    fragment.output(3, fragment.front_facing());

    software_shader::program_t program(
        std::move(vertex).finalize(),
        std::move(fragment).finalize()
    );
    software_shader::bindings_t bindings;
    software_shader::fragment_io_t io(vector4f_t({10.5F, 20.5F, 0.0F, 1.0F}), true);
    io.input(0, true);

    program.run(bindings, io);
    test::expect(std::identity(), io.discarded());
    test::expect(std::logical_not<>(), io.color().has_value());
    test::expect(std::logical_not<>(), io.output<vector4f_t>(0).has_value());
    test::expect(std::logical_not<>(), io.output<vector4f_t>(1).has_value());

    io.input(0, false);
    program.run(bindings, io);
    test::expect(std::logical_not<>(), io.discarded());
    test::expect(std::equal_to<>(), *io.color(), vector4f_t({1.0F, 0.0F, 0.0F, 1.0F}));
    test::expect(std::identity(), io.output<vector4f_t>(0).has_value());
    test::expect(std::identity(), io.output<vector4f_t>(1).has_value());
    test::expect(std::equal_to<>(), *io.output<vector4f_t>(2), vector4f_t({10.5F, 20.5F, 0.0F, 1.0F}));
    test::expect(std::identity(), *io.output<bool>(3));

    io.reset(vector4f_t({1.5F, 2.5F, 0.25F, 1.0F}), false);
    test::expect_throws<std::invalid_argument>([&] { program.run(bindings, io); });
    test::expect(std::logical_not<>(), io.discarded());
    test::expect(std::logical_not<>(), io.color().has_value());
    io.input(0, false);
    program.run(bindings, io);
    test::expect(std::equal_to<>(), *io.output<vector4f_t>(2), vector4f_t({1.5F, 2.5F, 0.25F, 1.0F}));
    test::expect(std::logical_not<>(), *io.output<bool>(3));
}

void test_bindings_own_uniforms_and_borrow_separate_resources() {
    static_assert(!accepts_temporary_texture<texture::texture_t>);
    static_assert(!accepts_temporary_sampler<texture::sampler_t>);

    shader::vertex_shader_ast_builder_t vertex;
    const auto coordinates = vertex.input<vector2f_t>(0);
    const auto uniform = vertex.uniform<float>(0);
    const auto texture_resource = vertex.resource<shader::shader_texture_2d_t>(0);
    const auto sampler_resource = vertex.resource<shader::shader_sampler_t>(0);
    vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
    vertex.output(0, uniform);
    vertex.output(1, shader::sample(texture_resource, sampler_resource, coordinates));

    shader::fragment_shader_ast_builder_t fragment;
    const auto input = fragment.input<float>(0);
    const auto sampled = fragment.input<vector4f_t>(1);
    const auto fragment_uniform = fragment.uniform<float>(0);
    const auto fragment_texture = fragment.resource<shader::shader_texture_2d_t>(0);
    const auto fragment_sampler = fragment.resource<shader::shader_sampler_t>(0);
    fragment.output(0, fragment.construct<vector4f_t>(fragment_uniform, input, 0.0F, 1.0F));
    fragment.output(1, shader::sample(fragment_texture, fragment_sampler, vector2f_t({0.5F, 0.5F})));
    fragment.output(2, sampled);

    software_shader::program_t program(
        std::move(vertex).finalize(),
        std::move(fragment).finalize()
    );
    auto image = single_texel({255, 128, 0, 255});
    const texture::sampler_t sampler(
        texture::filter_t::nearest,
        texture::address_mode_t::clamp_to_edge,
        texture::address_mode_t::clamp_to_edge
    );
    software_shader::bindings_t bindings;
    float source_uniform = 0.25F;
    bindings.uniform(0, source_uniform);
    bindings.texture(0, image);
    bindings.sampler(0, sampler);
    source_uniform = 0.75F;

    test::expect(std::equal_to<>(), bindings.uniform<float>(0), 0.25F);
    test::expect(std::identity(), &bindings.texture(0) == &image);
    test::expect(std::identity(), &bindings.sampler(0) == &sampler);

    software_shader::vertex_io_t vertex_io(0, 0);
    vertex_io.input(0, vector2f_t({0.5F, 0.5F}));
    program.run(bindings, vertex_io);
    test::expect(std::equal_to<>(), *vertex_io.output<float>(0), 0.25F);
    const auto color = *vertex_io.output<vector4f_t>(1);
    expect_near(color[0], 1.0F);
    expect_near(color[1], 128.0F / 255.0F);
    expect_near(color[2], 0.0F);
    expect_near(color[3], 1.0F);

    software_shader::fragment_io_t fragment_io(vector4f_t({0.5F, 0.5F, 0.0F, 1.0F}), true);
    fragment_io.input(0, *vertex_io.output<float>(0));
    fragment_io.input(1, color);
    program.run(bindings, fragment_io);
    test::expect(std::equal_to<>(), *fragment_io.output<vector4f_t>(1), color);
    test::expect(std::equal_to<>(), *fragment_io.output<vector4f_t>(2), color);

    software_shader::bindings_t wrong_bindings;
    wrong_bindings.uniform(0, std::int32_t(1));
    wrong_bindings.texture(0, image);
    wrong_bindings.sampler(0, sampler);
    test::expect_throws<std::invalid_argument>([&] { program.run(wrong_bindings, vertex_io); });
    test::expect(std::logical_not<>(), vertex_io.output<float>(0).has_value());

    bindings.clear_texture(0);
    test::expect_throws<std::invalid_argument>([&] { program.validate_bindings(bindings); });
    bindings.texture(0, image);
    bindings.clear_sampler(0);
    test::expect_throws<std::invalid_argument>([&] { program.validate_bindings(bindings); });
    bindings.sampler(0, sampler);
    test::expect_no_throw([&] { program.validate_bindings(bindings); });
}

template <typename T>
void expect_io_round_trip(
    software_shader::vertex_io_t& vertex_io,
    software_shader::fragment_io_t& fragment_io,
    std::uint32_t location,
    const T& value
) {
    vertex_io.input(location, value);
    vertex_io.output(location + 100, value);
    test::expect(std::equal_to<>(), vertex_io.input<T>(location), value);
    test::expect(std::equal_to<>(), *vertex_io.output<T>(location + 100), value);

    fragment_io.input(location, value);
    fragment_io.output(location + 100, value);
    test::expect(std::equal_to<>(), fragment_io.input<T>(location), value);
    test::expect(std::equal_to<>(), *fragment_io.output<T>(location + 100), value);
}

void test_complete_io_value_storage() {
    software_shader::vertex_io_t vertex_io(0, 0);
    software_shader::fragment_io_t fragment_io(vector4f_t(0.0F), true);
    std::uint32_t location = 0;
    const auto check = [&](const auto& value) {
        expect_io_round_trip(vertex_io, fragment_io, location, value);
        location += 3;
    };

    check(false);
    check(std::int32_t(-7));
    check(std::uint32_t(9));
    check(1.25F);
    check(shader::vector_t<bool, 2>({true, false}));
    check(shader::vector_t<bool, 3>({true, false, true}));
    check(shader::vector_t<bool, 4>({true, false, true, false}));
    check(shader::vector_t<std::int32_t, 2>({-1, 2}));
    check(shader::vector_t<std::int32_t, 3>({-1, 2, -3}));
    check(shader::vector_t<std::int32_t, 4>({-1, 2, -3, 4}));
    check(shader::vector_t<std::uint32_t, 2>({1, 2}));
    check(shader::vector_t<std::uint32_t, 3>({1, 2, 3}));
    check(shader::vector_t<std::uint32_t, 4>({1, 2, 3, 4}));
    check(shader::vector_t<float, 2>({1.0F, 2.0F}));
    check(shader::vector_t<float, 3>({1.0F, 2.0F, 3.0F}));
    check(shader::vector_t<float, 4>({1.0F, 2.0F, 3.0F, 4.0F}));
    check(shader::matrix_t<float, 2, 2>({1.0F, 2.0F, 3.0F, 4.0F}));
    check(shader::matrix_t<float, 2, 3>({1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F}));
    check(shader::matrix_t<float, 2, 4>({1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F}));
    check(shader::matrix_t<float, 3, 2>({1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F}));
    check(shader::matrix_t<float, 3, 3>({1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F}));
    check(shader::matrix_t<float, 3, 4>({1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F, 12.0F}));
    check(shader::matrix_t<float, 4, 2>({1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F}));
    check(shader::matrix_t<float, 4, 3>({1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F, 12.0F}));
    check(shader::matrix_t<float, 4, 4>({1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F, 12.0F, 13.0F, 14.0F, 15.0F, 16.0F}));
}

void test_value_operations_and_builtins() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto matrix = vertex.input<matrix2f_t>(0);
    const auto vector = vertex.input<vector2f_t>(1);
    const auto scalar = vertex.input<float>(2);
    const auto assembled = vertex.construct<vector4f_t>(9.0F, vector, 8.0F);

    vertex.position(vertex.construct<vector4f_t>(scalar, 0.0F, 0.0F, 1.0F));
    vertex.output(0, matrix * vector);
    vertex.output(1, matrix * matrix);
    vertex.output(2, shader::swizzle<3, 1>(assembled));
    vertex.output(3, shader::normalize(vector));
    vertex.output(4, shader::dot(vector, vector));
    vertex.output(5, shader::clamp(vector, 5.5F, 5.75F));
    vertex.output(6, shader::mix(vector, vertex.constant(vector2f_t({7.0F, 8.0F})), 0.5F));
    vertex.output(7, shader::smoothstep(5.0F, 7.0F, vector));
    vertex.output(8, shader::cross(
        vertex.constant(vector3f_t({1.0F, 0.0F, 0.0F})),
        vertex.constant(vector3f_t({0.0F, 1.0F, 0.0F}))
    ));
    vertex.output(9, vertex.vertex_index());
    vertex.output(10, vertex.instance_index());

    software_shader::program_t program(std::move(vertex).finalize(), trivial_fragment());
    software_shader::bindings_t bindings;
    software_shader::vertex_io_t io(12, 4);
    io.input(0, matrix2f_t({1.0F, 2.0F, 3.0F, 4.0F}));
    io.input(1, vector2f_t({5.0F, 6.0F}));
    io.input(2, 2.0F);
    program.run(bindings, io);

    test::expect(std::equal_to<>(), *io.output<vector2f_t>(0), vector2f_t({17.0F, 39.0F}));
    test::expect(std::equal_to<>(), *io.output<matrix2f_t>(1), matrix2f_t({7.0F, 10.0F, 15.0F, 22.0F}));
    test::expect(std::equal_to<>(), *io.output<vector2f_t>(2), vector2f_t({8.0F, 5.0F}));
    expect_vector_near(*io.output<vector2f_t>(3), vector2f_t({5.0F / std::sqrt(61.0F), 6.0F / std::sqrt(61.0F)}));
    expect_near(*io.output<float>(4), 61.0F);
    test::expect(std::equal_to<>(), *io.output<vector2f_t>(5), vector2f_t({5.5F, 5.75F}));
    test::expect(std::equal_to<>(), *io.output<vector2f_t>(6), vector2f_t({6.0F, 7.0F}));
    test::expect(std::equal_to<>(), *io.output<vector2f_t>(7), vector2f_t({0.0F, 0.5F}));
    test::expect(std::equal_to<>(), *io.output<vector3f_t>(8), vector3f_t({0.0F, 0.0F, 1.0F}));
    test::expect(std::equal_to<>(), *io.output<std::int32_t>(9), std::int32_t(12));
    test::expect(std::equal_to<>(), *io.output<std::int32_t>(10), std::int32_t(4));

    test::expect(std::identity(), !std::format("{}", bindings).empty());
    test::expect(std::identity(), !std::format("{}", io).empty());
    test::expect(std::identity(), !std::format("{}", program).empty());
}

void test_rectangular_matrix_operations() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto matrix = vertex.input<matrix2x3f_t>(0);
    const auto rhs = vertex.input<matrix3x2f_t>(1);
    const auto vector = vertex.input<vector3f_t>(2);
    const auto scalar = vertex.input<float>(3);
    vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
    vertex.output(0, matrix * rhs);
    vertex.output(1, matrix * vector);
    vertex.output(2, matrix * scalar);

    software_shader::program_t program(std::move(vertex).finalize(), trivial_fragment());
    software_shader::bindings_t bindings;
    software_shader::vertex_io_t io(0, 0);
    io.input(0, matrix2x3f_t({1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F}));
    io.input(1, matrix3x2f_t({7.0F, 8.0F, 9.0F, 10.0F, 11.0F, 12.0F}));
    io.input(2, vector3f_t({1.0F, 2.0F, 3.0F}));
    io.input(3, 2.0F);
    program.run(bindings, io);

    test::expect(std::equal_to<>(), *io.output<matrix2f_t>(0), matrix2f_t({58.0F, 64.0F, 139.0F, 154.0F}));
    test::expect(std::equal_to<>(), *io.output<vector2f_t>(1), vector2f_t({14.0F, 32.0F}));
    test::expect(std::equal_to<>(), *io.output<matrix2x3f_t>(2), matrix2x3f_t({2.0F, 4.0F, 6.0F, 8.0F, 10.0F, 12.0F}));
}

void test_scalar_operations() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto floating = vertex.input<float>(0);
    const auto integer = vertex.input<std::int32_t>(1);
    const auto unsigned_integer = vertex.input<std::uint32_t>(2);
    const auto boolean = vertex.input<bool>(3);
    const auto vector = vertex.input<vector2f_t>(4);

    vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
    vertex.output(0, floating + 1.0F);
    vertex.output(1, floating - 1.0F);
    vertex.output(2, floating * 2.0F);
    vertex.output(3, floating / 2.0F);
    vertex.output(4, integer % std::int32_t(3));
    vertex.output(5, -unsigned_integer);
    vertex.output(6, -floating);
    vertex.output(7, !boolean);
    vertex.output(8, shader::abs(integer));
    vertex.output(9, shader::sqrt(vertex.constant(4.0F)));
    vertex.output(10, shader::floor(floating));
    vertex.output(11, shader::ceil(floating));
    vertex.output(12, shader::fract(floating));
    vertex.output(13, shader::sin(vertex.constant(0.0F)));
    vertex.output(14, shader::cos(vertex.constant(0.0F)));
    vertex.output(15, shader::length(vector));
    vertex.output(16, shader::pow(floating, 2.0F));
    vertex.output(17, shader::reflect(floating, vertex.constant(1.0F)));
    vertex.output(18, shader::min(floating, 2.0F));
    vertex.output(19, shader::max(floating, 3.0F));
    vertex.output(20, shader::step(2.0F, floating));
    vertex.output(21, floating == 2.25F);
    vertex.output(22, floating != 2.25F);
    vertex.output(23, floating < 3.0F);
    vertex.output(24, floating <= 2.25F);
    vertex.output(25, floating > 2.0F);
    vertex.output(26, floating >= 2.25F);
    vertex.output(27, boolean && true);
    vertex.output(28, boolean || true);

    software_shader::program_t program(std::move(vertex).finalize(), trivial_fragment());
    software_shader::bindings_t bindings;
    software_shader::vertex_io_t io(0, 0);
    io.input(0, 2.25F);
    io.input(1, std::int32_t(-5));
    io.input(2, std::uint32_t(5));
    io.input(3, false);
    io.input(4, vector2f_t({3.0F, 4.0F}));
    program.run(bindings, io);

    expect_near(*io.output<float>(0), 3.25F);
    expect_near(*io.output<float>(1), 1.25F);
    expect_near(*io.output<float>(2), 4.5F);
    expect_near(*io.output<float>(3), 1.125F);
    test::expect(std::equal_to<>(), *io.output<std::int32_t>(4), std::int32_t(-2));
    test::expect(std::equal_to<>(), *io.output<std::uint32_t>(5), std::uint32_t(0) - std::uint32_t(5));
    expect_near(*io.output<float>(6), -2.25F);
    test::expect(std::identity(), *io.output<bool>(7));
    test::expect(std::equal_to<>(), *io.output<std::int32_t>(8), std::int32_t(5));
    expect_near(*io.output<float>(9), 2.0F);
    expect_near(*io.output<float>(10), 2.0F);
    expect_near(*io.output<float>(11), 3.0F);
    expect_near(*io.output<float>(12), 0.25F);
    expect_near(*io.output<float>(13), 0.0F);
    expect_near(*io.output<float>(14), 1.0F);
    expect_near(*io.output<float>(15), 5.0F);
    expect_near(*io.output<float>(16), 5.0625F);
    expect_near(*io.output<float>(17), -2.25F);
    expect_near(*io.output<float>(18), 2.0F);
    expect_near(*io.output<float>(19), 3.0F);
    expect_near(*io.output<float>(20), 1.0F);
    test::expect(std::identity(), *io.output<bool>(21));
    test::expect(std::logical_not<>(), *io.output<bool>(22));
    test::expect(std::identity(), *io.output<bool>(23));
    test::expect(std::identity(), *io.output<bool>(24));
    test::expect(std::identity(), *io.output<bool>(25));
    test::expect(std::identity(), *io.output<bool>(26));
    test::expect(std::logical_not<>(), *io.output<bool>(27));
    test::expect(std::identity(), *io.output<bool>(28));
}

} // namespace

int main() {
    return test::run([] {
        test_explicit_lod_execution();
        test_program_link_validation();
        test_vertex_execution_is_fresh_and_reads_current_state();
        test_vertex_transform_state_defaults_overrides_and_reset();
        test_vertex_position_is_required_on_the_executed_path();
        test_fragment_discard_terminates_and_invalidates_outputs();
        test_bindings_own_uniforms_and_borrow_separate_resources();
        test_complete_io_value_storage();
        test_value_operations_and_builtins();
        test_rectangular_matrix_operations();
        test_scalar_operations();
    });
}
