#include "helpers.h"

#include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

static_assert(std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::digits == 24);
static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53);

raster_bounds_t::raster_bounds_t(int width, int height, const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<int, 2>& view_rect):
    m_width(width),
    m_height(height),
    m_x(view_rect[0][0]),
    m_y(view_rect[1][0]),
    m_view_width(std::int64_t(view_rect[0][1]) - m_x),
    m_view_height(std::int64_t(view_rect[1][1]) - m_y),
    m_first_x(std::max<std::int64_t>(0, -std::int64_t(m_x))),
    m_first_y(std::max<std::int64_t>(0, -std::int64_t(m_y))),
    m_end_x(std::min(m_view_width, std::int64_t(width) - m_x)),
    m_end_y(std::min(m_view_height, std::int64_t(height) - m_y))
{
}

bool raster_bounds_t::empty() const {
    return m_end_x <= m_first_x || m_end_y <= m_first_y;
}

void clear(clipping_buffer_t& buffer) {
    buffer.m_vertices.clear();
    buffer.m_values.clear();
}

void append_vertex(clipping_buffer_t& destination, const pipeline_vertex_view_t& source) {
    const auto offset = destination.m_values.size();
    destination.m_values.insert(destination.m_values.end(), source.m_outputs.begin(), source.m_outputs.end());
    destination.m_vertices.push_back({source.m_clip_position, {offset, source.m_outputs.size()}});
}

varying_t interpolate(const varying_t& from, const varying_t& to, double factor) {
    return std::visit([&](const auto& first) -> varying_t {
        using type_t = std::remove_cvref_t<decltype(first)>;
        const auto& second = std::get<type_t>(to);
        if constexpr (std::is_same_v<type_t, noperspective_t>) {
            noperspective_t result;
            result.count = first.count;
            for (std::size_t i = 0; i < first.count; ++i) {
                result.numerators[i] = std::lerp(first.numerators[i], second.numerators[i], factor);
            }
            return result;
        } else if constexpr (std::is_same_v<type_t, float>) {
            return float(std::lerp(double(first), double(second), factor));
        } else {
            type_t result;
            std::size_t i = 0;
            for (float& component : result) {
                component = float(std::lerp(double(first[i]), double(second[i]), factor));
                ++i;
            }
            return result;
        }
    },
        from);
}

void append_intersection(clipping_buffer_t& destination, pipeline_vertex_view_t first, pipeline_vertex_view_t second, std::size_t plane) {
    if (std::lexicographical_compare(second.m_clip_position.begin(), second.m_clip_position.end(), first.m_clip_position.begin(), first.m_clip_position.end())) {
        std::swap(first, second);
    }
    unsigned pins = 1U << plane;
    for (std::size_t earlier = 0; earlier < plane; ++earlier) {
        if (clip_distance(first, earlier) == 0.0 && clip_distance(second, earlier) == 0.0) {
            pins |= 1U << earlier;
        }
    }
    double da = std::abs(clip_distance(first, plane));
    double db = std::abs(clip_distance(second, plane));
    if (db < da) {
        std::swap(first, second);
        std::swap(da, db);
    }
    // Ordering keeps shared-edge intersections identical in either direction;
    // widening avoids float overflow. A zero factor preserves an on-plane endpoint.
    const double factor = da / (da + db);
    vector4f_t position;
    for (std::size_t i = 0; i < 4; ++i) {
        position[i] = float(std::lerp(double(first.m_clip_position[i]), double(second.m_clip_position[i]), factor));
    }
    for (std::size_t axis = 0; axis < 3; ++axis) {
        if ((pins & (3U << (2 * axis))) == (3U << (2 * axis))) {
            position[3] = 0.0F;
        }
    }
    if (1 <= plane && position[3] < 0.0F) {
        position[3] = 0.0F;
    }
    for (std::size_t constraint = 0; constraint <= plane; ++constraint) {
        if (pins & (1U << constraint)) {
            position[constraint / 2] = constraint % 2 == 0 ? -position[3] : position[3];
        }
    }
    // Repair narrowing against all processed half-spaces, never with an epsilon.
    for (std::size_t constraint = 0; constraint <= plane; ++constraint) {
        auto& component = position[constraint / 2];
        component = constraint % 2 == 0 ? std::max(component, -position[3]) : std::min(component, position[3]);
    }
    const auto offset = destination.m_values.size();
    if (first.m_outputs.size() != second.m_outputs.size()) {
        throw std::logic_error("clipping has inconsistent varying counts");
    }
    for (std::size_t i = 0; i < first.m_outputs.size(); ++i) {
        const auto& [location, from] = first.m_outputs[i];
        const auto& [other_location, to] = second.m_outputs[i];
        if (location != other_location || from.index() != to.index()) {
            throw std::logic_error("clipping has inconsistent varying locations or types");
        }
        destination.m_values.emplace_back(location, interpolate(from, to, factor));
    }
    destination.m_vertices.push_back({position, {offset, first.m_outputs.size()}});
}

bool between(grid_point_t p, grid_point_t a, grid_point_t b) {
    return std::min(a[0], b[0]) <= p[0] && p[0] <= std::max(a[0], b[0]) && std::min(a[1], b[1]) <= p[1] && p[1] <= std::max(a[1], b[1]);
}

bool opposite(edge_value_t a, edge_value_t b) {
    return (a < 0 && 0 < b) || (b < 0 && 0 < a);
}

bool intersects(grid_point_t a, grid_point_t b, grid_point_t c, grid_point_t d) {
    const auto ac = edge(a, b, c), ad = edge(a, b, d);
    const auto ca = edge(c, d, a), cb = edge(c, d, b);
    const bool crosses_interior = opposite(ac, ad) && opposite(ca, cb);
    const bool first_touches = (ac == 0 && between(c, a, b)) || (ad == 0 && between(d, a, b));
    const bool second_touches = (ca == 0 && between(a, c, d)) || (cb == 0 && between(b, c, d));
    return crosses_interior || first_touches || second_touches;
}

bool simple_boundary(const raster_workspace_t& workspace) {
    const auto& ring = workspace.m_geometry;
    const auto point = [&](std::size_t i) { return workspace.m_vertices[ring[i % ring.size()]].m_point; };
    for (std::size_t i = 0; i < ring.size(); ++i) {
        const auto a = point(i), b = point(i + 1), c = point(i + 2);
        if (edge(a, b, c) == 0 && !between(b, a, c)) {
            return false;
        }
        for (std::size_t j = i + 2; j < ring.size(); ++j) {
            if (i == 0 && j + 1 == ring.size()) {
                continue;
            }
            if (intersects(a, b, point(j), point(j + 1))) {
                return false;
            }
        }
    }
    return true;
}

