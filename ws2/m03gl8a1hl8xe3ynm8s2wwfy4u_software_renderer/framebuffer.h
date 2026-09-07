#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_FRAMEBUFFER_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_FRAMEBUFFER_H

# include <cstddef>
# include <cstdint>
# include <format>
# include <span>
# include <utility>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/** @brief Selects RGB storage encoding; alpha is always linear UNORM8. */
enum class color_encoding_t {
    linear,
    srgb
};

/**
 * @brief Stores four eight-bit RGBA components without prescribing alpha association.
 */
struct rgba8_t {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};

static_assert(sizeof(rgba8_t) == 4);

/**
 * @brief Borrows row-major, top-left-origin RGBA8 color and optional float depth storage.
 *
 * Construction rejects negative dimensions, a pixel count that overflows
 * std::size_t, or storage whose size differs from width * height. Zero dimensions
 * are valid. The caller keeps storage valid during renderer operations and
 * replaces the framebuffer after reallocating color storage or changing dimensions.
 * After reallocating depth storage, rebind it through depth(). Samples remain mutable.
 * Copies share sample storage but have independent attachment views.
 * Encoding starts linear. Shaders and blend equations determine alpha association;
 * the renderer performs no implicit premultiplication or division by alpha.
 * Initialize color before blended draws; replacement equations do not read destination storage.
 * An empty depth span means no depth attachment;
 * a nonempty span must have width * height samples with the same layout as color.
 * Initialize depth before reading it through a draw. Renderer-written depths are
 * in [0,1]; comparisons against caller-written samples use ordinary float operators.
 * Construction validates color shape and leaves depth detached, without reading or
 * initializing attachment contents.
 */
class framebuffer_t {
public:
    framebuffer_t(std::span<rgba8_t> pixels, int width, int height);

    /**
     * @brief Returns width * height, rejecting negative dimensions and std::size_t overflow.
     */
    static std::size_t pixel_count(int width, int height);

    int width() const noexcept;
    int height() const noexcept;
    std::span<rgba8_t> pixels() const noexcept;

    /**
     * @brief Replaces borrowed depth storage after validating its sample count.
     *
     * A nonempty span must contain width * height samples; an empty span detaches
     * depth. Failure preserves the previous attachment. Samples are neither read
     * nor initialized. Changing a copied view does not rebind other copies.
     */
    void depth(std::span<float> samples);
    std::span<float> depth() const noexcept;

    /**
     * @brief Changes this view's RGB interpretation without converting or initializing storage.
     *
     * Rejects invalid encoding before changing the view. Copies retain independent
     * encoding; changing an external view does not rebind the renderer's copy.
     */
    void encoding(color_encoding_t encoding);
    color_encoding_t encoding() const;

private:
    std::span<rgba8_t> m_pixels;
    std::span<float> m_depth;
    int m_width;
    int m_height;
    color_encoding_t m_encoding = color_encoding_t::linear;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_encoding_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::rgba8_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::framebuffer_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_encoding_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid color_encoding_t format specifier");
        }
        return it;
    }

    auto format(m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_encoding_t option, auto& ctx) const {
        auto out = ctx.out();
        switch (option) {
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_encoding_t::linear: {
                out = std::format_to(out, "linear");
            } break;
            case m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::color_encoding_t::srgb: {
                out = std::format_to(out, "srgb");
            } break;
            default: {
                out = std::format_to(out, "invalid({})", std::to_underlying(option));
            } break;
        }
        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::rgba8_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid rgba8_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::rgba8_t& color, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "red: {}", color.red);
        out = std::format_to(out, ", green: {}", color.green);
        out = std::format_to(out, ", blue: {}", color.blue);
        out = std::format_to(out, ", alpha: {}", color.alpha);
        out = std::format_to(out, " }}");

        return out;
    }
};

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::framebuffer_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid framebuffer_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::framebuffer_t& framebuffer, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "width: {}", framebuffer.width());
        out = std::format_to(out, ", height: {}", framebuffer.height());
        out = std::format_to(out, ", pixels: {}", framebuffer.pixels().size());
        out = std::format_to(out, ", depth: {}", framebuffer.depth().size());
        out = std::format_to(out, ", encoding: {}", framebuffer.encoding());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_FRAMEBUFFER_H
