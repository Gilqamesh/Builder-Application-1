#ifndef M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_TEXTURE_H
# define M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_TEXTURE_H

# include "pixel_view.h"

# include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>

# include <cstddef>
# include <format>
# include <span>
# include <stdexcept>
# include <vector>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

/** @brief Describes a contiguous mip prefix with one format and floor-halved dimensions. */
struct texture_description_t {
    format_t format;
    std::size_t width;
    std::size_t height;
    std::size_t levels = 1;
};

/**
 * @brief Owns tightly packed, row-major two-dimensional RGBA texels.
 *
 * Existing view(), bytes(), width() and height() access level zero.
 * Texel `(0, 0)` begins at byte zero and `x` varies fastest. The texture defines no image-space orientation or implicit vertical flip. Construction from texel bytes requires non-zero extent. A moved-from texture retains its format but has zero extent and no bytes.
 * Copies own independent bytes at every level. Sequence writable view edits,
 * mip generation and sampling so no operation reads storage while it is written.
 * Sampling coordinates, filtering and LOD selection are defined in sampler.h.
 *
 * @code{.cpp}
 * #include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>
 * #include <m03gt0l0q3l4b1k27eab5k7py1_texture/texture.h>
 * #include <m03gt0l0q3l4b1k27eab5k7py1_texture/sampler.h>
 *
 * #include <cassert>
 * #include <cstddef>
 * #include <vector>
 *
 * int main() {
 *     using namespace m03gt0l0q3l4b1k27eab5k7py1_texture;
 *     using byte_stream_t = m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t;
 *     texture_t texture(texture_description_t {format_t::rgba8_unorm, 2, 2, 2},
 *         byte_stream_t(std::vector<std::byte>(2 * 2 * 4)));
 *     const sampler_t sampler(sampler_description_t {});
 *     const auto pixels = texture.view(0); // Writable borrow; texture stays alive.
 *     for (std::size_t offset = 0; offset < pixels.bytes().size(); offset += 4) {
 *         pixels.bytes()[offset] = std::byte{255}; // Opaque red base level.
 *         pixels.bytes()[offset + 3] = std::byte{255};
 *     }
 *     // Allocated 1x1 level is still transparent black, despite the edits above.
 *     assert(sample_lod(texture, sampler, {0.5F, 0.5F}, 1.0F) == color_t(0.0F));
 *     texture.generate_mipmaps();
 *     assert((sample_lod(texture, sampler, {0.5F, 0.5F}, 1.0F) == color_t{1, 0, 0, 1}));
 *     pixels.bytes()[0] = std::byte{0}; // Existing view remains valid.
 *     assert(sample_lod(texture, sampler, {0.5F, 0.5F}, 1.0F)[0] == 1.0F);
 *     texture.generate_mipmaps(); // Now the lower level reflects the edit.
 *     assert(sample_lod(texture, sampler, {0.5F, 0.5F}, 1.0F)[0] < 1.0F);
 * }
 * @endcode
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

    /**
     * @brief Initializes level zero from exact packed bytes and allocates zero-filled lower levels.
     *
     * Requires nonzero dimensions and 1..full-chain levels. Each next dimension is
     * max(1, previous/2), ending at 1x1; overlong chains and byte-count overflow fail.
     * Allocation and level addresses remain fixed until assignment, move or destruction.
     */
    texture_t(texture_description_t texture_description, m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t bytes);

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

    /** @brief Borrows an allocated mip level; an out-of-range level fails, including on an empty texture. */
    pixel_view_t view(std::size_t level) &;
    const_pixel_view_t view(std::size_t level) const&;
    pixel_view_t view(std::size_t level) && = delete;
    const_pixel_view_t view(std::size_t level) const&& = delete;

    std::size_t level_count() const noexcept;

    /**
     * @brief Replaces allocated lower levels with area-weighted box averages of their preceding levels.
     *
     * Includes complete odd-sized source footprints. RGB is averaged linearly and
     * encoded again for sRGB; alpha is averaged independently without association
     * changes. Float formats preserve HDR range, using IEEE arithmetic; binary16
     * conversion rounds to nearest, ties to even. UNORM rounds ties upward.
     * Storage and all borrowed views remain valid. Level zero is unchanged.
     * Edits never regenerate implicitly: lower levels retain their contents until
     * explicitly written or regenerated. Empty and one-level textures do no work.
     */
    void generate_mipmaps();

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
    // Copy assignment constructs a complete replacement before updating this owner.
    m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t m_bytes;
    pixel_view_t m_pixels;
    std::vector<pixel_view_t> m_levels; // Lower levels in the same allocation.
};

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::texture_description_t>;

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::texture_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::texture_description_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::texture_description_t& description, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ format: {}", description.format);
        out = std::format_to(out, ", width: {}", description.width);
        out = std::format_to(out, ", height: {}", description.height);
        out = std::format_to(out, ", levels: {} }}", description.levels);
        return out;
    }
};

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
        out = std::format_to(out, ", levels: {}", texture.level_count());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_TEXTURE_H
