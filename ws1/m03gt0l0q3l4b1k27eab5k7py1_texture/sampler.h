#ifndef M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_SAMPLER_H
# define M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_SAMPLER_H

# include "texture.h"

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

# include <format>
# include <stdexcept>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

/**
 * @brief Selects texture filtering.
 */
enum class filter_t {
    /** Nearest-neighbor filtering. */
    nearest,

    /** Bilinear filtering. */
    linear
};

/**
 * @brief Selects how texel indices outside the texture extent are addressed.
 */
enum class address_mode_t {
    /** Clamps each index to `[0, dimension - 1]`. */
    clamp_to_edge,

    /** Maps each index by mathematical modulo of the dimension. */
    repeat
};

/** @brief Describes independent magnification, minification, mip filtering and addressing. */
struct sampler_description_t {
    filter_t magnification_filter = filter_t::nearest;
    filter_t minification_filter = filter_t::nearest;
    filter_t mipmap_filter = filter_t::nearest;
    address_mode_t address_u = address_mode_t::clamp_to_edge;
    address_mode_t address_v = address_mode_t::clamp_to_edge;
};

/**
 * @brief Describes texture filtering and independent `u` and `v` addressing.
 */
class sampler_t {
public:
    /**
     * Fails if any argument is not a recognized enumerator.
     */
    sampler_t(filter_t filter, address_mode_t address_u, address_mode_t address_v);

    /** @brief Validates all description enumerators and stores immutable sampling settings. */
    explicit sampler_t(sampler_description_t sampler_description);

    /** @brief Returns the base-level magnification filter, preserving the legacy accessor. */
    filter_t filter() const noexcept;

    filter_t magnification_filter() const noexcept;
    filter_t minification_filter() const noexcept;
    filter_t mipmap_filter() const noexcept;

    address_mode_t address_u() const noexcept;

    address_mode_t address_v() const noexcept;

private:
    sampler_description_t m_description;
};

/**
 * @brief Samples level zero with the magnification filter at normalized `(u, v)` coordinates and returns linear RGBA without changing alpha association.
 *
 * `coordinates[0]` is `u`/`x` and `coordinates[1]` is `v`/`y`. Nearest-neighbor filtering addresses `floor(c * dimension)`. Bilinear filtering uses `p = c * dimension - 0.5` and addresses `floor(p)` and `floor(p) + 1` independently in each dimension. Stored sRGB color channels are decoded before filtering; RGBA components are filtered independently without premultiplication or division by alpha. Storage and sampling prescribe no alpha association; shaders and blend equations determine its interpretation.
 *
 * Fails if texture is empty.
 * Fails if either coordinate is not finite.
 */
m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4> sample(
    const texture_t& texture,
    const sampler_t& sampler,
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2> coordinates
);

/**
 * @brief Samples an explicit finite LOD, clamping level selection to allocated storage.
 *
 * Nonpositive LOD uses magnification filtering; positive LOD uses minification
 * filtering, even for one-level storage. Nearest mip filtering chooses the closest
 * level with exact halfway ties toward the lower level. Linear mip filtering blends
 * adjacent filtered levels in linear RGBA. Addressing and alpha semantics match sample().
 * Empty textures, non-finite coordinates and non-finite LOD fail.
 */
m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4> sample_lod(
    const texture_t& texture,
    const sampler_t& sampler,
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2> coordinates,
    float lod
);

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::filter_t>;

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::address_mode_t>;

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::sampler_description_t>;

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::sampler_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::filter_t> {
    constexpr auto parse(std::format_parse_context& context) {
        auto iterator = context.begin();
        if (iterator != context.end() && *iterator != '}') {
            throw std::format_error("invalid filter_t format specifier");
        }
        return iterator;
    }

    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::filter_t& filter, auto& ctx) const {
        auto out = ctx.out();

        switch (filter) {
            case m03gt0l0q3l4b1k27eab5k7py1_texture::filter_t::nearest:
                out = std::format_to(out, "nearest");
                break;
            case m03gt0l0q3l4b1k27eab5k7py1_texture::filter_t::linear:
                out = std::format_to(out, "linear");
                break;
            default:
                throw std::runtime_error("formatter<filter_t>::format: unknown filter");
        }

        return out;
    }
};

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::address_mode_t> {
    constexpr auto parse(std::format_parse_context& context) {
        auto iterator = context.begin();
        if (iterator != context.end() && *iterator != '}') {
            throw std::format_error("invalid address_mode_t format specifier");
        }
        return iterator;
    }

    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::address_mode_t& address_mode, auto& ctx) const {
        auto out = ctx.out();

        switch (address_mode) {
            case m03gt0l0q3l4b1k27eab5k7py1_texture::address_mode_t::clamp_to_edge:
                out = std::format_to(out, "clamp_to_edge");
                break;
            case m03gt0l0q3l4b1k27eab5k7py1_texture::address_mode_t::repeat:
                out = std::format_to(out, "repeat");
                break;
            default:
                throw std::runtime_error("formatter<address_mode_t>::format: unknown address mode");
        }

        return out;
    }
};

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::sampler_description_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::sampler_description_t& description, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ magnification: {}", description.magnification_filter);
        out = std::format_to(out, ", minification: {}", description.minification_filter);
        out = std::format_to(out, ", mipmap: {}", description.mipmap_filter);
        out = std::format_to(out, ", address_u: {}", description.address_u);
        out = std::format_to(out, ", address_v: {} }}", description.address_v);
        return out;
    }
};

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::sampler_t> {
    constexpr auto parse(std::format_parse_context& context) {
        auto iterator = context.begin();
        if (iterator != context.end() && *iterator != '}') {
            throw std::format_error("invalid sampler_t format specifier");
        }
        return iterator;
    }

    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::sampler_t& sampler, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "filter: {}", sampler.filter());
        out = std::format_to(out, ", minification: {}", sampler.minification_filter());
        out = std::format_to(out, ", mipmap: {}", sampler.mipmap_filter());
        out = std::format_to(out, ", address_u: {}", sampler.address_u());
        out = std::format_to(out, ", address_v: {}", sampler.address_v());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_SAMPLER_H
