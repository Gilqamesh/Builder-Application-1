# include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
# include <m03gt0l0q3l4b1k27eab5k7py1_texture/api.h>

# include <cmath>
# include <cstddef>
# include <cstdint>
# include <format>
# include <functional>
# include <initializer_list>
# include <limits>
# include <stdexcept>
# include <string>
# include <type_traits>
# include <utility>
# include <vector>

namespace byte_stream_api = m03gagbht2l61mj6qitacwbmea_byte_stream;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;
namespace texture_api = m03gt0l0q3l4b1k27eab5k7py1_texture;
namespace vector_api = m03ginwy24ng8o487c4beoms6l_vector;

using color_t = vector_api::vector_t<float, 4>;
using coordinates_t = vector_api::vector_t<float, 2>;

template <typename T>
concept exposes_bytes = requires(T&& texture) {
    std::forward<T>(texture).bytes();
};

byte_stream_api::byte_stream_t bytes(std::initializer_list<std::uint8_t> values) {
    std::vector<std::byte> result;
    result.reserve(values.size());
    for (const auto value : values) {
        result.push_back(static_cast<std::byte>(value));
    }
    return byte_stream_api::byte_stream_t(std::move(result));
}

void append_u32(std::vector<std::byte>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::byte>(value & 0xffu));
    bytes.push_back(static_cast<std::byte>((value >> 8) & 0xffu));
    bytes.push_back(static_cast<std::byte>((value >> 16) & 0xffu));
    bytes.push_back(static_cast<std::byte>((value >> 24) & 0xffu));
}

texture_api::texture_t rgba8_texture(
    std::size_t width,
    std::size_t height,
    std::initializer_list<std::uint8_t> values,
    texture_api::format_t format = texture_api::format_t::rgba8_unorm
) {
    return texture_api::texture_t(format, width, height, bytes(values));
}

void expect_color(const color_t& actual, const color_t& expected, float tolerance = 0.00001f) {
    for (std::size_t component = 0; component < 4; ++component) {
        test::expect(std::less_equal<>(), std::abs(actual[component] - expected[component]), tolerance);
    }
}

void test_texture_construction() {
    static_assert(!std::is_default_constructible_v<texture_api::texture_t>);
    static_assert(exposes_bytes<const texture_api::texture_t&>);
    static_assert(exposes_bytes<texture_api::texture_t&>);
    static_assert(!exposes_bytes<texture_api::texture_t>);
    static_assert(!exposes_bytes<const texture_api::texture_t>);

    test::expect(std::equal_to<>(), texture_api::bytes_per_texel(texture_api::format_t::rgba8_unorm), std::size_t(4));
    test::expect(std::equal_to<>(), texture_api::bytes_per_texel(texture_api::format_t::rgba8_srgb), std::size_t(4));
    test::expect(std::equal_to<>(), texture_api::bytes_per_texel(texture_api::format_t::rgba16_float), std::size_t(8));
    test::expect(std::equal_to<>(), texture_api::bytes_per_texel(texture_api::format_t::rgba32_float), std::size_t(16));

    const auto texture = rgba8_texture(1, 1, {1, 2, 3, 4});
    test::expect(std::equal_to<>(), texture.format(), texture_api::format_t::rgba8_unorm);
    test::expect(std::equal_to<>(), texture.width(), std::size_t(1));
    test::expect(std::equal_to<>(), texture.height(), std::size_t(1));
    test::expect(std::equal_to<>(), texture.bytes().size(), std::size_t(4));
    test::expect(std::identity(), texture.bytes()[0] == std::byte(1));

    const auto copy = texture;
    test::expect(std::identity(), copy.bytes().data() != texture.bytes().data());
    test::expect(std::equal_to<>(), copy.bytes().size(), texture.bytes().size());

    auto assigned = rgba8_texture(1, 1, {5, 6, 7, 8});
    assigned = texture;
    test::expect(std::identity(), assigned.bytes().data() != texture.bytes().data());
    test::expect(std::identity(), assigned.bytes()[0] == std::byte(1));

    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] const texture_api::texture_t invalid(texture_api::format_t::rgba8_unorm, 0, 1, bytes({}));
    });
    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] const texture_api::texture_t invalid(texture_api::format_t::rgba8_unorm, 1, 0, bytes({}));
    });
    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] const texture_api::texture_t invalid(texture_api::format_t::rgba8_unorm, 1, 1, bytes({1, 2, 3}));
    });
    test::expect_throws<std::length_error>([] {
        [[maybe_unused]] const texture_api::texture_t invalid(
            texture_api::format_t::rgba8_unorm,
            std::numeric_limits<std::size_t>::max(),
            2,
            bytes({})
        );
    });
    test::expect_throws<std::length_error>([] {
        [[maybe_unused]] const texture_api::texture_t invalid(
            texture_api::format_t::rgba8_unorm,
            std::numeric_limits<std::size_t>::max(),
            1,
            bytes({})
        );
    });
    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] const auto size = texture_api::bytes_per_texel(static_cast<texture_api::format_t>(-1));
    });
}

void test_texture_move_semantics() {
    static_assert(std::is_nothrow_move_constructible_v<texture_api::texture_t>);
    static_assert(std::is_nothrow_move_assignable_v<texture_api::texture_t>);

    auto source = rgba8_texture(1, 1, {1, 2, 3, 4}, texture_api::format_t::rgba8_srgb);
    texture_api::texture_t moved(std::move(source));

    test::expect(std::equal_to<>(), moved.format(), texture_api::format_t::rgba8_srgb);
    test::expect(std::equal_to<>(), moved.width(), std::size_t(1));
    test::expect(std::equal_to<>(), moved.height(), std::size_t(1));
    test::expect(std::equal_to<>(), moved.bytes().size(), std::size_t(4));
    test::expect(std::identity(), moved.bytes()[0] == std::byte(1));
    test::expect(std::equal_to<>(), source.format(), texture_api::format_t::rgba8_srgb);
    test::expect(std::equal_to<>(), source.width(), std::size_t(0));
    test::expect(std::equal_to<>(), source.height(), std::size_t(0));
    test::expect(std::identity(), source.bytes().empty());

    auto assigned = rgba8_texture(1, 1, {5, 6, 7, 8});
    assigned = std::move(moved);

    test::expect(std::equal_to<>(), assigned.format(), texture_api::format_t::rgba8_srgb);
    test::expect(std::equal_to<>(), assigned.width(), std::size_t(1));
    test::expect(std::equal_to<>(), assigned.height(), std::size_t(1));
    test::expect(std::identity(), assigned.bytes()[0] == std::byte(1));
    test::expect(std::equal_to<>(), moved.format(), texture_api::format_t::rgba8_srgb);
    test::expect(std::equal_to<>(), moved.width(), std::size_t(0));
    test::expect(std::equal_to<>(), moved.height(), std::size_t(0));
    test::expect(std::identity(), moved.bytes().empty());

    const texture_api::sampler_t sampler(
        texture_api::filter_t::nearest,
        texture_api::address_mode_t::clamp_to_edge,
        texture_api::address_mode_t::clamp_to_edge
    );
    test::expect_throws<std::invalid_argument>([&] {
        [[maybe_unused]] const auto color = texture_api::sample(moved, sampler, coordinates_t{0.5f, 0.5f});
    });
}

void test_sampler_construction() {
    static_assert(!std::is_default_constructible_v<texture_api::sampler_t>);

    const texture_api::sampler_t sampler(
        texture_api::filter_t::linear,
        texture_api::address_mode_t::repeat,
        texture_api::address_mode_t::clamp_to_edge
    );
    test::expect(std::equal_to<>(), sampler.filter(), texture_api::filter_t::linear);
    test::expect(std::equal_to<>(), sampler.address_u(), texture_api::address_mode_t::repeat);
    test::expect(std::equal_to<>(), sampler.address_v(), texture_api::address_mode_t::clamp_to_edge);

    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] const texture_api::sampler_t invalid(
            static_cast<texture_api::filter_t>(-1),
            texture_api::address_mode_t::repeat,
            texture_api::address_mode_t::repeat
        );
    });
    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] const texture_api::sampler_t invalid(
            texture_api::filter_t::nearest,
            static_cast<texture_api::address_mode_t>(-1),
            texture_api::address_mode_t::repeat
        );
    });
}

void test_format_decoding() {
    const texture_api::sampler_t sampler(
        texture_api::filter_t::nearest,
        texture_api::address_mode_t::clamp_to_edge,
        texture_api::address_mode_t::clamp_to_edge
    );

    const auto unorm = rgba8_texture(1, 1, {0, 127, 255, 64});
    expect_color(texture_api::sample(unorm, sampler, coordinates_t{0.5f, 0.5f}), color_t{0.0f, 127.0f / 255.0f, 1.0f, 64.0f / 255.0f});

    const auto srgb = rgba8_texture(1, 1, {188, 0, 255, 128}, texture_api::format_t::rgba8_srgb);
    expect_color(texture_api::sample(srgb, sampler, coordinates_t{0.5f, 0.5f}), color_t{0.5028865f, 0.0f, 1.0f, 128.0f / 255.0f});

    const texture_api::texture_t float16(
        texture_api::format_t::rgba16_float,
        1,
        1,
        bytes({0x00, 0x3c, 0x00, 0xc0, 0x00, 0x38, 0x00, 0x00})
    );
    expect_color(texture_api::sample(float16, sampler, coordinates_t{0.5f, 0.5f}), color_t{1.0f, -2.0f, 0.5f, 0.0f});

    const texture_api::texture_t float16_special(
        texture_api::format_t::rgba16_float,
        1,
        1,
        bytes({0x01, 0x00, 0x00, 0x80, 0x00, 0x7c, 0x00, 0x7e})
    );
    const auto special = texture_api::sample(float16_special, sampler, coordinates_t{0.5f, 0.5f});
    test::expect(std::equal_to<>(), special[0], std::ldexp(1.0f, -24));
    test::expect(std::identity(), special[1] == 0.0f && std::signbit(special[1]));
    test::expect(std::identity(), std::isinf(special[2]) && 0.0f < special[2]);
    test::expect(std::identity(), std::isnan(special[3]));

    std::vector<std::byte> float32_bytes;
    append_u32(float32_bytes, 0x3f800000u);
    append_u32(float32_bytes, 0xc0000000u);
    append_u32(float32_bytes, 0x3f000000u);
    append_u32(float32_bytes, 0x40800000u);
    const texture_api::texture_t float32(
        texture_api::format_t::rgba32_float,
        1,
        1,
        byte_stream_api::byte_stream_t(std::move(float32_bytes))
    );
    expect_color(texture_api::sample(float32, sampler, coordinates_t{0.5f, 0.5f}), color_t{1.0f, -2.0f, 0.5f, 4.0f});
}

void test_nearest_addressing() {
    const auto texture = rgba8_texture(2, 2, {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        255, 255, 255, 255
    });
    const texture_api::sampler_t clamp(
        texture_api::filter_t::nearest,
        texture_api::address_mode_t::clamp_to_edge,
        texture_api::address_mode_t::clamp_to_edge
    );
    const texture_api::sampler_t repeat(
        texture_api::filter_t::nearest,
        texture_api::address_mode_t::repeat,
        texture_api::address_mode_t::repeat
    );
    const texture_api::sampler_t repeat_u_clamp_v(
        texture_api::filter_t::nearest,
        texture_api::address_mode_t::repeat,
        texture_api::address_mode_t::clamp_to_edge
    );
    const texture_api::sampler_t clamp_u_repeat_v(
        texture_api::filter_t::nearest,
        texture_api::address_mode_t::clamp_to_edge,
        texture_api::address_mode_t::repeat
    );

    expect_color(texture_api::sample(texture, clamp, coordinates_t{0.75f, 0.25f}), color_t{0.0f, 1.0f, 0.0f, 1.0f});
    expect_color(texture_api::sample(texture, clamp, coordinates_t{0.499f, 0.25f}), color_t{1.0f, 0.0f, 0.0f, 1.0f});
    expect_color(texture_api::sample(texture, clamp, coordinates_t{0.5f, 0.25f}), color_t{0.0f, 1.0f, 0.0f, 1.0f});
    expect_color(texture_api::sample(texture, clamp, coordinates_t{-0.25f, 1.25f}), color_t{0.0f, 0.0f, 1.0f, 1.0f});
    expect_color(texture_api::sample(texture, repeat, coordinates_t{1.75f, -0.75f}), color_t{0.0f, 1.0f, 0.0f, 1.0f});
    expect_color(texture_api::sample(texture, repeat, coordinates_t{1.0f, 1.0f}), color_t{1.0f, 0.0f, 0.0f, 1.0f});
    expect_color(texture_api::sample(texture, repeat_u_clamp_v, coordinates_t{1.75f, 1.25f}), color_t{1.0f, 1.0f, 1.0f, 1.0f});
    expect_color(texture_api::sample(texture, clamp_u_repeat_v, coordinates_t{-0.25f, 1.75f}), color_t{0.0f, 0.0f, 1.0f, 1.0f});

    const auto maximum = std::numeric_limits<float>::max();
    expect_color(texture_api::sample(texture, repeat, coordinates_t{maximum, maximum}), color_t{1.0f, 0.0f, 0.0f, 1.0f});
    expect_color(texture_api::sample(texture, clamp, coordinates_t{maximum, -maximum}), color_t{0.0f, 1.0f, 0.0f, 1.0f});
}

