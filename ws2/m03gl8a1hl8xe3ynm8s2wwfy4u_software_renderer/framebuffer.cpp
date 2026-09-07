#include "framebuffer.h"

#include <cstddef>
#include <format>
#include <limits>
#include <utility>
#include <span>
#include <stdexcept>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

framebuffer_t::framebuffer_t(std::span<rgba8_t> pixels, int width, int height):
    framebuffer_t(texture::pixel_view_t(texture::format_t::rgba8_unorm, width < 0 ? 0 : std::size_t(width), height < 0 ? 0 : std::size_t(height), std::as_writable_bytes(pixels)))
{
    (void)pixel_count(width, height);
}

framebuffer_t::framebuffer_t(texture::pixel_view_t pixels):
    m_pixels(pixels)
{
    if (pixels.format() != texture::format_t::rgba8_unorm && pixels.format() != texture::format_t::rgba8_srgb) {
        throw std::invalid_argument(std::format("framebuffer_t rejects unsupported color attachment format {}", pixels.format()));
    }
    if (std::size_t(std::numeric_limits<int>::max()) < pixels.width() || std::size_t(std::numeric_limits<int>::max()) < pixels.height()) {
        throw std::out_of_range(std::format("framebuffer_t dimensions {}x{} cannot be represented by int", pixels.width(), pixels.height()));
    }
}

std::size_t framebuffer_t::pixel_count(int width, int height) {
    if (width < 0 || height < 0) {
        throw std::invalid_argument(std::format("framebuffer_t::pixel_count requires non-negative dimensions, got {}x{}", width, height));
    }

    const std::size_t width_size = static_cast<std::size_t>(width);
    const std::size_t height_size = static_cast<std::size_t>(height);
    if (width_size != 0 && std::numeric_limits<std::size_t>::max() / width_size < height_size) {
        throw std::length_error(std::format("framebuffer_t::pixel_count overflows size_t for dimensions {}x{}", width, height));
    }
    return width_size * height_size;
}

int framebuffer_t::width() const noexcept {
    return static_cast<int>(m_pixels.width());
}

int framebuffer_t::height() const noexcept {
    return static_cast<int>(m_pixels.height());
}

texture::pixel_view_t framebuffer_t::pixels() const noexcept {
    return m_pixels;
}

void framebuffer_t::depth(std::span<float> samples) {
    if (!samples.empty() && samples.size() != pixel_count(width(), height())) {
        throw std::invalid_argument(std::format(
            "framebuffer_t::depth has {} samples, expected {} for dimensions {}x{}",
            samples.size(),
            pixel_count(width(), height()),
            width(),
            height()
        ));
    }
    m_depth = samples;
}

std::span<float> framebuffer_t::depth() const noexcept {
    return m_depth;
}

void framebuffer_t::stencil(std::span<std::uint8_t> samples) {
    if (!samples.empty() && samples.size() != pixel_count(width(), height())) {
        throw std::invalid_argument(std::format("framebuffer_t::stencil has {} samples, expected {} for dimensions {}x{}", samples.size(), pixel_count(width(), height()), width(), height()));
    }
    m_stencil = samples;
}

std::span<std::uint8_t> framebuffer_t::stencil() const noexcept {
    return m_stencil;
}

void framebuffer_t::encoding(color_encoding_t encoding) {
    switch (encoding) {
        case color_encoding_t::linear:
        case color_encoding_t::srgb: {
            const auto format = encoding == color_encoding_t::linear ? texture::format_t::rgba8_unorm : texture::format_t::rgba8_srgb;
            m_pixels = texture::pixel_view_t(format, m_pixels.width(), m_pixels.height(), m_pixels.bytes());
        } break;
        default: {
            throw std::invalid_argument(std::format("framebuffer_t::encoding rejects invalid encoding {}", encoding));
        }
    }
}

color_encoding_t framebuffer_t::encoding() const {
    return m_pixels.format() == texture::format_t::rgba8_srgb ? color_encoding_t::srgb : color_encoding_t::linear;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
