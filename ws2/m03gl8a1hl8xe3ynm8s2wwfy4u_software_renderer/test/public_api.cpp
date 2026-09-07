#include "raster_fixtures.h"

#include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
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
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

struct application_metrics_t {
    std::size_t m_items = 0;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::application_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::application_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "application.frame items={}", metrics.m_items);
        return out;
    }
};

} // namespace std

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
    camera.rotation(m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>(0, 1, 0, 0));
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

    renderer.clear_color(clear_color);
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
    renderer.clear_color(texture_color);
    test::expect(std::identity(), std::ranges::all_of(replacement, [](const auto& pixel) {
        return same_color(pixel, texture_color);
    }));
}

void test_empty_framebuffer() {
    std::vector<api::rgba8_t> pixels;
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 0, 4));
    test::expect_no_throw([&] { renderer.clear_color(clear_color); });

    const auto camera = make_camera(8, 8);
    const auto render_item = make_render_item(
        make_geometry({{0.0F, 0.0F}}, {0}, api::vertex_primitive_topology_t::point),
        make_material(make_unorm_texture(red))
    );
    test::expect_no_throw([&] { renderer.draw(camera, render_item); });
    renderer.framebuffer() = api::framebuffer_t(pixels, 4, 0);
    test::expect_no_throw([&] { renderer.clear_color(clear_color); });
    test::expect_no_throw([&] { renderer.draw(camera, render_item); });
    renderer.framebuffer() = api::framebuffer_t(pixels, 0, 0);
    test::expect_no_throw([&] { renderer.clear_color(clear_color); });
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

    renderer.clear_color(clear_color);
    auto trs = make_render_item(
        make_geometry({{0.25F, 0.0F}}, {0}, api::vertex_primitive_topology_t::point),
        material
    );
    trs.scale() = {2.0F, 1.0F, 1.0F};
    trs.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, 0, std::numbers::pi_v<float> * 0.5F}));
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
    renderer.clear_color(clear_color);
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
    renderer.clear_color(clear_color);
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
    renderer.clear_color(clear_color);
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
        payloads[i] = {{3, float(i)}, {5, raster::vector2f_t({float(i), 7.0F})}, {7, m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>(float(i))}, {9, raster::vector4f_t(float(i))}};
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
        near(std::get<m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>>(output[2].second)[2], 0.5);
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
    api::render_item_t item;
    api::camera_t camera({{0, 16}, {0, 16}}, api::perspective_t(1, 1, 10));
    const auto check_rotation_storage = [](auto& owner) {
        static_assert(std::is_same_v<decltype(owner.rotation()), const m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>&>);
        near(owner.rotation().w(), 1);
        near(owner.rotation().x(), 0);
        near(owner.rotation().y(), 0);
        near(owner.rotation().z(), 0);
        for (float magnitude : {std::numeric_limits<float>::denorm_min(), 1.0F, std::numeric_limits<float>::max()}) {
            m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float> input(magnitude, magnitude, magnitude, magnitude);
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
            const m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float> input(invalid, 0, 0, 1);
            test::expect_throws<std::invalid_argument>([&] { owner.rotation(input); });
            test::expect_throws<std::invalid_argument>([&] { owner.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, invalid, 0})); });
            require(owner.rotation() == previous);
        }
        test::expect_throws<std::invalid_argument>([&] { owner.rotation(m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>(0, 0, 0, 0)); });
        require(owner.rotation() == previous);
    };
    check_rotation_storage(item);
    check_rotation_storage(camera);

    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> euler {0.31F, -0.72F, 1.27F};
    item.rotation(euler);
    camera.rotation(euler);
    require(item.rotation() == camera.rotation());
    const auto matrix = item.object_to_world();
    const auto rotation = item.rotation();
    item.rotation(m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>(-rotation.w(), -rotation.x(), -rotation.y(), -rotation.z()));
    require(item.object_to_world() == matrix);

    // Independent sequential Euler rotations establish the fixed-axis order.
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> point {0.4F, -0.3F, 0.9F};
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
    item.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, 0, std::numbers::pi_v<float> / 2}));
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
    camera.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, std::numbers::pi_v<float> / 2, 0}));
    const auto view = camera.world_to_view();
    const auto local = view * vector4f_t({2, 2, 5, 1});
    near(local[0], 0); near(local[1], 0); near(local[2], -1); near(local[3], 1);
    const auto camera_origin = view * vector4f_t({3, 2, 5, 1});
    for (std::size_t axis = 0; axis < 3; ++axis) { near(camera_origin[axis], 0); }

    for (const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> target : {m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({1, 0, 0}), m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({-1, 0, 0}), m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, 0, 1}), m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, 0, -1}), m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({1, 2, -3})}) {
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
    test::expect_throws<std::invalid_argument>([&] { camera.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, 0, std::numeric_limits<float>::infinity()})); });
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
    camera.rotation(m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>());
    camera.projection() = api::orthographic_t({{-2, 6}, {-3, 1}}, 0, 10);
    screen = camera.to_view({-2, 1, 0});
    near(screen[0], 100); near(screen[1], 50); near(screen[2], 0);
    screen = camera.to_view({6, -3, -10});
    near(screen[0], 900); near(screen[1], 450); near(screen[2], 1);
    require(!std::format("{}", camera).empty());
    static_assert(std::is_same_v<decltype(camera.rotation()), const m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>&>);
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
        renderer.clear_color(clear_color);
        renderer.draw(camera, item);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                expect_color(pixels[pixel_index(x, y, width)], rect.contains({x, y}) ? blue : clear_color);
            }
        }
        camera.position()[0] = std::numeric_limits<float>::quiet_NaN();
        renderer.clear_color(camera, green); // Clearing depends only on the rectangle.
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
        renderer.clear_color(clear_color);
        test::expect_no_throw([&] { renderer.draw(camera, invalid); });
        test::expect_no_throw([&] { renderer.draw(camera, api::render_item_t()); });
        renderer.clear_color(camera, green);
        require(colored_pixel_count(pixels) == 0);
    }
    camera.view_rect() = {{0, width}, {0, height}};
    test::expect_throws<std::runtime_error>([&] { renderer.draw(camera, invalid); });
    renderer.clear_color(red);
    for (const auto pixel : pixels) { expect_color(pixel, red); }

    // Absolute fragment coordinates include the viewport offset.
    shader::vertex_shader_ast_builder_t vertex_shader;
    vertex_shader.position(vertex_shader.input<vector4f_t>(0));
    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(fragment.fragment_coordinate() / vector4f_t({16, 16, 1, 1}));
    item.material() = std::make_shared<api::material_t>(std::make_shared<const software_shader::program_t>(std::move(vertex_shader).finalize(), std::move(fragment).finalize()));
    camera.view_rect() = {{3, 11}, {2, 9}};
    renderer.clear_color(clear_color);
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
            renderer.clear_color(clear_color);
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
    const auto position = vertex_shader.input<m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>>(0);
    vertex_shader.position(vertex_shader.world_to_clip() * vertex_shader.object_to_world() * vertex_shader.construct<vector4f_t>(position, 1.0F));
    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(vector4f_t({0, 0, 1, 1}));
    const auto program = std::make_shared<const software_shader::program_t>(std::move(vertex_shader).finalize(), std::move(fragment).finalize());
    const auto item = make_render_item(make_typed_geometry(std::vector<std::array<float, 3>>{{1.25F, 0, -2}}, api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 3), {0}, api::vertex_primitive_topology_t::point), std::make_shared<api::material_t>(program));
    const api::camera_t camera({{-8, 8}, {0, 8}}, api::perspective_t(std::numbers::pi_v<float> / 2, 1, 9));
    renderer.clear_color(clear_color);
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
    const auto position = vertex_shader.input<m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>>(0);
    vertex_shader.position(vertex_shader.world_to_clip() * vertex_shader.object_to_world() * vertex_shader.construct<vector4f_t>(position, 1.0F));
    vertex_shader.output(0, shader::swizzle<0, 1>(position) * 0.5F + vector2f_t({0.5F, 0.5F}));
    shader::fragment_shader_ast_builder_t fragment;
    const auto coordinates = fragment.input<vector2f_t>(0);
    fragment.color(shader::sample(fragment.resource<shader::shader_texture_2d_t>(0), fragment.resource<shader::shader_sampler_t>(0), coordinates));
    const auto program = std::make_shared<const software_shader::program_t>(std::move(vertex_shader).finalize(), std::move(fragment).finalize());
    const std::array texels {red, green, blue, white};
    auto item = make_render_item(make_typed_geometry(std::vector<std::array<float, 3>>{{-1, -1, 0}, {1, -1, 0}, {-1, 1, 0}, {1, 1, 0}}, api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 3), {0, 1, 2, 3}, api::vertex_primitive_topology_t::triangle_strip), make_material(make_unorm_texture(2, 2, texels), make_sampler(), program));
    item.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, std::atan2(0.6F, 0.8F), 0}));
    item.translation() = {0, 0, -1};
    item.scale() = {1, 1, 0}; // A collapsed Z scale still leaves a visible XY surface.
    std::vector<api::rgba8_t> pixels(32 * 32, clear_color);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 32, 32));
    for (bool perspective : {false, true}) {
        api::camera_t camera({{0, 32}, {0, 32}}, api::orthographic_t({{-1, 1}, {-1, 1}}, 0.75F, 10));
        if (perspective) { camera.projection() = api::perspective_t(std::numbers::pi_v<float> / 2, 0.75F, 10); }
        renderer.clear_color(clear_color);
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
        renderer.clear_color(clear_color);
        renderer.draw(camera, item);
        require(std::equal(pixels.begin(), pixels.end(), baseline.begin(), same_color));
        item.translation() = {0, 0, -1};
    }
}

program_ptr_t make_visibility_program(bool write_color = true, bool discard_left = false) {
    shader::vertex_shader_ast_builder_t vertex;
    vertex.position(vertex.input<vector4f_t>(0));
    shader::fragment_shader_ast_builder_t fragment;
    if (write_color) {
        fragment.color(fragment.uniform<vector4f_t>(0));
    }
    if (discard_left) {
        fragment.branch(shader::swizzle<0>(fragment.fragment_coordinate()) < 8.0F, [&] { fragment.discard(); });
    }
    return std::make_shared<const software_shader::program_t>(std::move(vertex).finalize(), std::move(fragment).finalize());
}

api::render_item_t make_visibility_item(
    std::vector<clip_position_fixture_t> positions,
    api::index_buffer_t::indices_t indices,
    api::vertex_primitive_topology_t topology,
    vector4f_t color = {1, 0, 0, 1},
    program_ptr_t program = nullptr
) {
    auto material = std::make_shared<api::material_t>(program ? program : make_visibility_program());
    material->uniform(0, color);
    material->depth_test(true);
    return make_render_item(make_typed_geometry(std::move(positions), api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 4), std::move(indices), topology), material);
}

std::vector<clip_position_fixture_t> visibility_quad(float ndc_z) {
    return {{-1, 1, ndc_z, 1}, {-1, -1, ndc_z, 1}, {1, 1, ndc_z, 1}, {1, -1, ndc_z, 1}};
}

void test_material_setting_invariants() {
    const auto program = make_visibility_program();
    auto material = std::make_shared<api::material_t>(program);
    material->uniform(0, vector4f_t({1, 0, 0, 1}));
    const auto& settings = std::as_const(*material);
    static_assert(std::is_same_v<decltype(material->depth_test()), bool>);
    static_assert(std::is_same_v<decltype(material->depth_write()), bool>);
    static_assert(std::is_same_v<decltype(material->depth_compare()), api::comparison_t>);
    static_assert(std::is_same_v<decltype(material->front_face()), api::winding_t>);
    static_assert(std::is_same_v<decltype(material->cull()), api::cull_mode_t>);
    require(!settings.depth_test() && settings.depth_write());
    require(settings.depth_compare() == api::comparison_t::less);
    require(settings.front_face() == api::winding_t::counter_clockwise && settings.cull() == api::cull_mode_t::none);

    material->depth_compare(api::comparison_t::greater);
    material->front_face(api::winding_t::clockwise);
    material->cull(api::cull_mode_t::front);
    for (int invalid : {-1, 99}) {
        test::expect_throws<std::invalid_argument>([&] { material->depth_compare(static_cast<api::comparison_t>(invalid)); });
        test::expect_throws<std::invalid_argument>([&] { material->front_face(static_cast<api::winding_t>(invalid)); });
        test::expect_throws<std::invalid_argument>([&] { material->cull(static_cast<api::cull_mode_t>(invalid)); });
        require(settings.depth_compare() == api::comparison_t::greater);
        require(settings.front_face() == api::winding_t::clockwise && settings.cull() == api::cull_mode_t::front);
        require(!settings.depth_test() && settings.depth_write());
    }
    auto copied_material = std::make_shared<api::material_t>(*material);
    require(copied_material->program() == material->program());
    require(copied_material->depth_compare() == api::comparison_t::greater);
    require(copied_material->front_face() == api::winding_t::clockwise && copied_material->cull() == api::cull_mode_t::front);
    copied_material->depth_test(true);
    copied_material->depth_write(false);
    copied_material->depth_compare(api::comparison_t::less);
    copied_material->front_face(api::winding_t::counter_clockwise);
    copied_material->cull(api::cull_mode_t::none);
    require(!settings.depth_test() && settings.depth_write());
    require(settings.depth_compare() == api::comparison_t::greater);
    require(settings.front_face() == api::winding_t::clockwise && settings.cull() == api::cull_mode_t::front);

    auto item = make_visibility_item({{0, 0, 0, 1}}, {0}, api::vertex_primitive_topology_t::point);
    item.material() = material;
    auto shared_item = item;
    material->depth_test(true);
    material->depth_compare(api::comparison_t::less);
    require(shared_item.material()->depth_test());
    material->depth_write(false);
    require(!shared_item.material()->depth_write());
    material->depth_write(true);
    auto overlay_material = std::make_shared<api::material_t>(program);
    overlay_material->uniform(0, vector4f_t({0, 0, 1, 1}));
    shared_item.material() = overlay_material;
    require(shared_item.material()->program() == item.material()->program());
    std::vector<api::rgba8_t> pixels(16 * 16, clear_color);
    std::vector<float> depths(pixels.size(), 0.75F);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 16, 16));
    renderer.framebuffer().depth(depths);
    renderer.draw(make_camera(16, 16), item);
    renderer.draw(make_camera(16, 16), shared_item);
    expect_color(pixels[pixel_index(8, 8, 16)], blue);
    require(depths[pixel_index(8, 8, 16)] == 0.5F);
}

void test_depth_attachment_updates() {
    std::vector<api::rgba8_t> pixels(16 * 16, clear_color);
    std::vector<float> depths(pixels.size(), 0.75F);
    std::vector<float> replacement_depth(pixels.size(), 0.25F);
    api::framebuffer_t framebuffer(pixels, 16, 16);
    static_assert(std::is_same_v<decltype(std::as_const(framebuffer).depth()), std::span<float>>);
    require(framebuffer.depth().empty());
    framebuffer.depth(depths);
    require(framebuffer.depth().data() == depths.data());
    require(std::ranges::all_of(depths, [](float depth) { return depth == 0.75F; }));
    test::expect_throws<std::invalid_argument>([&] { framebuffer.depth(std::span(depths).first(255)); });
    std::vector<float> oversized_depth(257, 0.5F);
    test::expect_throws<std::invalid_argument>([&] { framebuffer.depth(oversized_depth); });
    require(framebuffer.depth().data() == depths.data());
    require(std::ranges::all_of(depths, [](float depth) { return depth == 0.75F; }));
    require(std::ranges::all_of(pixels, [](auto pixel) { return same_color(pixel, clear_color); }));
    api::framebuffer_t empty({}, 0, 16);
    empty.depth({});
    test::expect_throws<std::invalid_argument>([&] { empty.depth(depths); });
    require(empty.depth().empty());

    api::software_renderer_t renderer(framebuffer);
    framebuffer.depth(replacement_depth); // Rebinding a copied view does not affect the renderer.
    require(renderer.framebuffer().depth().data() == depths.data());
    renderer.framebuffer().depth()[0] = 0.5F;
    require(depths[0] == 0.5F);
    auto copied_view = renderer.framebuffer();
    copied_view.depth({});
    require(renderer.framebuffer().depth().data() == depths.data());
    auto borrowed_depth = renderer.framebuffer().depth();
    borrowed_depth = borrowed_depth.first(1);
    require(borrowed_depth.size() == 1);
    require(renderer.framebuffer().depth().size() == pixels.size());
    renderer.framebuffer().depth(replacement_depth);
    require(renderer.framebuffer().depth().data() == replacement_depth.data());
    require(std::ranges::all_of(replacement_depth, [](float depth) { return depth == 0.25F; }));
    test::expect_throws<std::invalid_argument>([&] { renderer.framebuffer().depth(std::span(depths).first(255)); });
    require(renderer.framebuffer().depth().data() == replacement_depth.data());

    const auto camera = make_camera(16, 16);
    auto item = make_visibility_item({{0, 0, 0, 1}}, {0}, api::vertex_primitive_topology_t::point);
    renderer.draw(camera, item);
    require(colored_pixel_count(pixels) == 0); // The replacement attachment rejects the sample.
    renderer.framebuffer().depth(depths);
    renderer.draw(camera, item);
    expect_color(pixels[pixel_index(8, 8, 16)], red);
    renderer.framebuffer().depth({});
    require(renderer.framebuffer().depth().empty());
    renderer.clear_color(clear_color);
    test::expect_throws<std::invalid_argument>([&] { renderer.draw(camera, item); });
    item.material()->depth_compare(api::comparison_t::always);
    item.material()->depth_write(false);
    test::expect_throws<std::invalid_argument>([&] { renderer.draw(camera, item); });
    test::expect_throws<std::invalid_argument>([&] { renderer.clear_depth(1.0F); });
    require(colored_pixel_count(pixels) == 0);
    item.material()->depth_test(false);
    renderer.draw(camera, item);
    expect_color(pixels[pixel_index(8, 8, 16)], red);
    item.material()->depth_test(true);
    renderer.framebuffer() = api::framebuffer_t({}, 0, 0);
    test::expect_no_throw([&] { renderer.draw(camera, item); });
    renderer.framebuffer() = api::framebuffer_t(pixels, 16, 16);
    api::camera_t outside({{20, 30}, {20, 30}}, api::orthographic_t({{-1, 1}, {-1, 1}}, 0, 2));
    test::expect_no_throw([&] { renderer.draw(outside, item); });
}

