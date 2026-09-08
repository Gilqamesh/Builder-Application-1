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
    geometry->validate();
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
    auto& scratch = m_scratch;
    draw_context_t draw(*material, bounds, m_framebuffer, scratch);
    scratch.m_prepared_program.prepare(program, *material);
    validate_feedback(*material, m_framebuffer);

    const auto object_to_world = render_item.object_to_world();
    const auto world_to_clip = camera.world_to_clip();
    const auto mesh = geometry->mesh();
    const auto& streams = mesh->vertex_streams();
    const auto attributes = mesh->vertex_attributes();
    scratch.m_vertex_bindings.clear();
    for (const auto& input : program.vertex_interface().inputs()) {
        if (streams.size() <= input.index) {
            throw std::invalid_argument("shader vertex input location has no corresponding mesh stream");
        }
        scratch.m_vertex_bindings.emplace_back(streams[input.index], attributes[input.index], input.type);
    }
    scratch.m_interpolated_inputs.clear();
    scratch.m_flat_inputs.clear();
    const auto fragment_inputs = program.fragment_interface().inputs();
    for (std::size_t index = 0; index < fragment_inputs.size(); ++index) {
        const auto& input = fragment_inputs[index];
        if (!supported_fragment_input(input)) {
            throw std::invalid_argument("software renderer requires float scalar/vector interpolation or flat float/int32/uint32 inputs");
        }
        (input.interpolation == shader::interpolation_t::flat ? scratch.m_flat_inputs : scratch.m_interpolated_inputs).push_back(index);
    }

    scratch.m_vertex_inputs.resize(program.vertex_interface().inputs().size());
    scratch.m_vertex_outputs.resize(program.vertex_interface().outputs().size());
    scratch.m_fragment_values.resize(fragment_inputs.size());
    scratch.m_fragment_outputs.resize(program.fragment_interface().outputs().size());
    preparation_metric.stop();
    const auto indices = geometry->indices();
    auto vertex_metric = draw_metric.metric<vertex_metrics_t>();
    vertex_metric.update<vertex_metrics_t>([expected = indices.size()](vertex_metrics_t& metric) noexcept {
        metric.m_expected += expected;
    });
    scratch.m_vertex_results.clear();
    scratch.m_flat_values.clear();
    scratch.m_vertex_values.clear();
    scratch.m_vertex_results.reserve(indices.size());
    scratch.m_vertex_io.object_to_world(object_to_world);
    scratch.m_vertex_io.world_to_clip(world_to_clip);
    for (const std::uint32_t vertex_index : indices) {
        if (static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) < vertex_index) {
            throw std::out_of_range("vertex index cannot be represented by software shader vertex_io_t");
        }

        auto& io = scratch.m_vertex_io;
        io.reset(static_cast<std::int32_t>(vertex_index), 0);
        for (std::size_t index = 0; index < scratch.m_vertex_bindings.size(); ++index) {
            const auto& input = scratch.m_vertex_bindings[index];
            scratch.m_vertex_inputs[index] = input.read(input.stream, vertex_index);
        }
        vertex_metric.update<vertex_metrics_t>([](vertex_metrics_t& metric) noexcept {
            ++metric.m_invocations;
        });
        scratch.m_prepared_program.run(scratch.m_vertex_inputs, scratch.m_vertex_outputs, io, scratch.m_execution_context);
        const vector4f_t clip_position = io.position();
        if (!finite(clip_position)) {
            throw std::runtime_error("vertex shader produced a non-finite clip position");
        }

        const std::size_t output_offset = scratch.m_vertex_values.size();
        for (const auto index : scratch.m_interpolated_inputs) {
            const auto& input = fragment_inputs[index];
            auto output = vertex_output(scratch.m_vertex_outputs[program.fragment_sources()[index]], input);
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
            scratch.m_vertex_values.push_back(output);
        }
        const auto flat_offset = scratch.m_flat_values.size();
        for (const auto index : scratch.m_flat_inputs) {
            scratch.m_flat_values.push_back(flat_output(scratch.m_vertex_outputs[program.fragment_sources()[index]], fragment_inputs[index]));
        }
        scratch.m_vertex_results.push_back({
            .m_clip_position = clip_position,
            .m_outputs = {output_offset, scratch.m_interpolated_inputs.size()},
            .m_flat_outputs = {flat_offset, scratch.m_flat_inputs.size()}
        });
    }

    vertex_metric.stop();
    auto raster_metric = draw_metric.metric<raster_metrics_t>();
    draw.metric = &raster_metric;
    const auto vertex = [&](std::size_t index) {
        return view(scratch.m_vertex_results[index], scratch.m_vertex_values, scratch.m_flat_values);
    };
    const auto submit_line = [&](std::size_t first, std::size_t second) {
        rasterize_line(draw, vertex(first), vertex(second));
    };
    const auto submit_triangle = [&](std::size_t first, std::size_t second, std::size_t third, std::size_t provoking) {
        rasterize_triangle(draw, vertex(first), vertex(second), vertex(third), vertex(provoking).m_flat_outputs);
    };
    switch (geometry->primitive_topology()) {
        case vertex_primitive_topology_t::point: {
            for (std::size_t index = 0; index < indices.size(); ++index) {
                rasterize_point(draw, vertex(index));
            }
        } break;
        case vertex_primitive_topology_t::line: {
            for (std::size_t index = 0; index + 1 < indices.size(); index += 2) {
                submit_line(index, index + 1);
            }
        } break;
        case vertex_primitive_topology_t::line_strip: {
            for (std::size_t index = 0; index + 1 < indices.size(); ++index) {
                submit_line(index, index + 1);
            }
        } break;
        case vertex_primitive_topology_t::line_loop: {
            for (std::size_t index = 0; index < indices.size(); ++index) {
                submit_line(index, (index + 1) % indices.size());
            }
        } break;
        case vertex_primitive_topology_t::triangle: {
            for (std::size_t index = 0; index + 2 < indices.size(); index += 3) {
                const auto provoking = material->provoking_vertex() == provoking_vertex_t::first ? index : index + 2;
                submit_triangle(index, index + 1, index + 2, provoking);
            }
        } break;
        case vertex_primitive_topology_t::triangle_strip: {
            for (std::size_t index = 0; index + 2 < indices.size(); ++index) {
                const auto provoking = material->provoking_vertex() == provoking_vertex_t::first ? index : index + 2;
                if (index % 2 != 0) {
                    submit_triangle(index + 1, index, index + 2, provoking);
                } else {
                    submit_triangle(index, index + 1, index + 2, provoking);
                }
            }
        } break;
        case vertex_primitive_topology_t::triangle_fan: {
            for (std::size_t index = 1; index + 1 < indices.size(); ++index) {
                const auto provoking = material->provoking_vertex() == provoking_vertex_t::first ? index : index + 1;
                submit_triangle(0, index, index + 1, provoking);
            }
        } break;
    }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
