#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/software_renderer.h>
#include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>
#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>
#include <m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays/api.h>
#include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>
#include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>
#include <m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <format>
#include <iostream>
#include <memory>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

struct frame_metrics_t {
    std::size_t m_draws = 0;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::frame_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::frame_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "draws={}", metrics.m_draws);
        return out;
    }
};

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace soa = m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays;
namespace byte_stream = m03gagbht2l61mj6qitacwbmea_byte_stream;

using benchmark_metrics_t = std::variant<frame_metrics_t, clear_metrics_t, vertex_metrics_t, raster_metrics_t>;
using benchmark_profiler_t = profiling::profiler_t<benchmark_metrics_t>;

std::vector<render_item_t> make_workload(std::string_view name) {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector4f_t>(0);
    vertex.position(vertex.world_to_clip() * vertex.object_to_world() * position);
    vertex.output(0, shader::swizzle<0, 1>(position) * 0.5F + vector2f_t({0.5F, 0.5F}));
    shader::fragment_shader_ast_builder_t fragment;
    const auto coordinates = fragment.input<vector2f_t>(0);
    fragment.color(shader::sample(fragment.resource<shader::shader_texture_2d_t>(0), fragment.resource<shader::shader_sampler_t>(0), coordinates));
    const auto program = std::make_shared<const software_shader::program_t>(std::move(vertex).finalize(), std::move(fragment).finalize());
    auto material = std::make_shared<material_t>(program);
    const std::array<rgba8_t, 4> texels {{{255, 72, 72, 255}, {72, 255, 128, 255}, {72, 128, 255, 255}, {255, 232, 72, 255}}};
    material->texture(0, std::make_shared<texture::texture_t>(texture::format_t::rgba8_unorm, 2, 2, byte_stream::byte_stream_t(std::as_bytes(std::span(texels)))));
    material->sampler(0, std::make_shared<texture::sampler_t>(texture::filter_t::linear, texture::address_mode_t::clamp_to_edge, texture::address_mode_t::clamp_to_edge));
    material->depth_test(true);
    soa::structure_of_arrays_t<std::array<float, 4>> streams;
    for (const auto position : std::array<std::array<float, 4>, 4> {{{-1, -1, 0, 1}, {-1, 1, 0, 1}, {1, -1, 0, 1}, {1, 1, 0, 1}}}) {
        streams.push_back(position);
    }
    auto mesh = std::make_shared<mesh_t>(std::move(streams), std::vector<vertex_attribute_t>{vertex_attribute_t(vertex_attribute_type_t::R32, 4)});
    auto indices = std::make_shared<index_buffer_t>();
    indices->indices() = {0, 1, 2, 2, 1, 3};
    auto geometry = std::make_shared<geometry_t>(indices);
    geometry->mesh() = mesh;
    render_item_t item;
    item.geometry() = geometry;
    item.material() = material;
    item.translation() = {0, 0, -1};
    if (name == "textured_fill") { return {item}; }
    if (name == "depth_overdraw") {
        std::vector<render_item_t> items(4, item);
        for (std::size_t i = 0; i < items.size(); ++i) { items[i].translation()[2] -= float(i) * 0.1F; }
        return items;
    }
    if (name == "many_draws") {
        std::vector<render_item_t> items;
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                item.scale() = {0.125F, 0.125F, 1};
                item.translation() = {-0.875F + 0.25F * x, -0.875F + 0.25F * y, -1};
                items.push_back(item);
            }
        }
        return items;
    }
    if (name == "clipping") {
        item.rotation(m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>({0.8F, 0.5F, 0.4F}));
        item.translation() = {0.7F, 0, -0.4F};
        return {item};
    }
    throw std::invalid_argument("unknown benchmark workload");
}

template <typename Profiler>
std::int64_t render_frame(software_renderer_t<Profiler>& renderer, Profiler& profiler, profiling::region_id_t region, const camera_t& camera, const std::vector<render_item_t>& items) {
    profiler.reset();
    const auto start = std::chrono::steady_clock::now();
    {
        [[maybe_unused]] auto frame = profiler.template scope<frame_metrics_t>(region);
        renderer.clear_color({0, 0, 0, 255});
        renderer.clear_depth(1);
        for (const auto& item : items) {
            renderer.draw(camera, item);
            if constexpr (Profiler::enabled) { ++frame.metrics().m_draws; }
        }
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
}

int benchmark(int argc, char** argv) {
    if (argc != 7) { throw std::invalid_argument("usage: benchmark workload size warmup samples runs report_path"); }
    const std::string workload = argv[1];
    const auto size = std::stoi(argv[2]);
    const auto warmup = std::stoi(argv[3]);
    const auto samples = std::stoi(argv[4]);
    const auto runs = std::stoi(argv[5]);
    if (size <= 0 || warmup < 0 || samples <= 0 || runs <= 0) { throw std::invalid_argument("invalid benchmark dimensions or repetition counts"); }
    const auto items = make_workload(workload);
    const auto count = framebuffer_t::pixel_count(size, size);
    std::vector<rgba8_t> normal_pixels(count), measured_pixels(count);
    std::vector<float> normal_depth(count), measured_depth(count);
    framebuffer_t normal_buffer(normal_pixels, size, size), measured_buffer(measured_pixels, size, size);
    normal_buffer.depth(normal_depth); measured_buffer.depth(measured_depth);
    profiling::disabled_profiler_t normal_profiler;
    const auto normal_frame = normal_profiler.register_region("application.frame");
    software_renderer_t<> normal(normal_buffer);
    std::vector<benchmark_profiler_t::record_type_t> records(3 + 4 * items.size());
    benchmark_profiler_t measured_profiler(records);
    const regions_t regions(measured_profiler);
    const auto measured_frame = measured_profiler.register_region("application.frame");
    software_renderer_t<benchmark_profiler_t> measured(measured_buffer, measured_profiler, regions);
    normal_profiler.start(); measured_profiler.start();
    const camera_t camera({{0, size}, {0, size}}, orthographic_t({{-1, 1}, {-1, 1}}, 0.1F, 10.0F));
    std::vector<std::array<std::int64_t, 4>> observations;
    observations.reserve(std::size_t(samples) * std::size_t(runs));
    for (int run = 0; run < runs; ++run) {
        for (int sample = -warmup; sample < samples; ++sample) {
            std::int64_t normal_ns, measured_ns;
            if ((run + sample) % 2 == 0) {
                normal_ns = render_frame(normal, normal_profiler, normal_frame, camera, items);
                measured_ns = render_frame(measured, measured_profiler, measured_frame, camera, items);
            } else {
                measured_ns = render_frame(measured, measured_profiler, measured_frame, camera, items);
                normal_ns = render_frame(normal, normal_profiler, normal_frame, camera, items);
            }
            if (!std::equal(normal_pixels.begin(), normal_pixels.end(), measured_pixels.begin(), [](rgba8_t a, rgba8_t b) { return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b); }) || normal_depth != measured_depth || measured_profiler.omitted() != 0) {
                throw std::runtime_error("benchmark policy results differ or capture overflowed");
            }
            if (0 <= sample) { observations.push_back({run, sample, normal_ns, measured_ns}); }
        }
    }
    std::ofstream report(argv[6]);
    if (!report) { throw std::runtime_error("could not open benchmark report"); }
    measured_profiler.report(report);
    if (!report) { throw std::runtime_error("could not write benchmark report"); }
    for (const auto& row : observations) { std::cout << std::format("sample,{},{},{},{}\n", row[0], row[1], row[2], row[3]); }
    // VmHWM belongs to this executable's address space. Linux getrusage's
    // maximum can retain the launching process's pre-exec RSS high-water mark.
    std::ifstream status("/proc/self/status");
    std::string line;
    std::size_t peak_rss_bytes = 0;
    while (std::getline(status, line)) {
        if (line.starts_with("VmHWM:")) {
            std::istringstream fields(line.substr(6));
            std::size_t peak_rss_kib;
            std::string units;
            if (!(fields >> peak_rss_kib >> units) || units != "kB") { throw std::runtime_error("invalid VmHWM in /proc/self/status"); }
            peak_rss_bytes = peak_rss_kib * 1024;
            break;
        }
    }
    if (peak_rss_bytes == 0) { throw std::runtime_error("missing VmHWM in /proc/self/status"); }
    std::cout << std::format("peak_rss_bytes,{}\n", peak_rss_bytes);
    std::cout << std::format("records,{}\n", measured_profiler.records().size());
    return 0;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

int main(int argc, char** argv) {
    try { return m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark(argc, argv); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
