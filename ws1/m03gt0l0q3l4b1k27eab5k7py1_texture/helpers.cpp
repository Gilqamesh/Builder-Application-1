#include "helpers.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

static_assert(
    std::numeric_limits<float>::is_iec559 &&
    std::numeric_limits<float>::radix == 2 &&
    std::numeric_limits<float>::digits == 24 &&
    std::numeric_limits<float>::max_exponent == 128,
    "texture sampling requires IEEE-754 binary32 float"
);
static_assert(sizeof(float) == sizeof(std::uint32_t), "texture sampling requires 32-bit float storage");

bool valid(filter_t filter) {
    switch (filter) {
        case filter_t::nearest:
        case filter_t::linear:
            return true;
        default:
            return false;
    }
}

bool valid(address_mode_t address_mode) {
    switch (address_mode) {
        case address_mode_t::clamp_to_edge:
        case address_mode_t::repeat:
            return true;
        default:
            return false;
    }
}

std::uint8_t read_u8(std::span<const std::byte> bytes, std::size_t offset) {
    return std::to_integer<std::uint8_t>(bytes[offset]);
}

std::uint16_t read_u16(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(read_u8(bytes, offset)) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(read_u8(bytes, offset + 1)) << 8);
}

std::uint32_t read_u32(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(read_u8(bytes, offset)) |
        (static_cast<std::uint32_t>(read_u8(bytes, offset + 1)) << 8) |
        (static_cast<std::uint32_t>(read_u8(bytes, offset + 2)) << 16) |
        (static_cast<std::uint32_t>(read_u8(bytes, offset + 3)) << 24);
}

float decode_binary16(std::uint16_t bits) {
    const auto sign = static_cast<std::uint32_t>(bits & 0x8000u) << 16;
    const auto exponent = static_cast<std::uint32_t>((bits >> 10) & 0x1fu);
    auto fraction = static_cast<std::uint32_t>(bits & 0x03ffu);
    std::uint32_t result;

    if (exponent == 0) {
        if (fraction == 0) {
            result = sign;
        } else {
            int normalized_exponent = -14;
            while ((fraction & 0x0400u) == 0) {
                fraction <<= 1;
                --normalized_exponent;
            }
            fraction &= 0x03ffu;
            result = sign |
                (static_cast<std::uint32_t>(normalized_exponent + 127) << 23) |
                (fraction << 13);
        }
    } else if (exponent == 0x1fu) {
        result = sign | 0x7f800000u | (fraction << 13);
    } else {
        result = sign | ((exponent + 112u) << 23) | (fraction << 13);
    }

    return std::bit_cast<float>(result);
}

color_t decode_texel(format_t format, std::span<const std::byte> bytes) {
    if (bytes.size() != bytes_per_texel(format)) {
        throw std::invalid_argument("decode_texel requires exactly one texel of storage");
    }
    color_t result;

    switch (format) {
        case format_t::rgba8_unorm: return decode_rgba8<format_t::rgba8_unorm>(bytes.first<4>());
        case format_t::rgba8_srgb: return decode_rgba8<format_t::rgba8_srgb>(bytes.first<4>());
        case format_t::rgba16_float: {
            for (std::size_t component = 0; component < 4; ++component) {
                result[component] = decode_binary16(read_u16(bytes, component * 2));
            }
        } break;
        case format_t::rgba32_float: {
            for (std::size_t component = 0; component < 4; ++component) {
                result[component] = std::bit_cast<float>(read_u32(bytes, component * 4));
            }
        } break;
        default:
            throw std::invalid_argument("decode_texel: unknown texture format");
    }

    return result;
}

color_t decode_texel(const const_pixel_view_t& pixels, std::size_t x, std::size_t y) {
    const auto texel_size = bytes_per_texel(pixels.format());
    return decode_texel(pixels.format(), pixels.bytes().subspan((y * pixels.width() + x) * texel_size, texel_size));
}

double reduce_coordinate(float coordinate, address_mode_t address_mode) {
    const auto converted = static_cast<double>(coordinate);

    switch (address_mode) {
        case address_mode_t::clamp_to_edge:
            if (converted < 0.0) {
                return 0.0;
            }
            if (1.0 < converted) {
                return 1.0;
            }
            return converted;
        case address_mode_t::repeat:
            return converted - std::floor(converted);
        default:
            throw std::invalid_argument("sample: unknown address mode");
    }
}

std::size_t address_tap(std::size_t tap, std::size_t dimension, address_mode_t address_mode) {
    switch (address_mode) {
        case address_mode_t::clamp_to_edge:
            if (tap >= dimension) {
                return dimension - 1;
            }
            return tap;
        case address_mode_t::repeat:
            return tap % dimension;
        default:
            throw std::invalid_argument("sample: unknown address mode");
    }
}

std::size_t address_tap_before_zero(std::size_t dimension, address_mode_t address_mode) {
    switch (address_mode) {
        case address_mode_t::clamp_to_edge:
            return 0;
        case address_mode_t::repeat:
            return dimension - 1;
        default:
            throw std::invalid_argument("sample: unknown address mode");
    }
}

std::size_t nearest_tap(float coordinate, std::size_t dimension, address_mode_t address_mode) {
    const auto reduced = reduce_coordinate(coordinate, address_mode);
    const auto derived = std::floor(reduced * static_cast<double>(dimension));
    return address_tap(static_cast<std::size_t>(derived), dimension, address_mode);
}

linear_taps_t linear_taps(float coordinate, std::size_t dimension, address_mode_t address_mode) {
    const auto reduced = reduce_coordinate(coordinate, address_mode);
    const auto position = reduced * static_cast<double>(dimension) - 0.5;
    const auto first = std::floor(position);

    if (first < 0.0) {
        return {
            .first = address_tap_before_zero(dimension, address_mode),
            .second = address_tap(0, dimension, address_mode),
            .weight = static_cast<float>(position - first)
        };
    }

    const auto first_tap = static_cast<std::size_t>(first);

    return {
        .first = address_tap(first_tap, dimension, address_mode),
        .second = address_tap(first_tap + 1, dimension, address_mode),
        .weight = static_cast<float>(position - first)
    };
}

color_t interpolate(const color_t& first, const color_t& second, float weight) {
    color_t result;
    for (std::size_t component = 0; component < 4; ++component) {
        result[component] = std::lerp(first[component], second[component], weight);
    }
    return result;
}

color_t sample_level(const const_pixel_view_t& pixels, filter_t filter, address_mode_t address_u, address_mode_t address_v, m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2> coordinates) {
    switch (filter) {
        case filter_t::nearest: {
            return decode_texel(
                pixels,
                nearest_tap(coordinates[0], pixels.width(), address_u),
                nearest_tap(coordinates[1], pixels.height(), address_v)
            );
        }
        case filter_t::linear: {
            const auto horizontal = linear_taps(coordinates[0], pixels.width(), address_u);
            const auto vertical = linear_taps(coordinates[1], pixels.height(), address_v);
            const auto first_row = interpolate(
                decode_texel(pixels, horizontal.first, vertical.first),
                decode_texel(pixels, horizontal.second, vertical.first),
                horizontal.weight
            );
            const auto second_row = interpolate(
                decode_texel(pixels, horizontal.first, vertical.second),
                decode_texel(pixels, horizontal.second, vertical.second),
                horizontal.weight
            );
            return interpolate(first_row, second_row, vertical.weight);
        }
        default:
            throw std::invalid_argument("sample: unknown filter");
    }
}

std::uint16_t encode_binary16(float component) {
    const auto bits = std::bit_cast<std::uint32_t>(component);
    const auto sign = (bits >> 16) & 0x8000U;
    const auto exponent = (bits >> 23) & 0xffU;
    const auto fraction = bits & 0x7fffffU;
    if (exponent == 0xffU) {
        return static_cast<std::uint16_t>(sign | 0x7c00U | (fraction == 0 ? 0U : 0x200U | (fraction >> 13)));
    }
    int half_exponent = int(exponent) - 127 + 15;
    if (half_exponent <= 0) {
        if (half_exponent < -10) { return static_cast<std::uint16_t>(sign); }
        const auto significand = fraction | 0x800000U;
        const auto shift = unsigned(14 - half_exponent);
        const auto rounded = (significand + ((1U << (shift - 1)) - 1) + ((significand >> shift) & 1U)) >> shift;
        return static_cast<std::uint16_t>(sign | rounded);
    }
    auto rounded = (fraction + 0xfffU + ((fraction >> 13) & 1U)) >> 13;
    if (rounded == 0x400U) { rounded = 0; ++half_exponent; }
    if (31 <= half_exponent) { return static_cast<std::uint16_t>(sign | 0x7c00U); }
    return static_cast<std::uint16_t>(sign | (std::uint32_t(half_exponent) << 10) | rounded);
}

void encode_texel(format_t format, const color_t& color, std::span<std::byte> bytes) {
    const auto texel_size = bytes_per_texel(format);
    if (bytes.size() != texel_size) {
        throw std::invalid_argument("encode_texel requires exactly one texel of storage");
    }
    switch (format) {
        case format_t::rgba8_unorm: { encode_rgba8<format_t::rgba8_unorm>(color, bytes.first<4>()); return; }
        case format_t::rgba8_srgb: { encode_rgba8<format_t::rgba8_srgb>(color, bytes.first<4>()); return; }
        default: break;
    }
    for (std::size_t component = 0; component < 4; ++component) {
        const auto bits = format == format_t::rgba16_float ? std::uint32_t(encode_binary16(color[component])) : std::bit_cast<std::uint32_t>(color[component]);
        const auto component_size = texel_size / 4;
        for (std::size_t byte = 0; byte < component_size; ++byte) {
            bytes[component * component_size + byte] = std::byte((bits >> (byte * 8)) & 0xffU);
        }
    }
}

void encode_texel(const pixel_view_t& pixels, std::size_t x, std::size_t y, const color_t& color) {
    const auto texel_size = bytes_per_texel(pixels.format());
    encode_texel(pixels.format(), color, pixels.bytes().subspan((y * pixels.width() + x) * texel_size, texel_size));
}

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture
