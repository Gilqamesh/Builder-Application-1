#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/software_renderer.h>
#include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>
#include <m03gagbhsqfsqblhwvelrou7nc_json/api.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>
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
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include <sys/utsname.h>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

struct frame_metrics_t {
    std::size_t m_draws = 0;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::frame_metrics_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::frame_metrics_t& frame_metrics, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "application.frame draws={}", frame_metrics.m_draws);
        return out;
    }
};

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace soa = m03gjbxryz3suyoumjyd80j3r2_structure_of_arrays;
namespace byte_stream = m03gagbht2l61mj6qitacwbmea_byte_stream;

namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace process = m03gagbhsvr0m5w15urj0o291m_process;
using json_t = nlohmann::json;

// This executable owns workload execution and summaries. Builder owns its build.
class benchmark_t {
public:
    benchmark_t(int argc, char** argv);
    void run() const;

private:
    json_t run_workload() const;
    json_t metadata(const filesystem::path_t& program) const;
    static json_t summarize(const std::vector<std::array<std::int64_t, 4>>& observations, std::size_t column, int runs);
    static int number(std::string_view argument);
    static void write_json(const filesystem::path_t& path, const json_t& document);

    std::string m_output;
    std::string m_workload;
    int m_size = 128;
    int m_warmup = 3;
    int m_samples = 20;
    int m_runs = 5;
};

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

std::int64_t render_frame(software_renderer_t& software_renderer, const camera_t& camera, const std::vector<render_item_t>& items) {
    const auto start = std::chrono::steady_clock::now();
    {
        auto metric = software_renderer.profiler().metric<frame_metrics_t>();
        software_renderer.clear_color({0, 0, 0, 255});
        software_renderer.clear_depth(1);
        for (const auto& item : items) {
            software_renderer.draw(camera, item);
            metric.update([](frame_metrics_t& metric) noexcept { ++metric.m_draws; });
        }
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
}

benchmark_t::benchmark_t(int argc, char** argv) {
    for (int i = 1; i < argc; i += 2) {
        if (argc <= i + 1) { throw std::invalid_argument(std::format("benchmark option '{}' requires a value", argv[i])); }
        const std::string_view option = argv[i];
        const std::string_view argument = argv[i + 1];
        if (option == "--output") { m_output = argument; }
        else if (option == "--worker") { m_workload = argument; }
        else if (option == "--size") { m_size = number(argument); }
        else if (option == "--warmup") { m_warmup = number(argument); }
        else if (option == "--samples") { m_samples = number(argument); }
        else if (option == "--runs") { m_runs = number(argument); }
        else { throw std::invalid_argument(std::format("unknown benchmark option '{}'", option)); }
    }
    if (m_output.empty() || m_output.front() != '/') { throw std::invalid_argument("benchmark requires --output with an absolute directory path"); }
    if (m_size <= 0 || m_warmup < 0 || m_samples <= 0 || m_runs <= 0) { throw std::invalid_argument("benchmark requires positive size, samples and runs, and nonnegative warmup"); }
    if (!m_workload.empty() && m_workload != "textured_fill" && m_workload != "depth_overdraw" && m_workload != "many_draws" && m_workload != "clipping") {
        throw std::invalid_argument(std::format("unknown benchmark workload '{}'", m_workload));
    }
}

void benchmark_t::run() const {
    const filesystem::path_t output(m_output);
    if (!m_workload.empty()) {
        write_json(output / filesystem::relative_path_t(m_workload + ".json"), run_workload());
        return;
    }
    const auto program = filesystem::canonical(filesystem::path_t("/proc/self/exe"));
    if (filesystem::exists(output)) { throw std::invalid_argument(std::format("benchmark output directory already exists: {}", output)); }
    if (program.parent().parent().is_child(output)) { throw std::invalid_argument("benchmark output must be outside the installed binary artifact"); }
    filesystem::create_directories(output);
    json_t results {{"metadata", metadata(program)}, {"workloads", json_t::object()}};
    write_json(output / filesystem::relative_path_t("metadata.json"), results.at("metadata"));
    for (const std::string workload : {"textured_fill", "depth_overdraw", "many_draws", "clipping"}) {
        std::cout.flush(); // Keep buffered summaries out of the child process.
        process::create_and_wait_checked(process::command_t({
            program.string(), "--worker", workload, "--output", output.string(),
            "--size", std::to_string(m_size), "--warmup", std::to_string(m_warmup),
            "--samples", std::to_string(m_samples), "--runs", std::to_string(m_runs)
        }));
        const auto path = output / filesystem::relative_path_t(workload + ".json");
        std::ifstream input(path.c_str());
        if (!input) { throw std::runtime_error(std::format("could not read benchmark result {}", path)); }
        auto captured = json_t::parse(input);
        const auto& summary = captured.at("summary");
        std::cout << std::format("{}: normal {:.3f} ms, profiled {:.3f} ms, difference {:+.2f}%\n",
            workload, summary.at("normal").at("median_ns").get<double>() / 1e6,
            summary.at("profiled").at("median_ns").get<double>() / 1e6,
            captured.at("median_overhead_percent").get<double>());
        results["workloads"][workload] = std::move(captured);
    }
    write_json(output / filesystem::relative_path_t("results.json"), results);
    std::cout << std::format("Raw samples, summaries, build identity and reports: {}\n", output);
}

json_t benchmark_t::run_workload() const {
    const auto items = make_workload(m_workload);
    const auto count = framebuffer_t::pixel_count(m_size, m_size);
    std::vector<rgba8_t> normal_pixels(count), measured_pixels(count);
    std::vector<float> normal_depth(count), measured_depth(count);
    framebuffer_t normal_buffer(normal_pixels, m_size, m_size), measured_buffer(measured_pixels, m_size, m_size);
    normal_buffer.depth(normal_depth); measured_buffer.depth(measured_depth);
    software_renderer_t normal(normal_buffer);
    normal.profiler().enabled() = false;
    software_renderer_t measured(measured_buffer);
    auto& profiler = measured.profiler();
    profiler.enabled() = true;
    const camera_t camera({{0, m_size}, {0, m_size}}, orthographic_t({{-1, 1}, {-1, 1}}, 0.1F, 10.0F));
    std::vector<std::array<std::int64_t, 4>> observations;
    observations.reserve(std::size_t(m_samples) * std::size_t(m_runs));
    for (int run = 0; run < m_runs; ++run) {
        for (int sample = -m_warmup; sample < m_samples; ++sample) {
            std::int64_t normal_ns, measured_ns;
            if ((run % 2 + sample % 2) % 2 == 0) {
                normal_ns = render_frame(normal, camera, items);
                measured_ns = render_frame(measured, camera, items);
            } else {
                measured_ns = render_frame(measured, camera, items);
                normal_ns = render_frame(normal, camera, items);
            }
            if (!std::equal(normal_pixels.begin(), normal_pixels.end(), measured_pixels.begin(), [](rgba8_t a, rgba8_t b) { return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b); }) || normal_depth != measured_depth) {
                throw std::runtime_error("benchmark enabled/disabled profiling results differ");
            }
            if (0 <= sample) { observations.push_back({run, sample, normal_ns, measured_ns}); }
        }
    }
    const filesystem::path_t output(m_output);
    std::ofstream report((output / filesystem::relative_path_t(m_workload + ".txt")).c_str());
    if (!report) { throw std::runtime_error("could not open benchmark report"); }
    profiler.report(report);
    report.close();
    if (!report) { throw std::runtime_error("could not write benchmark report"); }
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
        {"summary", {{"normal", normal_summary}, {"profiled", profiled_summary}}},
        {"median_overhead_percent", (profiled_median / normal_median - 1) * 100},
        {"peak_rss_bytes", peak_rss_bytes},
        {"metrics", profiler.size()}
    };
}

json_t benchmark_t::metadata(const filesystem::path_t& program) const {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    std::string cpu = "unknown";
    while (std::getline(cpuinfo, line)) {
        if (line.starts_with("model name")) {
            const auto colon = line.find(':');
            if (colon != std::string::npos) { cpu = line.substr(colon + 1); }
            break;
        }
    }
    utsname platform {};
    if (uname(&platform) != 0) { throw std::runtime_error("benchmark could not read platform information"); }
    // Capture the actual loaded artifact paths, rather than rediscovering sources
    // or inferring dependency compiler settings from this translation unit.
    std::ifstream maps("/proc/self/maps");
    json_t loaded_files = json_t::array();
    while (std::getline(maps, line)) {
        const auto path_start = line.find('/');
        if (path_start == std::string::npos) { continue; }
        const auto path = line.substr(path_start);
        if (std::find(loaded_files.begin(), loaded_files.end(), path) == loaded_files.end()) { loaded_files.push_back(path); }
    }
    if (!maps.eof()) { throw std::runtime_error("benchmark could not read loaded artifact mappings"); }
    return {
        {"schema_version", 3}, {"workload_version", 1},
        {"size", m_size}, {"warmup_per_run", m_warmup}, {"samples_per_run", m_samples}, {"runs", m_runs},
        {"cpu", cpu}, {"platform", std::format("{} {} {}", platform.sysname, platform.release, platform.machine)},
        {"scope", "frame measurement, full color/depth clears, fixed draw sequence; setup, comparison, reporting excluded"},
        {"report", "persistent application data; inclusive timing statistics across all enabled completions, including warmup; descending total duration"},
        {"peak_rss_scope", "Linux VmHWM through workload capture and text report; both profiling configurations, setup and warmup included; summaries and JSON serialization excluded"},
        {"build", {
            {"system", "Builder"}, {"binary", program.string()}, {"loaded_files", std::move(loaded_files)},
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
            {"dependency_compile_options", nullptr},
            {"configuration_note", "Compiler and feature macros describe the benchmark translation unit. Dependency options are not inferred; retain Builder build logs and versioned artifacts."}
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
            std::cout << "usage: benchmark --output /absolute/new/run-directory [--size 128] [--warmup 3] [--samples 20] [--runs 5]\n"
                         "Runs four workloads in separate processes using the current Builder build configuration.\n";
            return 0;
        }
        m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::benchmark_t(argc, argv).run();
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
