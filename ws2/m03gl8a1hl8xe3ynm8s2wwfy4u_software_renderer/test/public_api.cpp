#include "raster_fixtures.h"

#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>
#include <m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays/api.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/framebuffer.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/software_renderer.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/helpers.h>
#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>
#include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>
#include <m03gt1djvvy5atia5evkbg6rqy_software_shader/api.h>
#include <m03gtgtrh2smvh28qlwgm7gdl4_quaternion/api.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <numbers>
#include <numeric>
#include <random>
#include <set>
#include <span>
#include <source_location>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace api = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
namespace raster = api;
namespace byte_stream = m03gagbht2l61mj6qitacwbmea_byte_stream;
namespace soa = m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;
namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
namespace texture = m03gt0l0q3l4b1k27eab5k7py1_texture;
namespace software_shader = m03gt1djvvy5atia5evkbg6rqy_software_shader;

using position_t = std::array<float, 2>;
using program_ptr_t = std::shared_ptr<const software_shader::program_t>;

constexpr api::rgba8_t clear_color {3, 5, 7, 11};
constexpr api::rgba8_t red {255, 0, 0, 255};
constexpr api::rgba8_t green {0, 255, 0, 255};
constexpr api::rgba8_t blue {0, 0, 255, 255};
constexpr api::rgba8_t white {255, 255, 255, 255};
constexpr api::rgba8_t texture_color {17, 34, 51, 68};

bool same_color(const api::rgba8_t& lhs, const api::rgba8_t& rhs) {
    return lhs.red == rhs.red
        && lhs.green == rhs.green
        && lhs.blue == rhs.blue
        && lhs.alpha == rhs.alpha;
}

void expect_color(const api::rgba8_t& actual, const api::rgba8_t& expected) {
    test::expect(std::identity(), same_color(actual, expected));
}

std::size_t pixel_index(int x, int y, int width) {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width)
        + static_cast<std::size_t>(x);
}

std::size_t colored_pixel_count(std::span<const api::rgba8_t> framebuffer) {
    return static_cast<std::size_t>(std::ranges::count_if(framebuffer, [](const auto& pixel) {
        return !same_color(pixel, clear_color);
    }));
}

std::shared_ptr<texture::sampler_t> make_sampler() {
    return std::make_shared<texture::sampler_t>(
        texture::filter_t::nearest,
        texture::address_mode_t::clamp_to_edge,
        texture::address_mode_t::clamp_to_edge
    );
}

std::shared_ptr<texture::texture_t> make_unorm_texture(
    std::size_t width,
    std::size_t height,
    std::span<const api::rgba8_t> texels
) {
    std::vector<std::byte> bytes;
    bytes.reserve(texels.size() * sizeof(api::rgba8_t));
    for (const auto texel : texels) {
        bytes.push_back(static_cast<std::byte>(texel.red));
        bytes.push_back(static_cast<std::byte>(texel.green));
        bytes.push_back(static_cast<std::byte>(texel.blue));
        bytes.push_back(static_cast<std::byte>(texel.alpha));
    }
    return std::make_shared<texture::texture_t>(
        texture::format_t::rgba8_unorm,
        width,
        height,
        byte_stream::byte_stream_t(std::move(bytes))
    );
}

std::shared_ptr<texture::texture_t> make_unorm_texture(api::rgba8_t color) {
    const std::array texels {color};
    return make_unorm_texture(1, 1, texels);
}

std::shared_ptr<texture::texture_t> make_float_texture(std::array<float, 4> color) {
    return std::make_shared<texture::texture_t>(
        texture::format_t::rgba32_float,
        1,
        1,
        byte_stream::byte_stream_t(std::as_bytes(std::span<const float>(color)))
    );
}

program_ptr_t make_textured_program() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector2f_t>(0);
    const auto local = vertex.construct<vector4f_t>(position, 0.0F, 1.0F);
    vertex.position(vertex.world_to_clip() * vertex.object_to_world() * local);
    vertex.output(0, position * 0.5F + vector2f_t({0.5F, 0.5F}));

    shader::fragment_shader_ast_builder_t fragment;
    const auto coordinates = fragment.input<vector2f_t>(0);
    const auto image = fragment.resource<shader::shader_texture_2d_t>(0);
    const auto sampler = fragment.resource<shader::shader_sampler_t>(0);
    fragment.color(shader::sample(image, sampler, coordinates));

    return std::make_shared<const software_shader::program_t>(
        std::move(vertex).finalize(),
        std::move(fragment).finalize()
    );
}

program_ptr_t make_constant_program(vector4f_t color) {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector2f_t>(0);
    const auto local = vertex.construct<vector4f_t>(position, 0.0F, 1.0F);
    vertex.position(vertex.world_to_clip() * vertex.object_to_world() * local);

    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(color);

    return std::make_shared<const software_shader::program_t>(
        std::move(vertex).finalize(),
        std::move(fragment).finalize()
    );
}

std::shared_ptr<api::index_buffer_t> make_indices(api::index_buffer_t::indices_t values) {
    auto result = std::make_shared<api::index_buffer_t>();
    result->indices() = std::move(values);
    return result;
}

template <typename Position>
std::shared_ptr<api::geometry_t> make_typed_geometry(
    std::vector<Position> positions,
    api::vertex_attribute_t attribute,
    api::index_buffer_t::indices_t indices,
    api::vertex_primitive_topology_t topology
) {
    soa::structure_of_arrays_t<Position> streams;
    for (const auto& position : positions) {
        streams.push_back(position);
    }
    auto mesh = std::make_shared<api::mesh_t>(
        std::move(streams),
        std::vector<api::vertex_attribute_t> {attribute}
    );
    auto geometry = std::make_shared<api::geometry_t>(make_indices(std::move(indices)));
    geometry->mesh() = std::move(mesh);
    geometry->primitive_topology() = topology;
    geometry->finalize();
    return geometry;
}

std::shared_ptr<api::geometry_t> make_geometry(
    std::vector<position_t> clip_positions,
    api::index_buffer_t::indices_t indices,
    api::vertex_primitive_topology_t topology
) {
    for (auto& position : clip_positions) {
        position[1] = -position[1];
    }
    return make_typed_geometry(
        std::move(clip_positions),
        api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 2),
        std::move(indices),
        topology
    );
}

std::shared_ptr<api::material_t> make_material(
    std::shared_ptr<texture::texture_t> image,
    std::shared_ptr<texture::sampler_t> sampler = make_sampler(),
    program_ptr_t program = nullptr
) {
    if (!program) {
        program = make_textured_program();
    }
    auto material = std::make_shared<api::material_t>(std::move(program));
    material->texture(0, std::move(image));
    material->sampler(0, std::move(sampler));
    return material;
}

api::render_item_t make_render_item(
    std::shared_ptr<api::geometry_t> geometry,
    std::shared_ptr<api::material_t> material
) {
    api::render_item_t render_item;
    render_item.geometry() = std::move(geometry);
    render_item.material() = std::move(material);
    render_item.translation() = {0.0F, 0.0F, 0.0F};
    render_item.scale() = {1.0F, 1.0F, 1.0F};
    return render_item;
}

api::camera_t make_camera(int width, int height) {
    api::camera_t camera(
        {{0, width}, {0, height}},
        api::orthographic_t({{-1.0F, 1.0F}, {-1.0F, 1.0F}}, 0.0F, 2.0F)
    );
    // Explicit planar migration: this camera looks along +world Z, with world Y down.
    camera.position() = {0, 0, -1};
    camera.rotation(api::quaternion_t(0, 1, 0, 0));
    return camera;
}

std::vector<api::rgba8_t> draw_scene(
    api::vertex_primitive_topology_t topology,
    std::vector<position_t> positions,
    api::index_buffer_t::indices_t indices,
    int width = 16,
    int height = 16,
    std::shared_ptr<texture::texture_t> image = make_unorm_texture(red)
) {
    std::vector<api::rgba8_t> pixels(
        api::framebuffer_t::pixel_count(width, height),
        clear_color
    );
    api::software_renderer_t renderer(api::framebuffer_t(pixels, width, height));
    const auto camera = make_camera(width, height);
    const auto render_item = make_render_item(
        make_geometry(std::move(positions), std::move(indices), topology),
        make_material(std::move(image))
    );
    renderer.draw(camera, render_item);
    return pixels;
}

void test_resource_model() {
    soa::structure_of_arrays_t<float> vertices;
    vertices.push_back(0.0F);
    vertices.push_back(1.0F);
    vertices.push_back(2.0F);
    const auto mesh = std::make_shared<api::mesh_t>(
        std::move(vertices),
        std::vector<api::vertex_attribute_t> {
            api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 1)
        }
    );
    test::expect(std::equal_to<>(), mesh->number_of_vertices(), std::size_t(3));

    const auto index_buffer = make_indices({0, 1, 2});
    api::geometry_t geometry(index_buffer);
    geometry.mesh() = mesh;
    test::expect_no_throw([&] { geometry.finalize(); });
    test::expect(std::equal_to<>(), geometry.indices().size(), std::size_t(3));

    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] const api::geometry_t invalid(nullptr);
    });
    test::expect_throws<std::out_of_range>([&] {
        [[maybe_unused]] const api::geometry_t invalid(
            index_buffer,
            api::index_range_t {.offset = 2, .count = 2}
        );
    });

    api::geometry_t missing_mesh(make_indices({0, 1, 2}));
    test::expect_throws<std::runtime_error>([&] { missing_mesh.finalize(); });

    api::geometry_t invalid_index(make_indices({0, 1, 3}));
    invalid_index.mesh() = mesh;
    test::expect_throws<std::runtime_error>([&] { invalid_index.finalize(); });

    api::geometry_t invalid_line(make_indices({0, 1, 2}));
    invalid_line.mesh() = mesh;
    invalid_line.primitive_topology() = api::vertex_primitive_topology_t::line;
    test::expect_throws<std::runtime_error>([&] { invalid_line.finalize(); });

    api::geometry_t mutable_range(index_buffer);
    index_buffer->indices().resize(1);
    test::expect_throws<std::out_of_range>([&] { (void)mutable_range.indices(); });

    soa::structure_of_arrays_t<float> overflow_vertices;
    overflow_vertices.push_back(0.0F);
    test::expect_throws<std::length_error>([&] {
        [[maybe_unused]] const api::mesh_t invalid(
            std::move(overflow_vertices),
            std::vector<api::vertex_attribute_t> {
                api::vertex_attribute_t(
                    api::vertex_attribute_type_t::R64,
                    std::numeric_limits<std::size_t>::max()
                )
            }
        );
    });
}

void test_framebuffer() {
    static_assert(!std::is_copy_constructible_v<api::software_renderer_t>);
    static_assert(!std::is_copy_assignable_v<api::software_renderer_t>);
    static_assert(!std::is_move_constructible_v<api::software_renderer_t>);
    static_assert(!std::is_move_assignable_v<api::software_renderer_t>);
    static_assert(std::is_same_v<decltype(std::declval<api::framebuffer_t&>().width()), int>);
    static_assert(std::is_same_v<decltype(std::declval<api::framebuffer_t&>().height()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const api::framebuffer_t&>().pixels()), std::span<api::rgba8_t>>);
    static_assert(std::is_same_v<decltype(std::declval<api::software_renderer_t&>().framebuffer()), api::framebuffer_t&>);
    static_assert(std::is_same_v<decltype(std::declval<const api::software_renderer_t&>().framebuffer()), const api::framebuffer_t&>);

    test::expect(std::equal_to<>(), api::framebuffer_t::pixel_count(3, 2), std::size_t(6));
    test::expect(std::equal_to<>(), api::framebuffer_t::pixel_count(0, 2), std::size_t(0));
    test::expect(std::equal_to<>(), api::framebuffer_t::pixel_count(3, 0), std::size_t(0));
    test::expect_throws<std::invalid_argument>([] { (void)api::framebuffer_t::pixel_count(-1, 2); });
    test::expect_throws<std::invalid_argument>([] { (void)api::framebuffer_t::pixel_count(0, -1); });

    constexpr int largest_dimension = std::numeric_limits<int>::max();
    constexpr auto largest_size = static_cast<std::size_t>(largest_dimension);
    if constexpr (std::numeric_limits<std::size_t>::max() / largest_size < largest_size) {
        test::expect_throws<std::length_error>([] {
            (void)api::framebuffer_t::pixel_count(largest_dimension, largest_dimension);
        });
        test::expect_throws<std::length_error>([] {
            [[maybe_unused]] const api::framebuffer_t invalid({}, largest_dimension, largest_dimension);
        });
    } else {
        test::expect(std::equal_to<>(), api::framebuffer_t::pixel_count(largest_dimension, largest_dimension), largest_size * largest_size);
    }

    std::vector<api::rgba8_t> pixels(6);
    test::expect_throws<std::invalid_argument>([&] {
        [[maybe_unused]] const api::framebuffer_t invalid(pixels, -1, 6);
    });
    test::expect_throws<std::invalid_argument>([&] {
        [[maybe_unused]] const api::framebuffer_t invalid({}, 0, -1);
    });
    test::expect_throws<std::invalid_argument>([&] {
        [[maybe_unused]] const api::framebuffer_t invalid(pixels, 2, 2);
    });
    test::expect_throws<std::invalid_argument>([&] {
        [[maybe_unused]] const api::framebuffer_t invalid(pixels, 4, 2);
    });
    test::expect_throws<std::invalid_argument>([&] {
        [[maybe_unused]] const api::framebuffer_t invalid(pixels, 0, 2);
    });

    api::software_renderer_t renderer(api::framebuffer_t(pixels, 3, 2));

    renderer.clear(clear_color);
    test::expect(std::identity(), std::ranges::all_of(pixels, [](const auto& pixel) {
        return same_color(pixel, clear_color);
    }));

    const auto& framebuffer = std::as_const(renderer).framebuffer();
    test::expect(std::identity(), framebuffer.pixels().data() == pixels.data());
    test::expect(std::equal_to<>(), framebuffer.width(), 3);
    test::expect(std::equal_to<>(), framebuffer.height(), 2);
    framebuffer.pixels()[0] = red;
    expect_color(pixels[0], red);
    auto borrowed_pixels = framebuffer.pixels();
    borrowed_pixels = borrowed_pixels.first(1);
    test::expect(std::equal_to<>(), framebuffer.pixels().size(), std::size_t(6));

    test::expect_throws<std::invalid_argument>([&] {
        renderer.framebuffer() = api::framebuffer_t(pixels, -1, 6);
    });
    test::expect_throws<std::invalid_argument>([&] {
        renderer.framebuffer() = api::framebuffer_t(pixels, 2, 2);
    });
    test::expect(std::identity(), renderer.framebuffer().pixels().data() == pixels.data());

    std::vector<api::rgba8_t> replacement(4);
    renderer.framebuffer() = api::framebuffer_t(replacement, 2, 2);
    test::expect(std::identity(), framebuffer.pixels().data() == replacement.data());
    test::expect(std::equal_to<>(), framebuffer.width(), 2);
    test::expect(std::equal_to<>(), framebuffer.height(), 2);
    renderer.clear(texture_color);
    test::expect(std::identity(), std::ranges::all_of(replacement, [](const auto& pixel) {
        return same_color(pixel, texture_color);
    }));
}

void test_empty_framebuffer() {
    std::vector<api::rgba8_t> pixels;
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 0, 4));
    test::expect_no_throw([&] { renderer.clear(clear_color); });

    const auto camera = make_camera(8, 8);
    const auto render_item = make_render_item(
        make_geometry({{0.0F, 0.0F}}, {0}, api::vertex_primitive_topology_t::point),
        make_material(make_unorm_texture(red))
    );
    test::expect_no_throw([&] { renderer.draw(camera, render_item); });
    renderer.framebuffer() = api::framebuffer_t(pixels, 4, 0);
    test::expect_no_throw([&] { renderer.clear(clear_color); });
    test::expect_no_throw([&] { renderer.draw(camera, render_item); });
    renderer.framebuffer() = api::framebuffer_t(pixels, 0, 0);
    test::expect_no_throw([&] { renderer.clear(clear_color); });
    test::expect_no_throw([&] { renderer.draw(camera, render_item); });
}

void test_topologies_and_clipping() {
    const auto point = draw_scene(
        api::vertex_primitive_topology_t::point,
        {{0.0F, 0.0F}},
        {0}
    );
    test::expect(std::equal_to<>(), colored_pixel_count(point), std::size_t(29));
    expect_color(point[pixel_index(8, 8, 16)], red);

    const auto clipped_point = draw_scene(
        api::vertex_primitive_topology_t::point,
        {{2.0F, 0.0F}},
        {0}
    );
    test::expect(std::equal_to<>(), colored_pixel_count(clipped_point), std::size_t(0));

    const auto line = draw_scene(
        api::vertex_primitive_topology_t::line,
        {{-2.0F, 0.0F}, {0.5F, 0.0F}},
        {0, 1}
    );
    expect_color(line[pixel_index(0, 8, 16)], red);
    expect_color(line[pixel_index(12, 8, 16)], red);
    expect_color(line[pixel_index(15, 8, 16)], clear_color);

    const std::vector<position_t> line_vertices {
        {-0.75F, -0.5F},
        {0.0F, 0.5F},
        {0.75F, -0.5F}
    };
    const auto line_strip = draw_scene(
        api::vertex_primitive_topology_t::line_strip,
        line_vertices,
        {0, 1, 2}
    );
    test::expect(std::greater<>(), colored_pixel_count(line_strip), std::size_t(12));

    const auto line_loop = draw_scene(
        api::vertex_primitive_topology_t::line_loop,
        line_vertices,
        {0, 1, 2}
    );
    test::expect(std::greater<>(), colored_pixel_count(line_loop), colored_pixel_count(line_strip));
    expect_color(line_loop[pixel_index(8, 12, 16)], red);

    const auto clipped_triangle = draw_scene(
        api::vertex_primitive_topology_t::triangle,
        {{-2.0F, -0.75F}, {0.75F, -0.75F}, {0.0F, 0.75F}},
        {0, 1, 2}
    );
    test::expect(std::greater<>(), colored_pixel_count(clipped_triangle), std::size_t(0));
    expect_color(clipped_triangle[pixel_index(8, 8, 16)], red);

    const auto triangle_strip = draw_scene(
        api::vertex_primitive_topology_t::triangle_strip,
        {{-0.75F, -0.75F}, {-0.75F, 0.75F}, {0.75F, -0.75F}, {0.75F, 0.75F}},
        {0, 1, 2, 3}
    );
    expect_color(triangle_strip[pixel_index(8, 8, 16)], red);
    expect_color(triangle_strip[pixel_index(12, 4, 16)], red);

    const auto triangle_fan = draw_scene(
        api::vertex_primitive_topology_t::triangle_fan,
        {{-0.75F, -0.75F}, {0.75F, -0.75F}, {0.75F, 0.75F}, {-0.75F, 0.75F}},
        {0, 1, 2, 3}
    );
    expect_color(triangle_fan[pixel_index(8, 8, 16)], red);
    expect_color(triangle_fan[pixel_index(4, 4, 16)], red);
}

void test_shared_edge_coverage() {
    const auto framebuffer = draw_scene(
        api::vertex_primitive_topology_t::triangle_strip,
        {{-1.0F, -1.0F}, {-1.0F, 1.0F}, {1.0F, -1.0F}, {1.0F, 1.0F}},
        {0, 1, 2, 3}
    );
    test::expect(std::equal_to<>(), colored_pixel_count(framebuffer), framebuffer.size());
    test::expect(std::identity(), std::ranges::all_of(framebuffer, [](const auto& pixel) {
        return same_color(pixel, red);
    }));
}

void test_texture_coordinate_interpolation() {
    const std::array texels {red, green, blue, white};
    const auto framebuffer = draw_scene(
        api::vertex_primitive_topology_t::triangle_strip,
        {{-1.0F, -1.0F}, {-1.0F, 1.0F}, {1.0F, -1.0F}, {1.0F, 1.0F}},
        {0, 1, 2, 3},
        8,
        8,
        make_unorm_texture(2, 2, texels)
    );
    expect_color(framebuffer[pixel_index(1, 1, 8)], red);
    expect_color(framebuffer[pixel_index(6, 1, 8)], green);
    expect_color(framebuffer[pixel_index(1, 6, 8)], blue);
    expect_color(framebuffer[pixel_index(6, 6, 8)], white);

    std::vector<api::rgba8_t> transformed_pixels(32 * 32, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(transformed_pixels, 32, 32));
    api::render_item_t transformed;
    transformed.geometry() = make_geometry(
        {{-1.0F, -1.0F}, {-1.0F, 1.0F}, {1.0F, -1.0F}, {1.0F, 1.0F}},
        {0, 1, 2, 3},
        api::vertex_primitive_topology_t::triangle_strip
    );
    transformed.material() = make_material(make_unorm_texture(2, 2, texels));
    transformed.translation() = {0.25F, 0.25F, 0.0F};
    transformed.scale() = {0.5F, 0.5F, 1.0F};
    renderer.draw(make_camera(32, 32), transformed);
    expect_color(transformed_pixels[pixel_index(14, 14, 32)], red);
    expect_color(transformed_pixels[pixel_index(26, 14, 32)], green);
    expect_color(transformed_pixels[pixel_index(14, 26, 32)], blue);
    expect_color(transformed_pixels[pixel_index(26, 26, 32)], white);
}

void test_shared_material_transform_semantics() {
    std::vector<api::rgba8_t> pixels(64 * 64, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 64, 64));
    const auto camera = make_camera(64, 64);
    const auto geometry = make_geometry(
        {{0.0F, 0.0F}},
        {0},
        api::vertex_primitive_topology_t::point
    );
    const auto material = std::make_shared<api::material_t>(
        make_constant_program(vector4f_t({1.0F, 0.0F, 0.0F, 1.0F}))
    );
    auto left = make_render_item(geometry, material);
    left.translation() = {-0.5F, 0.0F, 0.0F};
    auto right = make_render_item(geometry, material);
    right.translation() = {0.5F, 0.0F, 0.0F};
    renderer.draw(camera, left);
    renderer.draw(camera, right);
    expect_color(pixels[pixel_index(16, 32, 64)], red);
    expect_color(pixels[pixel_index(48, 32, 64)], red);

    renderer.clear(clear_color);
    auto trs = make_render_item(
        make_geometry({{0.25F, 0.0F}}, {0}, api::vertex_primitive_topology_t::point),
        material
    );
    trs.scale() = {2.0F, 1.0F, 1.0F};
    trs.rotation(vector3f_t({0, 0, std::numbers::pi_v<float> * 0.5F}));
    trs.translation() = {0.25F, -0.25F, 0.0F};
    renderer.draw(camera, trs);
    expect_color(pixels[pixel_index(40, 40, 64)], red);
    expect_color(pixels[pixel_index(48, 48, 64)], clear_color);
}

void test_matrix_zw_and_sparse_consumed_outputs() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector2f_t>(0);
    const auto local = vertex.construct<vector4f_t>(position, 0.25F, 1.0F);
    const auto world = vertex.object_to_world() * local;
    const auto clip = vertex.world_to_clip() * world;
    vertex.position(clip);
    vertex.output(4, true);
    vertex.output(11, shader::swizzle<2, 3>(world));
    vertex.output(29, shader::swizzle<2, 3>(clip));
    vertex.output(41, vertex.object_to_world());

    shader::fragment_shader_ast_builder_t fragment;
    const auto world_zw = fragment.input<vector2f_t>(11);
    const auto clip_zw = fragment.input<vector2f_t>(29);
    fragment.color(fragment.construct<vector4f_t>(world_zw, clip_zw));

    const auto program = std::make_shared<const software_shader::program_t>(
        std::move(vertex).finalize(),
        std::move(fragment).finalize()
    );
    const auto material = std::make_shared<api::material_t>(program);
    const auto item = make_render_item(
        make_geometry({{0.0F, 0.0F}}, {0}, api::vertex_primitive_topology_t::point),
        material
    );
    std::vector<api::rgba8_t> pixels(16 * 16, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 16, 16));
    renderer.draw(make_camera(16, 16), item);
    expect_color(pixels[pixel_index(8, 8, 16)], {64, 255, 64, 255});

    shader::vertex_shader_ast_builder_t smaller_vertex;
    const auto smaller_position = smaller_vertex.input<vector2f_t>(0);
    const auto smaller_local = smaller_vertex.construct<vector4f_t>(smaller_position, 0.0F, 1.0F);
    smaller_vertex.position(smaller_vertex.world_to_clip() * smaller_vertex.object_to_world() * smaller_local);
    smaller_vertex.output(73, 1.0F);
    shader::fragment_shader_ast_builder_t smaller_fragment;
    const auto green_component = smaller_fragment.input<float>(73);
    smaller_fragment.color(smaller_fragment.construct<vector4f_t>(0.0F, green_component, 0.0F, 1.0F));
    const auto smaller_program = std::make_shared<const software_shader::program_t>(
        std::move(smaller_vertex).finalize(),
        std::move(smaller_fragment).finalize()
    );
    const auto smaller_item = make_render_item(
        make_geometry({{0.0F, 0.0F}}, {0}, api::vertex_primitive_topology_t::point),
        std::make_shared<api::material_t>(smaller_program)
    );
    renderer.clear(clear_color);
    renderer.draw(make_camera(16, 16), smaller_item);
    expect_color(pixels[pixel_index(8, 8, 16)], green);
}

void test_selected_range_indices_and_pre_raster_validation() {
    shader::vertex_shader_ast_builder_t indexed_vertex;
    const auto position = indexed_vertex.input<vector2f_t>(0);
    const auto local = indexed_vertex.construct<vector4f_t>(position, 0.0F, 1.0F);
    const auto selected = indexed_vertex.local(0.0F);
    indexed_vertex.branch(indexed_vertex.vertex_index() == std::int32_t(5), [&] {
        indexed_vertex.assign(selected, 1.0F);
    });
    indexed_vertex.position(indexed_vertex.world_to_clip() * indexed_vertex.object_to_world() * local);
    indexed_vertex.output(2, false);
    indexed_vertex.output(17, selected);
    indexed_vertex.output(31, indexed_vertex.object_to_world());

    shader::fragment_shader_ast_builder_t indexed_fragment;
    const auto selected_input = indexed_fragment.input<float>(17);
    indexed_fragment.color(indexed_fragment.construct<vector4f_t>(selected_input, 0.0F, 0.0F, 1.0F));

    const auto program = std::make_shared<const software_shader::program_t>(
        std::move(indexed_vertex).finalize(),
        std::move(indexed_fragment).finalize()
    );
    std::vector<position_t> positions(128, position_t {2.0F, 2.0F});
    positions[5] = {0.0F, 0.0F};
    soa::structure_of_arrays_t<position_t> streams;
    for (const auto& vertex_position : positions) {
        streams.push_back(vertex_position);
    }
    auto mesh = std::make_shared<api::mesh_t>(
        std::move(streams),
        std::vector<api::vertex_attribute_t> {
            api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 2)
        }
    );
    auto index_buffer = make_indices({0, 5, 5, 0});
    auto geometry = std::make_shared<api::geometry_t>(
        std::move(index_buffer),
        api::index_range_t {.offset = 1, .count = 2}
    );
    geometry->mesh() = std::move(mesh);
    geometry->primitive_topology() = api::vertex_primitive_topology_t::point;
    geometry->finalize();

    std::vector<api::rgba8_t> pixels(16 * 16, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 16, 16));
    const auto item = make_render_item(
        std::move(geometry),
        std::make_shared<api::material_t>(program)
    );
    renderer.draw(make_camera(16, 16), item);
    expect_color(pixels[pixel_index(8, 8, 16)], red);

    shader::vertex_shader_ast_builder_t failing_vertex;
    const auto failing_position = failing_vertex.input<vector2f_t>(0);
    failing_vertex.position(failing_vertex.construct<vector4f_t>(failing_position, 0.0F, 1.0F));
    failing_vertex.branch(failing_vertex.vertex_index() == std::int32_t(0), [&] {
        failing_vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
    });
    failing_vertex.branch(failing_vertex.vertex_index() != std::int32_t(0), [&] {
        failing_vertex.position(vector4f_t({std::numeric_limits<float>::infinity(), 0.0F, 0.0F, 1.0F}));
    });
    shader::fragment_shader_ast_builder_t failing_fragment;
    failing_fragment.color(vector4f_t({0.0F, 1.0F, 0.0F, 1.0F}));
    const auto failing_program = std::make_shared<const software_shader::program_t>(
        std::move(failing_vertex).finalize(),
        std::move(failing_fragment).finalize()
    );
    const auto failing_item = make_render_item(
        make_geometry(
            {{0.0F, 0.0F}, {0.5F, 0.0F}},
            {0, 1},
            api::vertex_primitive_topology_t::point
        ),
        std::make_shared<api::material_t>(failing_program)
    );
    renderer.clear(clear_color);
    test::expect_throws<std::runtime_error>([&] {
        renderer.draw(make_camera(16, 16), failing_item);
    });
    test::expect(std::identity(), std::ranges::all_of(pixels, [](const auto& pixel) {
        return same_color(pixel, clear_color);
    }));

    renderer.draw(make_camera(16, 16), item);
    expect_color(pixels[pixel_index(8, 8, 16)], red);
}

void test_fragment_bindings_are_validated_before_clipped_geometry() {
    shader::vertex_shader_ast_builder_t vertex;
    vertex.position(vector4f_t({2.0F, 0.0F, 0.0F, 1.0F}));

    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(fragment.uniform<vector4f_t>(9));
    const auto program = std::make_shared<const software_shader::program_t>(
        std::move(vertex).finalize(),
        std::move(fragment).finalize()
    );
    const auto material = std::make_shared<api::material_t>(program);
    const auto item = make_render_item(
        make_geometry({{0.0F, 0.0F}}, {0}, api::vertex_primitive_topology_t::point),
        material
    );
    std::vector<api::rgba8_t> pixels(16 * 16, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 16, 16));
    test::expect_throws<std::invalid_argument>([&] {
        renderer.draw(make_camera(16, 16), item);
    });
    material->uniform(9, vector4f_t({1.0F, 0.0F, 0.0F, 1.0F}));
    test::expect_no_throw([&] { renderer.draw(make_camera(16, 16), item); });
    test::expect(std::identity(), std::ranges::all_of(pixels, [](const auto& pixel) {
        return same_color(pixel, clear_color);
    }));
}

void test_explicit_color_and_rgba8_conversion() {
    const float infinity = std::numeric_limits<float>::infinity();
    const auto special = draw_scene(
        api::vertex_primitive_topology_t::point,
        {{0.0F, 0.0F}},
        {0},
        8,
        8,
        make_float_texture({
            std::numeric_limits<float>::quiet_NaN(),
            -infinity,
            infinity,
            0.5F
        })
    );
    expect_color(special[pixel_index(4, 4, 8)], {0, 0, 255, 128});

    const auto clamped = draw_scene(
        api::vertex_primitive_topology_t::point,
        {{0.0F, 0.0F}},
        {0},
        8,
        8,
        make_float_texture({-1.0F, 2.0F, 0.5F, 1.5F})
    );
    expect_color(clamped[pixel_index(4, 4, 8)], {0, 255, 128, 255});
}

void test_vertex_layout_rejection() {
    std::vector<api::rgba8_t> pixels(8 * 8, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 8, 8));
    const auto camera = make_camera(8, 8);
    const auto material = make_material(make_unorm_texture(red));

    const auto expect_rejected = [&](auto geometry) {
        const auto render_item = make_render_item(std::move(geometry), material);
        test::expect_throws<std::invalid_argument>([&] { renderer.draw(camera, render_item); });
    };

    expect_rejected(make_typed_geometry(
        std::vector<float> {0.0F},
        api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 1),
        {0},
        api::vertex_primitive_topology_t::point
    ));
    expect_rejected(make_typed_geometry(
        std::vector<std::array<double, 2>> {{{0.0, 0.0}}},
        api::vertex_attribute_t(api::vertex_attribute_type_t::R64, 2),
        {0},
        api::vertex_primitive_topology_t::point
    ));
    expect_rejected(make_typed_geometry(
        std::vector<std::array<std::int32_t, 2>> {{{0, 0}}},
        api::vertex_attribute_t(api::vertex_attribute_type_t::I32, 2),
        {0},
        api::vertex_primitive_topology_t::point
    ));
    expect_rejected(make_typed_geometry(
        std::vector<std::array<std::uint32_t, 2>> {{{0, 0}}},
        api::vertex_attribute_t(api::vertex_attribute_type_t::U32, 2),
        {0},
        api::vertex_primitive_topology_t::point
    ));
}

