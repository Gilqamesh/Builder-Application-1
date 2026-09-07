# include "sampler.h"

# include <bit>
# include <cmath>
# include <cstddef>
# include <cstdint>
# include <limits>
# include <stdexcept>

namespace {

namespace texture_api = m03gt0l0q3l4b1k27eab5k7py1_texture;
namespace vector_api = m03ginwy24ng8o487c4beoms6l_vector;

using color_t = vector_api::vector_t<float, 4>;

static_assert(
    std::numeric_limits<float>::is_iec559 &&
    std::numeric_limits<float>::radix == 2 &&
    std::numeric_limits<float>::digits == 24 &&
    std::numeric_limits<float>::max_exponent == 128,
    "texture sampling requires IEEE-754 binary32 float"
);
static_assert(sizeof(float) == sizeof(std::uint32_t), "texture sampling requires 32-bit float storage");

bool valid(texture_api::filter_t filter) {
    switch (filter) {
        case texture_api::filter_t::nearest:
        case texture_api::filter_t::linear:
            return true;
        default:
            return false;
    }
}

bool valid(texture_api::address_mode_t address_mode) {
    switch (address_mode) {
        case texture_api::address_mode_t::clamp_to_edge:
        case texture_api::address_mode_t::repeat:
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

float decode_srgb(float encoded) {
    if (encoded <= 0.04045f) {
        return encoded / 12.92f;
    }
    return std::pow((encoded + 0.055f) / 1.055f, 2.4f);
}

color_t decode_texel(const texture_api::const_pixel_view_t& pixels, std::size_t x, std::size_t y) {
    const auto texel_size = texture_api::bytes_per_texel(pixels.format());
    const auto offset = (y * pixels.width() + x) * texel_size;
    const auto bytes = pixels.bytes();
    color_t result;

    switch (pixels.format()) {
        case texture_api::format_t::rgba8_unorm:
        case texture_api::format_t::rgba8_srgb: {
            for (std::size_t component = 0; component < 4; ++component) {
                result[component] = static_cast<float>(read_u8(bytes, offset + component)) / 255.0f;
            }
            if (pixels.format() == texture_api::format_t::rgba8_srgb) {
                for (std::size_t component = 0; component < 3; ++component) {
                    result[component] = decode_srgb(result[component]);
                }
            }
        } break;
        case texture_api::format_t::rgba16_float: {
            for (std::size_t component = 0; component < 4; ++component) {
                result[component] = decode_binary16(read_u16(bytes, offset + component * 2));
            }
        } break;
        case texture_api::format_t::rgba32_float: {
            for (std::size_t component = 0; component < 4; ++component) {
                result[component] = std::bit_cast<float>(read_u32(bytes, offset + component * 4));
            }
        } break;
        default:
            throw std::invalid_argument("sample: unknown texture format");
    }

    return result;
}

double reduce_coordinate(float coordinate, texture_api::address_mode_t address_mode) {
    const auto converted = static_cast<double>(coordinate);

    switch (address_mode) {
        case texture_api::address_mode_t::clamp_to_edge:
            if (converted < 0.0) {
                return 0.0;
            }
            if (1.0 < converted) {
                return 1.0;
            }
            return converted;
        case texture_api::address_mode_t::repeat:
            return converted - std::floor(converted);
        default:
            throw std::invalid_argument("sample: unknown address mode");
    }
}

std::size_t address_tap(std::size_t tap, std::size_t dimension, texture_api::address_mode_t address_mode) {
    switch (address_mode) {
        case texture_api::address_mode_t::clamp_to_edge:
            if (tap >= dimension) {
                return dimension - 1;
            }
            return tap;
        case texture_api::address_mode_t::repeat:
            return tap % dimension;
        default:
            throw std::invalid_argument("sample: unknown address mode");
    }
}

std::size_t address_tap_before_zero(std::size_t dimension, texture_api::address_mode_t address_mode) {
    switch (address_mode) {
        case texture_api::address_mode_t::clamp_to_edge:
            return 0;
        case texture_api::address_mode_t::repeat:
            return dimension - 1;
        default:
            throw std::invalid_argument("sample: unknown address mode");
    }
}

std::size_t nearest_tap(float coordinate, std::size_t dimension, texture_api::address_mode_t address_mode) {
    const auto reduced = reduce_coordinate(coordinate, address_mode);
    const auto derived = std::floor(reduced * static_cast<double>(dimension));
    return address_tap(static_cast<std::size_t>(derived), dimension, address_mode);
}

struct linear_taps_t {
    std::size_t first;
    std::size_t second;
    float weight;
};

linear_taps_t linear_taps(float coordinate, std::size_t dimension, texture_api::address_mode_t address_mode) {
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

} // namespace

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

sampler_t::sampler_t(filter_t filter, address_mode_t address_u, address_mode_t address_v):
    m_filter(filter),
    m_address_u(address_u),
    m_address_v(address_v)
{
    if (!valid(filter)) {
        throw std::invalid_argument("sampler_t::sampler_t: unknown filter");
    }
    if (!valid(address_u) || !valid(address_v)) {
        throw std::invalid_argument("sampler_t::sampler_t: unknown address mode");
    }
}

filter_t sampler_t::filter() const noexcept {
    return m_filter;
}

address_mode_t sampler_t::address_u() const noexcept {
    return m_address_u;
}

address_mode_t sampler_t::address_v() const noexcept {
    return m_address_v;
}

m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4> sample(
    const texture_t& texture,
    const sampler_t& sampler,
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2> coordinates
) {
    if (texture.width() == 0 || texture.height() == 0) {
        throw std::invalid_argument("sample: texture must not be empty");
    }

    if (!std::isfinite(coordinates[0]) || !std::isfinite(coordinates[1])) {
        throw std::domain_error("sample: coordinates must be finite");
    }

    const auto pixels = texture.view();
    switch (sampler.filter()) {
        case filter_t::nearest: {
            return decode_texel(
                pixels,
                nearest_tap(coordinates[0], texture.width(), sampler.address_u()),
                nearest_tap(coordinates[1], texture.height(), sampler.address_v())
            );
        }
        case filter_t::linear: {
            const auto horizontal = linear_taps(coordinates[0], texture.width(), sampler.address_u());
            const auto vertical = linear_taps(coordinates[1], texture.height(), sampler.address_v());
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

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture
