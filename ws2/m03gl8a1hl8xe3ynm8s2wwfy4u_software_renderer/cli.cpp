#include "software_renderer.h"

#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>
#include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
#include <m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays/api.h>
#include <m03gkcdy62bnz808pmk4uzkjra_glfw/glfw.h>
#include <m03gkcdy62bnz808pmk4uzkjra_glfw/window.h>
#include <m03gl22hn0dqmosreqjie9tg5m_opengl_renderer/api.h>
#include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>
#include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>
#include <m03gt1djvvy5atia5evkbg6rqy_software_shader/api.h>
#include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

struct frame_metrics_t {};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::frame_metrics_t> : formatter<string_view> {
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::frame_metrics_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "application.frame");
        return out;
    }
};

} // namespace std

namespace {

namespace glfw_api = m03gkcdy62bnz808pmk4uzkjra_glfw;
namespace opengl_renderer_api = m03gl22hn0dqmosreqjie9tg5m_opengl_renderer;
namespace software_renderer_api = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
namespace byte_stream = m03gagbht2l61mj6qitacwbmea_byte_stream;
namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
namespace soa = m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays;
namespace software_shader = m03gt1djvvy5atia5evkbg6rqy_software_shader;
namespace texture = m03gt0l0q3l4b1k27eab5k7py1_texture;

using steady_clock_t = std::chrono::steady_clock;
using rgba8_t = software_renderer_api::rgba8_t;
using vector2f_t = shader::vector_t<float, 2>;
using vector3f_t = shader::vector_t<float, 3>;
using vector4f_t = shader::vector_t<float, 4>;

std::shared_ptr<const software_shader::program_t> make_program() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector3f_t>(0);
    const auto local = vertex.construct<vector4f_t>(position, 1.0F);
    vertex.position(vertex.world_to_clip() * vertex.object_to_world() * local);
    vertex.output(0, shader::swizzle<0, 1>(position) * 0.5F + vector2f_t({0.5F, 0.5F}));

    shader::fragment_shader_ast_builder_t fragment;
    const auto coordinates = fragment.input<vector2f_t>(0);
    const auto image = fragment.resource<shader::shader_texture_2d_t>(0);
    const auto sampler = fragment.resource<shader::shader_sampler_t>(0);
    fragment.color(shader::sample(image, sampler, coordinates) * fragment.uniform<vector4f_t>(0));

    return std::make_shared<const software_shader::program_t>(
        std::move(vertex).finalize(),
        std::move(fragment).finalize()
    );
}

std::shared_ptr<software_renderer_api::geometry_t> make_geometry() {
    soa::structure_of_arrays_t<std::array<float, 3>> streams;
    for (const auto& position : std::array {
        std::array {-1.0F, -1.0F, 0.0F},
        std::array {-1.0F, 1.0F, 0.0F},
        std::array {1.0F, -1.0F, 0.0F},
        std::array {1.0F, 1.0F, 0.0F}
    }) {
        streams.push_back(position);
    }
    auto mesh = std::make_shared<software_renderer_api::mesh_t>(
        std::move(streams),
        std::vector<software_renderer_api::vertex_attribute_t> {
            software_renderer_api::vertex_attribute_t(
                software_renderer_api::vertex_attribute_type_t::R32,
                3
            )
        }
    );
    auto indices = std::make_shared<software_renderer_api::index_buffer_t>();
    indices->indices() = {0, 1, 2, 3};
    auto geometry = std::make_shared<software_renderer_api::geometry_t>(std::move(indices));
    geometry->mesh() = std::move(mesh);
    geometry->primitive_topology() = software_renderer_api::vertex_primitive_topology_t::triangle_strip;
    geometry->finalize();
    return geometry;
}

std::shared_ptr<texture::texture_t> make_texture() {
    const std::array texels {
        rgba8_t {255, 72, 72, 255},
        rgba8_t {72, 255, 128, 255},
        rgba8_t {72, 128, 255, 255},
        rgba8_t {255, 232, 72, 255}
    };
    return std::make_shared<texture::texture_t>(
        texture::format_t::rgba8_unorm,
        2,
        2,
        byte_stream::byte_stream_t(std::as_bytes(std::span<const rgba8_t>(texels)))
    );
}

} // namespace

