#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_HELPERS_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_HELPERS_H

# include "camera.h"
# include "framebuffer.h"
# include "render_item.h"
# include "vertex_attribute.h"

# include <m03gjfvd6i5jzbmngb2ldoooza_type_erased_array/api.h>
# include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>
# include <m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h>

# include <algorithm>
# include <array>
# include <cstddef>
# include <cstdint>
# include <format>
# include <optional>
# include <span>
# include <stdexcept>
# include <utility>
# include <variant>
# include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace software_shader = m03gt1djvvy5atia5evkbg6rqy_software_shader;
namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
namespace type_erased_array = m03gjfvd6i5jzbmngb2ldoooza_type_erased_array;

using matrix4f_t = shader::matrix_t<float, 4, 4>;
using vector2f_t = m03gsy25j4v7nccgmsdov9ioft_shader::vector_t<float, 2>;
using vector3f_t = m03gsy25j4v7nccgmsdov9ioft_shader::vector_t<float, 3>;
using vector4f_t = m03gsy25j4v7nccgmsdov9ioft_shader::vector_t<float, 4>;
using varying_t = std::variant<float, vector2f_t, vector3f_t, vector4f_t>;
using varying_entry_t = std::pair<std::uint32_t, varying_t>;
using varying_values_t = std::vector<varying_entry_t>;
using grid_point_t = std::array<std::int64_t, 2>;
using triangle_t = std::array<std::size_t, 3>;
// Unreduced nonnegative numerator and positive denominator.
using fraction_t = std::array<std::int64_t, 2>;

constexpr int maximum_extent = 1 << 23;
constexpr std::int64_t subpixels = 256;
constexpr std::int64_t center_offset = 128;

struct pipeline_vertex_t {
    vector4f_t m_clip_position;
    std::array<std::size_t, 2> m_outputs; // Offset and count in the owning value buffer.
};

struct pipeline_vertex_view_t {
    vector4f_t m_clip_position;
    std::span<const varying_entry_t> m_outputs;
};

struct clipping_buffer_t {
    std::vector<pipeline_vertex_t> m_vertices;
    varying_values_t m_values;
};

struct clipping_workspace_t {
    std::array<clipping_buffer_t, 2> m_buffers;
};

struct projected_vertex_t {
    grid_point_t m_point;
    double m_ndc_z;
    double m_reciprocal_w;
    pipeline_vertex_view_t m_source;
};

struct scan_event_t {
    fraction_t m_x;
    std::size_t m_lower;
    std::size_t m_upper;
    int m_delta;
};

struct sample_t {
    int m_x;
    int m_y;
    std::array<std::size_t, 4> m_vertices;
    std::array<double, 4> m_weights;
    std::size_t m_count;
};

struct raster_workspace_t {
    clipping_workspace_t m_clipping;
    std::vector<projected_vertex_t> m_vertices;
    std::vector<std::size_t> m_geometry;
    std::vector<std::size_t> m_ring;
    std::vector<triangle_t> m_triangles;
    std::vector<scan_event_t> m_events;
    bool m_empty = true;
    bool m_use_triangles = false;
    bool m_front_facing = false;
};

struct screen_vertex_t {
    float m_x;
    float m_y;
    float m_ndc_z;
    float m_reciprocal_w;
    std::span<const varying_entry_t> m_outputs;
};

// Renderer-owned storage retains its peak capacities across draws.
struct scratch_t {
    std::vector<pipeline_vertex_t> m_vertex_results;
    varying_values_t m_vertex_values;
    clipping_workspace_t m_clipping;
    raster_workspace_t m_raster;
    varying_values_t m_fragment_inputs;
    software_shader::vertex_io_t m_vertex_io {0, 0};
    software_shader::fragment_io_t m_fragment_io {vector4f_t(0.0F), true};
};

void clear(clipping_buffer_t& buffer);

void append_vertex(clipping_buffer_t& destination, const pipeline_vertex_view_t& source);

varying_t interpolate(const varying_t& from, const varying_t& to, double factor);

void append_intersection(clipping_buffer_t& destination, pipeline_vertex_view_t first, pipeline_vertex_view_t second, std::size_t plane);

bool between(grid_point_t p, grid_point_t a, grid_point_t b);

bool opposite(std::int64_t a, std::int64_t b);

bool intersects(grid_point_t a, grid_point_t b, grid_point_t c, grid_point_t d);

bool simple_boundary(const raster_workspace_t& workspace);

bool triangulate(raster_workspace_t& workspace);

bool crossed_facing(std::span<const projected_vertex_t> vertices);

int compare_varying(const varying_t& a, const varying_t& b);

int compare_record(const projected_vertex_t& a, const projected_vertex_t& b);

bool edge_record_less(const scan_event_t& a, const scan_event_t& b, std::span<const projected_vertex_t> vertices);

pipeline_vertex_view_t view(const pipeline_vertex_t& vertex, const varying_values_t& values);

double clip_distance(const pipeline_vertex_view_t& vertex, std::size_t plane);

bool inside_clip_volume(const pipeline_vertex_view_t& vertex);

std::optional<std::size_t> clip_line(const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, clipping_workspace_t& workspace);

std::optional<std::size_t> clip_triangle(const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, const pipeline_vertex_view_t& third, clipping_workspace_t& workspace);

double projectable_reciprocal_w(float w);

std::int64_t snap(double screen, int extent);

std::int64_t edge(grid_point_t a, grid_point_t b, grid_point_t p);

std::int64_t ceil_div(std::int64_t numerator, std::int64_t denominator);

int compare_fraction(fraction_t a, fraction_t b);

int sample_bound(fraction_t crossing, int extent);

void prepare_polygon(raster_workspace_t& workspace);

void prepare_triangle(const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, const pipeline_vertex_view_t& third, int width, int height, raster_workspace_t& workspace);

void scanline_events(std::span<const projected_vertex_t> vertices, std::int64_t y, std::vector<scan_event_t>& events);

sample_t span_sample(const scan_event_t& left, const scan_event_t& right, std::span<const projected_vertex_t> vertices, int x, int y);

// Returns window depth and reciprocal W, and writes perspective-correct varyings.
std::array<double, 2> interpolate_sample(std::span<const projected_vertex_t> vertices, const sample_t& sample, varying_values_t& outputs);

bool finite(const vector4f_t& vector);

std::size_t shader_component_count(shader::shader_data_type_t type);

vertex_attribute_type_t expected_attribute_type(shader::shader_scalar_type_t type);

void validate_vertex_attribute(
    const vertex_attribute_t& attribute,
    shader::shader_data_type_t input_type
);

template <typename T>
T read_scalar(
    const type_erased_array::type_erased_array_t& stream,
    std::uint32_t vertex_index
);

template <typename T, std::size_t N>
shader::vector_t<T, N> read_vector(
    const type_erased_array::type_erased_array_t& stream,
    std::uint32_t vertex_index
);

template <typename T>
void set_vertex_input_components(
    software_shader::vertex_io_t& io,
    std::uint32_t location,
    const type_erased_array::type_erased_array_t& stream,
    std::uint32_t vertex_index,
    std::size_t component_count
);

void set_vertex_input(
    software_shader::vertex_io_t& io,
    const shader::shader_interface_element_t& input,
    const type_erased_array::type_erased_array_t& stream,
    const vertex_attribute_t& attribute,
    std::uint32_t vertex_index
);

bool supported_fragment_input(shader::shader_data_type_t type);

template <typename T>
T require_vertex_output(
    const software_shader::vertex_io_t& io,
    std::uint32_t location
);

varying_t vertex_output(
    const software_shader::vertex_io_t& io,
    const shader::shader_interface_element_t& input
);

std::optional<screen_vertex_t> project(
    const pipeline_vertex_view_t& vertex,
    int width,
    int height
);

void set_fragment_inputs(
    software_shader::fragment_io_t& io,
    std::span<const varying_entry_t> inputs
);

std::uint8_t to_unorm8(float component);

rgba8_t to_rgba8(const vector4f_t& color);

void shade_sample(
    const software_shader::program_t& program,
    const software_shader::bindings_t& bindings,
    int width,
    int height,
    std::span<rgba8_t> framebuffer,
    int x,
    int y,
    float depth,
    float reciprocal_w,
    bool front_facing,
    std::span<const varying_entry_t> inputs,
    software_shader::fragment_io_t& io
);

void rasterize_point(
    const software_shader::program_t& program,
    const software_shader::bindings_t& bindings,
    int width,
    int height,
    std::span<rgba8_t> framebuffer,
    const pipeline_vertex_view_t& vertex,
    varying_values_t& fragment_inputs,
    software_shader::fragment_io_t& fragment_io
);

void rasterize_line(
    const software_shader::program_t& program,
    const software_shader::bindings_t& bindings,
    int width,
    int height,
    std::span<rgba8_t> framebuffer,
    const pipeline_vertex_view_t& first,
    const pipeline_vertex_view_t& second,
    clipping_workspace_t& clipping,
    varying_values_t& fragment_inputs,
    software_shader::fragment_io_t& fragment_io
);

void rasterize_triangle(
    const software_shader::program_t& program,
    const software_shader::bindings_t& bindings,
    int width,
    int height,
    std::span<rgba8_t> framebuffer,
    const pipeline_vertex_view_t& first,
    const pipeline_vertex_view_t& second,
    const pipeline_vertex_view_t& third,
    raster_workspace_t& workspace,
    varying_values_t& fragment_inputs,
    software_shader::fragment_io_t& fragment_io
);

matrix4f_t object_to_world_matrix(const render_item_t& render_item);

matrix4f_t world_to_clip_matrix(
    const camera_t<float, int, 2>& camera,
    framebuffer_t framebuffer
);

// The renderer and validation consume the same pre-shading coverage events.
template <typename Emit>
void visit_samples(raster_workspace_t& workspace, int width, int height, Emit&& emit);

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::pipeline_vertex_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::pipeline_vertex_view_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clipping_buffer_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clipping_workspace_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::projected_vertex_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::scan_event_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::sample_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_workspace_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::screen_vertex_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::scratch_t>;

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

template <typename T>
T read_scalar(
    const type_erased_array::type_erased_array_t& stream,
    std::uint32_t vertex_index
) {
    return stream.read<T>(vertex_index);
}

template <typename T, std::size_t N>
shader::vector_t<T, N> read_vector(
    const type_erased_array::type_erased_array_t& stream,
    std::uint32_t vertex_index
) {
    return shader::vector_t<T, N>(stream.read<std::array<T, N>>(vertex_index));
}

template <typename T>
void set_vertex_input_components(
    software_shader::vertex_io_t& io,
    std::uint32_t location,
    const type_erased_array::type_erased_array_t& stream,
    std::uint32_t vertex_index,
    std::size_t component_count
) {
    switch (component_count) {
        case 1: {
            io.input(location, read_scalar<T>(stream, vertex_index));
        } break;
        case 2: {
            io.input(location, read_vector<T, 2>(stream, vertex_index));
        } break;
        case 3: {
            io.input(location, read_vector<T, 3>(stream, vertex_index));
        } break;
        case 4: {
            io.input(location, read_vector<T, 4>(stream, vertex_index));
        } break;
        default: {
            throw std::logic_error("unsupported validated vertex component count");
        }
    }
}

template <typename T>
T require_vertex_output(
    const software_shader::vertex_io_t& io,
    std::uint32_t location
) {
    const auto output = io.output<T>(location);
    if (!output) {
        throw std::runtime_error(std::format(
            "vertex shader did not write output location {} required by the fragment shader",
            location
        ));
    }
    return *output;
}

template <typename Emit>
void visit_samples(raster_workspace_t& workspace, int width, int height, Emit&& emit) {
    if (workspace.m_empty) {
        return;
    }
    const auto& vertices = workspace.m_vertices;
    if (workspace.m_use_triangles) {
        for (const auto& indices : workspace.m_triangles) {
            const auto a = vertices[indices[0]].m_point;
            const auto b = vertices[indices[1]].m_point;
            const auto c = vertices[indices[2]].m_point;
            const auto area = edge(a, b, c);
            const auto inclusive = [](grid_point_t from, grid_point_t to) {
                return to[1] < from[1] || (to[1] == from[1] && from[0] < to[0]);
            };
            const std::array top_left {inclusive(b, c), inclusive(c, a), inclusive(a, b)};
            const int first_x = sample_bound({std::min({a[0], b[0], c[0]}), 1}, width);
            const int end_x = sample_bound({std::max({a[0], b[0], c[0]}) + 1, 1}, width);
            const int first_y = sample_bound({std::min({a[1], b[1], c[1]}), 1}, height);
            const int end_y = sample_bound({std::max({a[1], b[1], c[1]}) + 1, 1}, height);
            for (int y = first_y; y < end_y; ++y) {
                for (int x = first_x; x < end_x; ++x) {
                    const grid_point_t p {std::int64_t(x) * subpixels + center_offset, std::int64_t(y) * subpixels + center_offset};
                    const std::array values {edge(b, c, p), edge(c, a, p), edge(a, b, p)};
                    bool covered = true;
                    for (std::size_t i = 0; i < 3; ++i) {
                        covered = covered && (0 < values[i] || (values[i] == 0 && top_left[i]));
                    }
                    if (covered) {
                        const std::array weights {
                            double(values[0]) / double(area),
                            double(values[1]) / double(area),
                            double(values[2]) / double(area),
                            0.0
                        };
                        emit(sample_t {x, y, {indices[0], indices[1], indices[2], 0}, weights, 3});
                    }
                }
            }
        }
        return;
    }

    const auto [minimum, maximum] = std::ranges::minmax_element(vertices, {}, [](const auto& vertex) { return vertex.m_point[1]; });
    const int first_y = sample_bound({minimum->m_point[1], 1}, height);
    const int end_y = sample_bound({maximum->m_point[1], 1}, height);
    for (int y = first_y; y < end_y; ++y) {
        scanline_events(vertices, std::int64_t(y) * subpixels + center_offset, workspace.m_events);
        const auto& events = workspace.m_events;
        int winding = 0;
        scan_event_t left {};
        for (std::size_t begin = 0; begin < events.size();) {
            std::size_t end = begin;
            int delta = 0;
            do {
                delta += events[end++].m_delta;
            } while (end < events.size() && compare_fraction(events[begin].m_x, events[end].m_x) == 0);
            const int after = winding + delta;
            if ((winding == 0) != (after == 0)) {
                std::size_t selected = begin;
                while ((events[selected].m_delta < 0) != (delta < 0)) {
                    ++selected;
                }
                const auto& boundary = events[selected];
                if (winding == 0) {
                    left = boundary;
                } else {
                    const int first_x = sample_bound(left.m_x, width);
                    const int end_x = sample_bound(boundary.m_x, width);
                    for (int x = first_x; x < end_x; ++x) {
                        emit(span_sample(left, boundary, vertices, x, y));
                    }
                }
            }
            winding = after;
            begin = end;
        }
    }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::pipeline_vertex_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::pipeline_vertex_t& vertex, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "position: {}", vertex.m_clip_position);
        out = std::format_to(out, ", offset: {}", vertex.m_outputs[0]);
        out = std::format_to(out, ", count: {}", vertex.m_outputs[1]);
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::pipeline_vertex_view_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::pipeline_vertex_view_t& vertex, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "position: {}", vertex.m_clip_position);
        out = std::format_to(out, ", outputs: {}", vertex.m_outputs.size());
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clipping_buffer_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clipping_buffer_t& buffer, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "vertices: {}", buffer.m_vertices.size());
        out = std::format_to(out, ", values: {}", buffer.m_values.size());
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clipping_workspace_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::clipping_workspace_t& workspace, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "first: {}", workspace.m_buffers[0]);
        out = std::format_to(out, ", second: {}", workspace.m_buffers[1]);
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::projected_vertex_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::projected_vertex_t& vertex, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "x: {}", vertex.m_point[0]);
        out = std::format_to(out, ", y: {}", vertex.m_point[1]);
        out = std::format_to(out, ", ndc_z: {}", vertex.m_ndc_z);
        out = std::format_to(out, ", reciprocal_w: {}", vertex.m_reciprocal_w);
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::scan_event_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::scan_event_t& event, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "numerator: {}", event.m_x[0]);
        out = std::format_to(out, ", denominator: {}", event.m_x[1]);
        out = std::format_to(out, ", lower: {}", event.m_lower);
        out = std::format_to(out, ", upper: {}", event.m_upper);
        out = std::format_to(out, ", delta: {}", event.m_delta);
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::sample_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::sample_t& sample, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "x: {}", sample.m_x);
        out = std::format_to(out, ", y: {}", sample.m_y);
        out = std::format_to(out, ", vertices: {}", sample.m_count);
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_workspace_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_workspace_t& workspace, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "vertices: {}", workspace.m_vertices.size());
        out = std::format_to(out, ", triangles: {}", workspace.m_triangles.size());
        out = std::format_to(out, ", empty: {}", workspace.m_empty);
        out = std::format_to(out, ", front_facing: {}", workspace.m_front_facing);
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::screen_vertex_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::screen_vertex_t& vertex, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "x: {}", vertex.m_x);
        out = std::format_to(out, ", y: {}", vertex.m_y);
        out = std::format_to(out, ", ndc_z: {}", vertex.m_ndc_z);
        out = std::format_to(out, ", reciprocal_w: {}", vertex.m_reciprocal_w);
        out = std::format_to(out, ", outputs: {}", vertex.m_outputs.size());
        out = std::format_to(out, " }}");

        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::scratch_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::scratch_t& scratch, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "vertices: {}", scratch.m_vertex_results.size());
        out = std::format_to(out, ", values: {}", scratch.m_vertex_values.size());
        out = std::format_to(out, ", clipping: {}", scratch.m_clipping);
        out = std::format_to(out, ", raster: {}", scratch.m_raster);
        out = std::format_to(out, ", fragment_inputs: {}", scratch.m_fragment_inputs.size());
        out = std::format_to(out, " }}");
        return out;
    }
};

} // namespace std
#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_HELPERS_H
