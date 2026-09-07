#include "texture.h"

#include <stdexcept>
#include <utility>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

texture_t::texture_t(format_t format, std::size_t width, std::size_t height, m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t bytes):
    m_bytes(std::move(bytes)),
    m_pixels(format, width, height, m_bytes.bytes())
{
    if (width == 0 || height == 0) {
        throw std::invalid_argument("texture_t::texture_t requires nonzero dimensions");
    }
}

texture_t::texture_t(const texture_t& other):
    m_bytes(other.m_bytes),
    m_pixels(other.format(), other.width(), other.height(), m_bytes.bytes())
{
}

texture_t::texture_t(texture_t&& other) noexcept:
    m_bytes(std::move(other.m_bytes)),
    m_pixels(other.m_pixels)
{
    other.m_bytes.clear();
    // The source format is already valid and an empty view cannot overflow or mismatch.
    other.m_pixels = pixel_view_t(other.format(), 0, 0, {});
}

texture_t& texture_t::operator=(const texture_t& other) {
    if (this != &other) {
        m_bytes = other.m_bytes;
        m_pixels = pixel_view_t(other.format(), other.width(), other.height(), m_bytes.bytes());
    }
    return *this;
}

texture_t& texture_t::operator=(texture_t&& other) noexcept {
    if (this != &other) {
        m_bytes = std::move(other.m_bytes);
        m_pixels = other.m_pixels;
        other.m_bytes.clear();
        other.m_pixels = pixel_view_t(other.format(), 0, 0, {});
    }
    return *this;
}

pixel_view_t texture_t::view() & noexcept {
    return m_pixels;
}

const_pixel_view_t texture_t::view() const& noexcept {
    return m_pixels;
}

format_t texture_t::format() const noexcept {
    return m_pixels.format();
}

std::size_t texture_t::width() const noexcept {
    return m_pixels.width();
}

std::size_t texture_t::height() const noexcept {
    return m_pixels.height();
}

std::span<const std::byte> texture_t::bytes() const& noexcept {
    return m_pixels.bytes();
}

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture
