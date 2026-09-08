#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_HELPERS_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_HELPERS_H

# include "camera.h"
# include "framebuffer.h"
# include "material.h"
# include "profiling_metrics.h"
# include "render_item.h"
# include "vertex_attribute.h"

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03gjfvd6i5jzbmngb2ldoooza_type_erased_array/api.h>
# include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>
# include <m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h>
# include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

# include <algorithm>
# include <cmath>
# include <stdexcept>
# include <utility>
# include <type_traits>
# include <array>
# include <cstddef>
# include <cstdint>
# include <format>
# include <optional>
# include <span>
# include <variant>
# include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace software_shader = m03gt1djvvy5atia5evkbg6rqy_software_shader;
namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;
namespace type_erased_array = m03gjfvd6i5jzbmngb2ldoooza_type_erased_array;

using vector2f_t = m03gsy25j4v7nccgmsdov9ioft_shader::vector_t<float, 2>;
using vector4f_t = m03gsy25j4v7nccgmsdov9ioft_shader::vector_t<float, 4>;
// Carry W*attribute through clipping, postponing division until W is positive.
// Double storage avoids overflow and division by zero at intermediate clip vertices.
struct noperspective_t {
    std::array<double, 4> numerators {};
    std::size_t count = 0;
};

using varying_t = std::variant<float, vector2f_t, shader::vector_t<float, 3>, vector4f_t, noperspective_t>;
using flat_t = std::variant<float, vector2f_t, shader::vector_t<float, 3>, vector4f_t,
    std::int32_t, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 4>,
    std::uint32_t, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 4>>;
using flat_values_t = std::vector<flat_t>;
using varying_values_t = std::vector<varying_t>;
using grid_point_t = std::array<std::int64_t, 2>;
using triangle_t = std::array<std::size_t, 3>;
// Unreduced nonnegative numerator and positive denominator.
using edge_value_t = __int128;
using fraction_t = std::array<edge_value_t, 2>;

constexpr int maximum_extent = 1 << 23;
constexpr std::int64_t subpixels = 256;
constexpr std::int64_t center_offset = 128;

// Bounds in viewport-local pixels; the intersection limits work without changing mapping.
struct raster_bounds_t {
    int width;
    int height;
    int x;
    int y;
    std::int64_t view_width;
    std::int64_t view_height;
    std::int64_t first_x;
    std::int64_t first_y;
    std::int64_t end_x;
    std::int64_t end_y;

    raster_bounds_t(int width, int height, const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& view_rect);
    bool empty() const;
};

struct pipeline_vertex_t {
    vector4f_t clip_position;
    std::array<std::size_t, 2> outputs; // Offset and count in the owning value buffer.
    std::array<std::size_t, 2> flat_outputs {};
};

struct pipeline_vertex_view_t {
    vector4f_t clip_position;
    std::span<const varying_t> outputs;
    std::span<const flat_t> flat_outputs {};
};

struct clipping_buffer_t {
    std::vector<pipeline_vertex_t> vertices;
    varying_values_t values;
};

struct clipping_workspace_t {
    std::array<clipping_buffer_t, 2> buffers;
};

struct projected_vertex_t {
    grid_point_t point;
    double ndc_z;
    double reciprocal_w;
    pipeline_vertex_view_t source;
};

struct scan_event_t {
    fraction_t x;
    std::size_t lower;
    std::size_t upper;
    int delta;
};

struct sample_t {
    std::int64_t x;
    std::int64_t y;
    std::array<std::size_t, 4> vertices;
    std::array<double, 4> weights;
    std::size_t count;
};

struct raster_workspace_t {
    clipping_workspace_t clipping;
    std::vector<projected_vertex_t> vertices;
    std::vector<std::size_t> geometry;
    std::vector<std::size_t> ring;
    std::vector<triangle_t> triangles;
    std::vector<scan_event_t> events;
    bool empty = true;
    bool use_triangles = false;
    bool front_facing = false;
};

struct vertex_input_t {
    const type_erased_array::type_erased_array_t& stream;
    software_shader::value_t (*read)(const type_erased_array::type_erased_array_t&, std::uint32_t);

    vertex_input_t(const type_erased_array::type_erased_array_t& stream, const vertex_attribute_t& attribute, shader::shader_data_type_t type);
};

// Renderer-owned storage retains its peak capacities across draws.
struct scratch_t {
    std::vector<pipeline_vertex_t> vertex_results;
    varying_values_t vertex_values;
    flat_values_t flat_values;
    std::vector<std::size_t> interpolated_inputs;
    std::vector<std::size_t> flat_inputs;
    clipping_workspace_t clipping;
    raster_workspace_t raster;
    software_shader::prepared_program_t prepared_program;
    std::vector<vertex_input_t> vertex_bindings;
    std::vector<software_shader::value_t> vertex_inputs;
    std::vector<software_shader::value_t> fragment_values;
    std::vector<std::optional<software_shader::value_t>> vertex_outputs;
    std::vector<std::optional<software_shader::value_t>> fragment_outputs;
    software_shader::execution_context_t execution_context;
    software_shader::vertex_io_t vertex_io {0, 0};
    software_shader::fragment_io_t fragment_io {vector4f_t(0.0F), true};
};

// A draw's color equations and attachment-specific storage operation.
struct color_state_t {
    blend_equation_t rgb;
    blend_equation_t alpha;
    vector4f_t constant;
    color_mask_t mask;
    void (*write)(const color_state_t&, const vector4f_t&, std::byte*) = nullptr;

    color_state_t(const material_t& material, texture::format_t format);
};

// Borrows one draw's state; destruction releases prepared shader resources.
struct draw_context_t {
    const material_t& material;
    const raster_bounds_t& bounds;
    const framebuffer_t& framebuffer;
    scratch_t& scratch;
    color_state_t color;
    m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t* metric = nullptr;

    draw_context_t(const material_t& material, const raster_bounds_t& bounds, const framebuffer_t& framebuffer, scratch_t& scratch);
    ~draw_context_t();
    draw_context_t(const draw_context_t&) = delete;
    draw_context_t& operator=(const draw_context_t&) = delete;
};

pipeline_vertex_view_t view(const pipeline_vertex_t& vertex, const varying_values_t& values, const flat_values_t& flat_values = {});

double clip_distance(const pipeline_vertex_view_t& vertex, std::size_t plane);

std::optional<std::size_t> clip_line(const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, clipping_workspace_t& workspace);

std::optional<std::size_t> clip_triangle(const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, const pipeline_vertex_view_t& third, clipping_workspace_t& workspace);

double projectable_reciprocal_w(float w);

std::int64_t snap(double screen, std::int64_t extent);

edge_value_t edge(grid_point_t a, grid_point_t b, grid_point_t p);

int compare_fraction(fraction_t a, fraction_t b);

std::int64_t sample_bound(fraction_t crossing, std::int64_t extent);

void prepare_polygon(raster_workspace_t& workspace);

void prepare_triangle(const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, const pipeline_vertex_view_t& third, std::int64_t width, std::int64_t height, raster_workspace_t& workspace);

void scanline_events(std::span<const projected_vertex_t> vertices, std::int64_t y, std::vector<scan_event_t>& events);

sample_t span_sample(const scan_event_t& left, const scan_event_t& right, std::span<const projected_vertex_t> vertices, std::int64_t x, std::int64_t y);

// Writes interpolated payloads to the mapped fragment input slots, preserving other slots.
// Returns window depth and reciprocal W.
std::array<double, 2> interpolate_sample(std::span<const projected_vertex_t> vertices, const sample_t& sample, std::span<const std::size_t> input_slots, std::span<software_shader::value_t> outputs);

bool finite(const vector4f_t& vector);

std::size_t shader_component_count(shader::shader_data_type_t type);

bool supported_fragment_input(const shader::shader_interface_element_t& input);

varying_t vertex_output(
    const std::optional<software_shader::value_t>& output,
    const shader::shader_interface_element_t& input
);

flat_t flat_output(const std::optional<software_shader::value_t>& output, const shader::shader_interface_element_t& input);

// Sanitizes NaN/infinities and clamps finite values without vector normalization.
float sanitize_unorm(float component);

float depth_clear_value(float depth);

void validate_feedback(const material_t& material, const framebuffer_t& framebuffer);

void rasterize_point(draw_context_t& draw, const pipeline_vertex_view_t& vertex);

void rasterize_line(draw_context_t& draw, const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second);

void rasterize_triangle(draw_context_t& draw, const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, const pipeline_vertex_view_t& third, std::span<const flat_t> flat_inputs);

// The renderer and validation consume the same pre-shading coverage events.
template <typename emit_type_t>
void visit_samples(raster_workspace_t& workspace, std::int64_t width, std::int64_t height, emit_type_t&& emit, std::int64_t first_x = 0, std::int64_t first_y = 0);

struct screen_vertex_t {
    double x;
    double y;
    double ndc_z;
    double reciprocal_w;
    std::span<const varying_t> outputs;
};

template <typename T>
T require_vertex_output(
    const std::optional<software_shader::value_t>& output,
    std::uint32_t location
);

void clear(clipping_buffer_t& buffer);

void append_vertex(clipping_buffer_t& destination, const pipeline_vertex_view_t& source);

varying_t interpolate(const varying_t& from, const varying_t& to, double factor);

void append_intersection(clipping_buffer_t& destination, pipeline_vertex_view_t first, pipeline_vertex_view_t second, std::size_t plane);

bool between(grid_point_t p, grid_point_t a, grid_point_t b);

bool opposite(edge_value_t a, edge_value_t b);

bool intersects(grid_point_t a, grid_point_t b, grid_point_t c, grid_point_t d);

bool simple_boundary(const raster_workspace_t& workspace);

bool triangulate(raster_workspace_t& workspace);

bool crossed_facing(std::span<const projected_vertex_t> vertices);

int compare_varying(const varying_t& a, const varying_t& b);

int compare_record(const projected_vertex_t& a, const projected_vertex_t& b);

bool edge_record_less(const scan_event_t& a, const scan_event_t& b, std::span<const projected_vertex_t> vertices);

bool inside_clip_volume(const pipeline_vertex_view_t& vertex);

edge_value_t ceil_div(edge_value_t numerator, edge_value_t denominator);

vertex_attribute_type_t expected_attribute_type(shader::shader_scalar_type_t type);

void validate_vertex_attribute(
    const vertex_attribute_t& attribute,
    shader::shader_data_type_t input_type
);

std::optional<screen_vertex_t> project(
    const pipeline_vertex_view_t& vertex,
    std::int64_t width,
    std::int64_t height
);

float blend_factor_component(blend_factor_t factor, const vector4f_t& source, const vector4f_t& destination, const vector4f_t& constant, std::size_t component);

float blend_component(const blend_equation_t& blend_equation, const vector4f_t& source, const vector4f_t& destination, const vector4f_t& constant, std::size_t component);

template <texture::format_t Format, bool Replacement, bool Masked>
void write_color(const color_state_t& state, const vector4f_t& source, std::byte* pixel);

bool depth_passes(comparison_t comparison, float incoming, float stored);

bool stencil_passes(const stencil_state_t& stencil_state, std::uint8_t stored);

void write_stencil(const stencil_state_t& stencil_state, stencil_op_t operation, std::uint8_t& stored, m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t& metric);

bool storage_overlaps(std::span<const std::byte> left, std::span<const std::byte> right);

void load_flat_inputs(scratch_t& scratch, std::span<const flat_t> flat_inputs);

void shade_sample(draw_context_t& draw, std::int64_t x, std::int64_t y, float depth, float reciprocal_w, bool front_facing);

template <typename T, std::size_t N>
software_shader::value_t read_vertex_input(const type_erased_array::type_erased_array_t& stream, std::uint32_t vertex_index);

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::noperspective_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_bounds_t>;

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
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_input_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::scratch_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_state_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_context_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::screen_vertex_t>;

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

template <typename emit_type_t>
void visit_samples(raster_workspace_t& workspace, std::int64_t width, std::int64_t height, emit_type_t&& emit, std::int64_t clip_first_x, std::int64_t clip_first_y) {
    if (workspace.empty) {
        return;
    }
    const auto& vertices = workspace.vertices;
    // Top/left inclusion classifies a center infinitesimally to the right, then
    // infinitesimally below. Ears and winding spans apply the same convention.
    if (workspace.use_triangles) {
        for (const auto& indices : workspace.triangles) {
            const auto a = vertices[indices[0]].point;
            const auto b = vertices[indices[1]].point;
            const auto c = vertices[indices[2]].point;
            const auto area = edge(a, b, c);
            const auto inclusive = [](grid_point_t from, grid_point_t to) {
                return to[1] < from[1] || (to[1] == from[1] && from[0] < to[0]);
            };
            const std::array top_left {inclusive(b, c), inclusive(c, a), inclusive(a, b)};
            const auto first_x = std::max(clip_first_x, sample_bound({std::min({a[0], b[0], c[0]}), 1}, width));
            const auto end_x = sample_bound({std::max({a[0], b[0], c[0]}) + 1, 1}, width);
            const auto first_y = std::max(clip_first_y, sample_bound({std::min({a[1], b[1], c[1]}), 1}, height));
            const auto end_y = sample_bound({std::max({a[1], b[1], c[1]}) + 1, 1}, height);
            const grid_point_t origin {first_x * subpixels + center_offset, first_y * subpixels + center_offset};
            std::array row_values {edge(b, c, origin), edge(c, a, origin), edge(a, b, origin)};
            const std::array step_x {edge_value_t(b[1] - c[1]) * subpixels, edge_value_t(c[1] - a[1]) * subpixels, edge_value_t(a[1] - b[1]) * subpixels};
            const std::array step_y {edge_value_t(c[0] - b[0]) * subpixels, edge_value_t(a[0] - c[0]) * subpixels, edge_value_t(b[0] - a[0]) * subpixels};
            // An edge determinant is affine in X/Y. Wide-integer additions preserve
            // exactly the same edge values and top/left decisions at every sample.
            for (auto y = first_y; y < end_y; ++y) {
                auto values = row_values;
                for (auto x = first_x; x < end_x; ++x) {
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
                    for (std::size_t i = 0; i < 3; ++i) { values[i] += step_x[i]; }
                }
                for (std::size_t i = 0; i < 3; ++i) { row_values[i] += step_y[i]; }
            }
        }
        return;
    }

    const auto [minimum, maximum] = std::ranges::minmax_element(vertices, {}, [](const auto& vertex) { return vertex.point[1]; });
    const auto first_y = std::max(clip_first_y, sample_bound({minimum->point[1], 1}, height));
    const auto end_y = sample_bound({maximum->point[1], 1}, height);
    for (auto y = first_y; y < end_y; ++y) {
        scanline_events(vertices, std::int64_t(y) * subpixels + center_offset, workspace.events);
        const auto& events = workspace.events;
        int winding = 0;
        scan_event_t left {};
        for (std::size_t begin = 0; begin < events.size();) {
            std::size_t end = begin;
            int delta = 0;
            do {
                delta += events[end++].delta;
            } while (end < events.size() && compare_fraction(events[begin].x, events[end].x) == 0);
            const int after = winding + delta;
            if ((winding == 0) != (after == 0)) {
                // Equal crossings are ordered by edge_record_less. Select the
                // least endpoint-record key with the net crossing direction.
                std::size_t selected = begin;
                while ((events[selected].delta < 0) != (delta < 0)) {
                    ++selected;
                }
                const auto& boundary = events[selected];
                if (winding == 0) {
                    left = boundary;
                } else {
                    const auto first_x = std::max(clip_first_x, sample_bound(left.x, width));
                    const auto end_x = sample_bound(boundary.x, width);
                    for (auto x = first_x; x < end_x; ++x) {
                        emit(span_sample(left, boundary, vertices, x, y));
                    }
                }
            }
            winding = after;
            begin = end;
        }
    }
}

template <typename T>
T require_vertex_output(
    const std::optional<software_shader::value_t>& output,
    std::uint32_t location
) {
    if (!output) {
        throw std::runtime_error(std::format(
            "vertex shader did not write output location {} required by the fragment shader",
            location
        ));
    }
    return std::get<T>(*output);
}

template <texture::format_t Format, bool Replacement, bool Masked>
void write_color(const color_state_t& state, const vector4f_t& source, std::byte* pixel) {
    if constexpr (Replacement && !Masked) {
        texture::encode_rgba8<Format>(source, std::span<std::byte, 4>(pixel, 4));
        return;
    }
    const vector4f_t sanitized_source {
        sanitize_unorm(source[0]), sanitize_unorm(source[1]),
        sanitize_unorm(source[2]), sanitize_unorm(source[3])
    };
    vector4f_t result = sanitized_source;
    if constexpr (!Replacement) {
        const auto destination = texture::decode_rgba8<Format>(std::span<const std::byte, 4>(pixel, 4));
        // Both equations use the original destination before any channel is stored.
        for (std::size_t component = 0; component < 4; ++component) {
            result[component] = sanitize_unorm(blend_component(component == 3 ? state.alpha : state.rgb, sanitized_source, destination, state.constant, component));
        }
    }
    if constexpr (!Masked) {
        texture::encode_rgba8<Format>(result, std::span<std::byte, 4>(pixel, 4));
    } else {
        std::array<std::byte, 4> stored;
        texture::encode_rgba8<Format>(result, stored);
        // Disabled bytes never round-trip through float.
        if ((state.mask & color_mask_t::red) != color_mask_t::none) { pixel[0] = stored[0]; }
        if ((state.mask & color_mask_t::green) != color_mask_t::none) { pixel[1] = stored[1]; }
        if ((state.mask & color_mask_t::blue) != color_mask_t::none) { pixel[2] = stored[2]; }
        if ((state.mask & color_mask_t::alpha) != color_mask_t::none) { pixel[3] = stored[3]; }
    }
}

