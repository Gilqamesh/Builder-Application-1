#ifndef M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_PIXEL_VIEW_H
# define M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_PIXEL_VIEW_H

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

# include <algorithm>
# include <cmath>
# include <cstddef>
# include <format>
# include <span>
# include <stdexcept>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

/**
 * @brief Identifies the packed RGBA texel format.
 */
enum class format_t {
    /** Four RGBA UNORM8 components interpreted linearly. */
    rgba8_unorm,

    /** sRGB-encoded UNORM8 RGB and linear UNORM8 alpha. */
    rgba8_srgb,

    /** Four little-endian IEEE-754 binary16 RGBA components; no transfer function or clamping is applied. */
    rgba16_float,

    /** Four little-endian IEEE-754 binary32 RGBA components; no transfer function or clamping is applied. */
    rgba32_float
};

/**
 * @brief Returns the packed byte count of one texel in the specified format.
 *
 * Fails if format is not recognized.
 */
std::size_t bytes_per_texel(format_t format);

using color_t = m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4>;

/**
 * @brief Decodes one packed texel to linear RGBA, leaving alpha association unchanged.
 *
 * Requires exactly bytes_per_texel(format) bytes. sRGB affects only RGB;
 * floating-point formats retain their values without clamping.
 */
color_t decode_texel(format_t format, std::span<const std::byte> bytes);

/**
 * @brief Encodes linear RGBA into one packed texel without reading destination bytes.
 *
 * Requires exactly bytes_per_texel(format) bytes. UNORM channels clamp to [0,1],
 * with NaN mapped to zero; RGB uses the selected transfer function and alpha
 * stays linear. UNORM byte rounding is nearest with ties upward. Floating-point
 * formats retain their values, with binary16 rounding to nearest, ties to even.
 */
void encode_texel(format_t format, const color_t& color, std::span<std::byte> bytes);

/** @brief Decodes a fixed-size RGBA8 texel using a compile-time format under decode_texel's conversion rules. */
template <format_t Format> requires (Format == format_t::rgba8_unorm || Format == format_t::rgba8_srgb)
color_t decode_rgba8(std::span<const std::byte, 4> bytes);

/** @brief Encodes a fixed-size RGBA8 texel without reading it, under encode_texel's conversion rules. */
template <format_t Format> requires (Format == format_t::rgba8_unorm || Format == format_t::rgba8_srgb)
void encode_rgba8(const color_t& color, std::span<std::byte, 4> bytes);

class pixel_view_t;

/**
 * @brief Borrows read-only, tightly packed row-major RGBA texels without prescribing orientation or alpha association.
 *
 * Construction validates format and exact byte count, including size overflow,
 * without accessing texels. Either extent may be zero with empty storage.
 * Copies borrow the same bytes with independent metadata. The caller maintains
 * storage lifetime; changing allocation or layout requires replacing the view.
 */
class const_pixel_view_t {
public:
    const_pixel_view_t(format_t format, std::size_t width, std::size_t height, std::span<const std::byte> bytes);

    /** @brief Borrows an already validated writable view as read-only storage. */
    const_pixel_view_t(const pixel_view_t& pixels) noexcept;

    format_t format() const noexcept;
    std::size_t width() const noexcept;
    std::size_t height() const noexcept;
    std::span<const std::byte> bytes() const noexcept;

private:
    format_t m_format;
    std::size_t m_width;
    std::size_t m_height;
    std::span<const std::byte> m_bytes;
};

/**
 * @brief Borrows writable texels under const_pixel_view_t's layout and lifetime rules.
 *
 * Constness of this non-owning view does not restrict writes to its borrowed storage.
 * Metadata has no setters; assigning a different validated view replaces the description.
 */
class pixel_view_t {
public:
    pixel_view_t(format_t format, std::size_t width, std::size_t height, std::span<std::byte> bytes);

    format_t format() const noexcept;
    std::size_t width() const noexcept;
    std::size_t height() const noexcept;
    std::span<std::byte> bytes() const noexcept;

private:
    format_t m_format;
    std::size_t m_width;
    std::size_t m_height;
    std::span<std::byte> m_bytes;
};

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::format_t>;

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::const_pixel_view_t>;

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::pixel_view_t>;

} // namespace std

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

template <format_t Format> requires (Format == format_t::rgba8_unorm || Format == format_t::rgba8_srgb)
color_t decode_rgba8(std::span<const std::byte, 4> bytes) {
    color_t color;
    for (std::size_t component = 0; component < 4; ++component) {
        float channel = float(std::to_integer<unsigned char>(bytes[component])) / 255.0F;
        if constexpr (Format == format_t::rgba8_srgb) {
            if (component < 3) { channel = channel <= 0.04045F ? channel / 12.92F : std::pow((channel + 0.055F) / 1.055F, 2.4F); }
        }
        color[component] = channel;
    }
    return color;
}

template <format_t Format> requires (Format == format_t::rgba8_unorm || Format == format_t::rgba8_srgb)
void encode_rgba8(const color_t& color, std::span<std::byte, 4> bytes) {
    for (std::size_t component = 0; component < 4; ++component) {
        float channel = std::isnan(color[component]) ? 0.0F : std::clamp(color[component], 0.0F, 1.0F);
        if constexpr (Format == format_t::rgba8_srgb) {
            if (component < 3) { channel = channel <= 0.0031308F ? 12.92F * channel : 1.055F * std::pow(channel, 1.0F / 2.4F) - 0.055F; }
        }
        bytes[component] = std::byte(static_cast<unsigned char>(std::floor(channel * 255.0F + 0.5F)));
    }
}

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::format_t> {
    constexpr auto parse(std::format_parse_context& context) {
        auto iterator = context.begin();
        if (iterator != context.end() && *iterator != '}') {
            throw std::format_error("invalid format_t format specifier");
        }
        return iterator;
    }

    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::format_t& format, auto& ctx) const {
        auto out = ctx.out();

        switch (format) {
            case m03gt0l0q3l4b1k27eab5k7py1_texture::format_t::rgba8_unorm:
                out = std::format_to(out, "rgba8_unorm");
                break;
            case m03gt0l0q3l4b1k27eab5k7py1_texture::format_t::rgba8_srgb:
                out = std::format_to(out, "rgba8_srgb");
                break;
            case m03gt0l0q3l4b1k27eab5k7py1_texture::format_t::rgba16_float:
                out = std::format_to(out, "rgba16_float");
                break;
            case m03gt0l0q3l4b1k27eab5k7py1_texture::format_t::rgba32_float:
                out = std::format_to(out, "rgba32_float");
                break;
            default:
                throw std::runtime_error("formatter<format_t>::format: unknown texture format");
        }

        return out;
    }
};

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::const_pixel_view_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') { throw std::format_error("invalid const_pixel_view_t format specifier"); }
        return it;
    }
    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::const_pixel_view_t& pixels, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ format: {}", pixels.format());
        out = std::format_to(out, ", width: {}", pixels.width());
        out = std::format_to(out, ", height: {}", pixels.height());
        out = std::format_to(out, ", bytes: {} }}", pixels.bytes().size());
        return out;
    }
};

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::pixel_view_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') { throw std::format_error("invalid pixel_view_t format specifier"); }
        return it;
    }
    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::pixel_view_t& pixels, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ format: {}", pixels.format());
        out = std::format_to(out, ", width: {}", pixels.width());
        out = std::format_to(out, ", height: {}", pixels.height());
        out = std::format_to(out, ", bytes: {} }}", pixels.bytes().size());
        return out;
    }
};

} // namespace std

#endif // M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_PIXEL_VIEW_H
