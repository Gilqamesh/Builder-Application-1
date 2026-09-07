#include "invocation.h"
#include "helpers.h"

#include <format>
#include <functional>
#include <stdexcept>
#include <utility>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

void bindings_t::texture(std::uint32_t binding, const texture::texture_t& texture) {
    m_textures.insert_or_assign(binding, std::cref(texture));
}

const texture::texture_t& bindings_t::texture(std::uint32_t binding) const {
    const auto iterator = m_textures.find(binding);
    if (iterator == m_textures.end()) {
        throw std::invalid_argument(std::format("software shader texture binding {} is missing", binding));
    }
    return iterator->second.get();
}

void bindings_t::clear_texture(std::uint32_t binding) {
    m_textures.erase(binding);
}

void bindings_t::sampler(std::uint32_t binding, const texture::sampler_t& sampler) {
    m_samplers.insert_or_assign(binding, std::cref(sampler));
}

const texture::sampler_t& bindings_t::sampler(std::uint32_t binding) const {
    const auto iterator = m_samplers.find(binding);
    if (iterator == m_samplers.end()) {
        throw std::invalid_argument(std::format("software shader sampler binding {} is missing", binding));
    }
    return iterator->second.get();
}

void bindings_t::clear_sampler(std::uint32_t binding) {
    m_samplers.erase(binding);
}

vertex_io_t::vertex_io_t(std::int32_t vertex_index, std::int32_t instance_index):
    m_vertex_index(vertex_index),
    m_instance_index(instance_index),
    m_object_to_world(identity_matrix()),
    m_world_to_clip(identity_matrix())
{
}

void vertex_io_t::reset(std::int32_t vertex_index, std::int32_t instance_index) {
    m_vertex_index = vertex_index;
    m_instance_index = instance_index;
    m_inputs.clear();
    clear_results();
}

std::int32_t vertex_io_t::vertex_index() const {
    return m_vertex_index;
}

std::int32_t vertex_io_t::instance_index() const {
    return m_instance_index;
}

void vertex_io_t::object_to_world(shader::matrix_t<float, 4, 4> matrix) {
    m_object_to_world = std::move(matrix);
}

shader::matrix_t<float, 4, 4> vertex_io_t::object_to_world() const {
    return m_object_to_world;
}

void vertex_io_t::world_to_clip(shader::matrix_t<float, 4, 4> matrix) {
    m_world_to_clip = std::move(matrix);
}

shader::matrix_t<float, 4, 4> vertex_io_t::world_to_clip() const {
    return m_world_to_clip;
}

void vertex_io_t::position(shader::vector_t<float, 4> position) {
    m_position = std::move(position);
}

shader::vector_t<float, 4> vertex_io_t::position() const {
    if (!m_position) {
        throw std::logic_error("software shader vertex position is unavailable");
    }
    return *m_position;
}

void vertex_io_t::clear_results() {
    m_outputs.clear();
    m_position.reset();
}

void vertex_io_t::reserve_outputs(std::size_t count) {
    m_outputs.reserve(count);
}

fragment_io_t::fragment_io_t(shader::vector_t<float, 4> fragment_coordinate, bool front_facing):
    m_fragment_coordinate(std::move(fragment_coordinate)),
    m_front_facing(front_facing),
    m_discarded(false)
{
}

void fragment_io_t::reset(shader::vector_t<float, 4> fragment_coordinate, bool front_facing) {
    m_fragment_coordinate = std::move(fragment_coordinate);
    m_front_facing = front_facing;
    m_inputs.clear();
    clear_results();
}

shader::vector_t<float, 4> fragment_io_t::fragment_coordinate() const {
    return m_fragment_coordinate;
}

bool fragment_io_t::front_facing() const {
    return m_front_facing;
}

void fragment_io_t::color(shader::vector_t<float, 4> color) {
    m_color = std::move(color);
}

std::optional<shader::vector_t<float, 4>> fragment_io_t::color() const {
    return m_color;
}

bool fragment_io_t::discarded() const {
    return m_discarded;
}

void fragment_io_t::discard() {
    m_outputs.clear();
    m_color.reset();
    m_discarded = true;
}

void fragment_io_t::clear_results() {
    m_outputs.clear();
    m_color.reset();
    m_discarded = false;
}

void fragment_io_t::reserve_outputs(std::size_t count) {
    m_outputs.reserve(count);
}

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader
