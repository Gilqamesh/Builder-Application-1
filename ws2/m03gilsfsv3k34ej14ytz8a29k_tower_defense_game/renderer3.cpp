#include "renderer3.h"

#include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
#include <m03glv28yaiwc5hbnvz43r14zr_matrix/api.h>
#include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>

#include <array>
#include <cstddef>
#include <format>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace tower_defense_api = m03gilsfsv3k34ej14ytz8a29k_tower_defense_game;
namespace software_renderer_api = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
namespace vector_api = m03ginwy24ng8o487c4beoms6l_vector;
namespace texture_api = m03gt0l0q3l4b1k27eab5k7py1_texture;

using renderer3_color_t = tower_defense_api::renderer3_color_t;
using vector2f_t = vector_api::vector_t<float, 2>;

struct gl_color_t {
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
};

struct gl_position_t {
    GLfloat x;
    GLfloat y;
};

struct render_item_data_t {
    const m03gjfvd6i5jzbmngb2ldoooza_type_erased_array::type_erased_array_t* vertex_stream;
    std::span<const std::uint32_t> indices;
    software_renderer_api::vertex_primitive_topology_t primitive_topology;
    m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 4, 4> object_to_world;
};

constexpr renderer3_color_t ray_white_color() noexcept {
    return { 245, 245, 245, 255 };
}

constexpr renderer3_color_t green_color() noexcept {
    return { 0, 228, 48, 255 };
}

gl_color_t to_gl_color(renderer3_color_t color) noexcept {
    constexpr GLfloat scale = 1.0f / 255.0f;
    return {
        .red = static_cast<GLfloat>(color.red) * scale,
        .green = static_cast<GLfloat>(color.green) * scale,
        .blue = static_cast<GLfloat>(color.blue) * scale,
        .alpha = static_cast<GLfloat>(color.alpha) * scale
    };
}

std::string shader_log(const GladGLContext& gl, GLuint shader) {
    GLint length = 0;
    gl.GetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length <= 0) {
        return {};
    }

    std::string result(static_cast<std::size_t>(length), '\0');
    gl.GetShaderInfoLog(shader, length, nullptr, result.data());
    return result;
}

std::string program_log(const GladGLContext& gl, GLuint program) {
    GLint length = 0;
    gl.GetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    if (length <= 0) {
        return {};
    }

    std::string result(static_cast<std::size_t>(length), '\0');
    gl.GetProgramInfoLog(program, length, nullptr, result.data());
    return result;
}

GLuint compile_shader(const GladGLContext& gl, GLenum type, const char* source) {
    const GLuint shader = gl.CreateShader(type);
    if (shader == 0) {
        throw std::runtime_error("renderer3_t: failed to create shader");
    }

    gl.ShaderSource(shader, 1, &source, nullptr);
    gl.CompileShader(shader);

    GLint compiled = GL_FALSE;
    gl.GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (compiled == GL_TRUE) {
        return shader;
    }

    const std::string log = shader_log(gl, shader);
    gl.DeleteShader(shader);
    throw std::runtime_error(std::format("renderer3_t: shader compilation failed: {}", log));
}

GLuint create_program(const GladGLContext& gl) {
    static constexpr const char* vertex_source = R"(
#version 330 core

layout(location = 0) in vec2 position;

uniform vec2 viewport_size;

void main() {
    vec2 clip_position = vec2(
        position.x / viewport_size.x * 2.0 - 1.0,
        1.0 - position.y / viewport_size.y * 2.0
    );
    gl_Position = vec4(clip_position, 0.0, 1.0);
    gl_PointSize = 7.0;
}
)";

    static constexpr const char* fragment_source = R"(
#version 330 core

layout(location = 0) out vec4 fragment_color;

uniform vec4 entity_color;
uniform bool point_shape;

void main() {
    if (point_shape) {
        vec2 offset = gl_PointCoord - vec2(0.5);
        if (dot(offset, offset) > 0.25) {
            discard;
        }
    }

    fragment_color = entity_color;
}
)";

    const GLuint vertex_shader = compile_shader(gl, GL_VERTEX_SHADER, vertex_source);
    GLuint fragment_shader = 0;
    GLuint program = 0;

    try {
        fragment_shader = compile_shader(gl, GL_FRAGMENT_SHADER, fragment_source);
        program = gl.CreateProgram();

        if (program == 0) {
            throw std::runtime_error("renderer3_t: failed to create shader program");
        }

        gl.AttachShader(program, vertex_shader);
        gl.AttachShader(program, fragment_shader);
        gl.LinkProgram(program);

        GLint linked = GL_FALSE;
        gl.GetProgramiv(program, GL_LINK_STATUS, &linked);

        if (linked != GL_TRUE) {
            throw std::runtime_error(std::format("renderer3_t: program linking failed: {}", program_log(gl, program)));
        }
    } catch (...) {
        if (program != 0) {
            gl.DeleteProgram(program);
        }

        if (fragment_shader != 0) {
            gl.DeleteShader(fragment_shader);
        }

        gl.DeleteShader(vertex_shader);
        throw;
    }

    gl.DeleteShader(fragment_shader);
    gl.DeleteShader(vertex_shader);
    return program;
}

vector2f_t read_position(
    const m03gjfvd6i5jzbmngb2ldoooza_type_erased_array::type_erased_array_t& vertex_stream,
    std::uint32_t index
) {
    const auto values = vertex_stream.read<std::array<float, 2>>(index);
    return { values[0], values[1] };
}

m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 3> transform_position(
    const software_renderer_api::camera_t& camera,
    vector2f_t position,
    const m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t<float, 4, 4>& object_to_world
) {
    const auto world_position = object_to_world * vector_api::vector_t<float, 4>{position[0], position[1], 0.0F, 1.0F};
    return camera.world_to_framebuffer({world_position[0], world_position[1], world_position[2]});
}

void require_supported_texture_format(texture_api::format_t format) {
    switch (format) {
        case texture_api::format_t::rgba8_unorm:
        case texture_api::format_t::rgba8_srgb:
        case texture_api::format_t::rgba16_float:
        case texture_api::format_t::rgba32_float: {
            return;
        } break;
        default: {
            throw std::runtime_error(std::format(
                "renderer3_t::draw: does not support texture format {}",
                static_cast<int>(format)
            ));
        } break;
    }
}

render_item_data_t validate_render_item(const software_renderer_api::render_item_t& render_item) {
    const auto geometry = render_item.geometry();
    if (!geometry) {
        throw std::runtime_error("renderer3_t::draw: does not support a render item with no geometry");
    }

    const auto mesh = geometry->mesh();
    if (!mesh) {
        throw std::runtime_error("renderer3_t::draw: does not support geometry with no mesh");
    }

    const auto material = render_item.material();
    if (!material) {
        throw std::runtime_error("renderer3_t::draw: does not support a render item with no material");
    }

    require_supported_texture_format(material->texture(0).format());

    const auto& vertex_streams = mesh->vertex_streams();
    if (vertex_streams.size() == 0) {
        throw std::runtime_error("renderer3_t::draw: does not support entity_mesh with no vertex_streams");
    }
    const auto& vertex_stream = vertex_streams[0];

    const auto& vertex_attributes = mesh->vertex_attributes();
    if (vertex_attributes.size() == 0) {
        throw std::runtime_error("renderer3_t::draw: does not support entity_mesh with no vertex_attributes");
    }

    const auto expected_first_vertex_attribute_count = 2;
    const auto& first_vertex_attribute = vertex_attributes[0];
    if (first_vertex_attribute.component_count() != expected_first_vertex_attribute_count) {
        throw std::runtime_error(std::format(
            "renderer3_t::draw: does not support entity_mesh with vertex_attributes that do not have {} components",
            expected_first_vertex_attribute_count
        ));
    }

    const auto expected_vertex_attribute_type = software_renderer_api::vertex_attribute_type_t::R32;
    if (first_vertex_attribute.type() != expected_vertex_attribute_type) {
        throw std::runtime_error(std::format(
            "renderer3_t::draw: does not support entity_mesh with vertex_attributes that are not of type {}",
            static_cast<int>(expected_vertex_attribute_type)
        ));
    }

    return {
        .vertex_stream = &vertex_stream,
        .indices = geometry->indices(),
        .primitive_topology = geometry->primitive_topology(),
        .object_to_world = render_item.object_to_world()
    };
}

std::vector<gl_position_t> build_view_positions(
    const software_renderer_api::camera_t& camera,
    const render_item_data_t& render_data
) {
    std::vector<gl_position_t> view_positions;
    view_positions.reserve(render_data.vertex_stream->element_count());

    for (std::size_t i = 0; i < render_data.vertex_stream->element_count(); ++i) {
        const auto position = read_position(*render_data.vertex_stream, static_cast<std::uint32_t>(i));
        const auto view_position = transform_position(camera, position, render_data.object_to_world);
        view_positions.push_back({
            .x = static_cast<GLfloat>(view_position[0]),
            .y = static_cast<GLfloat>(view_position[1])
        });
    }

    return view_positions;
}

GLenum to_gl_primitive_topology(software_renderer_api::vertex_primitive_topology_t primitive_topology) {
    switch (primitive_topology) {
        case software_renderer_api::vertex_primitive_topology_t::point: {
            return GL_POINTS;
        } break;
        case software_renderer_api::vertex_primitive_topology_t::line: {
            return GL_LINES;
        } break;
        case software_renderer_api::vertex_primitive_topology_t::line_strip: {
            return GL_LINE_STRIP;
        } break;
        case software_renderer_api::vertex_primitive_topology_t::line_loop: {
            return GL_LINE_LOOP;
        } break;
        case software_renderer_api::vertex_primitive_topology_t::triangle: {
            return GL_TRIANGLES;
        } break;
        case software_renderer_api::vertex_primitive_topology_t::triangle_strip: {
            return GL_TRIANGLE_STRIP;
        } break;
        case software_renderer_api::vertex_primitive_topology_t::triangle_fan: {
            return GL_TRIANGLE_FAN;
        } break;
        default: {
            throw std::runtime_error(std::format(
                "renderer3_t::draw: does not support entity with vertex_primitive_topology_t: {}",
                primitive_topology
            ));
        } break;
    }
}

GLsizei checked_draw_count(std::size_t count) {
    if (count > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::length_error("renderer3_t::draw: index count exceeds GLsizei max");
    }
    return static_cast<GLsizei>(count);
}

GLsizeiptr checked_buffer_size(std::size_t count, std::size_t element_size, const char* description) {
    if (element_size != 0 && count > std::numeric_limits<std::size_t>::max() / element_size) {
        throw std::length_error(std::format("renderer3_t::draw: {} byte count overflows size_t", description));
    }

    const std::size_t byte_count = count * element_size;
    if (byte_count > static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())) {
        throw std::length_error(std::format("renderer3_t::draw: {} byte count exceeds GLsizeiptr max", description));
    }

    return static_cast<GLsizeiptr>(byte_count);
}

} // namespace

namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game {

renderer3_t::renderer3_t(std::shared_ptr<m03gkcdy62bnz808pmk4uzkjra_glfw::window_t> window):
    m_opengl_renderer(std::move(window)),
    m_width(0),
    m_height(0),
    m_program(0),
    m_vertex_array(0),
    m_vertex_buffer(0),
    m_index_buffer(0),
    m_viewport_size_location(-1),
    m_entity_color_location(-1),
    m_point_shape_location(-1),
    m_frame_active(false)
{
    try {
        create_resources();

        const auto size = m_opengl_renderer.window()->framebuffer_size();
        m_width = size[0];
        m_height = size[1];
    } catch (...) {
        destroy_resources();
        throw;
    }
}

renderer3_t::~renderer3_t() {
    destroy_resources();
}

bool renderer3_t::begin_frame() {
    return begin_frame(ray_white_color());
}

bool renderer3_t::begin_frame(renderer3_color_t clear_color) {
    if (!m_opengl_renderer.window()->context_current(true)) {
        throw std::runtime_error("renderer3_t::begin_frame: failed to make the OpenGL context current");
    }

    const auto size = m_opengl_renderer.window()->framebuffer_size();
    m_width = size[0];
    m_height = size[1];
    m_frame_active = m_width > 0 && m_height > 0;

    if (!m_frame_active) {
        return false;
    }

    const auto gl_color = to_gl_color(clear_color);
    const auto& gl = m_opengl_renderer.get_gl();

    gl.Viewport(0, 0, m_width, m_height);
    gl.Disable(GL_DEPTH_TEST);
    gl.Disable(GL_CULL_FACE);
    gl.Disable(GL_BLEND);
    gl.Enable(GL_PROGRAM_POINT_SIZE);
    gl.ClearColor(gl_color.red, gl_color.green, gl_color.blue, gl_color.alpha);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    return true;
}

void renderer3_t::present() {
    if (!m_frame_active) {
        return;
    }

    if (!m_opengl_renderer.window()->context_current(true)) {
        throw std::runtime_error("renderer3_t::present: failed to make the OpenGL context current");
    }

    m_opengl_renderer.window()->swap_buffers();
    m_frame_active = false;
}

int renderer3_t::width() const noexcept {
    return m_width;
}

int renderer3_t::height() const noexcept {
    return m_height;
}

void renderer3_t::draw(
    const software_renderer_api::camera_t& camera,
    const software_renderer_api::render_item_t& render_item
) {
    if (camera.view_rect().is_empty()) {
        return;
    }

    if (!m_frame_active) {
        throw std::runtime_error("renderer3_t::draw: begin_frame must be called before draw");
    }

    if (!m_opengl_renderer.window()->context_current(true)) {
        throw std::runtime_error("renderer3_t::draw: failed to make the OpenGL context current");
    }

    const auto render_data = validate_render_item(render_item);
    const auto view_positions = build_view_positions(camera, render_data);
    const auto indices = render_data.indices;
    const auto entity_color = to_gl_color(green_color());
    const auto gl_topology = to_gl_primitive_topology(render_data.primitive_topology);
    const auto draw_count = checked_draw_count(indices.size());
    const auto vertex_buffer_size = checked_buffer_size(view_positions.size(), sizeof(gl_position_t), "vertex buffer");
    const auto index_buffer_size = checked_buffer_size(indices.size(), sizeof(std::uint32_t), "index buffer");
    const auto& gl = m_opengl_renderer.get_gl();

    gl.UseProgram(m_program);
    gl.Uniform2f(m_viewport_size_location, static_cast<GLfloat>(m_width), static_cast<GLfloat>(m_height));
    gl.Uniform4f(m_entity_color_location, entity_color.red, entity_color.green, entity_color.blue, entity_color.alpha);
    gl.Uniform1i(m_point_shape_location, render_data.primitive_topology == software_renderer_api::vertex_primitive_topology_t::point ? GL_TRUE : GL_FALSE);

    gl.BindVertexArray(m_vertex_array);

    gl.BindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    gl.BufferData(GL_ARRAY_BUFFER, vertex_buffer_size, view_positions.data(), GL_STREAM_DRAW);

    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
    gl.BufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer_size, indices.data(), GL_STREAM_DRAW);

    gl.DrawElements(gl_topology, draw_count, GL_UNSIGNED_INT, nullptr);

    gl.BindVertexArray(0);
    gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    gl.UseProgram(0);
}

void renderer3_t::create_resources() {
    if (!m_opengl_renderer.window()->context_current(true)) {
        throw std::runtime_error("renderer3_t::create_resources: failed to make the OpenGL context current");
    }

    const auto& gl = m_opengl_renderer.get_gl();

    m_program = create_program(gl);

    gl.GenVertexArrays(1, &m_vertex_array);
    if (m_vertex_array == 0) {
        throw std::runtime_error("renderer3_t::create_resources: failed to create vertex array");
    }

    gl.GenBuffers(1, &m_vertex_buffer);
    if (m_vertex_buffer == 0) {
        throw std::runtime_error("renderer3_t::create_resources: failed to create vertex buffer");
    }

    gl.GenBuffers(1, &m_index_buffer);
    if (m_index_buffer == 0) {
        throw std::runtime_error("renderer3_t::create_resources: failed to create index buffer");
    }

    gl.BindVertexArray(m_vertex_array);
    gl.BindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    gl.EnableVertexAttribArray(0);
    gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(gl_position_t), nullptr);
    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
    gl.BindVertexArray(0);
    gl.BindBuffer(GL_ARRAY_BUFFER, 0);

    gl.UseProgram(m_program);

    m_viewport_size_location = gl.GetUniformLocation(m_program, "viewport_size");
    if (m_viewport_size_location < 0) {
        throw std::runtime_error("renderer3_t::create_resources: failed to find viewport_size uniform");
    }

    m_entity_color_location = gl.GetUniformLocation(m_program, "entity_color");
    if (m_entity_color_location < 0) {
        throw std::runtime_error("renderer3_t::create_resources: failed to find entity_color uniform");
    }

    m_point_shape_location = gl.GetUniformLocation(m_program, "point_shape");
    if (m_point_shape_location < 0) {
        throw std::runtime_error("renderer3_t::create_resources: failed to find point_shape uniform");
    }

    gl.UseProgram(0);
}

void renderer3_t::destroy_resources() noexcept {
    if (!m_opengl_renderer.window() || !m_opengl_renderer.window()->context_current(true)) {
        return;
    }

    const auto& gl = m_opengl_renderer.get_gl();

    if (m_index_buffer != 0) {
        gl.DeleteBuffers(1, &m_index_buffer);
        m_index_buffer = 0;
    }

    if (m_vertex_buffer != 0) {
        gl.DeleteBuffers(1, &m_vertex_buffer);
        m_vertex_buffer = 0;
    }

    if (m_vertex_array != 0) {
        gl.DeleteVertexArrays(1, &m_vertex_array);
        m_vertex_array = 0;
    }

    if (m_program != 0) {
        gl.DeleteProgram(m_program);
        m_program = 0;
    }
}

} // namespace m03gilsfsv3k34ej14ytz8a29k_tower_defense_game