void test_linear_filtering() {
    const auto texture = rgba8_texture(2, 2, {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        255, 255, 255, 255
    });
    const texture_api::sampler_t clamp(
        texture_api::filter_t::linear,
        texture_api::address_mode_t::clamp_to_edge,
        texture_api::address_mode_t::clamp_to_edge
    );
    const texture_api::sampler_t repeat(
        texture_api::filter_t::linear,
        texture_api::address_mode_t::repeat,
        texture_api::address_mode_t::repeat
    );
    const texture_api::sampler_t repeat_u_clamp_v(
        texture_api::filter_t::linear,
        texture_api::address_mode_t::repeat,
        texture_api::address_mode_t::clamp_to_edge
    );
    const texture_api::sampler_t clamp_u_repeat_v(
        texture_api::filter_t::linear,
        texture_api::address_mode_t::clamp_to_edge,
        texture_api::address_mode_t::repeat
    );

    expect_color(texture_api::sample(texture, clamp, coordinates_t{0.5f, 0.5f}), color_t{0.5f, 0.5f, 0.5f, 1.0f});
    expect_color(texture_api::sample(texture, repeat, coordinates_t{0.0f, 0.0f}), color_t{0.5f, 0.5f, 0.5f, 1.0f});
    expect_color(texture_api::sample(texture, repeat_u_clamp_v, coordinates_t{0.0f, 0.0f}), color_t{0.5f, 0.5f, 0.0f, 1.0f});
    expect_color(texture_api::sample(texture, clamp_u_repeat_v, coordinates_t{0.0f, 0.0f}), color_t{0.5f, 0.0f, 0.5f, 1.0f});

    const auto seam = rgba8_texture(2, 1, {
        0, 0, 0, 255,
        255, 255, 255, 255
    });
    expect_color(texture_api::sample(seam, repeat, coordinates_t{0.0f, 0.5f}), color_t{0.5f, 0.5f, 0.5f, 1.0f});
    expect_color(texture_api::sample(seam, repeat, coordinates_t{1.0f, 0.5f}), color_t{0.5f, 0.5f, 0.5f, 1.0f});
    expect_color(texture_api::sample(seam, repeat, coordinates_t{0.875f, 0.5f}), color_t{0.75f, 0.75f, 0.75f, 1.0f});
    expect_color(texture_api::sample(seam, repeat, coordinates_t{-0.125f, 0.5f}), color_t{0.75f, 0.75f, 0.75f, 1.0f});
    expect_color(texture_api::sample(seam, clamp, coordinates_t{0.0f, 0.5f}), color_t{0.0f, 0.0f, 0.0f, 1.0f});
    expect_color(texture_api::sample(seam, clamp, coordinates_t{1.0f, 0.5f}), color_t{1.0f, 1.0f, 1.0f, 1.0f});

    const auto maximum = std::numeric_limits<float>::max();
    expect_color(texture_api::sample(seam, repeat, coordinates_t{maximum, 0.5f}), color_t{0.5f, 0.5f, 0.5f, 1.0f});
    expect_color(texture_api::sample(seam, repeat, coordinates_t{-maximum, 0.5f}), color_t{0.5f, 0.5f, 0.5f, 1.0f});
    expect_color(texture_api::sample(seam, clamp, coordinates_t{maximum, 0.5f}), color_t{1.0f, 1.0f, 1.0f, 1.0f});

    const auto unassociated_alpha = rgba8_texture(2, 1, {
        255, 0, 0, 0,
        0, 0, 255, 255
    });
    expect_color(
        texture_api::sample(unassociated_alpha, clamp, coordinates_t{0.5f, 0.5f}),
        color_t{0.5f, 0.0f, 0.5f, 0.5f}
    );
}

void test_srgb_decode_before_filtering() {
    const auto texture = rgba8_texture(2, 1, {
        0, 0, 0, 255,
        255, 255, 255, 255
    }, texture_api::format_t::rgba8_srgb);
    const texture_api::sampler_t sampler(
        texture_api::filter_t::linear,
        texture_api::address_mode_t::clamp_to_edge,
        texture_api::address_mode_t::clamp_to_edge
    );

    expect_color(texture_api::sample(texture, sampler, coordinates_t{0.5f, 0.5f}), color_t{0.5f, 0.5f, 0.5f, 1.0f});
}

void test_non_finite_coordinates() {
    const auto texture = rgba8_texture(1, 1, {0, 0, 0, 255});
    const texture_api::sampler_t sampler(
        texture_api::filter_t::nearest,
        texture_api::address_mode_t::repeat,
        texture_api::address_mode_t::repeat
    );

    test::expect_throws<std::domain_error>([&] {
        [[maybe_unused]] const auto color = texture_api::sample(
            texture,
            sampler,
            coordinates_t{std::numeric_limits<float>::quiet_NaN(), 0.0f}
        );
    });
    test::expect_throws<std::domain_error>([&] {
        [[maybe_unused]] const auto color = texture_api::sample(
            texture,
            sampler,
            coordinates_t{0.0f, std::numeric_limits<float>::infinity()}
        );
    });
    test::expect_throws<std::domain_error>([&] {
        [[maybe_unused]] const auto color = texture_api::sample(
            texture,
            sampler,
            coordinates_t{-std::numeric_limits<float>::infinity(), 0.0f}
        );
    });
}

void test_formatting() {
    const auto texture = rgba8_texture(1, 1, {1, 2, 3, 4});
    const texture_api::sampler_t sampler(
        texture_api::filter_t::linear,
        texture_api::address_mode_t::repeat,
        texture_api::address_mode_t::clamp_to_edge
    );

    test::expect(std::equal_to<>(), std::format("{}", texture_api::format_t::rgba8_srgb), std::string("rgba8_srgb"));
    test::expect(std::equal_to<>(), std::format("{}", texture_api::filter_t::linear), std::string("linear"));
    test::expect(std::equal_to<>(), std::format("{}", texture_api::address_mode_t::repeat), std::string("repeat"));
    test::expect(std::equal_to<>(), std::format("{}", texture), std::string("{ format: rgba8_unorm, width: 1, height: 1 }"));
    test::expect(std::equal_to<>(), std::format("{}", sampler), std::string("{ filter: linear, address_u: repeat, address_v: clamp_to_edge }"));
}

template <typename T>
concept exposes_view = requires(T&& texture) { std::forward<T>(texture).view(); };

void test_pixel_views() {
    using texture_api::pixel_view_t;
    using texture_api::const_pixel_view_t;
    using texture_api::format_t;
    static_assert(exposes_view<texture_api::texture_t&> && exposes_view<const texture_api::texture_t&>);
    static_assert(!exposes_view<texture_api::texture_t> && !exposes_view<const texture_api::texture_t>);
    static_assert(std::is_same_v<decltype(std::declval<texture_api::texture_t&>().view()), pixel_view_t>);
    static_assert(std::is_same_v<decltype(std::declval<const texture_api::texture_t&>().view()), const_pixel_view_t>);
    static_assert(std::is_convertible_v<pixel_view_t, const_pixel_view_t>);
    static_assert(!std::is_convertible_v<const_pixel_view_t, pixel_view_t>);
    static_assert(std::is_same_v<decltype(std::declval<const pixel_view_t&>().bytes()), std::span<std::byte>>);
    std::vector<std::byte> storage(24);
    const pixel_view_t writable(format_t::rgba8_unorm, 3, 2, storage);
    const_pixel_view_t readonly = writable;
    test::expect(std::identity(), readonly.bytes().data() == storage.data());
    writable.bytes()[7] = std::byte{73};
    test::expect(std::identity(), readonly.bytes()[7] == std::byte{73});
    auto copied = writable;
    copied = pixel_view_t(format_t::rgba8_srgb, 1, 1, writable.bytes().first(4));
    test::expect(std::identity(), writable.format() == format_t::rgba8_unorm && writable.width() == 3);
    test::expect(std::identity(), copied.format() == format_t::rgba8_srgb && copied.width() == 1);
    test::expect(std::identity(), std::format("{} {}", writable, readonly).find("bytes: 24") != std::string::npos);
    const auto maximum = std::numeric_limits<std::size_t>::max();
    for (const auto format : {format_t::rgba8_unorm, format_t::rgba8_srgb, format_t::rgba16_float, format_t::rgba32_float}) {
        pixel_view_t zero_width(format, 0, maximum, {}), zero_height(format, maximum, 0, {});
        test::expect(std::identity(), zero_width.bytes().empty() && zero_height.bytes().empty());
        test::expect_throws([&] { (void)pixel_view_t(format, 0, 1, storage); });
        test::expect_throws([&] { (void)const_pixel_view_t(format, 1, 1, {}); });
        test::expect_throws([&] { (void)pixel_view_t(format, maximum, 2, {}); });
        test::expect_throws([&] { (void)const_pixel_view_t(format, maximum, 1, {}); });
        std::vector<std::byte> texel(texture_api::bytes_per_texel(format));
        test::expect_no_throw([&] { (void)pixel_view_t(format, 1, 1, texel); });
    }
    test::expect_throws([&] { (void)pixel_view_t(static_cast<format_t>(99), 0, 0, {}); });
    test::expect_throws([&] { (void)const_pixel_view_t(format_t::rgba8_unorm, 2, 3, std::span(storage).first(23)); });
    auto texture = rgba8_texture(1, 1, {1, 2, 3, 4});
    const auto borrowed = texture.view();
    borrowed.bytes()[0] = std::byte{255};
    test::expect(std::identity(), texture.bytes().data() == borrowed.bytes().data());
    auto independent = texture;
    independent.view().bytes()[0] = std::byte{0};
    test::expect(std::identity(), texture.bytes()[0] == std::byte{255});
    texture = rgba8_texture(2, 1, {128, 0, 0, 128, 0, 0, 0, 0});
    const auto rebound = texture.view();
    test::expect(std::identity(), rebound.width() == 2 && rebound.bytes().data() == texture.bytes().data());
    const texture_api::sampler_t linear(texture_api::filter_t::linear, texture_api::address_mode_t::clamp_to_edge, texture_api::address_mode_t::clamp_to_edge);
    expect_color(texture_api::sample(texture, linear, {0.5F, 0.5F}), {64.0F / 255, 0, 0, 64.0F / 255});
    texture.view().bytes()[0] = std::byte{64};
    expect_color(texture_api::sample(texture, linear, {0.25F, 0.5F}), {64.0F / 255, 0, 0, 128.0F / 255});
    auto moved = std::move(texture);
    test::expect(std::identity(), texture.view().bytes().empty() && texture.view().width() == 0);
    test::expect(std::identity(), moved.view().bytes().data() == moved.bytes().data());
    auto assigned = rgba8_texture(1, 1, {0, 0, 0, 0});
    assigned = std::move(moved);
    test::expect(std::identity(), moved.view().bytes().empty() && assigned.view().width() == 2);
    auto empty_copy = moved;
    test::expect(std::identity(), empty_copy.view().bytes().empty());
}

int main() {
    return test::run([] {
        test_pixel_views();
        test_texture_construction();
        test_texture_move_semantics();
        test_sampler_construction();
        test_format_decoding();
        test_nearest_addressing();
        test_linear_filtering();
        test_srgb_decode_before_filtering();
        test_non_finite_coordinates();
        test_formatting();
    });
}