bool triangulate(raster_workspace_t& workspace) {
    // Normalize screen winding, start at the least (X,Y), and remove the first
    // unblocked convex ear to make interpolation deterministic. Keep collinear
    // and coincident occurrences with their own payloads; a zero-area occurrence
    // may contribute no samples.
    auto& ring = workspace.m_ring;
    ring.resize(workspace.m_vertices.size());
    std::iota(ring.begin(), ring.end(), std::size_t(0));
    if (workspace.m_front_facing) {
        std::reverse(ring.begin(), ring.end());
    }
    const auto point = [&](std::size_t i) { return workspace.m_vertices[i].m_point; };
    std::size_t start = 0;
    bool found = false;
    for (std::size_t i = 0; i < ring.size(); ++i) {
        if (point(ring[i]) != point(ring[(i + ring.size() - 1) % ring.size()]) && (!found || point(ring[i]) < point(ring[start]))) {
            start = i;
            found = true;
        }
    }
    std::rotate(ring.begin(), ring.begin() + start, ring.end());
    while (3 <= ring.size()) {
        std::optional<std::size_t> selected;
        for (std::size_t i = 0; i < ring.size(); ++i) {
            const triangle_t ids {ring[(i + ring.size() - 1) % ring.size()], ring[i], ring[(i + 1) % ring.size()]};
            const auto a = point(ids[0]), b = point(ids[1]), c = point(ids[2]);
            if (edge(a, b, c) <= 0) {
                continue;
            }
            const bool blocked = std::ranges::any_of(ring, [&](std::size_t other) {
                const auto p = point(other);
                return p != a && p != b && p != c && 0 <= edge(a, b, p) && 0 <= edge(b, c, p) && 0 <= edge(c, a, p);
            });
            if (!blocked) {
                workspace.m_triangles.push_back(ids);
                selected = i;
                break;
            }
        }
        if (!selected) {
            for (std::size_t i = 0; i < ring.size(); ++i) {
                const auto a = point(ring[(i + ring.size() - 1) % ring.size()]);
                const auto b = point(ring[i]), c = point(ring[(i + 1) % ring.size()]);
                if (edge(a, b, c) == 0 && between(b, a, c)) {
                    selected = i;
                    break;
                }
            }
        }
        if (!selected) {
            workspace.m_triangles.clear();
            return false; // The complete boundary will use winding coverage.
        }
        ring.erase(ring.begin() + *selected);
    }
    return true;
}

bool crossed_facing(std::span<const projected_vertex_t> vertices) {
    // A non-simple boundary has no single winding orientation. Use its least
    // cyclic grid sequence over both directions, then the largest absolute fan
    // determinant (first on ties), restoring the submitted direction. This gives
    // every generated piece the same original-primitive facing value.
    const auto count = vertices.size();
    const auto at = [&](std::size_t start, bool reversed, std::size_t offset) {
        return vertices[(start + (reversed ? count - offset : offset)) % count].m_point;
    };
    std::size_t start = 0;
    bool reversed = false;
    for (bool candidate_reversed : {false, true}) {
        for (std::size_t candidate = 0; candidate < count; ++candidate) {
            for (std::size_t offset = 0; offset < count; ++offset) {
                const auto a = at(candidate, candidate_reversed, offset), b = at(start, reversed, offset);
                if (a != b) {
                    if (a < b) {
                        start = candidate;
                        reversed = candidate_reversed;
                    } break;
                }
            }
        }
    }
    edge_value_t largest = 0;
    for (std::size_t i = 1; i + 1 < count; ++i) {
        const auto area = edge(at(start, reversed, 0), at(start, reversed, i), at(start, reversed, i + 1));
        if ((largest < 0 ? -largest : largest) < (area < 0 ? -area : area)) {
            largest = area;
        }
    }
    return (reversed ? -largest : largest) < 0;
}

int compare_varying(const varying_t& a, const varying_t& b) {
    if (a.index() != b.index()) {
        return a.index() < b.index() ? -1 : 1;
    }
    const auto bits_compare = [](float first, float second) {
        const auto x = std::bit_cast<std::uint32_t>(first), y = std::bit_cast<std::uint32_t>(second);
        return x == y ? 0 : (x < y ? -1 : 1);
    };
    return std::visit([&](const auto& first) {
        using type_t = std::remove_cvref_t<decltype(first)>;
        const auto& second = std::get<type_t>(b);
        if constexpr (std::is_same_v<type_t, noperspective_t>) {
            if (first.count != second.count) { return first.count < second.count ? -1 : 1; }
            for (std::size_t i = 0; i < first.count; ++i) {
                const auto x = std::bit_cast<std::uint64_t>(first.numerators[i]);
                const auto y = std::bit_cast<std::uint64_t>(second.numerators[i]);
                if (x != y) { return x < y ? -1 : 1; }
            }
            return 0;
        } else if constexpr (std::is_same_v<type_t, float>) {
            return bits_compare(first, second);
        } else {
            std::size_t i = 0;
            for (float component : first) {
                const int comparison = bits_compare(component, second[i++]);
                if (comparison != 0) {
                    return comparison;
                }
            }
            return 0;
        }
    },
        a);
}

int compare_record(const projected_vertex_t& a, const projected_vertex_t& b) {
    for (const auto& [x, y] : {std::pair(a.m_ndc_z, b.m_ndc_z), std::pair(a.m_reciprocal_w, b.m_reciprocal_w)}) {
        if (x != y) {
            return x < y ? -1 : 1;
        }
    }
    for (std::size_t i = 0; i < 4; ++i) {
        if (a.m_source.m_clip_position[i] != b.m_source.m_clip_position[i]) {
            return a.m_source.m_clip_position[i] < b.m_source.m_clip_position[i] ? -1 : 1;
        }
    }
    const auto& first = a.m_source.m_outputs;
    const auto& second = b.m_source.m_outputs;
    for (std::size_t i = 0; i < std::min(first.size(), second.size()); ++i) {
        if (first[i].first != second[i].first) {
            return first[i].first < second[i].first ? -1 : 1;
        }
        const int comparison = compare_varying(first[i].second, second[i].second);
        if (comparison != 0) {
            return comparison;
        }
    }
    return first.size() == second.size() ? 0 : (first.size() < second.size() ? -1 : 1);
}

bool edge_record_less(const scan_event_t& a, const scan_event_t& b, std::span<const projected_vertex_t> vertices) {
    // Break equal crossing positions by endpoint geometry, then projection and
    // source records, ending with payload bits. Keep coincident payloads distinct.
    for (const auto& [first, second] : {std::pair(a.m_lower, b.m_lower), std::pair(a.m_upper, b.m_upper)}) {
        if (vertices[first].m_point != vertices[second].m_point) {
            return vertices[first].m_point < vertices[second].m_point;
        }
    }
    const int lower = compare_record(vertices[a.m_lower], vertices[b.m_lower]);
    return lower != 0 ? lower < 0 : compare_record(vertices[a.m_upper], vertices[b.m_upper]) < 0;
}

pipeline_vertex_view_t view(const pipeline_vertex_t& vertex, const varying_values_t& values, const flat_values_t& flat_values) {
    return {vertex.m_clip_position,
        std::span<const varying_entry_t>(values).subspan(vertex.m_outputs[0], vertex.m_outputs[1]),
        std::span<const flat_entry_t>(flat_values).subspan(vertex.m_flat_outputs[0], vertex.m_flat_outputs[1])};
}

double clip_distance(const pipeline_vertex_view_t& vertex, std::size_t plane) {
    const double component = vertex.m_clip_position[plane / 2];
    const double w = vertex.m_clip_position[3];
    return plane % 2 == 0 ? component + w : w - component;
}

bool inside_clip_volume(const pipeline_vertex_view_t& vertex) {
    for (std::size_t plane = 0; plane < 6; ++plane) {
        if (clip_distance(vertex, plane) < 0.0) {
            return false;
        }
    }
    return vertex.m_clip_position[3] != 0.0F;
}

std::optional<std::size_t> clip_line(const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, clipping_workspace_t& workspace) {
    clear(workspace.m_buffers[0]);
    append_vertex(workspace.m_buffers[0], first);
    append_vertex(workspace.m_buffers[0], second);
    std::size_t source_index = 0;
    for (std::size_t plane = 0; plane < 6; ++plane) {
        auto& source = workspace.m_buffers[source_index];
        auto& destination = workspace.m_buffers[1 - source_index];
        clear(destination);
        const auto from = view(source.m_vertices[0], source.m_values), to = view(source.m_vertices[1], source.m_values);
        const double da = clip_distance(from, plane), db = clip_distance(to, plane);
        if (da < 0.0 && db < 0.0) {
            return std::nullopt;
        }
        if (0.0 <= da && 0.0 <= db) {
            append_vertex(destination, from);
            append_vertex(destination, to);
        } else if (da == 0.0 || db == 0.0) {
            const auto& on_plane = da == 0.0 ? from : to;
            append_vertex(destination, on_plane);
            append_vertex(destination, on_plane);
        } else if (da < 0.0) {
            append_intersection(destination, from, to, plane);
            append_vertex(destination, to);
        } else {
            append_vertex(destination, from);
            append_intersection(destination, from, to, plane);
        }
        source_index = 1 - source_index;
    }
    for (const auto& vertex : workspace.m_buffers[source_index].m_vertices) {
        if (vertex.m_clip_position[3] == 0.0F) {
            return std::nullopt;
        }
    }
    return source_index;
}

std::optional<std::size_t> clip_triangle(const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, const pipeline_vertex_view_t& third, clipping_workspace_t& workspace) {
    clear(workspace.m_buffers[0]);
    append_vertex(workspace.m_buffers[0], first);
    append_vertex(workspace.m_buffers[0], second);
    append_vertex(workspace.m_buffers[0], third);
    std::size_t source_index = 0;
    for (std::size_t plane = 0; plane < 6 && !workspace.m_buffers[source_index].m_vertices.empty(); ++plane) {
        auto& source = workspace.m_buffers[source_index];
        auto& destination = workspace.m_buffers[1 - source_index];
        clear(destination);
        auto previous = view(source.m_vertices.back(), source.m_values);
        double dp = clip_distance(previous, plane);
        for (const auto& vertex : source.m_vertices) {
            const auto current = view(vertex, source.m_values);
            const double dc = clip_distance(current, plane);
            if (0.0 <= dc) {
                if (dp < 0.0 && 0.0 < dc) {
                    append_intersection(destination, previous, current, plane);
                }
                append_vertex(destination, current);
            } else if (0.0 < dp) {
                append_intersection(destination, previous, current, plane);
            }
            previous = current;
            dp = dc;
        }
        source_index = 1 - source_index;
    }
    const auto& vertices = workspace.m_buffers[source_index].m_vertices;
    if (vertices.empty() || std::ranges::any_of(vertices, [](const auto& vertex) { return vertex.m_clip_position[3] == 0.0F; })) {
        return std::nullopt;
    }
    return source_index;
}

double projectable_reciprocal_w(float w) {
    const double reciprocal = 1.0 / double(w);
    if (!(0.0F < w) || !std::isfinite(reciprocal) || double(std::numeric_limits<float>::max()) < reciprocal) {
        throw std::out_of_range(std::format("software renderer cannot project W={} into its float fragment coordinates", w));
    }
    return reciprocal;
}

std::int64_t snap(double screen, std::int64_t extent) {
    const double scaled = screen * double(subpixels);
    if (!std::isfinite(scaled) || scaled < 0.0 || double(extent) * double(subpixels) < scaled) {
        throw std::out_of_range("software renderer projected position is outside its subpixel grid");
    }
    const double lower = std::floor(scaled);
    return std::int64_t(lower) + (0.5 <= scaled - lower ? 1 : 0);
}

edge_value_t edge(grid_point_t a, grid_point_t b, grid_point_t p) {
    // Signed-int rectangle endpoints give viewport-local coordinates below 2^40
    // on the subpixel grid. Determinants fit in 81 signed bits.
    return edge_value_t(b[0] - a[0]) * (p[1] - a[1]) - edge_value_t(b[1] - a[1]) * (p[0] - a[0]);
}

edge_value_t ceil_div(edge_value_t numerator, edge_value_t denominator) {
    return numerator / denominator + (0 < numerator % denominator ? 1 : 0);
}

int compare_fraction(fraction_t a, fraction_t b) {
    // Quotient/remainder comparison keeps rational comparisons within their operand bounds.
    // Reciprocating the positive remainders reverses their order.
    int sign = 1;
    for (;;) {
        const auto qa = a[0] / a[1], qb = b[0] / b[1];
        if (qa != qb) {
            return sign * (qa < qb ? -1 : 1);
        }
        const auto ra = a[0] % a[1], rb = b[0] % b[1];
        if (ra == 0 || rb == 0) {
            return sign * (ra == rb ? 0 : (ra == 0 ? -1 : 1));
        }
        a = {a[1], ra};
        b = {b[1], rb};
        sign = -sign;
    }
}

std::int64_t sample_bound(fraction_t crossing, std::int64_t extent) {
    const auto bound = ceil_div(crossing[0] - center_offset * crossing[1], subpixels * crossing[1]);
    return std::int64_t(std::clamp<edge_value_t>(bound, 0, extent));
}

void prepare_polygon(raster_workspace_t& workspace) {
    // Collapse coincident positions only in the geometry ring. Interpolation keeps
    // each occurrence's payload, so distinct coincident values can cause discontinuities.
    workspace.m_empty = true;
    workspace.m_use_triangles = false;
    workspace.m_front_facing = false;
    workspace.m_triangles.clear();
    auto& geometry = workspace.m_geometry;
    geometry.clear();
    const auto& vertices = workspace.m_vertices;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        if (geometry.empty() || vertices[geometry.back()].m_point != vertices[i].m_point) {
            geometry.push_back(i);
        }
    }
    if (1 < geometry.size() && vertices[geometry.front()].m_point == vertices[geometry.back()].m_point) {
        geometry.pop_back();
    }
    if (geometry.size() < 3 || std::ranges::all_of(geometry, [&](std::size_t i) {
            return edge(vertices[geometry[0]].m_point, vertices[geometry[1]].m_point, vertices[i].m_point) == 0;
        })) {
        return;
    }
    workspace.m_empty = false;
    if (simple_boundary(workspace)) {
        const auto minimum = std::min_element(geometry.begin(), geometry.end(), [&](std::size_t a, std::size_t b) { return vertices[a].m_point < vertices[b].m_point; });
        const auto i = std::size_t(minimum - geometry.begin());
        const auto a = vertices[geometry[(i + geometry.size() - 1) % geometry.size()]].m_point;
        const auto b = vertices[*minimum].m_point;
        const auto c = vertices[geometry[(i + 1) % geometry.size()]].m_point;
        workspace.m_front_facing = edge(a, b, c) < 0;
        workspace.m_use_triangles = triangulate(workspace);
    } else {
        workspace.m_front_facing = crossed_facing(vertices);
    }
}

void prepare_triangle(const pipeline_vertex_view_t& first, const pipeline_vertex_view_t& second, const pipeline_vertex_view_t& third, std::int64_t width, std::int64_t height, raster_workspace_t& workspace) {
    workspace.m_empty = true;
    workspace.m_vertices.clear();
    const auto clipped = clip_triangle(first, second, third, workspace.m_clipping);
    if (!clipped) {
        return;
    }
    const auto& polygon = workspace.m_clipping.m_buffers[*clipped];
    for (const auto& vertex : polygon.m_vertices) {
        const auto source = view(vertex, polygon.m_values);
        const auto& p = source.m_clip_position;
        const double reciprocal = projectable_reciprocal_w(p[3]);
        const double ndc_x = double(p[0]) / double(p[3]);
        const double ndc_y = double(p[1]) / double(p[3]);
        const grid_point_t point {
            snap((ndc_x + 1.0) * (double(width) / 2.0), width),
            snap((1.0 - ndc_y) * (double(height) / 2.0), height)
        };
        workspace.m_vertices.push_back({point, double(p[2]) / double(p[3]), reciprocal, source});
    }
    prepare_polygon(workspace);
}

void scanline_events(std::span<const projected_vertex_t> vertices, std::int64_t y, std::vector<scan_event_t>& events) {
    events.clear();
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        std::size_t lower = i, upper = (i + 1) % vertices.size();
        if (vertices[lower].m_point[1] == vertices[upper].m_point[1]) {
            continue;
        }
        const int delta = vertices[lower].m_point[1] < vertices[upper].m_point[1] ? 1 : -1;
        if (delta < 0) {
            std::swap(lower, upper);
        }
        const auto a = vertices[lower].m_point, b = vertices[upper].m_point;
        if (a[1] <= y && y < b[1]) {
            const auto denominator = b[1] - a[1];
            const auto numerator = edge_value_t(a[0]) * (b[1] - y) + edge_value_t(b[0]) * (y - a[1]);
            events.push_back({{numerator, denominator}, lower, upper, delta});
        }
    }
    std::sort(events.begin(), events.end(), [&](const auto& a, const auto& b) {
        const int order = compare_fraction(a.m_x, b.m_x);
        return order != 0 ? order < 0 : edge_record_less(a, b, vertices);
    });
}

sample_t span_sample(const scan_event_t& left, const scan_event_t& right, std::span<const projected_vertex_t> vertices, std::int64_t x, std::int64_t y) {
    // Non-simple polygons use winding scanline spans. These weights interpolate
    // reciprocal W, Z/W and varying/W along each boundary edge and across the span.
    const auto px = std::int64_t(x) * subpixels + center_offset;
    const auto py = std::int64_t(y) * subpixels + center_offset;
    // Exact residuals survive even when two rational crossings round to the
    // same double screen coordinate. A covered sample has dl>=0 and dr>0.
    const double dl = double(px * left.m_x[1] - left.m_x[0]) / double(left.m_x[1]);
    const double dr = double(right.m_x[0] - px * right.m_x[1]) / double(right.m_x[1]);
    const double across = dl / (dl + dr);
    const double along_left = double(py - vertices[left.m_lower].m_point[1]) / double(left.m_x[1]);
    const double along_right = double(py - vertices[right.m_lower].m_point[1]) / double(right.m_x[1]);
    const std::array indices {left.m_lower, left.m_upper, right.m_lower, right.m_upper};
    const std::array weights {
        (1.0 - across) * (1.0 - along_left),
        (1.0 - across) * along_left,
        across * (1.0 - along_right),
        across * along_right
    };
    return {x, y, indices, weights, 4};
}

std::array<double, 2> interpolate_sample(std::span<const projected_vertex_t> vertices, const sample_t& sample, varying_values_t& outputs) {
    // Ear and span samples both recover perspective-correct varying values by
    // dividing interpolated varying/W by interpolated reciprocal W.
    double reciprocal = 0.0, ndc_z = 0.0;
    double minimum_q = std::numeric_limits<double>::infinity(), maximum_q = 0.0;
    double minimum_z = 1.0, maximum_z = -1.0;
    for (std::size_t i = 0; i < sample.m_count; ++i) {
        const auto& vertex = vertices[sample.m_vertices[i]];
        reciprocal += sample.m_weights[i] * vertex.m_reciprocal_w;
        ndc_z += sample.m_weights[i] * vertex.m_ndc_z;
        minimum_q = std::min(minimum_q, vertex.m_reciprocal_w);
        maximum_q = std::max(maximum_q, vertex.m_reciprocal_w);
        minimum_z = std::min(minimum_z, vertex.m_ndc_z);
        maximum_z = std::max(maximum_z, vertex.m_ndc_z);
    }
    reciprocal = std::clamp(reciprocal, minimum_q, maximum_q);
    ndc_z = std::clamp(ndc_z, minimum_z, maximum_z);
    outputs.clear();
    const auto first_outputs = vertices[sample.m_vertices[0]].m_source.m_outputs;
    for (std::size_t output = 0; output < first_outputs.size(); ++output) {
        const auto& [location, first] = first_outputs[output];
        const auto interpolated = std::visit([&](const auto& first_value) -> varying_t {
            using type_t = std::remove_cvref_t<decltype(first_value)>;
            if constexpr (std::is_same_v<type_t, noperspective_t>) {
                std::array<float, 4> components {};
                for (std::size_t axis = 0; axis < first_value.count; ++axis) {
                    double result = 0;
                    for (std::size_t i = 0; i < sample.m_count; ++i) {
                        const auto& vertex = vertices[sample.m_vertices[i]];
                        const auto& payload = std::get<noperspective_t>(vertex.m_source.m_outputs[output].second);
                        result += sample.m_weights[i] * (payload.numerators[axis] / double(vertex.m_source.m_clip_position[3]));
                    }
                    components[axis] = float(result);
                }
                switch (first_value.count) {
                    case 1: return components[0];
                    case 2: return vector2f_t({components[0], components[1]});
                    case 3: return shader::vector_t<float, 3>({components[0], components[1], components[2]});
                    case 4: return vector4f_t({components[0], components[1], components[2], components[3]});
                    default: throw std::logic_error("invalid noperspective component count");
                }
            } else {
                const auto component = [&](std::size_t axis) {
                    double numerator = 0.0;
                    double minimum = std::numeric_limits<double>::infinity(), maximum = -minimum;
                    for (std::size_t i = 0; i < sample.m_count; ++i) {
                        const auto& vertex = vertices[sample.m_vertices[i]];
                        if (vertex.m_source.m_outputs.size() != first_outputs.size() || vertex.m_source.m_outputs[output].first != location) {
                            throw std::logic_error("rasterization has inconsistent varying counts or locations");
                        }
                        const auto& typed = std::get<type_t>(vertex.m_source.m_outputs[output].second);
                        const double value = [&] {
                            if constexpr (std::is_same_v<type_t, float>) {
                                return double(typed);
                            } else {
                                return double(typed[axis]);
                            }
                        }();
                        numerator += sample.m_weights[i] * (vertex.m_reciprocal_w * value);
                        minimum = std::min(minimum, value);
                        maximum = std::max(maximum, value);
                    }
                    const double result = numerator / reciprocal;
                    return float(minimum <= maximum ? std::clamp(result, minimum, maximum) : result);
                };
                if constexpr (std::is_same_v<type_t, float>) {
                    return component(0);
                } else {
                    type_t result;
                    std::size_t i = 0;
                    for (float& value : result) {
                        value = component(i++);
                    }
                    return result;
                }
            }
        },
            first);
        outputs.emplace_back(location, interpolated);
    }
    return {ndc_z * 0.5 + 0.5, reciprocal};
}

bool finite(const vector4f_t& vector) {
    return std::ranges::all_of(vector, [](float component) { return std::isfinite(component); });
}

std::size_t shader_component_count(shader::shader_data_type_t type) {
    switch (type.category()) {
        case shader::shader_data_category_t::scalar: {
            return 1;
        }
        case shader::shader_data_category_t::vector: {
            return type.rows();
        }
        default: {
            return 0;
        }
    }
}

vertex_attribute_type_t expected_attribute_type(shader::shader_scalar_type_t type) {
    switch (type) {
        case shader::shader_scalar_type_t::floating_point: {
            return vertex_attribute_type_t::R32;
        }
        case shader::shader_scalar_type_t::signed_integer: {
            return vertex_attribute_type_t::I32;
        }
        case shader::shader_scalar_type_t::unsigned_integer: {
            return vertex_attribute_type_t::U32;
        }
        default: {
            throw std::invalid_argument("software renderer does not support this shader vertex input scalar type");
        }
    }
}

void validate_vertex_attribute(
    const vertex_attribute_t& attribute,
    shader::shader_data_type_t input_type
) {
    const std::size_t component_count = shader_component_count(input_type);
    if (component_count == 0 || 4 < component_count) {
        throw std::invalid_argument("software renderer only supports scalar and vector vertex inputs");
    }
    if (attribute.type() != expected_attribute_type(input_type.scalar()) || attribute.component_count() != component_count) {
        throw std::invalid_argument("mesh vertex attribute is incompatible with the reflected shader input");
    }
}

void set_vertex_input(
    software_shader::vertex_io_t& io,
    const shader::shader_interface_element_t& input,
    const type_erased_array::type_erased_array_t& stream,
    const vertex_attribute_t& attribute,
    std::uint32_t vertex_index
) {
    switch (attribute.type()) {
        case vertex_attribute_type_t::R32: {
            set_vertex_input_components<float>(io, input.index, stream, vertex_index, attribute.component_count());
        } break;
        case vertex_attribute_type_t::I32: {
            set_vertex_input_components<std::int32_t>(io, input.index, stream, vertex_index, attribute.component_count());
        } break;
        case vertex_attribute_type_t::U32: {
            set_vertex_input_components<std::uint32_t>(io, input.index, stream, vertex_index, attribute.component_count());
        } break;
        default: {
            throw std::logic_error("unsupported validated vertex attribute type");
        }
    }
}

bool supported_fragment_input(const shader::shader_interface_element_t& input) {
    const auto type = input.type;
    const bool integer = type.scalar() == shader::shader_scalar_type_t::signed_integer || type.scalar() == shader::shader_scalar_type_t::unsigned_integer;
    if (type.scalar() != shader::shader_scalar_type_t::floating_point && !(integer && input.interpolation == shader::interpolation_t::flat)) {
        return false;
    }
    if (type.category() == shader::shader_data_category_t::scalar) {
        return true;
    }
    return type.category() == shader::shader_data_category_t::vector && 2 <= type.rows() && type.rows() <= 4 && type.columns() == 1;
}

varying_t vertex_output(
    const software_shader::vertex_io_t& io,
    const shader::shader_interface_element_t& input
) {
    if (input.type == shader::shader_data_type<float>()) {
        return require_vertex_output<float>(io, input.index);
    }
    if (input.type == shader::shader_data_type<vector2f_t>()) {
        return require_vertex_output<vector2f_t>(io, input.index);
    }
    if (input.type == shader::shader_data_type<m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>>()) {
        return require_vertex_output<m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3>>(io, input.index);
    }
    if (input.type == shader::shader_data_type<vector4f_t>()) {
        return require_vertex_output<vector4f_t>(io, input.index);
    }
    throw std::logic_error("unsupported validated fragment input type");
}

flat_t flat_output(const software_shader::vertex_io_t& io, const shader::shader_interface_element_t& input) {
    if (input.type == shader::shader_data_type<float>()) { return require_vertex_output<float>(io, input.index); }
    if (input.type == shader::shader_data_type<shader::vector_t<float, 2>>()) { return require_vertex_output<shader::vector_t<float, 2>>(io, input.index); }
    if (input.type == shader::shader_data_type<shader::vector_t<float, 3>>()) { return require_vertex_output<shader::vector_t<float, 3>>(io, input.index); }
    if (input.type == shader::shader_data_type<shader::vector_t<float, 4>>()) { return require_vertex_output<shader::vector_t<float, 4>>(io, input.index); }
    if (input.type == shader::shader_data_type<std::int32_t>()) { return require_vertex_output<std::int32_t>(io, input.index); }
    if (input.type == shader::shader_data_type<shader::vector_t<std::int32_t, 2>>()) { return require_vertex_output<shader::vector_t<std::int32_t, 2>>(io, input.index); }
    if (input.type == shader::shader_data_type<shader::vector_t<std::int32_t, 3>>()) { return require_vertex_output<shader::vector_t<std::int32_t, 3>>(io, input.index); }
    if (input.type == shader::shader_data_type<shader::vector_t<std::int32_t, 4>>()) { return require_vertex_output<shader::vector_t<std::int32_t, 4>>(io, input.index); }
    if (input.type == shader::shader_data_type<std::uint32_t>()) { return require_vertex_output<std::uint32_t>(io, input.index); }
    if (input.type == shader::shader_data_type<shader::vector_t<std::uint32_t, 2>>()) { return require_vertex_output<shader::vector_t<std::uint32_t, 2>>(io, input.index); }
    if (input.type == shader::shader_data_type<shader::vector_t<std::uint32_t, 3>>()) { return require_vertex_output<shader::vector_t<std::uint32_t, 3>>(io, input.index); }
    if (input.type == shader::shader_data_type<shader::vector_t<std::uint32_t, 4>>()) { return require_vertex_output<shader::vector_t<std::uint32_t, 4>>(io, input.index); }
    throw std::logic_error("unsupported validated flat input type");
}

std::optional<screen_vertex_t> project(
    const pipeline_vertex_view_t& vertex,
    std::int64_t width,
    std::int64_t height
) {
    const float w = vertex.m_clip_position[3];
    if (w == 0.0F) {
        return std::nullopt;
    }
    const double reciprocal_w = projectable_reciprocal_w(w);
    const double ndc_x = double(vertex.m_clip_position[0]) / double(w);
    const double ndc_y = double(vertex.m_clip_position[1]) / double(w);
    const double ndc_z = double(vertex.m_clip_position[2]) / double(w);
    const double x = (ndc_x + 1.0) * (double(width) / 2.0);
    const double y = (1.0 - ndc_y) * (double(height) / 2.0);
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(ndc_z) || !std::isfinite(reciprocal_w)) {
        return std::nullopt;
    }
    return screen_vertex_t {
        .m_x = x,
        .m_y = y,
        .m_ndc_z = ndc_z,
        .m_reciprocal_w = reciprocal_w,
        .m_outputs = vertex.m_outputs
    };
}

