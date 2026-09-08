#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_FRAMEBUFFER_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_FRAMEBUFFER_H

# include <m03gt0l0q3l4b1k27eab5k7py1_texture/pixel_view.h>

# include <cstddef>
# include <cstdint>
# include <format>
# include <span>
# include <utility>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

namespace texture = m03gt0l0q3l4b1k27eab5k7py1_texture;

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
 * @brief Borrows top-left-origin RGBA8 color and optional independent depth/stencil storage.
 *
 * Construction rejects negative dimensions, a pixel count that overflows
 * std::size_t, or storage whose size differs from width * height. Zero dimensions
 * are valid. The caller keeps storage valid during renderer operations and
 * replaces the framebuffer after reallocating color storage or changing dimensions.
 * After reallocating depth storage, rebind it through depth(). Samples remain mutable.
 * Copies share sample storage but have independent attachment views.
 * The span constructor starts linear; a pixel view supplies its own format. Shaders and blend equations determine alpha association;
 * the renderer performs no implicit premultiplication or division by alpha.
 * Initialize color before blended draws; replacement equations do not read destination storage.
 * An empty depth span means no depth attachment;
 * a nonempty span must have width * height samples with the same layout as color.
 * Initialize depth before reading it through a draw. Renderer-written depths are
 * in [0,1]; comparisons against caller-written samples use ordinary float operators.
 * Construction validates color shape and leaves depth/stencil detached, without reading or
 * initializing attachment contents.
 */
class framebuffer_t {
public:
    framebuffer_t(std::span<rgba8_t> pixels, int width, int height);

    /**
     * @brief Borrows validated RGBA8 storage, deriving dimensions and encoding from its view.
     *
     * Supports rgba8_unorm and rgba8_srgb, including zero extents. Dimensions must
     * fit int. Depth and stencil start detached; replacing owner storage requires
     * rebinding this view. Construction does not acquire ownership or initialize bytes.
     */
    explicit framebuffer_t(texture::pixel_view_t pixels);

    /**
     * @brief Returns width * height, rejecting negative dimensions and std::size_t overflow.
     */
    static std::size_t pixel_count(int width, int height);

    int width() const noexcept;
    int height() const noexcept;
    /** @brief Returns this attachment's writable view by value, including through a const framebuffer. */
    texture::pixel_view_t pixels() const noexcept;

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
     * @brief Replaces borrowed stencil storage after validating its sample count.
     *
     * A nonempty span contains width * height bytes in color's row order; an empty
     * span detaches stencil. Failure preserves the previous view. No samples are
     * initialized. The caller maintains storage lifetime and rebinds after allocation
     * changes. Copies share samples with independent views. Initialize before reading.
     */
    void stencil(std::span<std::uint8_t> samples);
    std::span<std::uint8_t> stencil() const noexcept;

    /**
     * @brief Changes this view's RGB interpretation without converting or initializing storage.
     *
     * Accepts rgba8_unorm and rgba8_srgb; other formats fail before changing the view. Copies retain independent
     * encoding; changing an external view does not rebind the renderer's copy.
     * pixels().format() always reflects this interpretation. The owning texture's
     * format is unchanged; callers coordinate explicit reinterpretation with sampling.
     */
    void format(texture::format_t format);
    texture::format_t format() const;

private:
    texture::pixel_view_t m_pixels;
    std::span<float> m_depth;
    std::span<std::uint8_t> m_stencil;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::rgba8_t>;

template <>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::framebuffer_t>;

} // namespace std

namespace std {

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
        out = std::format_to(out, ", pixels: {}", framebuffer.pixels());
        out = std::format_to(out, ", depth: {}", framebuffer.depth().size());
        out = std::format_to(out, ", stencil: {}", framebuffer.stencil().size());
        out = std::format_to(out, ", format: {}", framebuffer.format());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_FRAMEBUFFER_H