void test_material_resource_mapping() {
    static_assert(!std::is_copy_assignable_v<api::material_t>);

    std::vector<api::rgba8_t> pixels(8 * 8, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 8, 8));
    const auto camera = make_camera(8, 8);
    const auto geometry = make_geometry(
        {{0.0F, 0.0F}},
        {0},
        api::vertex_primitive_topology_t::point
    );

    const auto program = make_textured_program();
    auto mapped_material = make_material(make_unorm_texture(red), make_sampler(), program);
    mapped_material->texture(0, make_unorm_texture(texture_color));
    mapped_material->texture(7, make_unorm_texture(blue));
    mapped_material->sampler(7, make_sampler());
    mapped_material->uniform(0, 17.0F);
    const auto mapped = make_render_item(geometry, std::move(mapped_material));
    renderer.draw(camera, mapped);
    expect_color(pixels[pixel_index(4, 4, 8)], texture_color);

    auto distinct_material = make_material(make_unorm_texture(green), make_sampler(), program);
    test::expect(std::identity(), distinct_material->program() == program);
    const auto distinct = make_render_item(geometry, distinct_material);
    renderer.clear(clear_color);
    renderer.draw(camera, distinct);
    expect_color(pixels[pixel_index(4, 4, 8)], green);

    distinct_material->texture(0, nullptr);
    test::expect_throws<std::invalid_argument>([&] { renderer.draw(camera, distinct); });
    test::expect_throws<std::invalid_argument>([&] { (void)distinct_material->bindings().texture(0); });

    distinct_material->texture(0, make_unorm_texture(red));
    distinct_material->sampler(0, nullptr);
    test::expect_throws<std::invalid_argument>([&] { renderer.draw(camera, distinct); });
    test::expect_throws<std::invalid_argument>([&] { (void)distinct_material->bindings().sampler(0); });

    auto owned_texture = make_unorm_texture(red);
    auto owned_sampler = make_sampler();
    const std::weak_ptr<texture::texture_t> weak_texture = owned_texture;
    const std::weak_ptr<texture::sampler_t> weak_sampler = owned_sampler;
    auto owning_material = make_material(owned_texture, owned_sampler, program);
    owned_texture.reset();
    owned_sampler.reset();
    test::expect(std::logical_not<>(), weak_texture.expired());
    test::expect(std::logical_not<>(), weak_sampler.expired());
    owning_material->texture(0, nullptr);
    owning_material->sampler(0, nullptr);
    test::expect(std::identity(), weak_texture.expired());
    test::expect(std::identity(), weak_sampler.expired());

    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] const api::material_t invalid(nullptr);
    });
}

void test_nonfinite_clip_position_rejection() {
    std::vector<api::rgba8_t> pixels(8 * 8, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 8, 8));
    const auto camera = make_camera(8, 8);
    const auto material = make_material(make_unorm_texture(red));

    const auto infinity = make_render_item(
        make_geometry(
            {{std::numeric_limits<float>::infinity(), 0.0F}},
            {0},
            api::vertex_primitive_topology_t::point
        ),
        material
    );
    test::expect_throws<std::runtime_error>([&] { renderer.draw(camera, infinity); });

    const auto nan = make_render_item(
        make_geometry(
            {{std::numeric_limits<float>::quiet_NaN(), 0.0F}},
            {0},
            api::vertex_primitive_topology_t::point
        ),
        material
    );
    test::expect_throws<std::runtime_error>([&] { renderer.draw(camera, nan); });
}

program_ptr_t make_clip_program(bool facing = false) {
    shader::vertex_shader_ast_builder_t vertex;
    vertex.position(vertex.input<vector4f_t>(0));
    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(vector4f_t({0, 0, 1, 1}));
    if (facing) {
        fragment.branch(fragment.front_facing(), [&] { fragment.color(vector4f_t({1, 0, 0, 1})); });
    }
    return std::make_shared<const software_shader::program_t>(std::move(vertex).finalize(), std::move(fragment).finalize());
}

std::vector<api::rgba8_t> draw_clip_scene(const std::vector<clip_position_fixture_t>& positions, api::index_buffer_t::indices_t indices, api::vertex_primitive_topology_t topology, program_ptr_t program) {
    std::vector<api::rgba8_t> pixels(32 * 32, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 32, 32));
    const api::vertex_attribute_t attribute(api::vertex_attribute_type_t::R32, 4);
    const auto geometry = make_typed_geometry(positions, attribute, std::move(indices), topology);
    const auto material = std::make_shared<api::material_t>(program);
    const auto item = make_render_item(geometry, material);
    renderer.draw(make_camera(32, 32), item);
    return pixels;
}

void test_grid_public_pipeline() {
    const auto program = make_clip_program();
    for (const auto bounds : {fractional, clipped}) {
        const auto [xmin, xmax, ymin, ymax] = bounds;
        const std::vector<clip_position_fixture_t> positions {{xmin, ymax, 0, 1}, {xmax, ymin, 0, 1}, {xmax, ymax, 0, 1}, {xmin, ymin, 0, 1}};
        for (const auto topology : {api::vertex_primitive_topology_t::triangle, api::vertex_primitive_topology_t::triangle_strip, api::vertex_primitive_topology_t::triangle_fan}) {
            api::index_buffer_t::indices_t indices;
            switch (topology) {
                case api::vertex_primitive_topology_t::triangle: {
                    indices = {0, 1, 2, 1, 0, 3};
                } break;
                case api::vertex_primitive_topology_t::triangle_strip: {
                    indices = {2, 0, 1, 3};
                } break;
                default: {
                    indices = {0, 2, 1, 3};
                } break;
            }
            for (bool reversed : {false, true}) {
                if (reversed) {
                    std::reverse(indices.begin(), indices.end());
                }
                const auto pixels = draw_clip_scene(positions, indices, topology, program);
                const auto facing_pixels = draw_clip_scene(positions, indices, topology, make_clip_program(true));
                const bool front = topology == api::vertex_primitive_topology_t::triangle ? !reversed : (topology == api::vertex_primitive_topology_t::triangle_fan && reversed);
                for (int y = 0; y < 32; ++y) {
                    for (int x = 0; x < 32; ++x) {
                        const bool expected = bounds == clipped || (4 <= x && x <= 28 && 8 <= y && y <= 24);
                        expect_color(pixels[pixel_index(x, y, 32)], expected ? blue : clear_color);
                        expect_color(facing_pixels[pixel_index(x, y, 32)], expected ? (front ? red : blue) : clear_color);
                    }
                }
            }
        }
    }
    const auto facing_program = make_clip_program(true);
    for (const auto& input : {crossing, concave}) {
        for (bool reversed : {false, true}) {
            const auto indices = reversed ? api::index_buffer_t::indices_t {2, 1, 0} : api::index_buffer_t::indices_t {0, 1, 2};
            const auto pixels = draw_clip_scene({input.begin(), input.end()}, indices, api::vertex_primitive_topology_t::triangle, facing_program);
            test::expect(std::equal_to<>(), colored_pixel_count(pixels), std::size_t(1));
            expect_color(pixels[0], reversed ? red : blue);
        }
    }
    for (const auto& input : collapsed) {
        const auto pixels = draw_clip_scene({input.begin(), input.end()}, {0, 1, 2}, api::vertex_primitive_topology_t::triangle, program);
        test::expect(std::equal_to<>(), colored_pixel_count(pixels), std::size_t(0));
    }
    for (const auto& indices : {api::index_buffer_t::indices_t {0, 1}, api::index_buffer_t::indices_t {1, 0}}) {
        const auto pixels = draw_clip_scene({{-2, 0, 0, 1}, {-1, 0, 0, 1}}, indices, api::vertex_primitive_topology_t::line, program);
        test::expect(std::equal_to<>(), colored_pixel_count(pixels), std::size_t(1));
        expect_color(pixels[pixel_index(0, 16, 32)], blue);
    }
}

void test_grid_fragment_state() {
    shader::vertex_shader_ast_builder_t vertex;
    vertex.position(vertex.input<vector4f_t>(0));
    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(fragment.fragment_coordinate() / vector4f_t({32, 32, 1, 1}));
    const auto coordinates_program = std::make_shared<const software_shader::program_t>(std::move(vertex).finalize(), std::move(fragment).finalize());
    const auto pixels = draw_clip_scene({{-1, 1, -0.5F, 1}, {1, 1, -0.5F, 1}, {-1, -1, -0.5F, 1}}, {0, 1, 2}, api::vertex_primitive_topology_t::triangle, coordinates_program);
    expect_color(pixels[0], {4, 4, 64, 255});

    shader::vertex_shader_ast_builder_t varying_vertex;
    varying_vertex.position(varying_vertex.input<vector4f_t>(0));
    const auto value = varying_vertex.local(0.0F);
    varying_vertex.branch(varying_vertex.vertex_index() == std::int32_t(1), [&] { varying_vertex.assign(value, 1.0F); });
    varying_vertex.branch(varying_vertex.vertex_index() == std::int32_t(2), [&] { varying_vertex.assign(value, 2.0F); });
    varying_vertex.output(7, value);
    shader::fragment_shader_ast_builder_t varying_fragment;
    const auto coordinate = varying_fragment.fragment_coordinate();
    const auto color = varying_fragment.construct<vector4f_t>(
        varying_fragment.input<float>(7),
        shader::swizzle<2>(coordinate),
        shader::swizzle<3>(coordinate),
        1.0F
    );
    varying_fragment.color(color);
    const auto varying_program = std::make_shared<const software_shader::program_t>(std::move(varying_vertex).finalize(), std::move(varying_fragment).finalize());
    const auto perspective = draw_clip_scene({{-1, 1, -0.5F, 1}, {2, 2, -1, 2}, {-4, -4, -2, 4}}, {0, 1, 2}, api::vertex_primitive_topology_t::triangle, varying_program);
    expect_color(perspective[0], {4, 64, 250, 255});

    test::expect_throws<std::out_of_range>([&] {
        const float w = std::numeric_limits<float>::denorm_min();
        (void)draw_clip_scene({{0, 0, 0, w}, {w, 0, 0, w}, {0, w, 0, w}}, {0, 1, 2}, api::vertex_primitive_topology_t::triangle, make_clip_program());
    });
    const auto zero_w = draw_clip_scene({{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, {0, 1, 2}, api::vertex_primitive_topology_t::triangle, make_clip_program());
    test::expect(std::equal_to<>(), colored_pixel_count(zero_w), std::size_t(0));
    std::vector<api::rgba8_t> wide_pixels(std::size_t(raster::maximum_extent) + 1);
    api::software_renderer_t wide(api::framebuffer_t(wide_pixels, raster::maximum_extent + 1, 1));
    const auto item = make_render_item(make_geometry({{0, 0}}, {0}, api::vertex_primitive_topology_t::point), make_material(make_unorm_texture(red)));
    test::expect_throws<std::out_of_range>([&] { wide.draw(make_camera(32, 32), item); });
}

using mask_t = std::set<std::array<std::int64_t, 2>>;

void require(bool condition, const std::source_location& location = std::source_location::current()) {
    test::expect_at(location, std::identity(), condition);
}

void near(double actual, double expected, const std::source_location& location = std::source_location::current()) {
    test::expect_at(location, [](double a, double b) { return std::abs(a - b) <= 2e-6 * std::max(1.0, std::abs(b)); }, actual, expected);
}

raster::pipeline_vertex_view_t vertex(clip_position_fixture_t p) {
    return {raster::vector4f_t(p), {}};
}

void prepare(clip_triangle_fixture_t input, raster::raster_workspace_t& workspace) {
    raster::prepare_triangle(vertex(input[0]), vertex(input[1]), vertex(input[2]), 32, 32, workspace);
}

mask_t count_samples(raster::raster_workspace_t& workspace, int width = 32, int height = 32) {
    mask_t result;
    raster::visit_samples(workspace, width, height, [&](const raster::sample_t& sample) {
        // Insert before interpolation/shading. A second ear/span hit is a failure.
        require(result.insert({sample.m_x, sample.m_y}).second);
    });
    return result;
}

mask_t rectangle(int first_x, int first_y, int end_x, int end_y) {
    mask_t result;
    for (int y = first_y; y < end_y; ++y) {
        for (int x = first_x; x < end_x; ++x) {
            result.insert({x, y});
        }
    }
    return result;
}

mask_t winding_oracle(std::span<const raster::projected_vertex_t> vertices, int width, int height) {
    // Independent signed ray crossing. Half-open Y and strict crossing to the
    // right implement P+(epsilon,epsilon^2); no event sorting or triangulation.
    mask_t result;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::int64_t px = x * 256 + 128, py = y * 256 + 128;
            int winding = 0;
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                const auto a = vertices[i].m_point, b = vertices[(i + 1) % vertices.size()].m_point;
                const __int128 lhs = __int128(px - a[0]) * (b[1] - a[1]);
                const __int128 rhs = __int128(py - a[1]) * (b[0] - a[0]);
                if (a[1] <= py && py < b[1] && lhs < rhs) {
                    ++winding;
                }
                if (b[1] <= py && py < a[1] && rhs < lhs) {
                    --winding;
                }
            }
            if (winding != 0) {
                result.insert({x, y});
            }
        }
    }
    return result;
}

void test_original_shared_edges() {
    for (const auto bounds : {fractional, clipped}) {
        const auto [xmin, xmax, ymin, ymax] = bounds;
        const clip_position_fixture_t a {xmin, ymax, 0, 1}, b {xmax, ymin, 0, 1};
        const clip_position_fixture_t c {xmax, ymax, 0, 1}, d {xmin, ymin, 0, 1};
        const auto expected = bounds == fractional ? rectangle(4, 8, 29, 25) : rectangle(0, 0, 32, 32);
        for (bool reverse_first : {false, true}) {
            for (bool reverse_second : {false, true}) {
                std::array<clip_triangle_fixture_t, 2> inputs {{{a, b, c}, {b, a, d}}};
                if (reverse_first) {
                    std::reverse(inputs[0].begin(), inputs[0].end());
                }
                if (reverse_second) {
                    std::reverse(inputs[1].begin(), inputs[1].end());
                }
                std::array<mask_t, 2> masks;
                raster::raster_workspace_t workspace;
                for (std::size_t i = 0; i < inputs.size(); ++i) {
                    prepare(inputs[i], workspace);
                    masks[i] = count_samples(workspace);
                    require(masks[i] == winding_oracle(workspace.m_vertices, 32, 32));
                    workspace.m_use_triangles = false;
                    require(count_samples(workspace) == masks[i]);
                }
                for (bool reverse_submission : {false, true}) {
                    auto united = masks[reverse_submission ? 1 : 0];
                    for (const auto p : masks[reverse_submission ? 0 : 1]) {
                        require(united.insert(p).second);
                    }
                    require(united == expected);
                }
            }
        }
    }
}

void test_clipped_boundaries() {
    for (const auto& original : {crossing, concave}) {
        for (bool unequal_w : {false, true}) {
            for (bool reversed : {false, true}) {
                for (std::size_t rotation = 0; rotation < 3; ++rotation) {
                    auto input = original;
                    if (unequal_w) {
                        for (std::size_t i = 0; i < 3; ++i) {
                            for (float& component : input[i]) {
                                component *= float(1 << i);
                            }
                        }
                    }
                    if (reversed) {
                        std::reverse(input.begin(), input.end());
                    }
                    std::rotate(input.begin(), input.begin() + rotation, input.end());
                    raster::raster_workspace_t workspace;
                    prepare(input, workspace);
                    require(workspace.m_use_triangles == (original == concave));
                    require(workspace.m_front_facing == reversed);
                    require(count_samples(workspace) == mask_t {{0, 0}});
                    require(winding_oracle(workspace.m_vertices, 32, 32) == mask_t {{0, 0}});
                    for (const auto& v : workspace.m_vertices) {
                        for (std::size_t plane = 0; plane < 6; ++plane) {
                            require(0.0 <= raster::clip_distance(v.m_source, plane));
                        }
                    }
                }
            }
        }
    }
    raster::raster_workspace_t workspace;
    for (const auto input : collapsed) {
        prepare(input, workspace);
        require(count_samples(workspace).empty());
    }
}

void test_polygon(std::vector<raster::grid_point_t> points, const mask_t& expected) {
    std::vector<raster::varying_values_t> payloads(points.size());
    raster::raster_workspace_t workspace;
    for (std::size_t i = 0; i < points.size(); ++i) {
        payloads[i].emplace_back(7, float(i * i + 1) / 16.0F);
        workspace.m_vertices.push_back({points[i], double(i % 3) / 4.0, 1.0 / double(1 + i % 3), {raster::vector4f_t({0, 0, 0, 1}), payloads[i]}});
    }
    const auto originals = workspace.m_vertices;
    std::vector<std::array<double, 3>> baseline;
    bool facing = false;
    for (bool reversed : {false, true}) {
        for (std::size_t rotation = 0; rotation < points.size(); ++rotation) {
            workspace.m_vertices = originals;
            if (reversed) {
                std::reverse(workspace.m_vertices.begin(), workspace.m_vertices.end());
            }
            std::rotate(workspace.m_vertices.begin(), workspace.m_vertices.begin() + rotation, workspace.m_vertices.end());
            raster::prepare_polygon(workspace);
            require(count_samples(workspace, 8, 8) == expected);
            require(winding_oracle(workspace.m_vertices, 8, 8) == expected);
            std::vector<std::array<double, 3>> actual(64);
            raster::varying_values_t output;
            raster::visit_samples(workspace, 8, 8, [&](const raster::sample_t& sample) {
                const auto dq = raster::interpolate_sample(workspace.m_vertices, sample, output);
                require(0.0 < dq[1]);
                actual[sample.m_y * 8 + sample.m_x] = {dq[0], dq[1], std::get<float>(output[0].second)};
            });
            if (!reversed && rotation == 0) {
                baseline = actual;
                facing = workspace.m_front_facing;
            } else {
                for (std::size_t i = 0; i < actual.size(); ++i) {
                    for (std::size_t j = 0; j < 3; ++j) {
                        near(actual[i][j], baseline[i][j]);
                    }
                }
                if (!expected.empty()) {
                    require(workspace.m_front_facing == (reversed ? !facing : facing));
                }
            }
            workspace.m_use_triangles = false;
            require(count_samples(workspace, 8, 8) == expected);
        }
    }
}

void test_general_boundaries() {
    auto l_mask = rectangle(0, 0, 6, 2);
    l_mask.merge(rectangle(0, 2, 2, 6));
    test_polygon({{0, 0}, {1536, 0}, {1536, 512}, {512, 512}, {512, 1536}, {0, 1536}}, l_mask);
    test_polygon({{124, 129}, {129, 127}, {133, 125}, {131, 130}}, {{0, 0}});
    test_polygon({{126, 128}, {132, 127}, {134, 126}, {127, 129}}, {{0, 0}});
    test_polygon({{0, 0}, {1024, 1024}, {0, 1024}, {1024, 0}}, {{0, 0}, {1, 0}, {2, 0}, {1, 1}, {1, 2}, {0, 3}, {1, 3}, {2, 3}});
    test_polygon({{0, 0}, {768, 0}, {1536, 0}, {1536, 1536}, {0, 1536}}, rectangle(0, 0, 6, 6));
    test_polygon({{0, 0}, {0, 0}, {1536, 0}, {1536, 0}, {1536, 1536}, {1536, 1536}, {0, 1536}, {0, 1536}}, rectangle(0, 0, 6, 6));
    test_polygon({{0, 0}, {1536, 0}, {1536, 1536}, {0, 1536}, {0, 0}, {1536, 0}, {1536, 1536}, {0, 1536}}, rectangle(0, 0, 6, 6));
    test_polygon({{0, 0}, {1536, 0}, {1536, 1536}, {0, 1536}, {0, 1536}, {1536, 1536}, {1536, 0}, {0, 0}}, {});
    auto touching = rectangle(0, 0, 2, 2);
    touching.merge(rectangle(2, 2, 4, 4));
    test_polygon({{0, 0}, {512, 0}, {512, 512}, {1024, 512}, {1024, 1024}, {512, 1024}, {512, 512}, {0, 512}}, touching);
    test_polygon({{128, 128}, {128, 128}, {128, 128}}, {});
    test_polygon({{0, 0}, {128, 128}, {256, 256}, {128, 128}}, {});
    test_polygon({{128, 128}, {512, 128}, {128, 128}}, {});

    std::mt19937 random(20260905);
    for (int iteration = 0; iteration < 200; ++iteration) {
        raster::raster_workspace_t workspace;
        const auto count = 3 + random() % 7;
        for (std::size_t i = 0; i < count; ++i) {
            workspace.m_vertices.push_back({{std::int64_t(random() % 1025), std::int64_t(random() % 1025)}, 0, 1, {raster::vector4f_t({0, 0, 0, 1}), {}}});
        }
        raster::prepare_polygon(workspace);
        require(count_samples(workspace, 4, 4) == winding_oracle(workspace.m_vertices, 4, 4));
    }
}

void test_interpolation() {
    const std::array<raster::grid_point_t, 4> points {{{126, 128}, {132, 127}, {134, 126}, {127, 129}}};
    const std::array q {1.0, 0.5, 0.25, 0.125};
    const std::array z {-0.5, 0.0, 0.5, 0.75};
    std::array<raster::varying_values_t, 4> payloads;
    raster::raster_workspace_t workspace;
    for (std::size_t i = 0; i < points.size(); ++i) {
        payloads[i] = {{3, float(i)}, {5, raster::vector2f_t({float(i), 7.0F})}, {7, raster::vector3f_t(float(i))}, {9, raster::vector4f_t(float(i))}};
        workspace.m_vertices.push_back({points[i], z[i], q[i], {raster::vector4f_t({0, 0, 0, 1}), payloads[i]}});
    }
    raster::prepare_polygon(workspace);
    int hits = 0;
    raster::visit_samples(workspace, 32, 32, [&](const auto& sample) {
        ++hits;
        raster::varying_values_t output;
        const auto dq = raster::interpolate_sample(workspace.m_vertices, sample, output);
        near(dq[0], 0.6);
        near(dq[1], 0.5);
        near(std::get<float>(output[0].second), 0.5);
        near(std::get<raster::vector2f_t>(output[1].second)[0], 0.5);
        near(std::get<raster::vector2f_t>(output[1].second)[1], 7.0);
        near(std::get<raster::vector3f_t>(output[2].second)[2], 0.5);
        near(std::get<raster::vector4f_t>(output[3].second)[3], 0.5);
    });
    require(hits == 1);
    // Triangle sample: grid (0,0),(1024,0),(0,1024), lambda=(3/4,1/8,1/8).
    workspace.m_vertices.resize(3);
    workspace.m_vertices[0].m_point = {0, 0};
    workspace.m_vertices[1].m_point = {1024, 0};
    workspace.m_vertices[2].m_point = {0, 1024};
    raster::prepare_polygon(workspace);
    raster::visit_samples(workspace, 4, 4, [&](const auto& sample) {
        if (sample.m_x == 0 && sample.m_y == 0) {
            raster::varying_values_t output;
            const auto dq = raster::interpolate_sample(workspace.m_vertices, sample, output);
            near(dq[1], 27.0 / 32.0);
            near(dq[0], 11.0 / 32.0);
            near(std::get<float>(output[0].second), 4.0 / 27.0);
        }
    });
    for (auto& payload : payloads) {
        payload = {{3, std::numeric_limits<float>::max()}};
    }
    for (std::size_t i = 0; i < workspace.m_vertices.size(); ++i) {
        workspace.m_vertices[i].m_source.m_outputs = payloads[i];
        workspace.m_vertices[i].m_reciprocal_w = std::numeric_limits<float>::max();
    }
    raster::visit_samples(workspace, 4, 4, [&](const auto& sample) {
        raster::varying_values_t output;
        const auto dq = raster::interpolate_sample(workspace.m_vertices, sample, output);
        require(std::isfinite(float(dq[1])));
        require(std::get<float>(output[0].second) == std::numeric_limits<float>::max());
    });
}

void test_plane_coverage() {
    for (std::size_t plane = 0; plane < 7; ++plane) {
        float xmin = -0.75F, xmax = 0.75F, ymin = -0.75F, ymax = 0.75F;
        int first_x = 4, end_x = 28, first_y = 4, end_y = 28;
        if (plane == 0) {
            xmin = -2;
            xmax = 0.5F;
            first_x = 0;
            end_x = 24;
        }
        if (plane == 1) {
            xmin = -0.5F;
            xmax = 2;
            first_x = 8;
            end_x = 32;
        }
        if (plane == 2) {
            ymin = -2;
            ymax = 0.5F;
            first_y = 8;
            end_y = 32;
        }
        if (plane == 3) {
            ymin = -0.5F;
            ymax = 2;
            first_y = 0;
            end_y = 24;
        }
        if (plane == 4 || plane == 5) {
            end_x = 16;
        }
        if (plane == 6) {
            xmin = -2;
            xmax = 2;
            ymin = -2;
            ymax = 2;
            first_x = 0;
            end_x = 32;
            first_y = 0;
            end_y = 32;
        }
        mask_t expected;
        for (int y = first_y; y < end_y; ++y) {
            for (int x = first_x; x < end_x; ++x) {
                if (plane != 6 || (-16 <= x - y && x - y < 16)) {
                    expected.insert({x, y});
                }
            }
        }
        const std::array<clip_position_fixture_t, 4> unscaled {{{xmin, ymax, 0, 1}, {xmax, ymin, 0, 1}, {xmax, ymax, 0, 1}, {xmin, ymin, 0, 1}}};
        for (bool unequal_w : {false, true}) {
            auto positions = unscaled;
            for (std::size_t i = 0; i < 4; ++i) {
                if (plane == 4 || plane == 5) {
                    positions[i][2] = (plane == 4 ? -1.0F : 1.0F) * (positions[i][0] + 1.0F);
                }
                if (plane == 6) {
                    positions[i][2] = positions[i][0] + positions[i][1];
                }
                if (unequal_w) {
                    for (float& c : positions[i]) {
                        c *= float(1 << i);
                    }
                }
            }
            for (bool reversed : {false, true}) {
                mask_t united;
                for (auto ids : {std::array<std::size_t, 3> {0, 1, 2}, std::array<std::size_t, 3> {1, 0, 3}}) {
                    if (reversed) {
                        std::reverse(ids.begin(), ids.end());
                    }
                    raster::raster_workspace_t workspace;
                    prepare({positions[ids[0]], positions[ids[1]], positions[ids[2]]}, workspace);
                    const auto mask = count_samples(workspace);
                    require(mask == winding_oracle(workspace.m_vertices, 32, 32));
                    for (const auto p : mask) {
                        require(united.insert(p).second);
                    }
                    if (4 <= plane) {
                        raster::varying_values_t outputs;
                        raster::visit_samples(workspace, 32, 32, [&](const auto& sample) {
                            const auto dq = raster::interpolate_sample(workspace.m_vertices, sample, outputs);
                            const double ndc_z = plane == 6 ? double(sample.m_x - sample.m_y) / 16.0
                                                            : (plane == 4 ? -1.0 : 1.0) * (double(sample.m_x) + 0.5) / 16.0;
                            near(dq[0], 0.5 * ndc_z + 0.5);
                        });
                    }
                }
                require(united == expected);
            }
        }
    }
    // Every generated vertex retains all processed constraints, even when W
    // crosses zero before reaching the final projectable part of the volume.
    std::mt19937 random(928);
    std::uniform_real_distribution<float> coordinate(-8, 8), w(-2, 4);
    for (int iteration = 0; iteration < 100; ++iteration) {
        std::array<raster::pipeline_vertex_view_t, 3> input;
        for (auto& v : input) {
            v = vertex({coordinate(random), coordinate(random), coordinate(random), w(random)});
        }
        raster::clipping_workspace_t clipping;
        const auto index = raster::clip_triangle(input[0], input[1], input[2], clipping);
        if (index) {
            const auto& polygon = clipping.m_buffers[*index];
            for (const auto& v : polygon.m_vertices) {
                for (std::size_t plane = 0; plane < 6; ++plane) {
                    require(0 <= raster::clip_distance(raster::view(v, polygon.m_values), plane));
                }
            }
        }
    }
}

void test_clipping() {
    raster::clipping_workspace_t workspace;
    const raster::varying_values_t from_values {{4, 0.0F}}, to_values {{4, 1.0F}};
    for (std::size_t plane = 0; plane < 6; ++plane) {
        auto outside = vertex({0, 0, 0, 1}), inside = outside;
        outside.m_clip_position[plane / 2] = plane % 2 == 0 ? -2.0F : 2.0F;
        outside.m_outputs = from_values;
        inside.m_outputs = to_values;
        auto index = raster::clip_line(outside, inside, workspace);
        require(index.has_value());
        auto& line = workspace.m_buffers[*index];
        const auto position = line.m_vertices[0].m_clip_position;
        require(raster::clip_distance(raster::view(line.m_vertices[0], line.m_values), plane) == 0.0);
        near(std::get<float>(raster::view(line.m_vertices[0], line.m_values).m_outputs[0].second), 0.5);
        index = raster::clip_line(inside, outside, workspace);
        require(index.has_value());
        for (std::size_t axis = 0; axis < 4; ++axis) {
            require(std::bit_cast<std::uint32_t>(position[axis]) == std::bit_cast<std::uint32_t>(workspace.m_buffers[*index].m_vertices[1].m_clip_position[axis]));
        }
        auto on_plane = inside;
        on_plane.m_clip_position = position;
        index = raster::clip_line(outside, on_plane, workspace);
        require(index.has_value());
        for (const auto& v : workspace.m_buffers[*index].m_vertices) {
            const auto actual = raster::view(v, workspace.m_buffers[*index].m_values);
            require(std::ranges::equal(actual.m_clip_position, on_plane.m_clip_position));
            near(std::get<float>(actual.m_outputs[0].second), 1.0);
        }
    }
    for (float sign : {-1.0F, 1.0F}) {
        auto first = vertex({sign * 2, sign * 2, sign * 2, 1}), second = vertex({0, 0, 0, 1});
        const auto index = raster::clip_line(first, second, workspace);
        require(index.has_value());
        for (const auto& v : workspace.m_buffers[*index].m_vertices) {
            for (std::size_t plane = 0; plane < 6; ++plane) {
                require(0 <= raster::clip_distance(raster::view(v, workspace.m_buffers[*index].m_values), plane));
            }
        }
    }
    const float huge = std::numeric_limits<float>::max();
    raster::raster_workspace_t polygon;
    raster::prepare_triangle(vertex({-huge, -huge, 0, huge}), vertex({huge, -huge, 0, huge}), vertex({0, huge, 0, huge}), 8, 8, polygon);
    require(!count_samples(polygon, 8, 8).empty());
    test::expect_throws<std::out_of_range>([] { raster::projectable_reciprocal_w(std::numeric_limits<float>::denorm_min()); });
    require(std::isfinite(float(raster::projectable_reciprocal_w(0x1p-127F))));
}

void test_integer_bounds() {
    constexpr std::int64_t limit = std::int64_t(1) << 31;
    constexpr std::array<raster::grid_point_t, 4> corners {{{0, 0}, {limit, 0}, {limit, limit}, {0, limit}}};
    raster::edge_value_t largest = 0;
    for (auto a : corners) {
        for (auto b : corners) {
            for (auto p : corners) {
                const auto determinant = raster::edge(a, b, p);
                require(determinant == -raster::edge(b, a, p));
                largest = std::max(largest, determinant < 0 ? -determinant : determinant);
            }
        }
    }
    require(largest == (std::int64_t(1) << 62));
    require(raster::snap(raster::maximum_extent, raster::maximum_extent) == limit);
    require(float(raster::maximum_extent - 1) + 0.5F == double(raster::maximum_extent) - 0.5);
    for (const std::int64_t i : {std::int64_t(0), std::int64_t(128), limit - 1}) {
        const double half = (double(i) + 0.5) / 256.0;
        require(raster::snap(half, raster::maximum_extent) == i + 1);
        require(raster::snap(std::nextafter(half, -std::numeric_limits<double>::infinity()), raster::maximum_extent) == i);
        require(raster::snap(std::nextafter(half, std::numeric_limits<double>::infinity()), raster::maximum_extent) == i + 1);
    }
    std::mt19937_64 random(20260905);
    for (int i = 0; i < 10000; ++i) {
        const raster::fraction_t a {std::int64_t(random() % (std::uint64_t(1) << 62)), std::int64_t(1 + random() % (limit - 1))};
        const raster::fraction_t b {std::int64_t(random() % (std::uint64_t(1) << 62)), std::int64_t(1 + random() % (limit - 1))};
        const __int128 lhs = __int128(a[0]) * b[1], rhs = __int128(b[0]) * a[1];
        require(raster::compare_fraction(a, b) == ((rhs < lhs) - (lhs < rhs)));
    }
    // Both rational crossings round to the same double, but contain the last X center.
    raster::raster_workspace_t workspace;
    for (const auto p : std::array<raster::grid_point_t, 3> {{{limit - 128, 0}, {limit - 127, limit}, {limit - 129, limit}}}) {
        workspace.m_vertices.push_back({p, 0, 1, {raster::vector4f_t({0, 0, 0, 1}), {}}});
    }
    raster::prepare_polygon(workspace);
    workspace.m_use_triangles = false;
    int hits = 0;
    raster::visit_samples(workspace, raster::maximum_extent, 1, [&](const auto& sample) {
        ++hits;
        require(sample.m_x == raster::maximum_extent - 1 && sample.m_y == 0);
        double sum = 0;
        for (double w : sample.m_weights) {
            require(std::isfinite(w) && 0 <= w);
            sum += w;
        }
        near(sum, 1.0);
    });
    require(hits == 1);
}

