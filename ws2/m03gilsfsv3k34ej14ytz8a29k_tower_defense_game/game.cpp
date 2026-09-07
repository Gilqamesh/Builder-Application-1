#include "game.h"

#include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
#include <m03gtgtrh2smvh28qlwgm7gdl4_quaternion/api.h>
#include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>
#include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>
#include <m03gt1djvvy5atia5evkbg6rqy_software_shader/api.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace {

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

}

namespace {

namespace glfw = m03gkcdy62bnz808pmk4uzkjra_glfw;
namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
namespace software_shader = m03gt1djvvy5atia5evkbg6rqy_software_shader;
namespace software_renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;

std::shared_ptr<glfw::window_t> make_window() {
    glfw::window_creation_settings_t settings;
    settings.opengl(4, 6, glfw::opengl_profile_t::core);
    auto window = glfw::window_t::create(
        "Tower Defense Game",
        m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 4> {100, 100, 1600, 1200},
        settings
    );
    if (!window) {
        throw std::runtime_error("game_t failed to create its window");
    }
    return window;
}

constexpr software_renderer::rgba8_t ray_white() {
    return {245, 245, 245, 255};
}

std::shared_ptr<const software_shader::program_t> make_program() {
    using vector2f_t = shader::vector_t<float, 2>;
    using vector4f_t = shader::vector_t<float, 4>;

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

} // namespace

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

namespace texture_api = m03gt0l0q3l4b1k27eab5k7py1_texture;
namespace software_renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;

using software_renderer::geometry_t;
using software_renderer::index_buffer_t;
using software_renderer::material_t;
using software_renderer::mesh_t;
using software_renderer::render_item_t;
using software_renderer::vertex_attribute_t;
using software_renderer::vertex_attribute_type_t;
using software_renderer::vertex_primitive_topology_t;

game_t::game_t():
    m_window(make_window()),
    m_pixels(),
    m_software_renderer(software_renderer::framebuffer_t(m_pixels, 0, 0)),
    m_opengl_renderer(m_window),
    m_camera({{0, 400}, {0, 200}}, software_renderer::orthographic_t({{-400, 400}, {-300, 300}}, 0.0F, 2.0F))
{
    // Keep the game's world +Y direction pointing down the framebuffer.
    m_camera.position() = {0.0F, 0.0F, -1.0F};
    m_camera.rotation(m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>(0.0F, 1.0F, 0.0F, 0.0F));

    std::vector<std::shared_ptr<texture_api::texture_t>> tile_textures;
    std::vector<std::string> tile_texture_paths = {
        "assets/grass.png",
        "assets/road_four_way.png",
        "assets/road_straight.png",
        "assets/road_three_way.png",
        "assets/road_turn.png"
    };
    for (const auto& tile_texture_path : tile_texture_paths) {
        int tile_texture_width;
        int tile_texture_height;
        int tile_texture_channels;
        unsigned char* tile_texture_data = stbi_load(tile_texture_path.c_str(), &tile_texture_width, &tile_texture_height, &tile_texture_channels, 4);
        m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t tile_texture_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(tile_texture_data), tile_texture_width * tile_texture_height * 4));
        std::shared_ptr<texture_api::texture_t> tile_texture = std::make_shared<texture_api::texture_t>(
            texture_api::format_t::rgba8_unorm,
            tile_texture_width,
            tile_texture_height,
            std::move(tile_texture_bytes)
        );
        tile_textures.push_back(tile_texture);
    }
    std::shared_ptr<texture_api::sampler_t> tile_sampler = std::make_shared<texture_api::sampler_t>(
        texture_api::filter_t::nearest,
        texture_api::address_mode_t::clamp_to_edge,
        texture_api::address_mode_t::clamp_to_edge
    );
    std::vector<std::shared_ptr<material_t>> materials;
    const auto program = make_program();
    for (const auto& tile_texture : tile_textures) {
        std::shared_ptr<material_t> material = std::make_shared<material_t>(program);
        material->texture(0, tile_texture);
        material->sampler(0, tile_sampler);
        materials.push_back(material);
    }

    std::vector<vertex_attribute_t> vertex_attributes = {
        { vertex_attribute_type_t::R32, 2 }
    };

    std::vector<std::shared_ptr<mesh_t>> meshes;
    const auto number_of_meshes = 10;
    for (size_t i = 0; i < number_of_meshes; ++i) {
        const auto minimum_number_of_vertices = 3;
        const auto maximum_number_of_vertices = 15;
        const auto number_of_vertices = minimum_number_of_vertices + rand() % (maximum_number_of_vertices - minimum_number_of_vertices + 1);
        m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays::structure_of_arrays_t<std::array<float, 2>> vertex_streams;

        if (number_of_vertices == 1) {
            vertex_streams.push_back({0.0f, 0.0f});
        } else {
            const auto rotation = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * std::numbers::pi_v<float>;
            const auto angle_step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(number_of_vertices);

            for (size_t j = 0; j < number_of_vertices; ++j) {
                const auto angle = rotation - static_cast<float>(j) * angle_step;
                const auto x = std::cos(angle);
                const auto y = std::sin(angle);
                vertex_streams.push_back({x, y});
            }
        }

        mesh_t mesh(std::move(vertex_streams), vertex_attributes);
        meshes.push_back(std::make_shared<mesh_t>(std::move(mesh)));
    }

    const auto number_of_entities = 1000;
    for (size_t i = 0; i < number_of_entities; ++i) {
        render_item_t render_item;

        const auto mesh_index = rand() % meshes.size();
        std::shared_ptr<mesh_t> mesh = meshes[mesh_index];

        const auto number_of_vertices = mesh->number_of_vertices();

        std::vector<uint32_t> indices;
        for (uint32_t i = 0; i < number_of_vertices; ++i) {
            indices.push_back(i);
        }
        auto index_buffer = std::make_shared<index_buffer_t>();
        index_buffer->indices() = std::move(indices);
        auto geometry = std::make_shared<geometry_t>(index_buffer);
        geometry->mesh() = mesh;

        geometry->primitive_topology() = vertex_primitive_topology_t::triangle_fan;
        geometry->finalize();
        render_item.geometry() = std::move(geometry);

        const auto material_index = rand() % materials.size();
        std::shared_ptr<material_t> material = materials[material_index];
        render_item.material() = material;

        const auto max_horizontal_translation = 5000;
        const auto max_vertical_translation = 3000;
        m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> translation = {
            static_cast<float>(rand() % max_horizontal_translation - max_horizontal_translation / 2),
            static_cast<float>(rand() % max_vertical_translation - max_vertical_translation / 2),
            0.0F
        };
        render_item.translation() = translation;

        render_item.rotation(m03gtgtrh2smvh28qlwgm7gdl4_quaternion::quaternion_t<float>());

        const auto max_horizontal_scale = 30;
        const auto max_vertical_scale = 20;
        m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> scale = {
            static_cast<float>(rand() % max_horizontal_scale + 1),
            static_cast<float>(rand() % max_vertical_scale + 1),
            1.0F
        };
        render_item.scale() = scale;

        m_render_items.push_back(std::move(render_item));
    }
}

