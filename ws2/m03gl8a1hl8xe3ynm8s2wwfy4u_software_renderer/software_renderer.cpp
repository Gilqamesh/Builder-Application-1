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
        metric.color_writes += m_framebuffer.pixels().bytes().size() / 4;
    });
}

void software_renderer_t::clear_color(const camera_t& camera, rgba8_t color, profiling::metric_t& parent_metric) {
    auto metric = parent_metric.metric<clear_color_metrics_t>();
    const raster_bounds_t bounds(m_framebuffer.width(), m_framebuffer.height(), camera.view_rect());
    if (bounds.empty()) {
        return;
    }
    const auto pixels = m_framebuffer.pixels().bytes();
    for (auto y = bounds.first_y; y < bounds.end_y; ++y) {
        const auto offset = std::size_t(y + bounds.y) * std::size_t(bounds.width) + std::size_t(bounds.first_x + bounds.x);
        for (std::size_t index = offset; index < offset + std::size_t(bounds.end_x - bounds.first_x); ++index) {
            pixels[index * 4] = std::byte(color.red); pixels[index * 4 + 1] = std::byte(color.green);
            pixels[index * 4 + 2] = std::byte(color.blue); pixels[index * 4 + 3] = std::byte(color.alpha);
        }
    }
    metric.update<clear_color_metrics_t>([&bounds](clear_color_metrics_t& metric) noexcept {
        metric.color_writes += std::size_t(bounds.end_x - bounds.first_x) * std::size_t(bounds.end_y - bounds.first_y);
    });
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
        metric.depth_writes += m_framebuffer.depth().size();
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
    for (auto y = bounds.first_y; y < bounds.end_y; ++y) {
        const auto offset = std::size_t(y + bounds.y) * std::size_t(bounds.width) + std::size_t(bounds.first_x + bounds.x);
        std::ranges::fill(samples.subspan(offset, std::size_t(bounds.end_x - bounds.first_x)), depth);
    }
    metric.update<clear_depth_metrics_t>([&bounds](clear_depth_metrics_t& metric) noexcept {
        metric.depth_writes += std::size_t(bounds.end_x - bounds.first_x) * std::size_t(bounds.end_y - bounds.first_y);
    });
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
        metric.stencil_writes += m_framebuffer.stencil().size();
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
    for (auto y = bounds.first_y; y < bounds.end_y; ++y) {
        const auto offset = std::size_t(y + bounds.y) * std::size_t(bounds.width) + std::size_t(bounds.first_x + bounds.x);
        std::ranges::fill(samples.subspan(offset, std::size_t(bounds.end_x - bounds.first_x)), stencil);
    }
    metric.update<clear_stencil_metrics_t>([&bounds](clear_stencil_metrics_t& metric) noexcept {
        metric.stencil_writes += std::size_t(bounds.end_x - bounds.first_x) * std::size_t(bounds.end_y - bounds.first_y);
    });
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
        draw_metric.update<draw_metrics_t>([](draw_metrics_t& metrics) noexcept { ++metrics.empty_draws; });
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
    scratch.prepared_program.prepare(program, *material);
    validate_feedback(*material, m_framebuffer);

    const auto object_to_world = render_item.object_to_world();
    const auto world_to_clip = camera.world_to_clip();
    const auto mesh = geometry->mesh();
    const auto& streams = mesh->vertex_streams();
    const auto attributes = mesh->vertex_attributes();
    scratch.vertex_bindings.clear();
    for (const auto& input : program.vertex_interface().inputs()) {
        if (streams.size() <= input.index) {
            throw std::invalid_argument(std::format("software_renderer_t::draw requires mesh stream {} for vertex input {}; mesh has {} streams", input.index, input.type, streams.size()));
        }
        scratch.vertex_bindings.emplace_back(streams[input.index], attributes[input.index], input.type);
    }
    scratch.interpolated_inputs.clear();
    scratch.flat_inputs.clear();
    const auto fragment_inputs = program.fragment_interface().inputs();
    for (std::size_t index = 0; index < fragment_inputs.size(); ++index) {
        const auto& input = fragment_inputs[index];
        if (!supported_fragment_input(input)) {
            throw std::invalid_argument(std::format("software_renderer_t::draw cannot interpolate fragment input {} of type {} with {}; requires float scalar/vector interpolation or flat float/int32/uint32 inputs", input.index, input.type, input.interpolation));
        }
        (input.interpolation == shader::interpolation_t::flat ? scratch.flat_inputs : scratch.interpolated_inputs).push_back(index);
    }

    scratch.vertex_inputs.resize(program.vertex_interface().inputs().size());
    scratch.vertex_outputs.resize(program.vertex_interface().outputs().size());
    scratch.fragment_values.resize(fragment_inputs.size());
    scratch.fragment_outputs.resize(program.fragment_interface().outputs().size());
    preparation_metric.update<preparation_metrics_t>([&](preparation_metrics_t& metrics) {
        metrics.vertex_inputs += program.vertex_interface().inputs().size();
        for (const auto* interface : {&program.vertex_interface(), &program.fragment_interface()}) {
            for (const auto& binding : interface->bindings()) {
                switch (binding.type.category()) {
                    case shader::shader_data_category_t::texture_2d: { ++metrics.texture_bindings; } break;
                    case shader::shader_data_category_t::sampler: { ++metrics.sampler_bindings; } break;
                    default: { ++metrics.uniform_bindings; } break;
                }
            }
        }
        for (const auto& input : fragment_inputs) {
            const auto components = shader_component_count(input.type);
            switch (input.interpolation) {
                case shader::interpolation_t::perspective: { metrics.perspective_components += components; } break;
                case shader::interpolation_t::noperspective: { metrics.noperspective_components += components; } break;
                case shader::interpolation_t::flat: { metrics.flat_components += components; } break;
            }
        }
        if (!draw.color.write) {
            ++metrics.color_disabled_draws;
        } else {
            if (draw.color.replacement) { ++metrics.replacement_draws; }
            else { ++metrics.blended_draws; }
            if (draw.color.mask != color_mask_t::all) { ++metrics.masked_draws; }
            if (m_framebuffer.format() == texture::format_t::rgba8_srgb) { ++metrics.srgb_draws; }
            else { ++metrics.linear_draws; }
        }
    });
    draw_metric.update<draw_metrics_t>([&](draw_metrics_t& metrics) noexcept {
        switch (geometry->primitive_topology()) {
            case vertex_primitive_topology_t::point: { ++metrics.point_draws; } break;
            case vertex_primitive_topology_t::line: { ++metrics.line_draws; } break;
            case vertex_primitive_topology_t::line_strip: { ++metrics.line_strip_draws; } break;
            case vertex_primitive_topology_t::line_loop: { ++metrics.line_loop_draws; } break;
            case vertex_primitive_topology_t::triangle: { ++metrics.triangle_draws; } break;
            case vertex_primitive_topology_t::triangle_strip: { ++metrics.triangle_strip_draws; } break;
            case vertex_primitive_topology_t::triangle_fan: { ++metrics.triangle_fan_draws; } break;
        }
    });
    preparation_metric.stop();
    const auto indices = geometry->indices();
    auto vertex_metric = draw_metric.metric<vertex_metrics_t>();
    {
        counter_batch_t<vertex_metrics_t> counter_batch(vertex_metric);
        auto* counters = counter_batch.counters();
        if (counters) { counters->expected = indices.size(); }
        scratch.vertex_results.clear();
        scratch.flat_values.clear();
        scratch.vertex_values.clear();
        scratch.vertex_results.reserve(indices.size());
        scratch.vertex_io.object_to_world(object_to_world);
        scratch.vertex_io.world_to_clip(world_to_clip);
        for (const std::uint32_t vertex_index : indices) {
            if (static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) < vertex_index) {
                throw std::out_of_range("vertex index cannot be represented by software shader vertex_io_t");
            }

            auto& io = scratch.vertex_io;
            io.reset(static_cast<std::int32_t>(vertex_index), 0);
            for (std::size_t index = 0; index < scratch.vertex_bindings.size(); ++index) {
                const auto& input = scratch.vertex_bindings[index];
                scratch.vertex_inputs[index] = input.read(input.stream, vertex_index);
            }
            if (counters) { ++counters->invocations; }
            scratch.prepared_program.run(scratch.vertex_inputs, scratch.vertex_outputs, io, scratch.execution_context);
            const vector4f_t clip_position = io.position();
            if (!finite(clip_position)) {
                throw std::runtime_error("vertex shader produced a non-finite clip position");
            }

            const std::size_t output_offset = scratch.vertex_values.size();
            for (const auto index : scratch.interpolated_inputs) {
                const auto& input = fragment_inputs[index];
                auto output = vertex_output(scratch.vertex_outputs[program.fragment_sources()[index]], input);
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
                scratch.vertex_values.push_back(output);
            }
            const auto flat_offset = scratch.flat_values.size();
            for (const auto index : scratch.flat_inputs) {
                scratch.flat_values.push_back(flat_output(scratch.vertex_outputs[program.fragment_sources()[index]], fragment_inputs[index]));
            }
            scratch.vertex_results.push_back({
                .clip_position = clip_position,
                .outputs = {output_offset, scratch.interpolated_inputs.size()},
                .flat_outputs = {flat_offset, scratch.flat_inputs.size()}
            });
        }
    }

    vertex_metric.stop();
    auto raster_metric = draw_metric.metric<raster_metrics_t>();
    counter_batch_t<raster_metrics_t> counter_batch(raster_metric);
    draw.counters = counter_batch.counters();
    const auto vertex = [&](std::size_t index) {
        return view(scratch.vertex_results[index], scratch.vertex_values, scratch.flat_values);
    };
    const auto submit_line = [&](std::size_t first, std::size_t second) {
        rasterize_line(draw, vertex(first), vertex(second));
    };
    const auto submit_triangle = [&](std::size_t first, std::size_t second, std::size_t third, std::size_t provoking) {
        rasterize_triangle(draw, vertex(first), vertex(second), vertex(third), vertex(provoking).flat_outputs);
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
