#include "sampler.h"
#include "helpers.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace m03gt0l0q3l4b1k27eab5k7py1_texture {

sampler_t::sampler_t(filter_t filter, address_mode_t address_u, address_mode_t address_v):
    sampler_t(sampler_description_t {filter, filter, filter_t::nearest, address_u, address_v})
{
}

sampler_t::sampler_t(sampler_description_t sampler_description):
    m_description(sampler_description)
{
    if (!valid(m_description.magnification_filter) || !valid(m_description.minification_filter) || !valid(m_description.mipmap_filter)) {
        throw std::invalid_argument("sampler_t::sampler_t rejects an unknown filter");
    }
    if (!valid(m_description.address_u) || !valid(m_description.address_v)) {
        throw std::invalid_argument("sampler_t::sampler_t rejects an unknown address mode");
    }
}

filter_t sampler_t::filter() const noexcept { return m_description.magnification_filter; }
filter_t sampler_t::magnification_filter() const noexcept { return m_description.magnification_filter; }
filter_t sampler_t::minification_filter() const noexcept { return m_description.minification_filter; }
filter_t sampler_t::mipmap_filter() const noexcept { return m_description.mipmap_filter; }
address_mode_t sampler_t::address_u() const noexcept { return m_description.address_u; }
address_mode_t sampler_t::address_v() const noexcept { return m_description.address_v; }

m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4> sample(
    const texture_t& texture,
    const sampler_t& sampler,
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2> coordinates
) {
    if (texture.width() == 0 || texture.height() == 0) {
        throw std::invalid_argument("sample requires a nonempty texture");
    }
    if (!std::isfinite(coordinates[0]) || !std::isfinite(coordinates[1])) {
        throw std::domain_error("sample requires finite coordinates");
    }
    return sample_level(texture.view(), sampler.filter(), sampler.address_u(), sampler.address_v(), coordinates);
}

m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 4> sample_lod(
    const texture_t& texture,
    const sampler_t& sampler,
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<float, 2> coordinates,
    float lod
) {
    if (texture.level_count() == 0) {
        throw std::invalid_argument("sample_lod requires a nonempty texture");
    }
    if (!std::isfinite(coordinates[0]) || !std::isfinite(coordinates[1]) || !std::isfinite(lod)) {
        throw std::domain_error("sample_lod requires finite coordinates and LOD");
    }
    const auto filter = lod <= 0 ? sampler.magnification_filter() : sampler.minification_filter();
    const auto last = texture.level_count() - 1;
    const double clamped = std::clamp(double(lod), 0.0, double(last));
    const auto lower = std::size_t(std::floor(clamped));
    const auto upper = std::min(lower + 1, last);
    const float fraction = float(clamped - double(lower));
    if (sampler.mipmap_filter() == filter_t::nearest) {
        const auto level = 0.5F < fraction ? upper : lower;
        return sample_level(texture.view(level), filter, sampler.address_u(), sampler.address_v(), coordinates);
    }
    const auto first = sample_level(texture.view(lower), filter, sampler.address_u(), sampler.address_v(), coordinates);
    if (lower == upper || fraction == 0) { return first; }
    const auto second = sample_level(texture.view(upper), filter, sampler.address_u(), sampler.address_v(), coordinates);
    return interpolate(first, second, fraction);
}

} // namespace m03gt0l0q3l4b1k27eab5k7py1_texture
