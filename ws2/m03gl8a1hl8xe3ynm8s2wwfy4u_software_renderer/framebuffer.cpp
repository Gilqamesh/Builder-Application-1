#include "framebuffer.h"

#include <cstddef>
#include <format>
#include <limits>
#include <utility>
#include <span>
#include <stdexcept>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

framebuffer_t::framebuffer_t(std::span<rgba8_t> pixels, int width, int height):
    m_pixels(pixels),
    m_width(width),
    m_height(height)
{
    const std::size_t expected_size = pixel_count(width, height);
    if (pixels.size() != expected_size) {
        throw std::invalid_argument(std::format(
            "framebuffer_t has {} pixels, expected {} for dimensions {}x{}",
            pixels.size(),
            expected_size,
            width,
            height
        ));
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
    return m_width;
}

int framebuffer_t::height() const noexcept {
    return m_height;
}

std::span<rgba8_t> framebuffer_t::pixels() const noexcept {
    return m_pixels;
}

void framebuffer_t::depth(std::span<float> samples) {
    if (!samples.empty() && samples.size() != m_pixels.size()) {
        throw std::invalid_argument(std::format(
            "framebuffer_t::depth has {} samples, expected {} for dimensions {}x{}",
            samples.size(),
            m_pixels.size(),
            m_width,
            m_height
        ));
    }
    m_depth = samples;
}

std::span<float> framebuffer_t::depth() const noexcept {
    return m_depth;
}

void framebuffer_t::encoding(color_encoding_t encoding) {
    switch (encoding) {
        case color_encoding_t::linear:
        case color_encoding_t::srgb: {
            m_encoding = encoding;
        } break;
        default: {
            throw std::invalid_argument(std::format("framebuffer_t::encoding rejects invalid encoding {}", encoding));
        }
    }
}

color_encoding_t framebuffer_t::encoding() const {
    return m_encoding;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
