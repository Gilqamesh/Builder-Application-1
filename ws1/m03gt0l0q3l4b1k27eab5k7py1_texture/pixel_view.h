#ifndef M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_PIXEL_VIEW_H
# define M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_PIXEL_VIEW_H

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