void test_wide_raster_bounds() {
    const int minimum = std::numeric_limits<int>::min();
    const int maximum = std::numeric_limits<int>::max();
    const raster::raster_bounds_t bounds(8, 8, {{minimum, maximum}, {minimum, maximum}});
    require(!bounds.empty());
    require(bounds.m_view_width == std::int64_t(maximum) - minimum);
    require(bounds.m_first_x == -std::int64_t(minimum));
    require(bounds.m_end_x - bounds.m_first_x == 8);
    const std::int64_t limit = bounds.m_view_width * raster::subpixels;
    const std::array<raster::grid_point_t, 4> corners {{{0, 0}, {limit, 0}, {limit, limit}, {0, limit}}};
    for (auto a : corners) {
        for (auto b : corners) {
            for (auto c : corners) {
                const auto determinant = raster::edge(a, b, c);
                require(determinant == -raster::edge(b, a, c));
                require((determinant < 0 ? -determinant : determinant) <= raster::edge_value_t(limit) * limit);
            }
        }
    }
    raster::raster_workspace_t workspace;
    const auto zero = vertex({0, 0, 0, 1});
    for (auto point : corners) {
        workspace.m_vertices.push_back({point, 0, 1, zero});
    }
    raster::prepare_polygon(workspace);
    for (bool triangles : {true, false}) {
        workspace.m_use_triangles = triangles;
        mask_t actual;
        raster::visit_samples(workspace, bounds.m_end_x, bounds.m_end_y, [&](const auto& sample) {
            require(actual.insert({sample.m_x - bounds.m_first_x, sample.m_y - bounds.m_first_y}).second);
            double sum = 0;
            for (double weight : sample.m_weights) {
                require(std::isfinite(weight) && 0 <= weight);
                sum += weight;
            }
            near(sum, 1);
        }, bounds.m_first_x, bounds.m_first_y);
        require(actual == rectangle(0, 0, 8, 8));
    }
    std::mt19937_64 random(20260907);
    for (int i = 0; i < 10000; ++i) {
        const raster::fraction_t a {(raster::edge_value_t(random()) << 16) + (random() & 65535), 1 + (random() & ((std::uint64_t(1) << 40) - 1))};
        const raster::fraction_t b {(raster::edge_value_t(random()) << 16) + (random() & 65535), 1 + (random() & ((std::uint64_t(1) << 40) - 1))};
        const auto lhs = a[0] * b[1], rhs = b[0] * a[1];
        require(raster::compare_fraction(a, b) == ((rhs < lhs) - (lhs < rhs)));
    }
}

void run_raster_tests() {
    test_original_shared_edges();
    test_clipped_boundaries();
    test_general_boundaries();
    test_interpolation();
    test_plane_coverage();
    test_clipping();
    test_integer_bounds();
    test_wide_raster_bounds();
}

void test_rotation_and_item_transform() {
    using shared_quaternion_t = m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>;
    static_assert(std::is_same_v<api::quaternion_t, shared_quaternion_t>);
    api::render_item_t item;
    api::camera_t camera({{0, 16}, {0, 16}}, api::perspective_t(1, 1, 10));
    const auto check_rotation_storage = [](auto& owner) {
        static_assert(std::is_same_v<decltype(owner.rotation()), const shared_quaternion_t&>);
        near(owner.rotation().w(), 1);
        near(owner.rotation().x(), 0);
        near(owner.rotation().y(), 0);
        near(owner.rotation().z(), 0);
        for (float magnitude : {std::numeric_limits<float>::denorm_min(), 1.0F, std::numeric_limits<float>::max()}) {
            shared_quaternion_t input(magnitude, magnitude, magnitude, magnitude);
            owner.rotation(input);
            require(input.w() == magnitude);
            input.x() = 0; // The setter owns a copy, not a reference to mutable components.
            near(owner.rotation().w(), 0.5);
            near(owner.rotation().x(), 0.5);
            near(owner.rotation().y(), 0.5);
            near(owner.rotation().z(), 0.5);
        }
        const auto previous = owner.rotation();
        for (float invalid : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
            const shared_quaternion_t input(invalid, 0, 0, 1);
            test::expect_throws<std::invalid_argument>([&] { owner.rotation(input); });
            test::expect_throws<std::invalid_argument>([&] { owner.rotation(vector3f_t({0, invalid, 0})); });
            require(owner.rotation() == previous);
        }
        test::expect_throws<std::invalid_argument>([&] { owner.rotation(shared_quaternion_t(0, 0, 0, 0)); });
        require(owner.rotation() == previous);
    };
    check_rotation_storage(item);
    check_rotation_storage(camera);

    const vector3f_t euler {0.31F, -0.72F, 1.27F};
    item.rotation(euler);
    camera.rotation(euler);
    require(item.rotation() == camera.rotation());
    const auto matrix = item.object_to_world();
    const auto rotation = item.rotation();
    item.rotation(shared_quaternion_t(-rotation.w(), -rotation.x(), -rotation.y(), -rotation.z()));
    require(item.object_to_world() == matrix);

    // Independent sequential Euler rotations establish the fixed-axis order.
    vector3f_t point {0.4F, -0.3F, 0.9F};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const auto a = (axis + 1) % 3, b = (axis + 2) % 3;
        const auto before = point;
        point[a] = std::cos(euler[axis]) * before[a] - std::sin(euler[axis]) * before[b];
        point[b] = std::sin(euler[axis]) * before[a] + std::cos(euler[axis]) * before[b];
    }
    const auto transformed = matrix * vector4f_t({0.4F, -0.3F, 0.9F, 1});
    for (std::size_t axis = 0; axis < 3; ++axis) { near(transformed[axis], point[axis]); }

    item.translation() = {1, 2, 3};
    item.scale() = {2, 3, 0};
    item.rotation(vector3f_t({0, 0, std::numbers::pi_v<float> / 2}));
    const auto result = item.object_to_world() * vector4f_t({1, 0, 4, 1});
    near(result[0], 1); near(result[1], 4); near(result[2], 3); near(result[3], 1);
    item.translation()[0] = std::numeric_limits<float>::infinity();
    test::expect_throws<std::invalid_argument>([&] { (void)item.object_to_world(); });
    require(!std::format("{}", rotation).empty());
}

void test_camera_pose_and_projection() {
    api::camera_t camera({{100, 900}, {50, 450}}, api::perspective_t(std::numbers::pi_v<float> / 2, 1, 9));
    const auto projection = camera.world_to_clip();
    near(projection(0, 0), 0.5);
    near(projection(1, 1), 1);
    for (float distance : {1.0F, 9.0F}) {
        const auto clip = projection * vector4f_t({0, 0, -distance, 1});
        near(clip[3], distance);
        near(clip[2] / clip[3], distance == 1 ? -1 : 1);
    }
    auto screen = camera.to_view({0, 1, -1});
    near(screen[0], 500); near(screen[1], 50); near(screen[2], 0);
    screen = camera.to_view({2, 0, -1});
    near(screen[0], 900); near(screen[1], 250);

    camera.position() = {3, 2, 5};
    camera.rotation(vector3f_t({0, std::numbers::pi_v<float> / 2, 0}));
    const auto view = camera.world_to_view();
    const auto local = view * vector4f_t({2, 2, 5, 1});
    near(local[0], 0); near(local[1], 0); near(local[2], -1); near(local[3], 1);
    const auto camera_origin = view * vector4f_t({3, 2, 5, 1});
    for (std::size_t axis = 0; axis < 3; ++axis) { near(camera_origin[axis], 0); }

    for (const vector3f_t target : {vector3f_t({1, 0, 0}), vector3f_t({-1, 0, 0}), vector3f_t({0, 0, 1}), vector3f_t({0, 0, -1}), vector3f_t({1, 2, -3})}) {
        camera.look_at({0, 0, 0}, target, {0, 1, 0});
        const auto result = camera.world_to_view() * vector4f_t({target[0], target[1], target[2], 1});
        near(result[0], 0); near(result[1], 0);
        near(result[2], -std::hypot(target[0], target[1], target[2]));
    }
    camera.look_at({1, 2, 3}, {1, 2, 2}, {0, 1, 0});
    const auto previous = camera.world_to_view();
    test::expect_throws<std::invalid_argument>([&] { camera.look_at({0, 0, 0}, {0, 0, 0}, {0, 1, 0}); });
    test::expect_throws<std::invalid_argument>([&] { camera.look_at({0, 0, 0}, {0, 0, -1}, {0, 0, 0}); });
    test::expect_throws<std::invalid_argument>([&] { camera.look_at({0, 0, 0}, {0, 0, -1}, {0, 0, 1}); });
    // Normalizing this diagonal before the cross product can hide exact parallelism.
    test::expect_throws<std::invalid_argument>([&] { camera.look_at({0, 0, 0}, {-1, -3, -7}, {1, 3, 7}); });
    test::expect_throws<std::invalid_argument>([&] { camera.look_at({0, 0, 0}, {-1, -3, -7}, {-1, -3, -7}); });
    test::expect_throws<std::invalid_argument>([&] { camera.rotation(vector3f_t({0, 0, std::numeric_limits<float>::infinity()})); });
    require(camera.world_to_view() == previous);
    test::expect_throws<std::invalid_argument>([&] { (void)camera.to_view({1, 2, 3}); });
    test::expect_throws<std::invalid_argument>([] { (void)api::perspective_t(0, 1, 10); });
    test::expect_throws<std::invalid_argument>([] { (void)api::perspective_t(std::numbers::pi_v<float>, 1, 10); });
    test::expect_throws<std::invalid_argument>([] { (void)api::perspective_t(1, 0, 10); });
    test::expect_throws<std::invalid_argument>([] { (void)api::perspective_t(1, 2, 1); });
    test::expect_throws<std::invalid_argument>([] { (void)api::perspective_t(1, 1, std::numeric_limits<float>::infinity()); });
    test::expect_throws<std::invalid_argument>([] { (void)api::orthographic_t({{0, 0}, {0, 1}}, 0, 1); });
    test::expect_throws<std::invalid_argument>([] { (void)api::orthographic_t({{0, 1}, {0, 1}}, -1, 1); });
    test::expect_throws<std::invalid_argument>([] { (void)api::orthographic_t({{0, 1}, {0, 1}}, 1, 1); });

    camera.position() = {0, 0, 0};
    camera.rotation(api::quaternion_t());
    camera.projection() = api::orthographic_t({{-2, 6}, {-3, 1}}, 0, 10);
    screen = camera.to_view({-2, 1, 0});
    near(screen[0], 100); near(screen[1], 50); near(screen[2], 0);
    screen = camera.to_view({6, -3, -10});
    near(screen[0], 900); near(screen[1], 450); near(screen[2], 1);
    require(!std::format("{}", camera).empty());
    static_assert(std::is_same_v<decltype(camera.rotation()), const api::quaternion_t&>);
}