void test_depth_comparisons_and_controls() {
    const std::array comparisons {
        api::comparison_t::never, api::comparison_t::less, api::comparison_t::equal, api::comparison_t::less_equal,
        api::comparison_t::greater, api::comparison_t::not_equal, api::comparison_t::greater_equal, api::comparison_t::always
    };
    // Independent expected acceptance for incoming depths {0.25, 0.5, 0.75} against 0.5.
    const std::array<std::array<bool, 3>, 8> expected {{
        {false, false, false}, {true, false, false}, {false, true, false}, {true, true, false},
        {false, false, true}, {true, false, true}, {false, true, true}, {true, true, true}
    }};
    std::vector<api::rgba8_t> pixels(16 * 16, clear_color);
    std::vector<float> depths(pixels.size());
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 16, 16));
    renderer.framebuffer().depth(depths);
    const auto camera = make_camera(16, 16);
    const auto center = pixel_index(8, 8, 16);
    for (std::size_t comparison = 0; comparison < comparisons.size(); ++comparison) {
        for (std::size_t sample = 0; sample < 3; ++sample) {
            const float incoming = float(sample + 1) * 0.25F;
            auto item = make_visibility_item({{0, 0, incoming * 2 - 1, 1}}, {0}, api::vertex_primitive_topology_t::point);
            item.material()->depth_compare(comparisons[comparison]);
            for (bool write : {false, true}) {
                renderer.clear_color(clear_color);
                std::ranges::fill(depths, 0.5F);
                item.material()->depth_write(write);
                renderer.draw(camera, item);
                expect_color(pixels[center], expected[comparison][sample] ? red : clear_color);
                require(depths[center] == (write && expected[comparison][sample] ? incoming : 0.5F));
            }
        }
    }
    auto item = make_visibility_item({{0, 0, 0, 1}}, {0}, api::vertex_primitive_topology_t::point);
    for (bool test_depth : {false, true}) {
        for (bool write_depth : {false, true}) {
            for (float stored : {0.25F, 0.75F}) {
                renderer.clear_color(clear_color);
                std::ranges::fill(depths, stored);
                item.material()->depth_test(test_depth);
                item.material()->depth_write(write_depth);
                renderer.draw(camera, item);
                const bool passes = !test_depth || stored == 0.75F;
                expect_color(pixels[center], passes ? red : clear_color);
                require(depths[center] == (passes && test_depth && write_depth ? 0.5F : stored));
            }
        }
    }
    for (float ndc_z : {-1.0F, 1.0F}) {
        item = make_visibility_item({{0, 0, ndc_z, 1}}, {0}, api::vertex_primitive_topology_t::point);
        for (auto comparison : {api::comparison_t::less, api::comparison_t::less_equal}) {
            renderer.clear_color(clear_color);
            std::ranges::fill(depths, 1.0F);
            item.material()->depth_compare(comparison);
            renderer.draw(camera, item);
            const bool passes = ndc_z == -1 || comparison == api::comparison_t::less_equal;
            expect_color(pixels[center], passes ? red : clear_color);
            require(depths[center] == (ndc_z == -1 ? 0.0F : 1.0F));
        }
    }
}

void test_depth_visibility_and_fragment_results() {
    std::vector<api::rgba8_t> pixels(16 * 16, clear_color);
    std::vector<float> depths(pixels.size());
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 16, 16));
    renderer.framebuffer().depth(depths);
    const auto camera = make_camera(16, 16);
    const api::index_buffer_t::indices_t indices {0, 1, 2, 2, 1, 3};
    auto near_item = make_visibility_item(visibility_quad(-0.5F), indices, api::vertex_primitive_topology_t::triangle);
    auto far_item = make_visibility_item(visibility_quad(0.5F), indices, api::vertex_primitive_topology_t::triangle, {0, 0, 1, 1});
    for (bool reverse : {false, true}) {
        renderer.clear_color(clear_color);
        std::ranges::fill(depths, 1.0F);
        renderer.draw(camera, reverse ? near_item : far_item);
        renderer.draw(camera, reverse ? far_item : near_item);
        for (std::size_t i = 0; i < pixels.size(); ++i) {
            expect_color(pixels[i], red);
            require(depths[i] == 0.25F);
        }
    }
    // A normally completed fragment may write depth without supplying color.
    // Conditional discard occurs after color assignment and must cancel both writes.
    for (bool write_color : {false, true}) {
        near_item.material() = std::make_shared<api::material_t>(make_visibility_program(write_color, true));
        near_item.material()->uniform(0, vector4f_t({1, 0, 0, 1}));
        near_item.material()->depth_test(true);
        renderer.clear_color(clear_color);
        std::ranges::fill(depths, 1.0F);
        renderer.draw(camera, near_item);
        renderer.draw(camera, far_item);
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                const auto i = pixel_index(x, y, 16);
                expect_color(pixels[i], x < 8 ? blue : (write_color ? red : clear_color));
                require(depths[i] == (x < 8 ? 0.75F : 0.25F));
            }
        }
    }
    // Equal depths follow the exact comparison, including across material changes.
    auto equal_item = make_visibility_item(visibility_quad(0.5F), indices, api::vertex_primitive_topology_t::triangle);
    for (auto comparison : {api::comparison_t::less, api::comparison_t::less_equal}) {
        equal_item.material()->depth_compare(comparison);
        renderer.clear_color(clear_color);
        std::ranges::fill(depths, 1.0F);
        renderer.draw(camera, far_item);
        renderer.draw(camera, equal_item);
        for (const auto pixel : pixels) { expect_color(pixel, comparison == api::comparison_t::less ? blue : red); }
    }
    // Two intersecting planes with unequal W: their analytical depth is linear in NDC X.
    for (bool unequal_w : {false, true}) {
        auto rising = visibility_quad(0), falling = rising;
        for (std::size_t i = 0; i < rising.size(); ++i) {
            const float w = unequal_w ? float(std::size_t(1) << i) : 1.0F;
            rising[i][2] = 0.5F * rising[i][0];
            falling[i][2] = -0.5F * falling[i][0];
            for (std::size_t axis = 0; axis < 4; ++axis) { rising[i][axis] *= w; falling[i][axis] *= w; }
        }
        near_item = make_visibility_item(rising, indices, api::vertex_primitive_topology_t::triangle);
        far_item = make_visibility_item(falling, indices, api::vertex_primitive_topology_t::triangle, {0, 0, 1, 1});
        for (bool reverse : {false, true}) {
            renderer.clear_color(clear_color);
            std::ranges::fill(depths, 1.0F);
            renderer.draw(camera, reverse ? near_item : far_item);
            renderer.draw(camera, reverse ? far_item : near_item);
            for (int y = 0; y < 16; ++y) {
                for (int x = 0; x < 16; ++x) {
                    const float ndc_x = (float(x) + 0.5F) / 8 - 1;
                    const auto i = pixel_index(x, y, 16);
                    expect_color(pixels[i], x < 8 ? red : blue);
                    require(depths[i] == 0.5F - 0.25F * std::abs(ndc_x));
                }
            }
        }
    }
}

