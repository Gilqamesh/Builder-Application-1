#ifndef M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_TEXTURE_H
# define M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_TEXTURE_H

# include "pixel_view.h"

# include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>

# include <cstddef>
# include <format>
# include <span>
# include <stdexcept>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

/**
 * @brief Owns tightly packed, row-major two-dimensional RGBA texels.
 *
 * Texel `(0, 0)` begins at byte zero and `x` varies fastest. The texture defines no image-space orientation or implicit vertical flip. Construction from texel bytes requires non-zero extent. A moved-from texture retains its format but has zero extent and no bytes.
 */
class texture_t {
public:
    /**
     * @brief Constructs a texture from packed texel bytes.
     *
     * Fails if the format is not recognized, either dimension is zero, or the byte count does not match the format and dimensions.
     * Fails if the required byte count is not representable by std::size_t.
     */
    texture_t(
        format_t format,
        std::size_t width,
        std::size_t height,
        m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t bytes
    );

    texture_t(const texture_t& other);

    texture_t(texture_t&& other) noexcept;

    texture_t& operator=(const texture_t& other);

    texture_t& operator=(texture_t&& other) noexcept;

    /**
     * @brief Returns validated borrowed texel access without allocating or copying storage.
     *
     * Mutable textures expose writable views; const textures expose read-only views.
     * Assignment, moving from the texture, or destruction invalidates borrowed views.
     * Pixel edits through a view preserve allocation, dimensions, format and other views.
     * A moved-from texture returns an empty view. Views cannot be obtained from temporaries.
     */
    pixel_view_t view() & noexcept;
    const_pixel_view_t view() const& noexcept;
    pixel_view_t view() && = delete;
    const_pixel_view_t view() const&& = delete;

    format_t format() const noexcept;

    std::size_t width() const noexcept;

    std::size_t height() const noexcept;

    /**
     * @brief Provides read-only access to the packed texel bytes.
     *
     * The view remains valid until the texture is assigned, moved from, or destroyed.
     */
    std::span<const std::byte> bytes() const& noexcept;
    std::span<const std::byte> bytes() const&& = delete;

private:
    // Copy assignment updates the potentially throwing storage before the non-throwing metadata so an allocation failure preserves the invariant.
    m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t m_bytes;
    pixel_view_t m_pixels;
};

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::texture_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::texture_t> {
    constexpr auto parse(std::format_parse_context& context) {
        auto iterator = context.begin();
        if (iterator != context.end() && *iterator != '}') {
            throw std::format_error("invalid texture_t format specifier");
        }
        return iterator;
    }

    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::texture_t& texture, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "format: {}", texture.format());
        out = std::format_to(out, ", width: {}", texture.width());
        out = std::format_to(out, ", height: {}", texture.height());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_TEXTURE_H
