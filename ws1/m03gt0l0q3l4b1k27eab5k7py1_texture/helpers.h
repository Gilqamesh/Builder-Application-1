#ifndef M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_HELPERS_H
# define M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_HELPERS_H

# include "sampler.h"

# include <cstddef>
# include <cstdint>
# include <format>
# include <span>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

using color_t = m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4>;

struct linear_taps_t {
    std::size_t first;
    std::size_t second;
    float weight;
};

bool valid(filter_t filter);
bool valid(address_mode_t address_mode);
std::uint8_t read_u8(std::span<const std::byte> bytes, std::size_t offset);
std::uint16_t read_u16(std::span<const std::byte> bytes, std::size_t offset);
std::uint32_t read_u32(std::span<const std::byte> bytes, std::size_t offset);
float decode_binary16(std::uint16_t bits);
float decode_srgb(float encoded);
color_t decode_texel(const const_pixel_view_t& pixels, std::size_t x, std::size_t y);
double reduce_coordinate(float coordinate, address_mode_t address_mode);
std::size_t address_tap(std::size_t tap, std::size_t dimension, address_mode_t address_mode);
std::size_t address_tap_before_zero(std::size_t dimension, address_mode_t address_mode);
std::size_t nearest_tap(float coordinate, std::size_t dimension, address_mode_t address_mode);
linear_taps_t linear_taps(float coordinate, std::size_t dimension, address_mode_t address_mode);
color_t interpolate(const color_t& first, const color_t& second, float weight);
color_t sample_level(const const_pixel_view_t& pixels, filter_t filter, address_mode_t address_u, address_mode_t address_v, m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2> coordinates);
std::uint16_t encode_binary16(float component);
void encode_texel(const pixel_view_t& pixels, std::size_t x, std::size_t y, const color_t& color);

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::linear_taps_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gt0l0q3l4b1k27eab5k7py1_texture::linear_taps_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gt0l0q3l4b1k27eab5k7py1_texture::linear_taps_t& linear_taps, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ first: {}", linear_taps.first);
        out = std::format_to(out, ", second: {}", linear_taps.second);
        out = std::format_to(out, ", weight: {} }}", linear_taps.weight);
        return out;
    }
};

} // namespace std

#endif // M03GT0L0Q3L4B1K27EAB5K7PY1_TEXTURE_HELPERS_H