void test_culling_and_topology_depth() {
    const std::array topologies {
        api::vertex_primitive_topology_t::point, api::vertex_primitive_topology_t::line,
        api::vertex_primitive_topology_t::line_strip, api::vertex_primitive_topology_t::line_loop,
        api::vertex_primitive_topology_t::triangle, api::vertex_primitive_topology_t::triangle_strip,
        api::vertex_primitive_topology_t::triangle_fan
    };
    std::vector<api::rgba8_t> pixels(16 * 16, clear_color);
    std::vector<float> depths(pixels.size());
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 16, 16));
    renderer.framebuffer().depth(depths);
    const auto camera = make_camera(16, 16);
    for (const auto topology : topologies) {
        const bool triangle = topology == api::vertex_primitive_topology_t::triangle || topology == api::vertex_primitive_topology_t::triangle_strip || topology == api::vertex_primitive_topology_t::triangle_fan;
        // These three assembly sequences produce CCW NDC faces under the existing topology contract.
        api::index_buffer_t::indices_t indices {0, 1, 2, 3};
        if (topology == api::vertex_primitive_topology_t::triangle) { indices = {0, 1, 2, 2, 1, 3}; }
        if (topology == api::vertex_primitive_topology_t::triangle_strip) { indices = {1, 0, 3, 2}; }
        if (topology == api::vertex_primitive_topology_t::triangle_fan) { indices = {0, 1, 3, 2}; }
        auto positions = visibility_quad(-0.5F);
        for (auto& position : positions) { position[0] *= 0.75F; position[1] *= 0.75F; }
        auto item = make_visibility_item(positions, indices, topology, {1, 0, 0, 1}, make_clip_program(true));
        renderer.clear_color(clear_color);
        std::ranges::fill(depths, 1.0F);
        renderer.draw(camera, item);
        const auto baseline = pixels;
        require(0 < colored_pixel_count(baseline));
        for (bool reverse : {false, true}) {
            if (reverse && !triangle) { continue; }
            auto selected_positions = positions;
            if (reverse) { for (auto& position : selected_positions) { position[0] = -position[0]; } }
            item.geometry() = make_typed_geometry(selected_positions, api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 4), indices, topology);
            for (auto winding : {api::winding_t::counter_clockwise, api::winding_t::clockwise}) {
                for (auto cull : {api::cull_mode_t::none, api::cull_mode_t::front, api::cull_mode_t::back, api::cull_mode_t::both}) {
                    auto& material = *item.material();
                    material.front_face(winding);
                    material.cull(cull);
                    renderer.clear_color(clear_color);
                    std::ranges::fill(depths, 1.0F);
                    renderer.draw(camera, item);
                    const bool front = !triangle || (reverse == (winding == api::winding_t::clockwise));
                    const bool culled = triangle && (cull == api::cull_mode_t::both || (front ? cull == api::cull_mode_t::front : cull == api::cull_mode_t::back));
                    for (std::size_t i = 0; i < pixels.size(); ++i) {
                        const bool covered = !same_color(baseline[i], clear_color) && !culled;
                        expect_color(pixels[i], covered ? (front ? red : blue) : clear_color);
                        require(depths[i] == (covered ? 0.25F : 1.0F));
                    }
                }
            }
        }
        item.material()->depth_test(true);
        item.material()->depth_write(true);
        item.material()->depth_compare(api::comparison_t::less);
        item.material()->front_face(api::winding_t::counter_clockwise);
        item.material()->cull(api::cull_mode_t::none);
        std::ranges::fill(depths, 0.0F);
        renderer.clear_color(clear_color);
        renderer.draw(camera, item);
        require(colored_pixel_count(pixels) == 0);
        require(std::ranges::all_of(depths, [](float depth) { return depth == 0.0F; }));
    }
    // Preserve the existing per-original-triangle facing through pathological clipping/snapping.
    std::vector<api::rgba8_t> clipped_pixels(32 * 32);
    std::vector<float> clipped_depth(clipped_pixels.size());
    renderer.framebuffer() = api::framebuffer_t(clipped_pixels, 32, 32);
    renderer.framebuffer().depth(clipped_depth);
    for (const auto& fixture : {crossing, concave}) {
        for (bool reverse : {false, true}) {
            auto item = make_visibility_item({fixture.begin(), fixture.end()}, reverse ? api::index_buffer_t::indices_t {2, 1, 0} : api::index_buffer_t::indices_t {0, 1, 2}, api::vertex_primitive_topology_t::triangle);
            for (auto cull : {api::cull_mode_t::front, api::cull_mode_t::back}) {
                item.material()->cull(cull);
                item.material()->depth_compare(api::comparison_t::always);
                renderer.clear_color(clear_color);
                std::ranges::fill(clipped_depth, 1.0F);
                renderer.draw(make_camera(32, 32), item);
                const bool visible = reverse ? cull == api::cull_mode_t::back : cull == api::cull_mode_t::front;
                require(colored_pixel_count(clipped_pixels) == (visible ? 1U : 0U));
                expect_color(clipped_pixels[0], visible ? red : clear_color);
            }
        }
    }
}

void test_projected_depth_visibility() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>>(0);
    vertex.position(vertex.world_to_clip() * vertex.object_to_world() * vertex.construct<vector4f_t>(position, 1.0F));
    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(fragment.uniform<vector4f_t>(0));
    const auto program = std::make_shared<const software_shader::program_t>(std::move(vertex).finalize(), std::move(fragment).finalize());
    const auto geometry = make_typed_geometry(std::vector<std::array<float, 3>>{{-1, -1, 0}, {1, -1, 0}, {-1, 1, 0}, {1, 1, 0}}, api::vertex_attribute_t(api::vertex_attribute_type_t::R32, 3), {0, 1, 2, 2, 1, 3}, api::vertex_primitive_topology_t::triangle);
    auto red_material = std::make_shared<api::material_t>(program);
    auto blue_material = std::make_shared<api::material_t>(program);
    red_material->uniform(0, vector4f_t({1, 0, 0, 1}));
    blue_material->uniform(0, vector4f_t({0, 0, 1, 1}));
    red_material->depth_test(true);
    red_material->cull(api::cull_mode_t::back);
    blue_material->depth_test(true);
    blue_material->cull(api::cull_mode_t::back);
    auto red_item = make_render_item(geometry, red_material);
    auto blue_item = make_render_item(geometry, blue_material);
    const float angle = std::atan2(0.6F, 0.8F);
    red_item.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, angle, 0}));
    blue_item.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0, -angle, 0}));
    red_item.translation() = blue_item.translation() = {0, 0, -1};
    std::vector<api::rgba8_t> pixels(32 * 32);
    std::vector<float> depths(pixels.size());
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 32, 32));
    renderer.framebuffer().depth(depths);
    for (bool perspective : {false, true}) {
        api::camera_t camera({{0, 32}, {0, 32}}, api::orthographic_t({{-1, 1}, {-1, 1}}, 0.75F, 10));
        if (perspective) { camera.projection() = api::perspective_t(std::numbers::pi_v<float> / 2, 0.75F, 10); }
        std::vector<api::rgba8_t> baseline;
        std::vector<float> baseline_depth;
        for (bool reverse : {false, true}) {
            renderer.clear_color(clear_color);
            std::ranges::fill(depths, 1.0F);
            renderer.draw(camera, reverse ? blue_item : red_item);
            renderer.draw(camera, reverse ? red_item : blue_item);
            if (!reverse) { baseline = pixels; baseline_depth = depths; }
            else {
                require(std::equal(pixels.begin(), pixels.end(), baseline.begin(), same_color));
                require(depths == baseline_depth);
            }
            int checked = 0, overlapping = 0, clipped = 0;
            for (int y = 0; y < 32; ++y) {
                for (int x = 0; x < 32; ++x) {
                    const double nx = (double(x) + 0.5) / 16 - 1, ny = 1 - (double(y) + 0.5) / 16;
                    std::array<double, 2> distances;
                    std::array<bool, 2> visible;
                    bool boundary = false;
                    for (std::size_t plane = 0; plane < 2; ++plane) {
                        const double slope = plane == 0 ? 0.75 : -0.75;
                        const double distance = perspective ? 1 / (1 - slope * nx) : 1 + slope * nx;
                        const double local_x = (perspective ? distance * nx : nx) / 0.8;
                        const double local_y = perspective ? distance * ny : ny;
                        boundary |= std::abs(std::abs(local_x) - 1) < 0.03 || std::abs(std::abs(local_y) - 1) < 0.03 || std::abs(distance - 0.75) < 0.03;
                        const bool inside = std::abs(local_x) < 1 && std::abs(local_y) < 1;
                        visible[plane] = inside && 0.75 <= distance;
                        distances[plane] = distance;
                        clipped += inside && distance < 0.75;
                    }
                    if (boundary) { continue; }
                    const bool red_visible = visible[0] && (!visible[1] || distances[0] < distances[1]);
                    const bool any_visible = visible[0] || visible[1];
                    const double distance = distances[red_visible ? 0 : 1];
                    // Analytical projection of the winning ray/plane intersection.
                    const double expected_depth = !any_visible ? 1 : (perspective ? 10.0 / 9.25 - 7.5 / (9.25 * distance) : (distance - 0.75) / 9.25);
                    const auto i = pixel_index(x, y, 32);
                    expect_color(pixels[i], any_visible ? (red_visible ? red : blue) : clear_color);
                    require(std::abs(depths[i] - expected_depth) < 0.0002); // Includes the established 1/256-pixel snapping.
                    ++checked;
                    overlapping += visible[0] && visible[1];
                }
            }
            require(500 < checked && 50 < overlapping && 10 < clipped);
        }
        // Negative object scale reverses winding; culling follows the transformed face.
        red_item.scale()[0] = -1;
        renderer.clear_color(clear_color);
        std::ranges::fill(depths, 1.0F);
        renderer.draw(camera, red_item);
        require(colored_pixel_count(pixels) == 0);
        require(std::ranges::all_of(depths, [](float depth) { return depth == 1.0F; }));
        red_material->front_face(api::winding_t::clockwise);
        renderer.draw(camera, red_item);
        require(100 < colored_pixel_count(pixels));
        red_material->front_face(api::winding_t::counter_clockwise);
        red_item.scale()[0] = 1;
    }
}

void test_bounded_depth_writes() {
    std::vector<api::rgba8_t> pixels(16 * 16);
    std::vector<float> depths(pixels.size());
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 16, 16));
    renderer.framebuffer().depth(depths);
    auto item = make_visibility_item(visibility_quad(0), {0, 1, 2, 2, 1, 3}, api::vertex_primitive_topology_t::triangle);
    using rect_t = m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>;
    const std::array rectangles {
        rect_t {{3, 12}, {5, 14}}, rect_t {{-6, 10}, {-4, 12}},
        rect_t {{10, 24}, {6, 30}}, rect_t {{-10, -1}, {0, 16}}, rect_t {{4, 4}, {2, 12}},
        rect_t {{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}, {0, 16}}
    };
    for (const auto& rectangle : rectangles) {
        const api::camera_t camera(rectangle, api::orthographic_t({{-1, 1}, {-1, 1}}, 0, 2));
        renderer.clear_color(clear_color);
        std::ranges::fill(depths, 1.0F);
        renderer.draw(camera, item);
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                const auto i = pixel_index(x, y, 16);
                const bool covered = rectangle.contains({x, y});
                expect_color(pixels[i], covered ? red : clear_color);
                require(depths[i] == (covered ? 0.5F : 1.0F));
            }
        }
    }
    // Rebinding replaces both borrowed attachments; subsequent draws must not use old storage.
    std::vector<api::rgba8_t> replacement_pixels(4 * 4, clear_color);
    std::vector<float> replacement_depth(4 * 4, 1.0F);
    const auto old_pixels = pixels;
    const auto old_depth = depths;
    api::framebuffer_t replacement(replacement_pixels, 4, 4);
    replacement.depth(replacement_depth);
    renderer.framebuffer() = replacement;
    renderer.draw(make_camera(4, 4), item);
    require(std::equal(pixels.begin(), pixels.end(), old_pixels.begin(), same_color));
    require(depths == old_depth);
    for (std::size_t i = 0; i < replacement_pixels.size(); ++i) {
        expect_color(replacement_pixels[i], red);
        require(replacement_depth[i] == 0.5F);
    }
}