void set_fragment_inputs(
    software_shader::fragment_io_t& io,
    std::span<const varying_entry_t> inputs
) {
    for (const auto& [location, input] : inputs) {
        std::visit([&](const auto& typed_input) {
            if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(typed_input)>, noperspective_t>) {
                io.input(location, typed_input);
            } else {
                throw std::logic_error("unresolved noperspective fragment input");
            }
        }, input);
    }
}

float sanitize_unorm(float component) {
    return std::isnan(component) ? 0.0F : std::clamp(component, 0.0F, 1.0F);
}

std::uint8_t to_unorm8(float component) {
    if (std::isnan(component) || component == -std::numeric_limits<float>::infinity()) {
        return 0;
    }
    if (component == std::numeric_limits<float>::infinity()) {
        return 255;
    }
    const float clamped = std::clamp(component, 0.0F, 1.0F);
    return static_cast<std::uint8_t>(std::floor(clamped * 255.0F + 0.5F));
}

rgba8_t to_rgba8(const vector4f_t& color) {
    return {
        .red = to_unorm8(color[0]),
        .green = to_unorm8(color[1]),
        .blue = to_unorm8(color[2]),
        .alpha = to_unorm8(color[3])
    };
}

float decode_srgb(float component) {
    return component <= 0.04045F ? component / 12.92F : std::pow((component + 0.055F) / 1.055F, 2.4F);
}

float encode_srgb(float component) {
    return component <= 0.0031308F ? component * 12.92F : 1.055F * std::pow(component, 1.0F / 2.4F) - 0.055F;
}

float blend_factor_component(blend_factor_t factor, const vector4f_t& source, const vector4f_t& destination, const vector4f_t& constant, std::size_t component) {
    switch (factor) {
        case blend_factor_t::zero: return 0;
        case blend_factor_t::one: return 1;
        case blend_factor_t::src_color: return source[component];
        case blend_factor_t::one_minus_src_color: return 1 - source[component];
        case blend_factor_t::dst_color: return destination[component];
        case blend_factor_t::one_minus_dst_color: return 1 - destination[component];
        case blend_factor_t::src_alpha: return source[3];
        case blend_factor_t::one_minus_src_alpha: return 1 - source[3];
        case blend_factor_t::dst_alpha: return destination[3];
        case blend_factor_t::one_minus_dst_alpha: return 1 - destination[3];
        case blend_factor_t::constant_color: return constant[component];
        case blend_factor_t::one_minus_constant_color: return 1 - constant[component];
        case blend_factor_t::constant_alpha: return constant[3];
        case blend_factor_t::one_minus_constant_alpha: return 1 - constant[3];
        case blend_factor_t::src_alpha_saturate: return component == 3 ? 1.0F : std::min(source[3], 1 - destination[3]);
    }
    throw std::invalid_argument(std::format("blend_factor_component rejects invalid factor {}", factor));
}

float blend_component(const blend_equation_t& blend_equation, const vector4f_t& source, const vector4f_t& destination, const vector4f_t& constant, std::size_t component) {
    // Min/max ignore factors. Material setters validate even ignored fields.
    if (blend_equation.operation == blend_op_t::min) {
        return std::min(source[component], destination[component]);
    }
    if (blend_equation.operation == blend_op_t::max) {
        return std::max(source[component], destination[component]);
    }
    const float source_term = source[component] * blend_factor_component(blend_equation.source, source, destination, constant, component);
    const float destination_term = destination[component] * blend_factor_component(blend_equation.destination, source, destination, constant, component);
    switch (blend_equation.operation) {
        case blend_op_t::add: return source_term + destination_term;
        case blend_op_t::subtract: return source_term - destination_term;
        case blend_op_t::reverse_subtract: return destination_term - source_term;
        default: {
            throw std::invalid_argument(std::format("blend_component rejects invalid operation {}", blend_equation.operation));
        }
    }
}

void write_color(const material_t& material, color_encoding_t encoding, const vector4f_t& source, std::byte* pixel) {
    const auto color_mask = material.color_write();
    if (color_mask == color_mask_t::none) {
        return;
    }
    const auto replaces = [](const blend_equation_t& equation) {
        return equation.source == blend_factor_t::one && equation.destination == blend_factor_t::zero && equation.operation == blend_op_t::add;
    };
    const bool replacement = !material.blend() || (replaces(material.blend_color()) && replaces(material.blend_alpha()));
    if (replacement && encoding == color_encoding_t::linear && color_mask == color_mask_t::all) {
        const auto stored = to_rgba8(source);
        pixel[0] = std::byte(stored.red); pixel[1] = std::byte(stored.green);
        pixel[2] = std::byte(stored.blue); pixel[3] = std::byte(stored.alpha);
        return;
    }
    const vector4f_t sanitized_source {
        sanitize_unorm(source[0]), sanitize_unorm(source[1]),
        sanitize_unorm(source[2]), sanitize_unorm(source[3])
    };
    vector4f_t result = sanitized_source;
    if (!replacement) {
        vector4f_t destination {std::to_integer<unsigned>(pixel[0]) / 255.0F, std::to_integer<unsigned>(pixel[1]) / 255.0F, std::to_integer<unsigned>(pixel[2]) / 255.0F, std::to_integer<unsigned>(pixel[3]) / 255.0F};
        if (encoding == color_encoding_t::srgb) {
            for (std::size_t component = 0; component < 3; ++component) {
                destination[component] = decode_srgb(destination[component]);
            }
        }
        const auto color_equation = material.blend_color();
        const auto alpha_equation = material.blend_alpha();
        const auto& constant = material.blend_constant();
        // Both equations read the original inputs, before any channel is stored.
        for (std::size_t component = 0; component < 4; ++component) {
            result[component] = sanitize_unorm(blend_component(component == 3 ? alpha_equation : color_equation, sanitized_source, destination, constant, component));
        }
    }
    if (encoding == color_encoding_t::srgb) {
        for (std::size_t component = 0; component < 3; ++component) {
            result[component] = encode_srgb(result[component]);
        }
    }
    const auto stored = to_rgba8(result);
    if (color_mask == color_mask_t::all) {
        pixel[0] = std::byte(stored.red); pixel[1] = std::byte(stored.green);
        pixel[2] = std::byte(stored.blue); pixel[3] = std::byte(stored.alpha);
    } else {
        // Masking affects storage only; disabled bytes never round-trip through float.
        if ((color_mask & color_mask_t::red) != color_mask_t::none) { pixel[0] = std::byte(stored.red); }
        if ((color_mask & color_mask_t::green) != color_mask_t::none) { pixel[1] = std::byte(stored.green); }
        if ((color_mask & color_mask_t::blue) != color_mask_t::none) { pixel[2] = std::byte(stored.blue); }
        if ((color_mask & color_mask_t::alpha) != color_mask_t::none) { pixel[3] = std::byte(stored.alpha); }
    }
}

float depth_clear_value(float depth) {
    if (std::isnan(depth)) {
        throw std::invalid_argument("software_renderer_t::clear_depth rejects NaN");
    }
    return std::clamp(depth, 0.0F, 1.0F);
}

bool depth_passes(comparison_t comparison, float incoming, float stored) {
    switch (comparison) {
        case comparison_t::never: return false;
        case comparison_t::less: return incoming < stored;
        case comparison_t::equal: return incoming == stored;
        case comparison_t::less_equal: return incoming <= stored;
        case comparison_t::greater: return stored < incoming;
        case comparison_t::not_equal: return incoming != stored;
        case comparison_t::greater_equal: return stored <= incoming;
        case comparison_t::always: return true;
    }
    throw std::invalid_argument(std::format("software_renderer_t::draw has invalid depth comparison {}", comparison));
}

bool stencil_passes(const stencil_state_t& stencil_state, std::uint8_t stored) {
    const auto reference = stencil_state.reference & stencil_state.compare_mask;
    const auto masked = stored & stencil_state.compare_mask;
    switch (stencil_state.comparison) {
        case comparison_t::never: return false;
        case comparison_t::less: return reference < masked;
        case comparison_t::equal: return reference == masked;
        case comparison_t::less_equal: return reference <= masked;
        case comparison_t::greater: return masked < reference;
        case comparison_t::not_equal: return reference != masked;
        case comparison_t::greater_equal: return masked <= reference;
        case comparison_t::always: return true;
    }
    throw std::invalid_argument(std::format("stencil_passes rejects invalid comparison {}", stencil_state.comparison));
}

void write_stencil(const stencil_state_t& stencil_state, stencil_op_t operation, std::uint8_t& stored, m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t& metric) {
    if (operation == stencil_op_t::keep || stencil_state.write_mask == 0) {
        return;
    }
    unsigned candidate = stored;
    switch (operation) {
        case stencil_op_t::keep: { } break;
        case stencil_op_t::zero: { candidate = 0; } break;
        case stencil_op_t::replace: { candidate = stencil_state.reference; } break;
        case stencil_op_t::increment_clamp: { candidate = std::min(candidate + 1, 255U); } break;
        case stencil_op_t::decrement_clamp: { candidate = candidate == 0 ? 0 : candidate - 1; } break;
        case stencil_op_t::increment_wrap: { candidate = (candidate + 1) & 255U; } break;
        case stencil_op_t::decrement_wrap: { candidate = (candidate + 255) & 255U; } break;
        case stencil_op_t::invert: { candidate ^= 255U; } break;
        default: { throw std::invalid_argument(std::format("write_stencil rejects invalid operation {}", operation)); }
    }
    stored = static_cast<std::uint8_t>((stored & ~stencil_state.write_mask) | (candidate & stencil_state.write_mask));
    metric.update<raster_metrics_t>([](raster_metrics_t& metric) noexcept { ++metric.m_stencil_writes; });
}

bool storage_overlaps(std::span<const std::byte> left, std::span<const std::byte> right) {
    if (left.empty() || right.empty()) {
        return false;
    }
    const std::less<const std::byte*> before;
    return before(left.data(), right.data() + right.size()) && before(right.data(), left.data() + left.size());
}

void validate_feedback(const material_t& material, const framebuffer_t& framebuffer) {
    const auto& program = *material.program();
    for (const auto* interface : {&program.vertex_interface(), &program.fragment_interface()}) {
        for (const auto& binding : interface->bindings()) {
            if (binding.type.category() != software_shader::shader::shader_data_category_t::texture_2d) {
                continue;
            }
            const auto& texture = material.bindings().texture(binding.index);
            for (std::size_t level = 0; level < texture.level_count(); ++level) {
                const auto sampled = texture.view(level).bytes();
                if (storage_overlaps(sampled, framebuffer.pixels().bytes()) ||
                    storage_overlaps(sampled, std::as_bytes(framebuffer.depth())) ||
                    storage_overlaps(sampled, std::as_bytes(framebuffer.stencil()))) {
                    throw std::invalid_argument(std::format("software_renderer_t::draw texture binding {} overlaps writable attachment storage", binding.index));
                }
            }
        }
    }
}

void shade_sample(
    const material_t& material,
    const raster_bounds_t& bounds,
    const framebuffer_t& framebuffer,
    std::int64_t x,
    std::int64_t y,
    float depth,
    float reciprocal_w,
    bool front_facing,
    std::span<const varying_entry_t> inputs,
    software_shader::fragment_io_t& io,
    m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t& metric,
    std::span<const flat_entry_t> flat_inputs
) {
    if (x < bounds.m_first_x || y < bounds.m_first_y || bounds.m_end_x <= x || bounds.m_end_y <= y) {
        return;
    }

    x += bounds.m_x;
    y += bounds.m_y;
    depth = std::clamp(depth, 0.0F, 1.0F);
    io.reset(
        vector4f_t({static_cast<float>(x) + 0.5F,
            static_cast<float>(y) + 0.5F,
            depth,
            reciprocal_w}),
        front_facing
    );
    set_fragment_inputs(io, inputs);
    for (const auto& [location, input] : flat_inputs) {
        std::visit([&](const auto& typed) { io.input(location, typed); }, input);
    }
    metric.update<raster_metrics_t>([](raster_metrics_t& metric) noexcept {
        ++metric.m_invocations;
    });
    material.program()->run(material.bindings(), io);
    if (io.discarded()) {
        metric.update<raster_metrics_t>([](raster_metrics_t& metric) noexcept {
            ++metric.m_discards;
        });
        return;
    }
    const auto index = static_cast<std::size_t>(y) * static_cast<std::size_t>(bounds.m_width) + static_cast<std::size_t>(x);
    const bool stencil_test = material.stencil_test();
    const auto stencil_state = stencil_test ? (front_facing ? material.stencil_front() : material.stencil_back()) : stencil_state_t{};
    if (stencil_test && !stencil_passes(stencil_state, framebuffer.stencil()[index])) {
        write_stencil(stencil_state, stencil_state.fail, framebuffer.stencil()[index], metric);
        metric.update<raster_metrics_t>([](raster_metrics_t& metric) noexcept { ++metric.m_stencil_rejections; });
        return;
    }
    if (material.depth_test()) {
        const auto comparison = material.depth_compare();
        const bool passes = comparison == comparison_t::always || (comparison != comparison_t::never && depth_passes(comparison, depth, framebuffer.depth()[index]));
        if (!passes) {
            if (stencil_test) {
                write_stencil(stencil_state, stencil_state.depth_fail, framebuffer.stencil()[index], metric);
            }
            metric.update<raster_metrics_t>([](raster_metrics_t& metric) noexcept {
                ++metric.m_depth_rejections;
            });
            return;
        }
    }
    if (stencil_test) {
        write_stencil(stencil_state, stencil_state.pass, framebuffer.stencil()[index], metric);
    }
    if (material.depth_test() && material.depth_write()) {
        framebuffer.depth()[index] = depth;
        metric.update<raster_metrics_t>([](raster_metrics_t& metric) noexcept { ++metric.m_depth_writes; });
    }
    if (const auto color = io.color(); color && material.color_write() != color_mask_t::none) {
        write_color(material, framebuffer.encoding(), *color, framebuffer.pixels().bytes().data() + index * 4);
        metric.update<raster_metrics_t>([](raster_metrics_t& metric) noexcept {
            ++metric.m_color_writes;
        });
    }
}