void test_camera_regions_and_clears() {
    constexpr int width = 16, height = 12;
    std::vector<api::rgba8_t> pixels(width * height, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, width, height));
    api::camera_t camera({{3, 11}, {2, 9}}, api::perspective_t(1, 0.1F, 10));
    const auto quad = make_typed_geometry(std::vector<clip_position_fixture_t>{{-1, 1, 0, 1}, {-1, -1, 0, 1}, {1, 1, 0, 1}, {1, -1, 0, 1}}, api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 4), {0, 1, 2, 3}, api::vertex_primitive_topology_t::triangle_strip);
    auto item = make_render_item(quad, std::make_shared<api::material_t>(make_clip_program()));
    for (const api::view_rect_t rect : {api::view_rect_t({{3, 11}, {2, 9}}), api::view_rect_t({{-4, 7}, {-3, 6}}), api::view_rect_t({{10, 25}, {7, 30}})}) {
        camera.view_rect() = rect;
        renderer.clear(clear_color);
        renderer.draw(camera, item);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                expect_color(pixels[pixel_index(x, y, width)], rect.contains({x, y}) ? blue : clear_color);
            }
        }
        camera.position()[0] = std::numeric_limits<float>::quiet_NaN();
        renderer.clear(camera, green); // Clearing depends only on the rectangle.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                expect_color(pixels[pixel_index(x, y, width)], rect.contains({x, y}) ? green : clear_color);
            }
        }
        camera.position()[0] = 0;
    }
    // A valid program with a non-finite vertex result proves empty regions skip execution.
    auto invalid = make_render_item(make_typed_geometry(std::vector<clip_position_fixture_t>{{std::numeric_limits<float>::infinity(), 0, 0, 1}}, api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 4), {0}, api::vertex_primitive_topology_t::point), item.material());
    for (const api::view_rect_t rect : {api::view_rect_t({{2, 2}, {0, 5}}), api::view_rect_t({{0, 5}, {1, 1}}), api::view_rect_t({{-8, -1}, {0, 5}}), api::view_rect_t({{20, 30}, {0, 5}}), api::view_rect_t({{0, 5}, {20, 30}})}) {
        camera.view_rect() = rect;
        renderer.clear(clear_color);
        test::expect_no_throw([&] { renderer.draw(camera, invalid); });
        test::expect_no_throw([&] { renderer.draw(camera, api::render_item_t()); });
        renderer.clear(camera, green);
        require(colored_pixel_count(pixels) == 0);
    }
    camera.view_rect() = {{0, width}, {0, height}};
    test::expect_throws<std::runtime_error>([&] { renderer.draw(camera, invalid); });
    renderer.clear(red);
    for (const auto pixel : pixels) { expect_color(pixel, red); }

    // Absolute fragment coordinates include the viewport offset.
    shader::vertex_shader_ast_builder_t vertex_shader;
    vertex_shader.position(vertex_shader.input<vector4f_t>(0));
    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(fragment.fragment_coordinate() / vector4f_t({16, 16, 1, 1}));
    item.material() = std::make_shared<api::material_t>(std::make_shared<const software_shader::program_t>(std::move(vertex_shader).finalize(), std::move(fragment).finalize()));
    camera.view_rect() = {{3, 11}, {2, 9}};
    renderer.clear(clear_color);
    renderer.draw(camera, item);
    expect_color(pixels[pixel_index(3, 2, width)], {56, 40, 128, 255});
}

void test_region_topologies_and_original_aspect() {
    constexpr int width = 8, height = 8;
    std::vector<api::rgba8_t> pixels(width * height, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, width, height));
    const auto material = std::make_shared<api::material_t>(make_clip_program());
    const int minimum = std::numeric_limits<int>::min(), maximum = std::numeric_limits<int>::max();
    for (const api::view_rect_t rect : {api::view_rect_t({{2, 6}, {1, 7}}), api::view_rect_t({{-2, 6}, {-1, 7}}), api::view_rect_t({{minimum, maximum}, {0, height}})}) {
        api::camera_t camera(rect, api::perspective_t(1, 0.1F, 10));
        for (const auto topology : {api::vertex_primitive_topology_t::point, api::vertex_primitive_topology_t::line, api::vertex_primitive_topology_t::line_strip, api::vertex_primitive_topology_t::line_loop, api::vertex_primitive_topology_t::triangle, api::vertex_primitive_topology_t::triangle_strip, api::vertex_primitive_topology_t::triangle_fan}) {
            std::vector<clip_position_fixture_t> positions;
            api::index_buffer_t::indices_t indices;
            switch (topology) {
                case api::vertex_primitive_topology_t::point: {
                    positions = {{0, 0, 0, 1}}; indices = {0};
                } break;
                case api::vertex_primitive_topology_t::line:
                case api::vertex_primitive_topology_t::line_strip:
                case api::vertex_primitive_topology_t::line_loop: {
                    positions = {{-1, 0, 0, 1}, {1, 0, 0, 1}}; indices = {0, 1};
                } break;
                case api::vertex_primitive_topology_t::triangle: {
                    positions = {{-1, 1, 0, 1}, {-1, -1, 0, 1}, {1, 1, 0, 1}, {1, -1, 0, 1}}; indices = {0, 1, 2, 1, 3, 2};
                } break;
                case api::vertex_primitive_topology_t::triangle_strip: {
                    positions = {{-1, 1, 0, 1}, {-1, -1, 0, 1}, {1, 1, 0, 1}, {1, -1, 0, 1}}; indices = {0, 1, 2, 3};
                } break;
                default: {
                    positions = {{-1, 1, 0, 1}, {-1, -1, 0, 1}, {1, -1, 0, 1}, {1, 1, 0, 1}}; indices = {0, 1, 2, 3};
                } break;
            }
            const auto item = make_render_item(make_typed_geometry(positions, api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 4), indices, topology), material);
            renderer.clear(clear_color);
            renderer.draw(camera, item);
            require(0 < colored_pixel_count(pixels));
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (!rect.contains({x, y})) { expect_color(pixels[pixel_index(x, y, width)], clear_color); }
                }
            }
        }
    }
    shader::vertex_shader_ast_builder_t vertex_shader;
    const auto position = vertex_shader.input<vector3f_t>(0);
    vertex_shader.position(vertex_shader.world_to_clip() * vertex_shader.object_to_world() * vertex_shader.construct<vector4f_t>(position, 1.0F));
    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(vector4f_t({0, 0, 1, 1}));
    const auto program = std::make_shared<const software_shader::program_t>(std::move(vertex_shader).finalize(), std::move(fragment).finalize());
    const auto item = make_render_item(make_typed_geometry(std::vector<std::array<float, 3>>{{1.25F, 0, -2}}, api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 3), {0}, api::vertex_primitive_topology_t::point), std::make_shared<api::material_t>(program));
    const api::camera_t camera({{-8, 8}, {0, 8}}, api::perspective_t(std::numbers::pi_v<float> / 2, 1, 9));
    renderer.clear(clear_color);
    renderer.draw(camera, item);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const bool expected = (x - 2) * (x - 2) + (y - 4) * (y - 4) <= 9;
            expect_color(pixels[pixel_index(x, y, width)], expected ? blue : clear_color);
        }
    }
}

void test_textured_3d_near_plane() {
    shader::vertex_shader_ast_builder_t vertex_shader;
    const auto position = vertex_shader.input<vector3f_t>(0);
    vertex_shader.position(vertex_shader.world_to_clip() * vertex_shader.object_to_world() * vertex_shader.construct<vector4f_t>(position, 1.0F));
    vertex_shader.output(0, shader::swizzle<0, 1>(position) * 0.5F + vector2f_t({0.5F, 0.5F}));
    shader::fragment_shader_ast_builder_t fragment;
    const auto coordinates = fragment.input<vector2f_t>(0);
    fragment.color(shader::sample(fragment.resource<shader::shader_texture_2d_t>(0), fragment.resource<shader::shader_sampler_t>(0), coordinates));
    const auto program = std::make_shared<const software_shader::program_t>(std::move(vertex_shader).finalize(), std::move(fragment).finalize());
    const std::array texels {red, green, blue, white};
    auto item = make_render_item(make_typed_geometry(std::vector<std::array<float, 3>>{{-1, -1, 0}, {1, -1, 0}, {-1, 1, 0}, {1, 1, 0}}, api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 3), {0, 1, 2, 3}, api::vertex_primitive_topology_t::triangle_strip), make_material(make_unorm_texture(2, 2, texels), make_sampler(), program));
    item.rotation(vector3f_t({0, std::atan2(0.6F, 0.8F), 0}));
    item.translation() = {0, 0, -1};
    item.scale() = {1, 1, 0}; // A collapsed Z scale still leaves a visible XY surface.
    std::vector<api::rgba8_t> pixels(32 * 32, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 32, 32));
    for (bool perspective : {false, true}) {
        api::camera_t camera({{0, 32}, {0, 32}}, api::orthographic_t({{-1, 1}, {-1, 1}}, 0.75F, 10));
        if (perspective) { camera.projection() = api::perspective_t(std::numbers::pi_v<float> / 2, 0.75F, 10); }
        renderer.clear(clear_color);
        renderer.draw(camera, item);
        int checked = 0, visible = 0, clipped = 0;
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 32; ++x) {
                const double nx = (double(x) + 0.5) / 16 - 1, ny = 1 - (double(y) + 0.5) / 16;
                // Independently intersect a camera ray with z = -0.75*x - 1.
                const double distance = perspective ? 1 / (1 - 0.75 * nx) : 1 + 0.75 * nx;
                const double local_x = (perspective ? distance * nx : nx) / 0.8;
                const double local_y = perspective ? distance * ny : ny;
                if (std::abs(std::abs(local_x) - 1) < 0.03 || std::abs(std::abs(local_y) - 1) < 0.03 || std::abs(distance - 0.75) < 0.03 || std::abs(local_x) < 0.03 || std::abs(local_y) < 0.03) { continue; }
                const bool inside = std::abs(local_x) < 1 && std::abs(local_y) < 1;
                const bool expected = inside && 0.75 <= distance;
                const auto color = expected ? texels[(local_y < 0 ? 0 : 2) + (local_x < 0 ? 0 : 1)] : clear_color;
                expect_color(pixels[pixel_index(x, y, 32)], color);
                ++checked;
                visible += expected;
                clipped += inside && distance < 0.75;
            }
        }
        require(500 < checked && 100 < visible && 10 < clipped);
        // Translating both camera and item preserves the resulting image.
        const auto baseline = pixels;
        camera.position() = {2, 3, 4};
        item.translation() = {2, 3, 3};
        renderer.clear(clear_color);
        renderer.draw(camera, item);
        require(std::equal(pixels.begin(), pixels.end(), baseline.begin(), same_color));
        item.translation() = {0, 0, -1};
    }
}

void run_resource_tests() {
    test_rotation_and_item_transform();
    test_resource_model();
    test_vertex_layout_rejection();
    test_material_resource_mapping();
    test_selected_range_indices_and_pre_raster_validation();
    test_fragment_bindings_are_validated_before_clipped_geometry();
}

void run_framebuffer_tests() {
    test_framebuffer();
    test_empty_framebuffer();
}

void run_pipeline_tests() {
    test_camera_pose_and_projection();
    test_camera_regions_and_clears();
    test_region_topologies_and_original_aspect();
    test_textured_3d_near_plane();
    test_topologies_and_clipping();
    test_shared_edge_coverage();
    test_texture_coordinate_interpolation();
    test_shared_material_transform_semantics();
    test_matrix_zw_and_sparse_consumed_outputs();
    test_explicit_color_and_rgba8_conversion();
    test_nonfinite_clip_position_rejection();
    test_grid_public_pipeline();
    test_grid_fragment_state();
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

int main() {
    namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
    return m03gn97n4iusbtl7uthb01wu9m_test_framework::run([] {
        static_assert(sizeof(renderer::rgba8_t) == 4);
        renderer::run_resource_tests();
        renderer::run_framebuffer_tests();
        renderer::run_pipeline_tests();
        renderer::run_raster_tests();
    });
}