int main() {
    try {
        glfw_api::glfw_t glfw;

        glfw_api::window_creation_settings_t settings;
        settings.opengl(3, 3, glfw_api::opengl_profile_t::core);

        auto window = glfw_api::window_t::create("Software Renderer", {300, 200, 960, 540}, settings);
        if (!window) {
            throw std::runtime_error("software_renderer CLI failed to create its window");
        }
        window->swap_interval(1);

        std::vector<rgba8_t> pixels;
        std::vector<float> depth;
        software_renderer_api::software_renderer_t software_renderer(software_renderer_api::framebuffer_t(pixels, 0, 0));
        m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t profiler;
        profiler.enabled() = true;

        opengl_renderer_api::opengl_renderer_t opengl_renderer(window);
        auto material = std::make_shared<software_renderer_api::material_t>(make_program());
        material->texture(0, make_texture());
        material->sampler(0, std::make_shared<texture::sampler_t>(
            texture::filter_t::nearest,
            texture::address_mode_t::clamp_to_edge,
            texture::address_mode_t::clamp_to_edge
        ));
        material->uniform(0, vector4f_t({1.0F, 0.6F, 0.6F, 1.0F}));
        material->depth_test(true);
        material->cull(software_renderer_api::cull_mode_t::back);
        auto second_material = std::make_shared<software_renderer_api::material_t>(*material);
        second_material->uniform(0, vector4f_t({0.6F, 0.7F, 1.0F, 1.0F}));
        software_renderer_api::render_item_t render_item;
        render_item.geometry() = make_geometry();
        render_item.material() = std::move(material);
        render_item.scale() = {0.72F, 0.72F, 1.0F};
        auto second_item = render_item;
        second_item.material() = std::move(second_material);
        second_item.rotation(vector3f_t({-0.2F, -0.65F, 0.0F}));
        second_item.translation() = {0.15F, 0.0F, -1.35F};

        auto transparent_item = render_item;
        auto transparent_material = std::make_shared<software_renderer_api::material_t>(*transparent_item.material());
        transparent_material->uniform(0, vector4f_t({0.5F, 1.0F, 0.7F, 0.45F}));
        transparent_material->blend(true);
        transparent_material->blend_color({software_renderer_api::blend_factor_t::src_alpha, software_renderer_api::blend_factor_t::one_minus_src_alpha, software_renderer_api::blend_op_t::add});
        transparent_material->blend_alpha({software_renderer_api::blend_factor_t::one, software_renderer_api::blend_factor_t::one_minus_src_alpha, software_renderer_api::blend_op_t::add});
        transparent_material->depth_write(false);
        transparent_material->cull(software_renderer_api::cull_mode_t::none);
        transparent_item.material() = std::move(transparent_material);
        transparent_item.scale() = {0.55F, 0.55F, 1.0F};
        transparent_item.translation() = {0.0F, -0.15F, -0.95F};
        transparent_item.rotation(vector3f_t({0.2F, 0.35F, 0.1F}));

        const auto started_at = steady_clock_t::now();
        auto previous_frame_started_at = started_at;

        while (!window->should_close()) {
            auto frame_metric = profiler.metric<software_renderer_api::frame_metrics_t>();
            const auto frame_started_at = steady_clock_t::now();
            const auto seconds = std::chrono::duration<float>(frame_started_at - started_at).count();

            glfw_api::poll_events();

            const auto size = window->framebuffer_size();
            auto framebuffer = software_renderer.framebuffer();
            if (framebuffer.width() != size[0] || framebuffer.height() != size[1]) {
                pixels.resize(software_renderer_api::framebuffer_t::pixel_count(size[0], size[1]));
                depth.resize(pixels.size());
                software_renderer_api::framebuffer_t replacement(pixels, size[0], size[1]);
                replacement.depth(depth);
                replacement.encoding(software_renderer_api::color_encoding_t::srgb);
                software_renderer.framebuffer() = replacement;
                framebuffer = software_renderer.framebuffer();
            }

            if (framebuffer.width() == 0 || framebuffer.height() == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            } else {
                software_renderer.clear_color({0, 0, 0, 255}, frame_metric);
                software_renderer.clear_depth(1.0F, frame_metric);
                render_item.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0.25F, seconds * 0.35F, 0.0F}));
                render_item.translation() = {-0.15F, 0.0F, -1.1F + 0.25F * std::sin(seconds * 0.4F)};
                const software_renderer_api::camera_t camera(
                    {{0, framebuffer.width()}, {0, framebuffer.height()}},
                    software_renderer_api::perspective_t(std::numbers::pi_v<float> / 3, 0.5F, 20.0F)
                );
                // Alternate submission order while the surfaces intersect and cross the near plane.
                if (static_cast<int>(seconds) % 2 == 0) {
                    software_renderer.draw(camera, render_item, frame_metric);
                    software_renderer.draw(camera, second_item, frame_metric);
                } else {
                    software_renderer.draw(camera, second_item, frame_metric);
                    software_renderer.draw(camera, render_item, frame_metric);
                }
                // Composite after opaque visibility. The opaque clear keeps output alpha one.
                software_renderer.draw(camera, transparent_item, frame_metric);
                opengl_renderer.present_rgba8(
                    std::as_bytes(std::span<const rgba8_t>(pixels)),
                    framebuffer.width(),
                    framebuffer.height()
                );
            }

            const auto frame_time = std::chrono::duration<double, std::milli>(frame_started_at - previous_frame_started_at);
            previous_frame_started_at = frame_started_at;
            std::cout << std::format("frame: {:.2f} ms, framebuffer: {}x{}\n", frame_time.count(), framebuffer.width(), framebuffer.height());
        }

        profiler.report(std::cout);

        return 0;
    } catch (const std::exception& error) {
        std::cerr << std::format("software_renderer: {}\n", error.what());
        return 1;
    }
}