game_t::~game_t() {
}

void game_t::run() {
    m_software_renderer.profiler().enabled() = true;

    m_window->swap_interval(1);

    auto& input_states = m_window->input_states();
    input_states.commit();

    auto previous_frame_time = std::chrono::steady_clock::now();
    while (!m_window->should_close()) {
        const auto frame_time = std::chrono::steady_clock::now();
        const auto dt = std::chrono::duration<float>(frame_time - previous_frame_time).count();

        m03gkcdy62bnz808pmk4uzkjra_glfw::poll_events();
        input_states.commit();

        update(dt);
        render();

        const auto frame_time_ms = std::chrono::duration<double, std::milli>(frame_time - previous_frame_time);
        previous_frame_time = frame_time;
        const auto framebuffer = m_software_renderer.framebuffer();
        std::cout << std::format("frame: {:.2f} ms, framebuffer: {}x{}\n", frame_time_ms.count(), framebuffer.width(), framebuffer.height());
    }

    m_software_renderer.profiler().report(std::cout);
}

void game_t::update(float dt) {
    const auto& input_states = m_window->input_states();
    const auto& current_input_state = input_states.history(0);

    auto camera_view_dp = m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>{0, 0};
    auto camera_world_dp = m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>{0.0F, 0.0F, 0.0F};
    auto camera_view_lengths_dp = m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>{0, 0};
    auto camera_world_lengths_dp = m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>{0, 0};

    const auto& projection = std::get<software_renderer::orthographic_t>(m_camera.projection());
    const auto& camera_world_rect_bounds = projection.bounds().bounds();
    const auto camera_world_rect_horizontal_length = camera_world_rect_bounds[0].length();
    const auto camera_world_rect_vertical_length = camera_world_rect_bounds[1].length();
    const auto camera_world_horizontal_speed = std::max(1.0f, camera_world_rect_horizontal_length * dt);
    const auto camera_world_vertical_speed = std::max(1.0f, camera_world_rect_vertical_length * dt);

    const auto camera_view_rect = m_camera.view_rect();
    const auto camera_view_rect_bounds = camera_view_rect.bounds();
    const auto camera_view_rect_horizontal_length = camera_view_rect_bounds[0].length();
    const auto camera_view_rect_vertical_length = camera_view_rect_bounds[1].length();
    const auto camera_view_horizontal_speed = std::max(1, static_cast<int>(camera_view_rect_horizontal_length * dt));
    const auto camera_view_vertical_speed = std::max(1, static_cast<int>(camera_view_rect_vertical_length * dt));

    if (current_input_state.button_state(glfw::button_t::button_w).is_down()) {
        camera_world_dp[1] -= camera_world_vertical_speed;
    }
    if (current_input_state.button_state(glfw::button_t::button_s).is_down()) {
        camera_world_dp[1] += camera_world_vertical_speed;
    }
    if (current_input_state.button_state(glfw::button_t::button_a).is_down()) {
        camera_world_dp[0] -= camera_world_horizontal_speed;
    }
    if (current_input_state.button_state(glfw::button_t::button_d).is_down()) {
        camera_world_dp[0] += camera_world_horizontal_speed;
    }

    if (current_input_state.button_state(glfw::button_t::button_up).is_down()) {
        camera_view_dp[1] -= camera_view_vertical_speed;
    }
    if (current_input_state.button_state(glfw::button_t::button_down).is_down()) {
        camera_view_dp[1] += camera_view_vertical_speed;
    }
    if (current_input_state.button_state(glfw::button_t::button_left).is_down()) {
        camera_view_dp[0] -= camera_view_horizontal_speed;
    }
    if (current_input_state.button_state(glfw::button_t::button_right).is_down()) {
        camera_view_dp[0] += camera_view_horizontal_speed;
    }

    if (current_input_state.button_state(glfw::button_t::button_q).is_down()) {
        camera_world_lengths_dp += m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>{std::max(1.0f, camera_world_rect_horizontal_length * dt), std::max(1.0f, camera_world_rect_vertical_length * dt)};
    }
    if (current_input_state.button_state(glfw::button_t::button_e).is_down()) {
        camera_world_lengths_dp -= m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>{std::max(1.0f, camera_world_rect_horizontal_length * dt), std::max(1.0f, camera_world_rect_vertical_length * dt)};
    }
    if (current_input_state.button_state(glfw::button_t::button_f).is_down()) {
        camera_view_lengths_dp += m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>{std::max(1, static_cast<int>(camera_view_rect_horizontal_length * dt)), std::max(1, static_cast<int>(camera_view_rect_vertical_length * dt))};
    }
    if (current_input_state.button_state(glfw::button_t::button_r).is_down()) {
        camera_view_lengths_dp -= m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>{std::max(1, static_cast<int>(camera_view_rect_horizontal_length * dt)), std::max(1, static_cast<int>(camera_view_rect_vertical_length * dt))};
    }

    m_camera.position() += camera_world_dp;
    const auto projection_bounds = projection.bounds().inflate(camera_world_lengths_dp);
    if (!projection_bounds.is_empty()) {
        m_camera.projection() = software_renderer::orthographic_t(projection_bounds, projection.near_distance(), projection.far_distance());
    }
    m_camera.view_rect() += camera_view_dp;
    m_camera.view_rect() = m_camera.view_rect().inflate(camera_view_lengths_dp);
}

void game_t::render() {
    const auto size = m_window->framebuffer_size();
    auto framebuffer = m_software_renderer.framebuffer();
    if (framebuffer.width() != size[0] || framebuffer.height() != size[1]) {
        m_pixels.resize(software_renderer::framebuffer_t::pixel_count(size[0], size[1]));
        m_software_renderer.framebuffer() = software_renderer::framebuffer_t(m_pixels, size[0], size[1]);
        framebuffer = m_software_renderer.framebuffer();
    }
    if (framebuffer.width() == 0 || framebuffer.height() == 0) {
        return;
    }

    m_software_renderer.clear_color(ray_white());
    for (const auto& render_item : m_render_items) {
        m_software_renderer.draw(m_camera, render_item);
    }

    m_opengl_renderer.present_rgba8(
        std::as_bytes(std::span<const software_renderer::rgba8_t>(m_pixels)),
        framebuffer.width(),
        framebuffer.height()
    );
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