template <typename T, std::size_t N>
software_shader::value_t read_vertex_input(const type_erased_array::type_erased_array_t& stream, std::uint32_t vertex_index) {
    if constexpr (N == 1) { return stream.read<T>(vertex_index); }
    else { return shader::vector_t<T, N>(stream.read<std::array<T, N>>(vertex_index)); }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::noperspective_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::noperspective_t& noperspective, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ count: {}", noperspective.count);
        for (std::size_t i = 0; i < noperspective.count; ++i) {
            out = std::format_to(out, ", {}", noperspective.numerators[i]);
        }
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_bounds_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::raster_bounds_t& bounds, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ origin: ({}, {})", bounds.x, bounds.y);
        out = std::format_to(out, ", size: ({}, {})", bounds.view_width, bounds.view_height);
        out = std::format_to(out, ", intersection: [{}, {}) x [{}, {}) }}", bounds.first_x, bounds.end_x, bounds.first_y, bounds.end_y);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::pipeline_vertex_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::pipeline_vertex_t& vertex, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "position: {}", vertex.clip_position);
        out = std::format_to(out, ", offset: {}", vertex.outputs[0]);
        out = std::format_to(out, ", count: {}", vertex.outputs[1]);
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
        out = std::format_to(out, "position: {}", vertex.clip_position);
        out = std::format_to(out, ", outputs: {}", vertex.outputs.size());
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
        out = std::format_to(out, "vertices: {}", buffer.vertices.size());
        out = std::format_to(out, ", values: {}", buffer.values.size());
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
        out = std::format_to(out, "first: {}", workspace.buffers[0]);
        out = std::format_to(out, ", second: {}", workspace.buffers[1]);
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
        out = std::format_to(out, "x: {}", vertex.point[0]);
        out = std::format_to(out, ", y: {}", vertex.point[1]);
        out = std::format_to(out, ", ndc_z: {}", vertex.ndc_z);
        out = std::format_to(out, ", reciprocal_w: {}", vertex.reciprocal_w);
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
        out = std::format_to(out, "numerator: {}", event.x[0]);
        out = std::format_to(out, ", denominator: {}", event.x[1]);
        out = std::format_to(out, ", lower: {}", event.lower);
        out = std::format_to(out, ", upper: {}", event.upper);
        out = std::format_to(out, ", delta: {}", event.delta);
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
        out = std::format_to(out, "x: {}", sample.x);
        out = std::format_to(out, ", y: {}", sample.y);
        out = std::format_to(out, ", vertices: {}", sample.count);
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
        out = std::format_to(out, "vertices: {}", workspace.vertices.size());
        out = std::format_to(out, ", triangles: {}", workspace.triangles.size());
        out = std::format_to(out, ", empty: {}", workspace.empty);
        out = std::format_to(out, ", front_facing: {}", workspace.front_facing);
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_input_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::vertex_input_t& input, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "vertex input elements={}", input.stream.element_count());
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
        out = std::format_to(out, "vertices: {}", scratch.vertex_results.size());
        out = std::format_to(out, ", values: {}", scratch.vertex_values.size());
        out = std::format_to(out, ", clipping: {}", scratch.clipping);
        out = std::format_to(out, ", raster: {}", scratch.raster);
        out = std::format_to(out, ", fragment_inputs: {}", scratch.fragment_values.size());
        out = std::format_to(out, " }}");
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_state_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_state_t& color, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "rgb={} alpha={}", color.rgb, color.alpha);
        out = std::format_to(out, " constant={} mask={}", color.constant, color.mask);
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_context_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::draw_context_t& draw, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "draw bounds={}", draw.bounds);
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
        out = std::format_to(out, "x: {}", vertex.x);
        out = std::format_to(out, ", y: {}", vertex.y);
        out = std::format_to(out, ", ndc_z: {}", vertex.ndc_z);
        out = std::format_to(out, ", reciprocal_w: {}", vertex.reciprocal_w);
        out = std::format_to(out, ", outputs: {}", vertex.outputs.size());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_HELPERS_H
