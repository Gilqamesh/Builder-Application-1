#include "software_renderer.h"
#include "helpers.h"

#include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>
#include <m03gt1djvvy5atia5evkbg6rqy_software_shader/software_shader.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>

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

void software_renderer_t::clear(rgba8_t color) {
    std::ranges::fill(m_framebuffer.pixels(), color);
}

void software_renderer_t::draw(
    const camera_t<float, int, 2>& camera,
    const render_item_t& render_item
) {
    if (m_framebuffer.width() == 0 || m_framebuffer.height() == 0) {
        throw std::invalid_argument("software_renderer_t::draw requires a non-empty framebuffer");
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
    const auto& program = *material->program();
    const auto& bindings = material->bindings();
    program.validate_bindings(bindings);

    const auto& world_rect = camera.world_rect();
    const float world_width = world_rect[0].length();
    const float world_height = world_rect[1].length();
    if (world_width == 0.0F || world_height == 0.0F) {
        throw std::invalid_argument("software_renderer_t::draw requires non-empty camera world bounds");
    }

    const auto object_to_world = object_to_world_matrix(render_item);
    const auto world_to_clip = world_to_clip_matrix(camera, m_framebuffer);
    const int width = m_framebuffer.width();
    const int height = m_framebuffer.height();
    const auto framebuffer = m_framebuffer.pixels();
    const auto mesh = geometry->mesh();
    const auto& streams = mesh->vertex_streams();
    const auto attributes = mesh->vertex_attributes();
    for (const auto& input : program.vertex_interface().inputs()) {
        if (streams.size() <= input.index) {
            throw std::invalid_argument("shader vertex input location has no corresponding mesh stream");
        }
        validate_vertex_attribute(attributes[input.index], input.type);
    }
    for (const auto& input : program.fragment_interface().inputs()) {
        if (!supported_fragment_input(input.type)) {
            throw std::invalid_argument("software renderer cannot interpolate this fragment input type");
        }
    }

    const auto indices = geometry->indices();
    auto& scratch = m_scratch;
    scratch.m_vertex_results.clear();
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
        program.run(bindings, io);
        const vector4f_t clip_position = io.position();
        if (!finite(clip_position)) {
            throw std::runtime_error("vertex shader produced a non-finite clip position");
        }

        const std::size_t output_offset = scratch.m_vertex_values.size();
        for (const auto& input : program.fragment_interface().inputs()) {
            scratch.m_vertex_values.emplace_back(input.index, vertex_output(io, input));
        }
        scratch.m_vertex_results.push_back({
            .m_clip_position = clip_position,
            .m_outputs = {output_offset, program.fragment_interface().inputs().size()}
        });
    }

    const auto vertex = [&](std::size_t index) {
        return view(scratch.m_vertex_results[index], scratch.m_vertex_values);
    };
    switch (geometry->primitive_topology()) {
        case vertex_primitive_topology_t::point: {
            for (std::size_t index = 0; index < indices.size(); ++index) {
                rasterize_point(
                    program,
                    bindings,
                    width,
                    height,
                    framebuffer,
                    vertex(index),
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io
                );
            }
        } break;
        case vertex_primitive_topology_t::line: {
            for (std::size_t index = 0; index + 1 < indices.size(); index += 2) {
                rasterize_line(
                    program,
                    bindings,
                    width,
                    height,
                    framebuffer,
                    vertex(index),
                    vertex(index + 1),
                    scratch.m_clipping,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io
                );
            }
        } break;
        case vertex_primitive_topology_t::line_strip: {
            for (std::size_t index = 0; index + 1 < indices.size(); ++index) {
                rasterize_line(
                    program,
                    bindings,
                    width,
                    height,
                    framebuffer,
                    vertex(index),
                    vertex(index + 1),
                    scratch.m_clipping,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io
                );
            }
        } break;
        case vertex_primitive_topology_t::line_loop: {
            for (std::size_t index = 0; index < indices.size(); ++index) {
                rasterize_line(
                    program,
                    bindings,
                    width,
                    height,
                    framebuffer,
                    vertex(index),
                    vertex((index + 1) % indices.size()),
                    scratch.m_clipping,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io
                );
            }
        } break;
        case vertex_primitive_topology_t::triangle: {
            for (std::size_t index = 0; index + 2 < indices.size(); index += 3) {
                rasterize_triangle(
                    program,
                    bindings,
                    width,
                    height,
                    framebuffer,
                    vertex(index),
                    vertex(index + 1),
                    vertex(index + 2),
                    scratch.m_raster,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io
                );
            }
        } break;
        case vertex_primitive_topology_t::triangle_strip: {
            for (std::size_t index = 0; index + 2 < indices.size(); ++index) {
                if (index % 2 == 0) {
                    rasterize_triangle(
                        program,
                        bindings,
                        width,
                        height,
                        framebuffer,
                        vertex(index + 1),
                        vertex(index),
                        vertex(index + 2),
                        scratch.m_raster,
                        scratch.m_fragment_inputs,
                        scratch.m_fragment_io
                    );
                } else {
                    rasterize_triangle(
                        program,
                        bindings,
                        width,
                        height,
                        framebuffer,
                        vertex(index),
                        vertex(index + 1),
                        vertex(index + 2),
                        scratch.m_raster,
                        scratch.m_fragment_inputs,
                        scratch.m_fragment_io
                    );
                }
            }
        } break;
        case vertex_primitive_topology_t::triangle_fan: {
            for (std::size_t index = 1; index + 1 < indices.size(); ++index) {
                rasterize_triangle(
                    program,
                    bindings,
                    width,
                    height,
                    framebuffer,
                    vertex(0),
                    vertex(index),
                    vertex(index + 1),
                    scratch.m_raster,
                    scratch.m_fragment_inputs,
                    scratch.m_fragment_io
                );
            }
        } break;
    }
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