void rasterize_point(
    const material_t& material,
    const raster_bounds_t& bounds,
    const framebuffer_t& framebuffer,
    const pipeline_vertex_view_t& vertex,
    varying_values_t& fragment_inputs,
    software_shader::fragment_io_t& fragment_io,
    m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t& metric
) {
    if (!inside_clip_volume(vertex)) {
        return;
    }
    const auto screen = project(vertex, bounds.m_view_width, bounds.m_view_height);
    if (!screen) {
        return;
    }

    const std::array<projected_vertex_t, 1> projected {{{{0, 0}, screen->m_ndc_z, screen->m_reciprocal_w, vertex}}};
    (void)interpolate_sample(projected, {0, 0, {0, 0, 0, 0}, {1, 0, 0, 0}, 1}, fragment_inputs);
    constexpr int radius = 3;
    constexpr int radius_squared = radius * radius;
    const auto center_x = static_cast<std::int64_t>(std::floor(screen->m_x));
    const auto center_y = static_cast<std::int64_t>(std::floor(screen->m_y));
    const float depth = screen->m_ndc_z * 0.5F + 0.5F;
    for (auto y = center_y - radius; y <= center_y + radius; ++y) {
        for (auto x = center_x - radius; x <= center_x + radius; ++x) {
            const auto dx = x - center_x;
            const auto dy = y - center_y;
            if (dx * dx + dy * dy <= radius_squared) {
                shade_sample(
                    material,
                    bounds,
                    framebuffer,
                    x,
                    y,
                    depth,
                    screen->m_reciprocal_w,
                    true,
                    fragment_inputs,
                    fragment_io,
                    metric,
                    vertex.m_flat_outputs
                );
            }
        }
    }
}

void rasterize_line(
    const material_t& material,
    const raster_bounds_t& bounds,
    const framebuffer_t& framebuffer,
    const pipeline_vertex_view_t& first,
    const pipeline_vertex_view_t& second,
    clipping_workspace_t& clipping,
    varying_values_t& fragment_inputs,
    software_shader::fragment_io_t& fragment_io,
    m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t& metric
) {
    const auto flat_inputs = material.provoking_vertex() == provoking_vertex_t::first ? first.m_flat_outputs : second.m_flat_outputs;
    const auto clipped_index = clip_line(first, second, clipping);
    if (!clipped_index) {
        return;
    }
    const auto& clipped = clipping.m_buffers[*clipped_index];
    const auto clipped_first = view(clipped.m_vertices[0], clipped.m_values);
    const auto clipped_second = view(clipped.m_vertices[1], clipped.m_values);
    const auto first_screen = project(clipped_first, bounds.m_view_width, bounds.m_view_height);
    const auto second_screen = project(clipped_second, bounds.m_view_width, bounds.m_view_height);
    if (!first_screen || !second_screen) {
        return;
    }

    // Direct evaluation of inclusive Bresenham samples lets us skip invisible
    // major-axis steps without changing endpoint rounding or tie ownership.
    const auto start_x = std::int64_t(std::floor(first_screen->m_x));
    const auto start_y = std::int64_t(std::floor(first_screen->m_y));
    const auto target_x = std::int64_t(std::floor(second_screen->m_x));
    const auto target_y = std::int64_t(std::floor(second_screen->m_y));
    const auto dx = std::abs(target_x - start_x), dy = std::abs(target_y - start_y);
    const std::int64_t step_x = start_x < target_x ? 1 : -1, step_y = start_y < target_y ? 1 : -1;
    const bool horizontal = dy <= dx;
    const auto major = horizontal ? dx : dy, minor = horizontal ? dy : dx;
    const auto start = horizontal ? start_x : start_y, step = horizontal ? step_x : step_y;
    const auto first_pixel = horizontal ? bounds.m_first_x : bounds.m_first_y;
    const auto end = horizontal ? bounds.m_end_x : bounds.m_end_y;
    const auto first_step = std::max<std::int64_t>(0, step == 1 ? first_pixel - start : start - end + 1);
    const auto last_step = std::min(major, step == 1 ? end - 1 - start : start - first_pixel);
    const std::array<projected_vertex_t, 2> endpoints {{
        {{0, 0}, first_screen->m_ndc_z, first_screen->m_reciprocal_w, clipped_first},
        {{0, 0}, second_screen->m_ndc_z, second_screen->m_reciprocal_w, clipped_second}
    }};
    const double line_x = second_screen->m_x - first_screen->m_x;
    const double line_y = second_screen->m_y - first_screen->m_y;
    const double length_squared = line_x * line_x + line_y * line_y;
    for (auto i = first_step; i <= last_step; ++i) {
        const auto offset = major == 0 ? 0 : std::int64_t((edge_value_t(i) * minor + major / 2) / major);
        const auto x = start_x + step_x * (horizontal ? i : offset);
        const auto y = start_y + step_y * (horizontal ? offset : i);
        double factor = 0.0;
        if (length_squared != 0.0) {
            factor = ((double(x) + 0.5 - first_screen->m_x) * line_x + (double(y) + 0.5 - first_screen->m_y) * line_y) / length_squared;
            factor = std::clamp(factor, 0.0, 1.0);
        }
        const sample_t sample {x, y, {0, 1, 0, 0}, {1.0 - factor, factor, 0.0, 0.0}, 2};
        const auto depth_w = interpolate_sample(endpoints, sample, fragment_inputs);
        shade_sample(material, bounds, framebuffer, x, y, float(depth_w[0]), float(depth_w[1]), true, fragment_inputs, fragment_io, metric, flat_inputs);
    }
}

void rasterize_triangle(
    const material_t& material,
    const raster_bounds_t& bounds,
    const framebuffer_t& framebuffer,
    const pipeline_vertex_view_t& first,
    const pipeline_vertex_view_t& second,
    const pipeline_vertex_view_t& third,
    raster_workspace_t& workspace,
    varying_values_t& fragment_inputs,
    software_shader::fragment_io_t& fragment_io,
    m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t& metric,
    std::span<const flat_entry_t> flat_inputs
) {
    prepare_triangle(first, second, third, bounds.m_view_width, bounds.m_view_height, workspace);
    if (workspace.m_empty) {
        return;
    }
    // Geometric winding also drives triangulation. Derive the material's effective
    // facing separately so a front-face selection cannot change sample coverage.
    const bool front_facing = workspace.m_front_facing == (material.front_face() == winding_t::counter_clockwise);
    const auto cull = material.cull();
    if (cull == cull_mode_t::both || (cull == cull_mode_t::front && front_facing) || (cull == cull_mode_t::back && !front_facing)) {
        return;
    }
    visit_samples(workspace, bounds.m_end_x, bounds.m_end_y, [&](const sample_t& sample) {
        const auto depth_w = interpolate_sample(workspace.m_vertices, sample, fragment_inputs);
        shade_sample(
            material,
            bounds,
            framebuffer,
            sample.m_x,
            sample.m_y,
            float(depth_w[0]),
            float(depth_w[1]),
            front_facing,
            fragment_inputs,
            fragment_io,
            metric,
            flat_inputs
        );
    }, bounds.m_first_x, bounds.m_first_y);
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
