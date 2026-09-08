#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/software_renderer.h>
#include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>
#include <m03gagbhsqfsqblhwvelrou7nc_json/api.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>
#include <m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays/api.h>
#include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>
#include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>
#include <m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h>

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

struct frame_metrics_t {
    std::size_t draws = 0;
};

struct benchmark_mask_metrics_t {};
struct benchmark_scene_metrics_t {};
struct benchmark_composition_metrics_t {};
struct benchmark_mip_generation_metrics_t {
    std::size_t levels_generated = 0;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::frame_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::frame_metrics_t& frame_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "application.frame draws={}", frame_metrics.draws);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_mask_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_mask_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "application.mask");
        (void)metrics;
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_scene_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_scene_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "application.scene");
        (void)metrics;
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_composition_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_composition_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "application.composition");
        (void)metrics;
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_mip_generation_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_mip_generation_metrics_t& metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "application.mip_generation");
        out = std::format_to(out, " levels_generated={}", metrics.levels_generated);
        return out;
    }
};

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace soa = m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays;
namespace byte_stream = m03gagbht2l61mj6qitacwbmea_byte_stream;

namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
using json_t = nlohmann::json;

class benchmark_t {
public:
    benchmark_t(int argc, char** argv);
    void run() const;

private:
    static render_item_t make_postprocess(const render_item_t& source, const std::shared_ptr<texture::texture_t>& target, int size, bool mipmaps = false);
    json_t run_workload(std::string_view workload) const;
    json_t metadata(const filesystem::path_t& program) const;
    static json_t summarize(const std::vector<std::array<std::int64_t, 4>>& observations, std::size_t column, int runs);
    static int number(std::string_view argument);
    static void write_json(const filesystem::path_t& path, const json_t& document);

    std::string m_output;
    bool m_report = false;
    int m_size = 128;
    int m_warmup = 3;
    int m_samples = 20;
    int m_runs = 5;
};

std::vector<render_item_t> make_workload(std::string_view name) {
    const bool varyings = name == "indexed_varyings" || name == "unshared_varyings" || name == "sparse_varyings" || name == "tiny_varyings" || name == "point_varyings";
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector4f_t>(0);
    vertex.position(vertex.world_to_clip() * vertex.object_to_world() * position);
    vertex.output(0, shader::swizzle<0, 1>(position) * 0.5F + vector2f_t({0.5F, 0.5F}));
    if (varyings) {
        vertex.output(1, position * 0.25F + vector4f_t(0.5F));
        vertex.output(2, shader::swizzle<0, 1>(position) * 0.5F + vector2f_t(0.5F));
    }
    shader::fragment_shader_ast_builder_t fragment;
    const bool constant = name == "constant_fill" || name == "indexed_mesh" || name == "tiny_triangles" || name == "rejected_triangles";
    const auto interpolation = name == "flat_fill" ? shader::interpolation_t::flat : (name == "noperspective_fill" ? shader::interpolation_t::noperspective : shader::interpolation_t::perspective);
    if (constant) {
        fragment.color(vector4f_t({0.25F, 0.5F, 0.75F, 1.0F}));
    } else {
        auto coordinates = fragment.input<vector2f_t>(0, interpolation);
        if (varyings) { coordinates = (coordinates + fragment.input<vector2f_t>(2, shader::interpolation_t::noperspective)) * 0.5F; }
        const auto sampled_texture = fragment.resource<shader::shader_texture_2d_t>(0);
        const auto sampled_sampler = fragment.resource<shader::shader_sampler_t>(0);
        auto color = name == "mipmapped_fill" ? shader::sample_lod(sampled_texture, sampled_sampler, coordinates, 0.5F) : shader::sample(sampled_texture, sampled_sampler, coordinates);
        if (varyings) { color = color * fragment.input<vector4f_t>(1, shader::interpolation_t::flat); }
        fragment.color(color);
    }
    const auto program = std::make_shared<const software_shader::program_t>(std::move(vertex).finalize(), std::move(fragment).finalize());
    auto material = std::make_shared<material_t>(program);
    const bool translucent = name == "translucent_linear" || name == "translucent_srgb" || name == "two_pass_linear" || name == "two_pass_srgb" || name == "mipmapped_two_pass";
    const std::uint8_t alpha = translucent ? 128 : 255;
    const std::array<rgba8_t, 4> texels {{{255, 72, 72, alpha}, {72, 255, 128, alpha}, {72, 128, 255, alpha}, {255, 232, 72, alpha}}};
    material->texture(0, std::make_shared<texture::texture_t>(texture::texture_description_t {texture::format_t::rgba8_unorm, 2, 2, name == "mipmapped_fill" ? 2U : 1U}, byte_stream::byte_stream_t(std::as_bytes(std::span(texels)))));
    material->sampler(0, std::make_shared<texture::sampler_t>(texture::filter_t::linear, texture::address_mode_t::clamp_to_edge, texture::address_mode_t::clamp_to_edge));
    if (name == "mipmapped_fill") {
        // Static source generation belongs to setup; this workload measures sampling.
        auto target = std::make_shared<texture::texture_t>(material->texture(0));
        target->generate_mipmaps();
        material->texture(0, target);
        material->sampler(0, std::make_shared<texture::sampler_t>(texture::sampler_description_t {texture::filter_t::linear, texture::filter_t::linear, texture::filter_t::linear}));
    }
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
    if (name == "stencil_mask" || name == "two_pass_linear" || name == "two_pass_srgb" || name == "mipmapped_two_pass") {
        material->stencil_test(true);
        material->stencil_front({.comparison = comparison_t::equal, .reference = 1});
        material->stencil_back(material->stencil_front());
    }
    if (translucent) {
        material->blend(true);
        material->blend_color({blend_factor_t::src_alpha, blend_factor_t::one_minus_src_alpha, blend_op_t::add});
        material->blend_alpha({blend_factor_t::one, blend_factor_t::one_minus_src_alpha, blend_op_t::add});
        material->depth_write(false);
        std::vector<render_item_t> items(4, item);
        // Four translucent layers, supplied back to front by the application.
        for (std::size_t i = 0; i < items.size(); ++i) { items[i].translation()[2] = -1.3F + float(i) * 0.1F; }
        return items;
    }
    if (constant || varyings) {
        if (name == "constant_fill") { return {item}; }
        soa::structure_of_arrays_t<std::array<float, 4>> grid;
        auto selected_indices = std::make_shared<index_buffer_t>();
        constexpr std::uint32_t side = 32;
        const std::uint32_t prefix = name == "sparse_varyings" ? (1U << 20) - (side + 1) * (side + 1) : 0;
        for (std::uint32_t i = 0; i < prefix; ++i) { grid.push_back({0, 0, 0, 1}); }
        for (std::uint32_t y = 0; y <= side; ++y) {
            for (std::uint32_t x = 0; x <= side; ++x) {
                grid.push_back({-1.0F + 2.0F * float(x) / side, -1.0F + 2.0F * float(y) / side, 0, 1});
            }
        }
        for (std::uint32_t y = 0; y < side; ++y) {
            for (std::uint32_t x = 0; x < side; ++x) {
                const auto first = prefix + y * (side + 1) + x;
                for (const auto index : {first, first + 1, first + side + 1, first + 1, first + side + 2, first + side + 1}) {
                    selected_indices->indices().push_back(index);
                }
            }
        }
        if (name == "unshared_varyings") {
            soa::structure_of_arrays_t<std::array<float, 4>> expanded;
            std::uint32_t expanded_index = 0;
            for (auto& index : selected_indices->indices()) {
                const auto x = index % (side + 1), y = index / (side + 1);
                expanded.push_back({-1.0F + 2.0F * float(x) / side, -1.0F + 2.0F * float(y) / side, 0, 1});
                index = expanded_index++;
            }
            grid = std::move(expanded);
        }
        if (name == "point_varyings") {
            selected_indices->indices().clear();
            for (std::uint32_t index = 0; index < (side + 1) * (side + 1); ++index) {
                selected_indices->indices().push_back(index);
            }
        }
        auto selected_geometry = std::make_shared<geometry_t>(selected_indices);
        if (name == "point_varyings") { selected_geometry->primitive_topology() = vertex_primitive_topology_t::point; }
        selected_geometry->mesh() = std::make_shared<mesh_t>(std::move(grid), std::vector<vertex_attribute_t>{vertex_attribute_t(vertex_attribute_type_t::R32, 4)});
        item.geometry() = selected_geometry;
        if (name == "tiny_triangles" || name == "tiny_varyings") { item.scale() = {0.0625F, 0.0625F, 1}; }
        if (name == "rejected_triangles") { item.translation()[0] = 4; }
        return {item};
    }
    if (name == "textured_fill" || name == "stencil_mask" || name == "flat_fill" || name == "noperspective_fill" || name == "mipmapped_fill") { return {item}; }
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

std::int64_t render_frame(software_renderer_t& software_renderer, const camera_t& camera, const std::vector<render_item_t>& items, profiling::profiler_t& profiler, const render_item_t* mask = nullptr, const framebuffer_t* offscreen = nullptr, const render_item_t* postprocess = nullptr, texture::texture_t* mip_target = nullptr) {
    const auto start = std::chrono::steady_clock::now();
    {
        auto metric = profiler.metric<frame_metrics_t>();
        const auto output = software_renderer.framebuffer();
        if (offscreen) { software_renderer.framebuffer() = *offscreen; }
        software_renderer.clear_color({0, 0, 0, std::uint8_t(offscreen ? 0 : 255)}, metric);
        software_renderer.clear_depth(1, metric);
        if (mask) {
            auto pass_metric = metric.metric<benchmark_mask_metrics_t>();
            software_renderer.clear_stencil(0, pass_metric);
            software_renderer.draw(camera, *mask, pass_metric);
            metric.update<frame_metrics_t>([](frame_metrics_t& metric) noexcept { ++metric.draws; });
        }
        {
            auto pass_metric = metric.metric<benchmark_scene_metrics_t>();
            for (const auto& item : items) {
                software_renderer.draw(camera, item, pass_metric);
                metric.update<frame_metrics_t>([](frame_metrics_t& metric) noexcept { ++metric.draws; });
            }
        }
        if (offscreen) {
            if (mip_target) {
                auto mip_metric = metric.metric<benchmark_mip_generation_metrics_t>();
                mip_target->generate_mipmaps();
                mip_metric.update<benchmark_mip_generation_metrics_t>([&](auto& metrics) noexcept {
                    metrics.levels_generated += mip_target->level_count() - 1;
                });
            }
            auto pass_metric = metric.metric<benchmark_composition_metrics_t>();
            software_renderer.framebuffer() = output;
            software_renderer.clear_color({16, 24, 32, 255}, pass_metric);
            software_renderer.draw(camera, *postprocess, pass_metric);
            metric.update<frame_metrics_t>([](frame_metrics_t& metric) noexcept { ++metric.draws; });
        }
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
}

benchmark_t::benchmark_t(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view option = argv[i];
        if (option == "--report") { m_report = true; continue; }
        if (argc <= i + 1) { throw std::invalid_argument(std::format("benchmark option '{}' requires a value", argv[i])); }
        const std::string_view argument = argv[++i];
        if (option == "--output") { m_output = argument; }
        else if (option == "--size") { m_size = number(argument); }
        else if (option == "--warmup") { m_warmup = number(argument); }
        else if (option == "--samples") { m_samples = number(argument); }
        else if (option == "--runs") { m_runs = number(argument); }
        else { throw std::invalid_argument(std::format("unknown benchmark option '{}'", option)); }
    }
    if (m_output.empty() || m_output.front() != '/') { throw std::invalid_argument("benchmark requires --output with an absolute directory path"); }
    if (m_size <= 0 || m_warmup < 0 || m_samples <= 0 || m_runs <= 0) { throw std::invalid_argument("benchmark requires positive size, samples and runs, and nonnegative warmup"); }

}

void benchmark_t::run() const {
    const filesystem::path_t output(m_output);
    const auto program = filesystem::canonical(filesystem::path_t("/proc/self/exe"));
    if (filesystem::exists(output)) { throw std::invalid_argument(std::format("benchmark output directory already exists: {}", output)); }
    if (program.parent().parent().is_child(output)) { throw std::invalid_argument("benchmark output must be outside the installed binary artifact"); }
    filesystem::create_directories(output);
    json_t results {{"metadata", metadata(program)}, {"workloads", json_t::object()}};
    for (const std::string workload : {"textured_fill", "depth_overdraw", "many_draws", "clipping", "translucent_linear", "translucent_srgb", "stencil_mask", "two_pass_linear", "two_pass_srgb", "flat_fill", "noperspective_fill", "mipmapped_fill", "mipmapped_two_pass", "constant_fill", "indexed_mesh", "tiny_triangles", "rejected_triangles", "indexed_varyings", "unshared_varyings", "sparse_varyings", "tiny_varyings", "point_varyings"}) {
        auto captured = run_workload(workload);
        const auto& summary = captured.at("summary");
        std::cout << std::format("{}: normal {:.3f} ms, profiled {:.3f} ms, difference {:+.2f}%\n",
            workload, summary.at("normal").at("median_ns").get<double>() / 1e6,
            summary.at("profiled").at("median_ns").get<double>() / 1e6,
            captured.at("median_overhead_percent").get<double>());
        results["workloads"][workload] = std::move(captured);
    }
    write_json(output / filesystem::relative_path_t("results.json"), results);
    std::cout << std::format("Benchmark results: {}\n", output);
}

render_item_t benchmark_t::make_postprocess(const render_item_t& source, const std::shared_ptr<texture::texture_t>& target, int size, bool mipmaps) {
    shader::vertex_shader_ast_builder_t vertex;
    vertex.position(vertex.input<vector4f_t>(0));
    shader::fragment_shader_ast_builder_t fragment;
    const auto coordinates = shader::swizzle<0, 1>(fragment.fragment_coordinate()) / float(size);
    const auto sampled_texture = fragment.resource<shader::shader_texture_2d_t>(0);
    const auto sampled_sampler = fragment.resource<shader::shader_sampler_t>(0);
    fragment.color(mipmaps ? shader::sample_lod(sampled_texture, sampled_sampler, coordinates, 1.5F) : shader::sample(sampled_texture, sampled_sampler, coordinates));
    auto item = source;
    item.material() = std::make_shared<material_t>(std::make_shared<const software_shader::program_t>(std::move(vertex).finalize(), std::move(fragment).finalize()));
    item.material()->texture(0, target);
    item.material()->sampler(0, std::make_shared<texture::sampler_t>(texture::filter_t::linear, texture::address_mode_t::clamp_to_edge, texture::address_mode_t::clamp_to_edge));
    if (mipmaps) {
        item.material()->sampler(0, std::make_shared<texture::sampler_t>(texture::sampler_description_t {texture::filter_t::linear, texture::filter_t::linear, texture::filter_t::linear}));
    }
    item.material()->blend(true);
    item.material()->blend_color({blend_factor_t::one, blend_factor_t::one_minus_src_alpha, blend_op_t::add});
    item.material()->blend_alpha({blend_factor_t::one, blend_factor_t::one_minus_src_alpha, blend_op_t::add});
    return item;
}

json_t benchmark_t::run_workload(std::string_view workload) const {
    const auto items = make_workload(workload);
    const auto count = framebuffer_t::pixel_count(m_size, m_size);
    std::vector<rgba8_t> normal_pixels(count), measured_pixels(count);
    std::vector<float> normal_depth(count), measured_depth(count);
    framebuffer_t normal_buffer(normal_pixels, m_size, m_size), measured_buffer(measured_pixels, m_size, m_size);
    normal_buffer.depth(normal_depth); measured_buffer.depth(measured_depth);
    if (workload == "translucent_srgb" || workload == "two_pass_srgb") {
        normal_buffer.format(texture::format_t::rgba8_srgb);
        measured_buffer.format(texture::format_t::rgba8_srgb);
    }
    const bool mipmaps = workload == "mipmapped_two_pass";
    const bool two_pass = workload == "two_pass_linear" || workload == "two_pass_srgb" || mipmaps;
    const bool masked = two_pass || workload == "stencil_mask";
    std::vector<std::uint8_t> normal_stencil(masked ? count : 0), measured_stencil(masked ? count : 0);
    normal_buffer.stencil(normal_stencil); measured_buffer.stencil(measured_stencil);
    auto mask = items.front();
    if (masked) {
        mask.material() = std::make_shared<material_t>(*mask.material());
        mask.scale() = {0.75F, 0.75F, 1};
        mask.material()->color_write(color_mask_t::none);
        mask.material()->depth_test(false);
        mask.material()->stencil_front({.reference = 1, .pass = stencil_op_t::replace});
        mask.material()->stencil_back(mask.material()->stencil_front());
    }
    std::shared_ptr<texture::texture_t> normal_target, measured_target;
    std::optional<framebuffer_t> normal_offscreen, measured_offscreen;
    std::optional<render_item_t> normal_postprocess, measured_postprocess;
    if (two_pass) {
        const auto format = workload == "two_pass_srgb" ? texture::format_t::rgba8_srgb : texture::format_t::rgba8_unorm;
        normal_target = std::make_shared<texture::texture_t>(texture::texture_description_t {format, std::size_t(m_size), std::size_t(m_size), mipmaps ? std::size_t(std::bit_width(unsigned(m_size))) : 1}, byte_stream::byte_stream_t(std::vector<std::byte>(count * 4)));
        measured_target = std::make_shared<texture::texture_t>(*normal_target);
        normal_offscreen.emplace(normal_target->view()); measured_offscreen.emplace(measured_target->view());
        normal_offscreen->depth(normal_depth); measured_offscreen->depth(measured_depth);
        normal_offscreen->stencil(normal_stencil); measured_offscreen->stencil(measured_stencil);
        normal_postprocess = make_postprocess(items.front(), normal_target, m_size, mipmaps);
        measured_postprocess = make_postprocess(items.front(), measured_target, m_size, mipmaps);
    }
    software_renderer_t normal(normal_buffer);
    profiling::profiler_t normal_profiler;
    normal_profiler.enabled() = false;
    software_renderer_t measured(measured_buffer);
    profiling::profiler_t profiler;
    profiler.enabled() = true;
    const camera_t camera({{0, m_size}, {0, m_size}}, orthographic_t({{-1, 1}, {-1, 1}}, 0.1F, 10.0F));
    std::vector<std::array<std::int64_t, 4>> observations;
    observations.reserve(std::size_t(m_samples) * std::size_t(m_runs));
    std::optional<std::size_t> metric_nodes;
    std::optional<std::array<std::int64_t, 2>> first_frame;
    for (int run = 0; run < m_runs; ++run) {
        for (int sample = -m_warmup; sample < m_samples; ++sample) {
            std::int64_t normal_ns, measured_ns;
            if ((run % 2 + sample % 2) % 2 == 0) {
                normal_ns = render_frame(normal, camera, items, normal_profiler, masked ? &mask : nullptr, two_pass ? &*normal_offscreen : nullptr, two_pass ? &*normal_postprocess : nullptr, mipmaps ? normal_target.get() : nullptr);
                measured_ns = render_frame(measured, camera, items, profiler, masked ? &mask : nullptr, two_pass ? &*measured_offscreen : nullptr, two_pass ? &*measured_postprocess : nullptr, mipmaps ? measured_target.get() : nullptr);
            } else {
                measured_ns = render_frame(measured, camera, items, profiler, masked ? &mask : nullptr, two_pass ? &*measured_offscreen : nullptr, two_pass ? &*measured_postprocess : nullptr, mipmaps ? measured_target.get() : nullptr);
                normal_ns = render_frame(normal, camera, items, normal_profiler, masked ? &mask : nullptr, two_pass ? &*normal_offscreen : nullptr, two_pass ? &*normal_postprocess : nullptr, mipmaps ? normal_target.get() : nullptr);
            }
            if (!first_frame) { first_frame = std::array{normal_ns, measured_ns}; }
            if (!std::equal(normal_pixels.begin(), normal_pixels.end(), measured_pixels.begin(), [](rgba8_t a, rgba8_t b) { return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b); }) || normal_depth != measured_depth || normal_stencil != measured_stencil || (two_pass && !std::equal(normal_target->bytes().begin(), normal_target->bytes().end(), measured_target->bytes().begin()))) {
                throw std::runtime_error("benchmark enabled/disabled profiling results differ");
            }
            if (mipmaps) {
                for (std::size_t level = 1; level < normal_target->level_count(); ++level) {
                    if (!std::ranges::equal(normal_target->view(level).bytes(), measured_target->view(level).bytes())) {
                        throw std::runtime_error("benchmark generated mip levels differ with profiling enabled");
                    }
                }
            }
            if (!metric_nodes) { metric_nodes = profiler.size(); }
            if (profiler.size() != *metric_nodes || normal_profiler.size() != 0) {
                throw std::runtime_error("benchmark profiling paths grow after the first frame or record while disabled");
            }
            if (0 <= sample) { observations.push_back({run, sample, normal_ns, measured_ns}); }
        }
    }
    if (m_report) {
        const filesystem::path_t output(m_output);
        const auto path = output / filesystem::relative_path_t(std::string(workload) + ".txt");
        std::ofstream report(path.c_str());
        if (!report) { throw std::runtime_error(std::format("could not open benchmark report {}", path)); }
        profiler.report(report);
        report.close();
        if (!report) { throw std::runtime_error(std::format("could not write benchmark report {}", path)); }
    }
    // Store exact final attachment bytes outside timing for cross-build comparisons.
    const auto attachment_path = filesystem::path_t(m_output) / filesystem::relative_path_t(std::string(workload) + ".attachments");
    std::ofstream attachments(attachment_path.c_str(), std::ios::binary);
    const auto write = [&](std::span<const std::byte> bytes) {
        if (!bytes.empty()) { attachments.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())); }
    };
    write(std::as_bytes(std::span(normal_pixels)));
    write(std::as_bytes(std::span(normal_depth)));
    write(std::as_bytes(std::span(normal_stencil)));
    if (two_pass) {
        for (std::size_t level = 0; level < normal_target->level_count(); ++level) { write(normal_target->view(level).bytes()); }
    }
    attachments.close();
    if (!attachments) { throw std::runtime_error(std::format("could not write benchmark attachments {}", attachment_path)); }
    const auto* vertices = profiler.metrics<frame_metrics_t, benchmark_scene_metrics_t, draw_metrics_t, vertex_metrics_t>();
    const auto normal_summary = summarize(observations, 2, m_runs);
    const auto profiled_summary = summarize(observations, 3, m_runs);
    const auto normal_median = normal_summary.at("median_ns").get<double>();
    const auto profiled_median = profiled_summary.at("median_ns").get<double>();
    json_t samples = json_t::array();
    for (const auto& row : observations) {
        samples.push_back({{"run", row[0]}, {"sample", row[1]}, {"normal_ns", row[2]}, {"profiled_ns", row[3]}});
    }
    return {
        {"samples", std::move(samples)},
        {"first_frame_ns", {{"normal", (*first_frame)[0]}, {"profiled", (*first_frame)[1]}}},
        {"vertices", {{"expected", vertices->expected}, {"invocations", vertices->invocations}, {"reuses", vertices->reuses}}},
        {"vertex_storage_bytes", {{"lookup", vertices->lookup_bytes}, {"touched", vertices->touched_bytes}, {"results", vertices->result_bytes}, {"varyings", vertices->varying_bytes}, {"flat", vertices->flat_bytes}}},
        {"summary", {{"normal", normal_summary}, {"profiled", profiled_summary}}},
        {"median_overhead_percent", (profiled_median / normal_median - 1) * 100},
        {"metric_nodes", profiler.size()}
    };
}

json_t benchmark_t::metadata(const filesystem::path_t& program) const {
    return {
        {"schema_version", 7}, {"workload_version", 7},
        {"size", m_size}, {"warmup_per_run", m_warmup}, {"samples_per_run", m_samples}, {"runs", m_runs},
        {"scope", "frame measurement, full color/depth clears, fixed draw sequence; two-pass workloads also include stencil mask, target selection, output clear and sampled composite; mipmapped_two_pass also regenerates all lower levels; setup, comparison, reporting excluded"},
        {"build", {
            {"binary", program.string()},
            {"benchmark_compiler", __VERSION__},
#ifdef __OPTIMIZE__
            {"benchmark_optimized", true},
#else
            {"benchmark_optimized", false},
#endif
#ifdef NDEBUG
            {"benchmark_assertions", false},
#else
            {"benchmark_assertions", true},
#endif
            {"dependency_compile_options", nullptr}
        }}
    };
}

json_t benchmark_t::summarize(const std::vector<std::array<std::int64_t, 4>>& observations, std::size_t column, int runs) {
    const auto median = [](const std::vector<std::int64_t>& ordered) {
        const auto middle = ordered.size() / 2;
        return ordered.size() % 2 == 0 ? double(ordered[middle - 1]) / 2 + double(ordered[middle]) / 2 : double(ordered[middle]);
    };
    std::vector<std::int64_t> ordered;
    ordered.reserve(observations.size());
    for (const auto& observation : observations) { ordered.push_back(observation[column]); }
    std::sort(ordered.begin(), ordered.end());
    json_t run_medians = json_t::array();
    for (int run = 0; run < runs; ++run) {
        std::vector<std::int64_t> values;
        for (const auto& observation : observations) {
            if (observation[0] == run) { values.push_back(observation[column]); }
        }
        std::sort(values.begin(), values.end());
        run_medians.push_back(median(values));
    }
    // ceil(0.95 * n), using integer arithmetic without multiplication overflow.
    const auto rank = ordered.size() - ordered.size() / 20;
    return {{"median_ns", median(ordered)}, {"p95_ns", ordered[rank - 1]}, {"maximum_ns", ordered.back()}, {"run_medians_ns", std::move(run_medians)}};
}

int benchmark_t::number(std::string_view argument) {
    int parsed;
    const auto [end, error] = std::from_chars(argument.data(), argument.data() + argument.size(), parsed);
    if (error != std::errc {} || end != argument.data() + argument.size()) { throw std::invalid_argument(std::format("invalid benchmark integer '{}'", argument)); }
    return parsed;
}

void benchmark_t::write_json(const filesystem::path_t& path, const json_t& document) {
    std::ofstream output(path.c_str());
    if (!output) { throw std::runtime_error(std::format("could not open benchmark output {}", path)); }
    output << document.dump(2) << '\n';
    output.close();
    if (!output) { throw std::runtime_error(std::format("could not write benchmark output {}", path)); }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            std::cout << "usage: benchmark --output /absolute/new/run-directory [--size 128] [--warmup 3] [--samples 20] [--runs 5] [--report]\n"
                         "Runs twenty workloads; --report writes per-workload stage reports.\n";
            return 0;
        }
        m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_t(argc, argv).run();
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
