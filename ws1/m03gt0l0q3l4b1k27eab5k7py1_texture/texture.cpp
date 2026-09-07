#include "texture.h"
#include "helpers.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <stdexcept>
#include <utility>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

texture_t::texture_t(format_t format, std::size_t width, std::size_t height, m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t bytes):
    texture_t(texture_description_t {format, width, height, 1}, std::move(bytes))
{
}

texture_t::texture_t(texture_description_t description, m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t bytes):
    m_bytes(std::move(bytes)),
    m_pixels(description.format, description.width, description.height, m_bytes.bytes())
{
    if (description.width == 0 || description.height == 0 || description.levels == 0) {
        throw std::invalid_argument("texture_t::texture_t requires nonzero dimensions and level count");
    }
    const auto texel_size = bytes_per_texel(description.format);
    auto width = description.width, height = description.height;
    const auto base_size = m_bytes.bytes().size();
    auto total_size = base_size;
    for (std::size_t level = 1; level < description.levels; ++level) {
        if (width == 1 && height == 1) {
            throw std::invalid_argument(std::format("texture_t::texture_t rejects {} levels beyond the full mip chain", description.levels));
        }
        width = std::max(std::size_t(1), width / 2);
        height = std::max(std::size_t(1), height / 2);
        const auto level_size = width * height * texel_size; // Bounded by the validated base size.
        if (std::numeric_limits<std::size_t>::max() - total_size < level_size) {
            throw std::length_error("texture_t::texture_t mip storage size overflows");
        }
        total_size += level_size;
    }
    if (description.levels == 1) { return; }
    std::vector<std::byte> storage(total_size);
    std::ranges::copy(m_bytes.bytes(), storage.begin());
    m_bytes = m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t(std::move(storage));
    m_pixels = pixel_view_t(description.format, description.width, description.height, m_bytes.bytes().first(base_size));
    m_levels.reserve(description.levels - 1);
    width = description.width;
    height = description.height;
    auto offset = base_size;
    for (std::size_t level = 1; level < description.levels; ++level) {
        width = std::max(std::size_t(1), width / 2);
        height = std::max(std::size_t(1), height / 2);
        const auto level_size = width * height * texel_size;
        m_levels.emplace_back(description.format, width, height, m_bytes.bytes().subspan(offset, level_size));
        offset += level_size;
    }
}

texture_t::texture_t(const texture_t& other):
    m_bytes(other.m_bytes),
    m_pixels(other.format(), other.width(), other.height(), m_bytes.bytes().first(other.bytes().size()))
{
    m_levels.reserve(other.m_levels.size());
    auto offset = other.bytes().size();
    for (const auto& pixels : other.m_levels) {
        m_levels.emplace_back(pixels.format(), pixels.width(), pixels.height(), m_bytes.bytes().subspan(offset, pixels.bytes().size()));
        offset += pixels.bytes().size();
    }
}

texture_t::texture_t(texture_t&& other) noexcept:
    m_bytes(std::move(other.m_bytes)),
    m_pixels(other.m_pixels),
    m_levels(std::move(other.m_levels))
{
    other.m_levels.clear();
    other.m_bytes.clear();
    // The source format is already valid and an empty view cannot overflow or mismatch.
    other.m_pixels = pixel_view_t(other.format(), 0, 0, {});
}

texture_t& texture_t::operator=(const texture_t& other) {
    if (this != &other) {
        texture_t copy(other);
        *this = std::move(copy);
    }
    return *this;
}

texture_t& texture_t::operator=(texture_t&& other) noexcept {
    if (this != &other) {
        m_bytes = std::move(other.m_bytes);
        m_pixels = other.m_pixels;
        m_levels = std::move(other.m_levels);
        other.m_levels.clear();
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

pixel_view_t texture_t::view(std::size_t level) & {
    if (level_count() <= level) {
        throw std::out_of_range(std::format("texture_t::view level {} exceeds allocated count {}", level, level_count()));
    }
    return level == 0 ? m_pixels : m_levels[level - 1];
}

const_pixel_view_t texture_t::view(std::size_t level) const& {
    if (level_count() <= level) {
        throw std::out_of_range(std::format("texture_t::view level {} exceeds allocated count {}", level, level_count()));
    }
    return level == 0 ? m_pixels : m_levels[level - 1];
}

std::size_t texture_t::level_count() const noexcept {
    return width() == 0 ? 0 : m_levels.size() + 1;
}

void texture_t::generate_mipmaps() {
    for (std::size_t level = 1; level < level_count(); ++level) {
        const auto source = view(level - 1);
        const auto destination = view(level);
        // Map each destination cell to its full source footprint. Fractional edge
        // weights retain odd rows/columns instead of dropping them in a 2x2 kernel.
        const double scale_x = double(source.width()) / double(destination.width());
        const double scale_y = double(source.height()) / double(destination.height());
        for (std::size_t y = 0; y < destination.height(); ++y) {
            const double top = double(y) * scale_y, bottom = double(y + 1) * scale_y;
            for (std::size_t x = 0; x < destination.width(); ++x) {
                const double left = double(x) * scale_x, right = double(x + 1) * scale_x;
                std::array<double, 4> sum {};
                double area = 0;
                const auto end_y = std::min(source.height(), std::size_t(std::ceil(bottom)));
                const auto end_x = std::min(source.width(), std::size_t(std::ceil(right)));
                for (auto source_y = std::size_t(std::floor(top)); source_y < end_y; ++source_y) {
                    const double overlap_y = std::min(bottom, double(source_y + 1)) - std::max(top, double(source_y));
                    for (auto source_x = std::size_t(std::floor(left)); source_x < end_x; ++source_x) {
                        const double overlap_x = std::min(right, double(source_x + 1)) - std::max(left, double(source_x));
                        const double weight = overlap_x * overlap_y;
                        if (weight <= 0) { continue; }
                        const auto color = decode_texel(source, source_x, source_y);
                        for (std::size_t component = 0; component < 4; ++component) {
                            sum[component] += double(color[component]) * weight;
                        }
                        area += weight;
                    }
                }
                color_t color;
                for (std::size_t component = 0; component < 4; ++component) { color[component] = float(sum[component] / area); }
                encode_texel(destination, x, y, color);
            }
        }
    }
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
