#include "pixel_view.h"

#include <limits>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

std::size_t bytes_per_texel(format_t format) {
    switch (format) {
        case format_t::rgba8_unorm:
        case format_t::rgba8_srgb:
            return 4;
        case format_t::rgba16_float:
            return 8;
        case format_t::rgba32_float:
            return 16;
        default:
            throw std::invalid_argument(std::format("bytes_per_texel: unknown texture format {}", static_cast<int>(format)));
    }
}

const_pixel_view_t::const_pixel_view_t(format_t format, std::size_t width, std::size_t height, std::span<const std::byte> bytes):
    m_format(format),
    m_width(width),
    m_height(height),
    m_bytes(bytes)
{
    const auto texel_size = bytes_per_texel(format);
    const auto maximum = std::numeric_limits<std::size_t>::max();
    if (height != 0 && maximum / height < width) {
        throw std::length_error(std::format("const_pixel_view_t dimensions {}x{} overflow the texel count", width, height));
    }
    const auto count = width * height;
    if (maximum / texel_size < count) {
        throw std::length_error(std::format("const_pixel_view_t dimensions {}x{} with format {} overflow the byte count", width, height, format));
    }
    const auto expected = count * texel_size;
    if (bytes.size() != expected) {
        throw std::invalid_argument(std::format("const_pixel_view_t has {} bytes, expected {} for {}x{} {}", bytes.size(), expected, width, height, format));
    }
}

const_pixel_view_t::const_pixel_view_t(const pixel_view_t& pixels) noexcept:
    m_format(pixels.format()),
    m_width(pixels.width()),
    m_height(pixels.height()),
    m_bytes(pixels.bytes())
{
}

format_t const_pixel_view_t::format() const noexcept {
    return m_format;
}

std::size_t const_pixel_view_t::width() const noexcept {
    return m_width;
}

std::size_t const_pixel_view_t::height() const noexcept {
    return m_height;
}

std::span<const std::byte> const_pixel_view_t::bytes() const noexcept {
    return m_bytes;
}

pixel_view_t::pixel_view_t(format_t format, std::size_t width, std::size_t height, std::span<std::byte> bytes):
    m_format(format),
    m_width(width),
    m_height(height),
    m_bytes(bytes)
{
    // Share validation without reading storage or repeating it during view copies.
    (void)const_pixel_view_t(format, width, height, bytes);
}

format_t pixel_view_t::format() const noexcept {
    return m_format;
}

std::size_t pixel_view_t::width() const noexcept {
    return m_width;
}

std::size_t pixel_view_t::height() const noexcept {
    return m_height;
}

std::span<std::byte> pixel_view_t::bytes() const noexcept {
    return m_bytes;
}

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture
