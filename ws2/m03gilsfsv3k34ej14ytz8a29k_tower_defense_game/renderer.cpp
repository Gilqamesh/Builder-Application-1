#include "renderer.h"

#include <unordered_map>

#include <m03gagbht17w4tser1fescqxye_raylib/raylib.h>
#include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>

namespace {

namespace texture_api = m03gt0l0q3l4b1k27eab5k7py1_texture;
namespace software_renderer_api = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;

int texture_format_to_raylib_pixel_format(texture_api::format_t format) {
// Pixel formats
// NOTE: Support depends on OpenGL version and platform
// typedef enum {
//     PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1, // 8 bit per pixel (no alpha)
//     PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,    // 8*2 bpp (2 channels)
//     PIXELFORMAT_UNCOMPRESSED_R5G6B5,        // 16 bpp
//     PIXELFORMAT_UNCOMPRESSED_R8G8B8,        // 24 bpp
//     PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,      // 16 bpp (1 bit alpha)
//     PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,      // 16 bpp (4 bit alpha)
//     PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,      // 32 bpp
//     PIXELFORMAT_UNCOMPRESSED_R32,           // 32 bpp (1 channel - float)
//     PIXELFORMAT_UNCOMPRESSED_R32G32B32,     // 32*3 bpp (3 channels - float)
//     PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,  // 32*4 bpp (4 channels - float)
//     PIXELFORMAT_UNCOMPRESSED_R16,           // 16 bpp (1 channel - half float)
//     PIXELFORMAT_UNCOMPRESSED_R16G16B16,     // 16*3 bpp (3 channels - half float)
//     PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,  // 16*4 bpp (4 channels - half float)
//     PIXELFORMAT_COMPRESSED_DXT1_RGB,        // 4 bpp (no alpha)
//     PIXELFORMAT_COMPRESSED_DXT1_RGBA,       // 4 bpp (1 bit alpha)
//     PIXELFORMAT_COMPRESSED_DXT3_RGBA,       // 8 bpp
//     PIXELFORMAT_COMPRESSED_DXT5_RGBA,       // 8 bpp
//     PIXELFORMAT_COMPRESSED_ETC1_RGB,        // 4 bpp
//     PIXELFORMAT_COMPRESSED_ETC2_RGB,        // 4 bpp
//     PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,   // 8 bpp
//     PIXELFORMAT_COMPRESSED_PVRT_RGB,        // 4 bpp
//     PIXELFORMAT_COMPRESSED_PVRT_RGBA,       // 4 bpp
//     PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,   // 8 bpp
//     PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA    // 2 bpp
// } PixelFormat;
    switch (format) {
        case texture_api::format_t::rgba8_unorm: return PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        case texture_api::format_t::rgba8_srgb: return PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        case texture_api::format_t::rgba16_float: return PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;
        case texture_api::format_t::rgba32_float: return PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
        default: throw std::runtime_error(std::format("texture_format_to_raylib_pixel_format: does not support texture format {}", static_cast<int>(format)));
    }
}

}

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

renderer_t::renderer_t(): renderer_t({800, 600})
{
}

renderer_t::renderer_t(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& window_bounds):
    m_window_bounds(window_bounds)
{
    InitWindow(m_window_bounds[0], m_window_bounds[1], "m03gilsfsv3k34ej14ytz8a29k_tower_defense_game");
    SetTargetFPS(60);
}

renderer_t::~renderer_t() {
    CloseWindow();
}

void renderer_t::window_bounds(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& bounds) {
    m_window_bounds = bounds;
}

const m03ginwy24ng8o487c4beoms6l_vector::vector_t<int, 2>& renderer_t::window_bounds() const {
    return m_window_bounds;
}

void renderer_t::draw(
    const software_renderer_api::camera_t& camera,
    const software_renderer_api::render_item_t& render_item
) {
    if (camera.view_rect().is_empty()) {
        return;
    }

    const auto geometry = render_item.geometry();
    if (!geometry || !geometry->mesh()) {
        throw std::runtime_error("renderer_t::draw: render item has incomplete geometry");
    }

    const auto mesh = geometry->mesh();
    const auto indices = geometry->indices();
    const auto primitive_topology = geometry->primitive_topology();
    const auto material = render_item.material();
    if (!material) {
        throw std::runtime_error("renderer_t::draw: render item has no material");
    }
    const auto object_to_world = render_item.object_to_world();
    const auto transform_position = [&](const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>& position) {
        const auto world_position = object_to_world * m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4>{position[0], position[1], 0.0F, 1.0F};
        return camera.world_to_framebuffer({world_position[0], world_position[1], world_position[2]});
    };

    const auto& vertex_streams = mesh->vertex_streams();
    if (vertex_streams.size() == 0) {
        throw std::runtime_error("renderer_t::draw: does not support entity_mesh with no vertex_streams");
    }
    const auto& vertex_stream = vertex_streams[0];

    const auto& vertex_attributes = mesh->vertex_attributes();
    if (vertex_attributes.size() == 0) {
        throw std::runtime_error("renderer_t::draw: does not support entity_mesh with no vertex_attributes");
    }

    const auto expected_first_vertex_attribute_count = 2;
    const auto& first_vertex_attribute = vertex_attributes[0];
    if (first_vertex_attribute.component_count() != expected_first_vertex_attribute_count) {
        throw std::runtime_error(std::format("renderer_t::draw: does not support entity_mesh with vertex_attributes that do not have {} components", expected_first_vertex_attribute_count));
    }

    const auto expected_vertex_attribute_type = software_renderer_api::vertex_attribute_type_t::R32;
    if (first_vertex_attribute.type() != expected_vertex_attribute_type) {
        throw std::runtime_error(std::format("renderer_t::draw: does not support entity_mesh with vertex_attributes that are not of type {}", static_cast<int>(expected_vertex_attribute_type)));
    }

    const auto& texture = material->texture(0);
    Image image;
    image.data = const_cast<std::byte*>(texture.bytes().data());
    image.width = texture.width();
    image.height = texture.height();
    image.mipmaps = 1;
    image.format = texture_format_to_raylib_pixel_format(texture.format());

    const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>* positions = reinterpret_cast<const m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2>*>(vertex_stream.data().data());
    switch (primitive_topology) {
        case software_renderer_api::vertex_primitive_topology_t::point: {
            for (size_t i = 0; i < indices.size(); ++i) {
                const auto view_position = transform_position(positions[indices[i]]);
                const auto& view_bounds = camera.view_rect().bounds();
                if (view_position[0] < static_cast<double>(view_bounds[0][0]) || static_cast<double>(view_bounds[0][1]) <= view_position[0]
                    || view_position[1] < static_cast<double>(view_bounds[1][0]) || static_cast<double>(view_bounds[1][1]) <= view_position[1]) {
                    continue;
                }
                
                const auto point_size = 3.0f;
                const auto point_color = GREEN;
                // const auto point_color = GetImageColor(image, view_position[0], view_position[1]);
                DrawCircleV({view_position[0], view_position[1]}, point_size, point_color);
            }
        } break;
        case software_renderer_api::vertex_primitive_topology_t::line: {
            for (size_t i = 0; i < indices.size(); i += 2) {
                const auto view_position_0 = transform_position(positions[indices[i]]);
                const auto view_position_1 = transform_position(positions[indices[i + 1]]);
                DrawLineV({view_position_0[0], view_position_0[1]}, {view_position_1[0], view_position_1[1]}, GREEN);
            }
        } break;
        case software_renderer_api::vertex_primitive_topology_t::line_strip: {
            for (size_t i = 0; i + 1 < indices.size(); ++i) {
                const auto view_position_0 = transform_position(positions[indices[i]]);
                const auto view_position_1 = transform_position(positions[indices[i + 1]]);
                DrawLineV({view_position_0[0], view_position_0[1]}, {view_position_1[0], view_position_1[1]}, GREEN);
            }
        } break;
        case software_renderer_api::vertex_primitive_topology_t::line_loop: {
            for (size_t i = 0; i < indices.size(); ++i) {
                const auto view_position_0 = transform_position(positions[indices[i]]);
                const auto view_position_1 = transform_position(positions[indices[(i + 1) % indices.size()]]);
                DrawLineV({view_position_0[0], view_position_0[1]}, {view_position_1[0], view_position_1[1]}, GREEN);
            }
        } break;
        case software_renderer_api::vertex_primitive_topology_t::triangle: {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                const auto view_position_0 = transform_position(positions[indices[i]]);
                const auto view_position_1 = transform_position(positions[indices[i + 1]]);
                const auto view_position_2 = transform_position(positions[indices[i + 2]]);
                DrawTriangle(
                    {static_cast<float>(view_position_0[0]), static_cast<float>(view_position_0[1])},
                    {static_cast<float>(view_position_1[0]), static_cast<float>(view_position_1[1])},
                    {static_cast<float>(view_position_2[0]), static_cast<float>(view_position_2[1])},
                    GREEN
                );
            }
        } break;
        case software_renderer_api::vertex_primitive_topology_t::triangle_strip: {
            for (size_t i = 0; i + 2 < indices.size(); ++i) {
                const auto view_position_0 = transform_position(positions[indices[i]]);
                const auto view_position_1 = transform_position(positions[indices[i + 1]]);
                const auto view_position_2 = transform_position(positions[indices[i + 2]]);
                if (i % 2 == 0) {
                    DrawTriangle(
                        {static_cast<float>(view_position_1[0]), static_cast<float>(view_position_1[1])},
                        {static_cast<float>(view_position_0[0]), static_cast<float>(view_position_0[1])},
                        {static_cast<float>(view_position_2[0]), static_cast<float>(view_position_2[1])},
                        GREEN
                    );
                } else {
                    DrawTriangle(
                        {static_cast<float>(view_position_0[0]), static_cast<float>(view_position_0[1])},
                        {static_cast<float>(view_position_1[0]), static_cast<float>(view_position_1[1])},
                        {static_cast<float>(view_position_2[0]), static_cast<float>(view_position_2[1])},
                        GREEN
                    );
                }
            }
        } break;
        case software_renderer_api::vertex_primitive_topology_t::triangle_fan: {
            for (size_t i = 1; i + 1 < indices.size(); ++i) {
                const auto view_position_0 = transform_position(positions[indices[0]]);
                const auto view_position_1 = transform_position(positions[indices[i]]);
                const auto view_position_2 = transform_position(positions[indices[i + 1]]);
                DrawTriangle(
                    {static_cast<float>(view_position_0[0]), static_cast<float>(view_position_0[1])},
                    {static_cast<float>(view_position_1[0]), static_cast<float>(view_position_1[1])},
                    {static_cast<float>(view_position_2[0]), static_cast<float>(view_position_2[1])},
                    GREEN
                );
            }
        } break;
        default: {
            throw std::runtime_error(std::format("renderer_t::draw: does not support entity with vertex_primitive_topology_t: {}", primitive_topology));
        } break;
    }
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