void test_depth_clears() {
    std::vector<api::rgba8_t> pixels(16 * 16, clear_color);
    std::vector<float> depths(pixels.size(), 0.75F);
    api::software_renderer_t renderer(api::framebuffer_t(pixels, 16, 16));
    renderer.framebuffer().depth(depths);
    using rect_t = m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>;
    const std::array rectangles {
        rect_t {{3, 12}, {5, 14}}, rect_t {{-6, 10}, {-4, 12}},
        rect_t {{10, 24}, {6, 30}}, rect_t {{-10, -1}, {0, 16}}, rect_t {{4, 4}, {2, 12}},
        rect_t {{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}, {0, 16}}
    };
    auto item = make_visibility_item(visibility_quad(0), {0, 1, 2, 2, 1, 3}, api::vertex_primitive_topology_t::triangle);
    item.material()->depth_write(false);
    for (const auto& rectangle : rectangles) {
        api::camera_t camera(rectangle, api::orthographic_t({{-1, 1}, {-1, 1}}, 0, 2));
        renderer.clear_color(clear_color);
        renderer.clear_depth(1.0F);
        renderer.draw(camera, item); // A previous draw with writes disabled cannot mask clearing.
        const auto previous_colors = pixels;
        camera.position() = {std::numeric_limits<float>::quiet_NaN(), 0, 0};
        renderer.clear_depth(camera, 0.25F);
        require(std::equal(pixels.begin(), pixels.end(), previous_colors.begin(), same_color));
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                require(depths[pixel_index(x, y, 16)] == (rectangle.contains({x, y}) ? 0.25F : 1.0F));
            }
        }
        const auto previous_depths = depths;
        renderer.clear_color(camera, green);
        require(depths == previous_depths);
    }
    // Clamping includes infinities; NaN is rejected before either clear can write.
    for (float supplied : {-std::numeric_limits<float>::infinity(), -0.5F, 0.0F, 0.5F, 1.0F, 2.0F, std::numeric_limits<float>::infinity()}) {
        renderer.clear_depth(supplied);
        require(std::ranges::all_of(depths, [&](float depth) { return depth == std::clamp(supplied, 0.0F, 1.0F); }));
        renderer.clear_depth(make_camera(16, 16), supplied);
        require(std::ranges::all_of(depths, [&](float depth) { return depth == std::clamp(supplied, 0.0F, 1.0F); }));
    }
    const float nan = std::numeric_limits<float>::quiet_NaN();
    renderer.clear_depth(0.75F);
    test::expect_throws<std::invalid_argument>([&] { renderer.clear_depth(nan); });
    test::expect_throws<std::invalid_argument>([&] { renderer.clear_depth(make_camera(16, 16), nan); });
    require(std::ranges::all_of(depths, [](float depth) { return depth == 0.75F; }));
    renderer.framebuffer() = api::framebuffer_t(pixels, 16, 16);
    test::expect_throws<std::invalid_argument>([&] { renderer.clear_depth(1.0F); });
    test::expect_throws<std::invalid_argument>([&] { renderer.clear_depth(make_camera(16, 16), 1.0F); });
    api::camera_t outside({{20, 30}, {20, 30}}, api::orthographic_t({{-1, 1}, {-1, 1}}, 0, 2));
    test::expect_no_throw([&] { renderer.clear_depth(outside, nan); });
    renderer.framebuffer() = api::framebuffer_t({}, 0, 16);
    test::expect_no_throw([&] { renderer.clear_depth(nan); });
    test::expect_no_throw([&] { renderer.clear_depth(make_camera(16, 16), nan); });
}

void test_profiling() {
    using profiler_t = profiling::profiler_t;
    std::array<std::byte, 8192> storage;
    profiler_t profiler(storage);
    std::vector<api::rgba8_t> measured_pixels(256), normal_pixels(256);
    std::vector<float> measured_depth(256), normal_depth(256);
    api::framebuffer_t measured_framebuffer(measured_pixels, 16, 16), normal_framebuffer(normal_pixels, 16, 16);
    measured_framebuffer.depth(measured_depth);
    normal_framebuffer.depth(normal_depth);
    api::software_renderer_t measured(measured_framebuffer);
    measured.profiler(profiler);
    api::software_renderer_t normal(normal_framebuffer);
    profiler.start();
    const auto camera = make_camera(16, 16);
    for (int mode = 0; mode < 6; ++mode) {
        auto item = make_visibility_item(visibility_quad(0), {0, 1, 2, 2, 1, 3}, api::vertex_primitive_topology_t::triangle);
        if (mode == 2 || mode == 3) {
            item.material() = std::make_shared<api::material_t>(make_visibility_program(mode != 3, true));
            item.material()->uniform(0, vector4f_t({1, 0, 0, 1}));
            item.material()->depth_test(true);
        }
        if (mode == 4) { item.material()->cull(api::cull_mode_t::both); }
        const std::size_t draws = mode == 5 ? 2 : 1;
        const auto render = [&](auto& renderer) {
            renderer.clear_color(clear_color);
            renderer.clear_depth(mode == 1 ? 0.0F : 1.0F);
            for (std::size_t i = 0; i < draws; ++i) { renderer.draw(camera, item); }
        };
        profiler.reset();
        {
            auto metric = profiler.metric<application_metrics_t>();
            render(measured);
            if (metric) { metric->m_items = draws; }
        }
        render(normal);
        require(std::equal(measured_pixels.begin(), measured_pixels.end(), normal_pixels.begin(), same_color));
        require(measured_depth == normal_depth);
        require(profiler.records().size() == 3 + 4 * draws && profiler.omitted() == 0);
        require(profiler.records()[0].metrics<application_metrics_t>()->m_items == draws);
        require(profiler.records()[1].metrics<clear_metrics_t>()->m_color_writes == 256);
        require(profiler.records()[2].metrics<clear_metrics_t>()->m_depth_writes == 256);
        std::size_t vertices = 0;
        raster_metrics_t accumulated_raster_metrics;
        for (const auto& record : profiler.records()) {
            if (const auto* vertex_metrics = record.metrics<vertex_metrics_t>()) {
                vertices += vertex_metrics->m_invocations;
                require(vertex_metrics->m_expected == 6);
            }
            if (const auto* recorded_raster_metrics = record.metrics<raster_metrics_t>()) {
                accumulated_raster_metrics.m_invocations += recorded_raster_metrics->m_invocations;
                accumulated_raster_metrics.m_discards += recorded_raster_metrics->m_discards;
                accumulated_raster_metrics.m_depth_rejections += recorded_raster_metrics->m_depth_rejections;
                accumulated_raster_metrics.m_color_writes += recorded_raster_metrics->m_color_writes;
                accumulated_raster_metrics.m_depth_writes += recorded_raster_metrics->m_depth_writes;
                require(record.parent() && profiler.records()[*record.parent()].metrics<draw_metrics_t>());
            }
        }
        require(vertices == 6 * draws);
        require(accumulated_raster_metrics.m_invocations == (mode == 4 ? 0 : 256 * draws));
        require(accumulated_raster_metrics.m_discards == (mode == 2 || mode == 3 ? 128 : 0));
        require(accumulated_raster_metrics.m_depth_rejections == (mode == 1 || mode == 5 ? 256 : 0));
        const std::size_t writes = mode == 1 || mode == 4 ? 0 : (mode == 2 || mode == 3 ? 128 : 256);
        require(accumulated_raster_metrics.m_depth_writes == writes);
        require(accumulated_raster_metrics.m_color_writes == (mode == 3 ? 0 : writes));
    }
    std::ostringstream report;
    profiler.report(report);
    require(report.str().find("application.frame") != std::string::npos && report.str().find("items=2") != std::string::npos);
    require(report.str().find("vertex_invocations=6") != std::string::npos);

    // Attached and unattached renderers reject the same invalid resources; completed scope records survive.
    profiler.reset();
    test::expect_throws([&] { measured.draw(camera, api::render_item_t{}); });
    test::expect_throws([&] { normal.draw(camera, api::render_item_t{}); });
    require(profiler.records().size() == 2 && profiler.records()[0].unwinding() && profiler.records()[1].unwinding());

    // Empty intersections retain their original validation bypass in both configurations.
    profiler.reset();
    auto empty_camera = make_camera(0, 0);
    test::expect_no_throw([&] { measured.draw(empty_camera, api::render_item_t{}); });
    test::expect_no_throw([&] { normal.draw(empty_camera, api::render_item_t{}); });
    require(profiler.records().size() == 2 && !profiler.records()[0].unwinding());

    // Points, lines, all assembly forms, and clipping use the same instrumented path.
    for (const auto topology : {api::vertex_primitive_topology_t::point, api::vertex_primitive_topology_t::line,
             api::vertex_primitive_topology_t::line_strip, api::vertex_primitive_topology_t::line_loop,
             api::vertex_primitive_topology_t::triangle, api::vertex_primitive_topology_t::triangle_strip,
             api::vertex_primitive_topology_t::triangle_fan}) {
        auto indices = topology == api::vertex_primitive_topology_t::triangle ? api::index_buffer_t::indices_t{0, 1, 2} : api::index_buffer_t::indices_t{0, 1, 2, 3};
        auto positions = visibility_quad(0);
        positions[0] = {-2, 2, -2, 1};
        auto item = make_visibility_item(positions, indices, topology);
        profiler.reset();
        measured.clear_color(clear_color); normal.clear_color(clear_color);
        measured.clear_depth(1); normal.clear_depth(1);
        measured.draw(camera, item); normal.draw(camera, item);
        require(std::equal(measured_pixels.begin(), measured_pixels.end(), normal_pixels.begin(), same_color));
        require(measured_depth == normal_depth);
    }

    // Count a shader invocation before entering it, even when program.run throws.
    profiler.reset();
    shader::vertex_shader_ast_builder_t throwing_vertex;
    throwing_vertex.branch(throwing_vertex.uniform<bool>(0), [&] {
        throwing_vertex.position(vector4f_t({0, 0, 0, 1}));
    });
    shader::fragment_shader_ast_builder_t throwing_fragment;
    throwing_fragment.color(vector4f_t({1, 0, 0, 1}));
    auto throwing_program = std::make_shared<const software_shader::program_t>(
        std::move(throwing_vertex).finalize(), std::move(throwing_fragment).finalize());
    auto throwing_item = make_visibility_item(visibility_quad(0), {0, 1, 2, 2, 1, 3}, api::vertex_primitive_topology_t::triangle);
    throwing_item.material() = std::make_shared<api::material_t>(throwing_program);
    throwing_item.material()->uniform(0, false);
    test::expect_throws<std::runtime_error>([&] { measured.draw(camera, throwing_item); });
    test::expect_throws<std::runtime_error>([&] { normal.draw(camera, throwing_item); });
    require(profiler.records().size() == 3);
    require(profiler.records()[2].unwinding());
    require(profiler.records()[2].metrics<vertex_metrics_t>()->m_invocations == 1);
    require(profiler.records()[2].metrics<vertex_metrics_t>()->m_expected == 6);
    require(!profiler.records()[1].unwinding());

    // Attachment changes require both collectors to be quiescent and leave
    // the old attachment intact on failure.
    profiler.reset();
    {
        auto metric = profiler.metric<application_metrics_t>();
        test::expect_throws([&] { measured.profiler(nullptr); });
        test::expect_throws([&] { normal.profiler(profiler); });
    }
    profiler.reset();
    measured.profiler(nullptr);
    measured.clear_color(clear_color);
    require(profiler.records().empty());
    measured.profiler(profiler);
    measured.clear_color(empty_camera, clear_color);
    measured.clear_depth(empty_camera, 1);
    require(profiler.records()[0].metrics<clear_metrics_t>()->m_target == clear_target_t::color);
    require(profiler.records()[1].metrics<clear_metrics_t>()->m_target == clear_target_t::depth);
    require(profiler.records()[0].metrics<clear_metrics_t>()->m_color_writes == 0);
    require(profiler.records()[1].metrics<clear_metrics_t>()->m_depth_writes == 0);

    // Record exhaustion never changes rendering or prevents future capture resets.
    // Find the smallest borrowed buffer that retains one timing-only scope;
    // capacity checks must not depend on the collector's private record layout.
    std::array<std::byte, 512> tiny_storage;
    std::size_t capacity = 0;
    for (; capacity < tiny_storage.size(); ++capacity) {
        profiler_t probe(std::span<std::byte>(tiny_storage).first(capacity));
        probe.start();
        auto metric = probe.metric<draw_metrics_t>();
        if (metric) { break; }
    }
    require(capacity < tiny_storage.size());
    profiler_t tiny(std::span<std::byte>(tiny_storage).first(capacity));
    api::software_renderer_t overflowing(measured_framebuffer);
    overflowing.profiler(tiny);
    tiny.start();
    auto item = make_visibility_item(visibility_quad(0), {0, 1, 2, 2, 1, 3}, api::vertex_primitive_topology_t::triangle);
    std::ranges::fill(measured_depth, 1); std::ranges::fill(normal_depth, 1);
    overflowing.draw(camera, item); normal.draw(camera, item);
    require(tiny.omitted() == 3 && !tiny.records()[0].self());
    require(std::equal(measured_pixels.begin(), measured_pixels.end(), normal_pixels.begin(), same_color));
    require(measured_depth == normal_depth);
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
    test_profiling();
    test_material_setting_invariants();
    test_depth_attachment_updates();
    test_depth_comparisons_and_controls();
    test_depth_visibility_and_fragment_results();
    test_culling_and_topology_depth();
    test_projected_depth_visibility();
    test_bounded_depth_writes();
    test_depth_clears();
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
