#include "software_renderer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

software_renderer_t::software_renderer_t(framebuffer_t framebuffer):
    m_framebuffer(framebuffer)
{
}

framebuffer_t& software_renderer_t::framebuffer() noexcept {
    return m_framebuffer;
}

const framebuffer_t& software_renderer_t::framebuffer() const noexcept {
    return m_framebuffer;
}

void software_renderer_t::clear_color(rgba8_t color, profiling::metric_t& parent_metric) {
    auto metric = parent_metric.metric<clear_color_metrics_t>();
    const auto bytes = m_framebuffer.pixels().bytes();
    for (std::size_t offset = 0; offset < bytes.size(); offset += 4) {
        bytes[offset] = std::byte(color.red); bytes[offset + 1] = std::byte(color.green);
        bytes[offset + 2] = std::byte(color.blue); bytes[offset + 3] = std::byte(color.alpha);
    }
    metric.update<clear_color_metrics_t>([this](clear_color_metrics_t& metric) {
        metric.m_color_writes += m_framebuffer.pixels().bytes().size() / 4;
    });
}

void software_renderer_t::clear_color(const camera_t& camera, rgba8_t color, profiling::metric_t& parent_metric) {
    auto metric = parent_metric.metric<clear_color_metrics_t>();
    const raster_bounds_t bounds(m_framebuffer.width(), m_framebuffer.height(), camera.view_rect());
    if (bounds.empty()) {
        return;
    }
    const auto pixels = m_framebuffer.pixels().bytes();
    for (auto y = bounds.m_first_y; y < bounds.m_end_y; ++y) {
        const auto offset = std::size_t(y + bounds.m_y) * std::size_t(bounds.m_width) + std::size_t(bounds.m_first_x + bounds.m_x);
        for (std::size_t index = offset; index < offset + std::size_t(bounds.m_end_x - bounds.m_first_x); ++index) {
            pixels[index * 4] = std::byte(color.red); pixels[index * 4 + 1] = std::byte(color.green);
            pixels[index * 4 + 2] = std::byte(color.blue); pixels[index * 4 + 3] = std::byte(color.alpha);
        }
        metric.update<clear_color_metrics_t>([&bounds](clear_color_metrics_t& metric) noexcept {
            metric.m_color_writes += std::size_t(bounds.m_end_x - bounds.m_first_x);
        });
    }
}

void software_renderer_t::clear_depth(float depth, profiling::metric_t& parent_metric) {
    auto metric = parent_metric.metric<clear_depth_metrics_t>();
    if (m_framebuffer.pixels().bytes().empty()) {
        return;
    }
    if (m_framebuffer.depth().empty()) {
        throw std::invalid_argument("software_renderer_t::clear_depth requires a depth attachment");
    }
    std::ranges::fill(m_framebuffer.depth(), depth_clear_value(depth));
    metric.update<clear_depth_metrics_t>([this](clear_depth_metrics_t& metric) {
        metric.m_depth_writes += m_framebuffer.depth().size();
    });
}

void software_renderer_t::clear_depth(const camera_t& camera, float depth, profiling::metric_t& parent_metric) {
    auto metric = parent_metric.metric<clear_depth_metrics_t>();
    const raster_bounds_t bounds(m_framebuffer.width(), m_framebuffer.height(), camera.view_rect());
    if (bounds.empty()) {
        return;
    }
    const auto samples = m_framebuffer.depth();
    if (samples.empty()) {
        throw std::invalid_argument("software_renderer_t::clear_depth requires a depth attachment");
    }
    depth = depth_clear_value(depth);
    for (auto y = bounds.m_first_y; y < bounds.m_end_y; ++y) {
        const auto offset = std::size_t(y + bounds.m_y) * std::size_t(bounds.m_width) + std::size_t(bounds.m_first_x + bounds.m_x);
        std::ranges::fill(samples.subspan(offset, std::size_t(bounds.m_end_x - bounds.m_first_x)), depth);
        metric.update<clear_depth_metrics_t>([&bounds](clear_depth_metrics_t& metric) noexcept {
            metric.m_depth_writes += std::size_t(bounds.m_end_x - bounds.m_first_x);
        });
    }
}

void software_renderer_t::clear_stencil(std::uint8_t stencil, profiling::metric_t& parent_metric) {
    auto metric = parent_metric.metric<clear_stencil_metrics_t>();
    if (m_framebuffer.pixels().bytes().empty()) {
        return;
    }
    if (m_framebuffer.stencil().empty()) {
        throw std::invalid_argument("software_renderer_t::clear_stencil requires a stencil attachment");
    }
    std::ranges::fill(m_framebuffer.stencil(), stencil);
    metric.update<clear_stencil_metrics_t>([this](clear_stencil_metrics_t& metric) {
        metric.m_stencil_writes += m_framebuffer.stencil().size();
    });
}

void software_renderer_t::clear_stencil(const camera_t& camera, std::uint8_t stencil, profiling::metric_t& parent_metric) {
    auto metric = parent_metric.metric<clear_stencil_metrics_t>();
    const raster_bounds_t bounds(m_framebuffer.width(), m_framebuffer.height(), camera.view_rect());
    if (bounds.empty()) {
        return;
    }
    const auto samples = m_framebuffer.stencil();
    if (samples.empty()) {
        throw std::invalid_argument("software_renderer_t::clear_stencil requires a stencil attachment");
    }
    for (auto y = bounds.m_first_y; y < bounds.m_end_y; ++y) {
        const auto offset = std::size_t(y + bounds.m_y) * std::size_t(bounds.m_width) + std::size_t(bounds.m_first_x + bounds.m_x);
        std::ranges::fill(samples.subspan(offset, std::size_t(bounds.m_end_x - bounds.m_first_x)), stencil);
        metric.update<clear_stencil_metrics_t>([&bounds](clear_stencil_metrics_t& metric) noexcept {
            metric.m_stencil_writes += std::size_t(bounds.m_end_x - bounds.m_first_x);
        });
    }
}

void software_renderer_t::draw(
    const camera_t& camera,
    const render_item_t& render_item,
    profiling::metric_t& parent_metric
) {
    auto draw_metric = parent_metric.metric<draw_metrics_t>();
    auto preparation_metric = draw_metric.metric<preparation_metrics_t>();
    const raster_bounds_t bounds(m_framebuffer.width(), m_framebuffer.height(), camera.view_rect());
    if (bounds.empty()) {
        return;
    }

    if (maximum_extent < m_framebuffer.width() || maximum_extent < m_framebuffer.height()) {
        throw std::out_of_range(std::format("software_renderer_t::draw dimensions {}x{} exceed the supported limit {}", m_framebuffer.width(), m_framebuffer.height(), maximum_extent));
    }

    const auto geometry = render_item.geometry();
    if (!geometry) {
        throw std::invalid_argument("software_renderer_t::draw requires geometry");
    }
    geometry->finalize();
    const auto material = render_item.material();
    if (!material) {
        throw std::invalid_argument("software_renderer_t::draw requires a material");
    }
    if (material->depth_test() && m_framebuffer.depth().empty()) {
        throw std::invalid_argument("software_renderer_t::draw requires a depth attachment when depth testing is enabled");
    }
    if (material->stencil_test() && m_framebuffer.stencil().empty()) {
        throw std::invalid_argument("software_renderer_t::draw requires a stencil attachment when stencil testing is enabled");
    }
    const auto& program = *material->program();
    const auto& bindings = material->bindings();
    program.validate_bindings(bindings);
    validate_feedback(*material, m_framebuffer);

    const auto object_to_world = render_item.object_to_world();
    const auto world_to_clip = camera.world_to_clip();
    const auto& framebuffer = m_framebuffer;
    const auto mesh = geometry->mesh();
    const auto& streams = mesh->vertex_streams();
    const auto attributes = mesh->vertex_attributes();
    for (const auto& input : program.vertex_interface().inputs()) {
        if (streams.size() <= input.index) {
            throw std::invalid_argument("shader vertex input location has no corresponding mesh stream");
        }
        validate_vertex_attribute(attributes[input.index], input.type);
    }
    auto& scratch = m_scratch;
    scratch.m_interpolated_inputs.clear();
    scratch.m_flat_inputs.clear();
    for (const auto& input : program.fragment_interface().inputs()) {
        if (!supported_fragment_input(input)) {
            throw std::invalid_argument("software renderer requires float scalar/vector interpolation or flat float/int32/uint32 inputs");
        }
        (input.interpolation == shader::interpolation_t::flat ? scratch.m_flat_inputs : scratch.m_interpolated_inputs).push_back(input);
    }

    preparation_metric.stop();
    const auto indices = geometry->indices();
    auto vertex_metric = draw_metric.metric<vertex_metrics_t>();
    vertex_metric.update<vertex_metrics_t>([expected = indices.size()](vertex_metrics_t& metric) noexcept {
        metric.m_expected += expected;
    });
    scratch.m_vertex_results.clear();
    scratch.m_flat_values.clear();
    scratch.m_vertex_values.clear();
    scratch.m_fragment_inputs.clear();
    scratch.m_vertex_results.reserve(indices.size());
    scratch.m_vertex_io.object_to_world(object_to_world);
    scratch.m_vertex_io.world_to_clip(world_to_clip);
    for (const std::uint32_t vertex_index : indices) {
        if (static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) < vertex_index) {
            throw std::out_of_range("vertex index cannot be represented by software shader vertex_io_t");
        }

        auto& io = scratch.m_vertex_io;
        io.reset(static_cast<std::int32_t>(vertex_index), 0);
        for (const auto& input : program.vertex_interface().inputs()) {
            set_vertex_input(io, input, streams[input.index], attributes[input.index], vertex_index);
        }
        vertex_metric.update<vertex_metrics_t>([](vertex_metrics_t& metric) noexcept {
            ++metric.m_invocations;
        });
        program.run(bindings, io);
        const vector4f_t clip_position = io.position();
        if (!finite(clip_position)) {
            throw std::runtime_error("vertex shader produced a non-finite clip position");
        }

        const std::size_t output_offset = scratch.m_vertex_values.size();
        for (const auto& input : scratch.m_interpolated_inputs) {
            auto output = vertex_output(io, input);
            if (input.interpolation == shader::interpolation_t::noperspective) {
                noperspective_t noperspective;
                noperspective.count = shader_component_count(input.type);
                std::visit([&](const auto& typed) {
                    using type_t = std::remove_cvref_t<decltype(typed)>;
                    if constexpr (std::is_same_v<type_t, float>) {
                        noperspective.numerators[0] = double(typed) * double(clip_position[3]);
                    } else if constexpr (!std::is_same_v<type_t, noperspective_t>) {
                        for (std::size_t i = 0; i < noperspective.count; ++i) {
                            noperspective.numerators[i] = double(typed[i]) * double(clip_position[3]);
                        }
                    }
                }, output);
                output = noperspective;
            }
            scratch.m_vertex_values.emplace_back(input.index, output);
        }
        const auto flat_offset = scratch.m_flat_values.size();
        for (const auto& input : scratch.m_flat_inputs) {
            scratch.m_flat_values.emplace_back(input.index, flat_output(io, input));
        }
        scratch.m_vertex_results.push_back({
            .m_clip_position = clip_position,
            .m_outputs = {output_offset, scratch.m_interpolated_inputs.size()},
            .m_flat_outputs = {flat_offset, scratch.m_flat_inputs.size()}
        });
    }

    vertex_metric.stop();
    auto raster_metric = draw_metric.metric<raster_metrics_t>();
    const auto vertex = [&](std::size_t index) {
        return view(scratch.m_vertex_results[index], scratch.m_vertex_values, scratch.m_flat_values);
    };
    switch (geometry->primitive_topology()) {
        case vertex_primitive_topology_t::point: {
            for (std::size_t index = 0; index < indices.size(); ++index) {
                rasterize_point(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    raster_metric
                );
            }
        } break;
        case vertex_primitive_topology_t::line: {
            for (std::size_t index = 0; index + 1 < indices.size(); index += 2) {
                rasterize_line(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    vertex(index + 1),
                    scratch.m_clipping,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    raster_metric
                );
            }
        } break;
        case vertex_primitive_topology_t::line_strip: {
            for (std::size_t index = 0; index + 1 < indices.size(); ++index) {
                rasterize_line(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    vertex(index + 1),
                    scratch.m_clipping,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    raster_metric
                );
            }
        } break;
        case vertex_primitive_topology_t::line_loop: {
            for (std::size_t index = 0; index < indices.size(); ++index) {
                rasterize_line(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    vertex((index + 1) % indices.size()),
                    scratch.m_clipping,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    raster_metric
                );
            }
        } break;
        case vertex_primitive_topology_t::triangle: {
            for (std::size_t index = 0; index + 2 < indices.size(); index += 3) {
                rasterize_triangle(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(index),
                    vertex(index + 1),
                    vertex(index + 2),
                    scratch.m_raster,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    raster_metric,
                    vertex(material->provoking_vertex() == provoking_vertex_t::first ? index : index + 2).m_flat_outputs
                );
            }
        } break;
        case vertex_primitive_topology_t::triangle_strip: {
            for (std::size_t index = 0; index + 2 < indices.size(); ++index) {
                if (index % 2 == 0) {
                    rasterize_triangle(
                        *material,
                        bounds,
                        framebuffer,
                        vertex(index + 1),
                        vertex(index),
                        vertex(index + 2),
                        scratch.m_raster,
                        scratch.m_fragment_inputs,
                        scratch.m_fragment_io,
                        raster_metric,
                        vertex(material->provoking_vertex() == provoking_vertex_t::first ? index : index + 2).m_flat_outputs
                    );
                } else {
                    rasterize_triangle(
                        *material,
                        bounds,
                        framebuffer,
                        vertex(index),
                        vertex(index + 1),
                        vertex(index + 2),
                        scratch.m_raster,
                        scratch.m_fragment_inputs,
                        scratch.m_fragment_io,
                        raster_metric,
                        vertex(material->provoking_vertex() == provoking_vertex_t::first ? index : index + 2).m_flat_outputs
                    );
                }
            }
        } break;
        case vertex_primitive_topology_t::triangle_fan: {
            for (std::size_t index = 1; index + 1 < indices.size(); ++index) {
                rasterize_triangle(
                    *material,
                    bounds,
                    framebuffer,
                    vertex(0),
                    vertex(index),
                    vertex(index + 1),
                    scratch.m_raster,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io,
                    raster_metric,
                    vertex(material->provoking_vertex() == provoking_vertex_t::first ? index : index + 1).m_flat_outputs
                );
            }
        } break;
    }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
