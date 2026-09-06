#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_CAMERA_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_CAMERA_H

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03gintxczohr63y44o77b4pyj_hyperrectangle/api.h>

# include <cstddef>
# include <format>
# include <stdexcept>
# include <typeinfo>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

/**
 * @brief Maps corresponding world and view rectangle axes without changing their orientation.
 *
 * The renderer's established 2D mapping sends increasing world Y toward increasing
 * framebuffer Y. Mathematical counter-clockwise object rotation consequently appears
 * clockwise in the top-left-origin framebuffer. For a 2D draw, vertex invocation
 * receives a world-to-clip matrix that composes this mapping with framebuffer-to-clip
 * conversion and preserves Z and W.
 */
template <typename WorldT, typename ViewT, std::size_t N>
class camera_t {
public:
    camera_t(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N>& world_rect, const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N>& view_rect);

    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N>& world_rect();
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N>& view_rect();

    const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N>& world_rect() const;
    const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N>& view_rect() const;

    m03ginwy24ng8o487c4beoms6l_vector::vector_t<ViewT, N> to_view(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<WorldT, N>& world_position) const;
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<WorldT, N> to_world(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<ViewT, N>& view_position) const;

    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N> to_view(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N>& world_rect) const;
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N> to_world(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N>& view_rect) const;

private:
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N> m_world_rect;
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N> m_view_rect;
};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <typename WorldT, typename ViewT, std::size_t N>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::camera_t<WorldT, ViewT, N>>;

} // namespace std

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

template <typename WorldT, typename ViewT, std::size_t N>
camera_t<WorldT, ViewT, N>::camera_t(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N>& world_rect, const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N>& view_rect):
    m_world_rect(world_rect),
    m_view_rect(view_rect)
{
}

template <typename WorldT, typename ViewT, std::size_t N>
m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N>& camera_t<WorldT, ViewT, N>::world_rect() {
    return m_world_rect;
}

template <typename WorldT, typename ViewT, std::size_t N>
m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N>& camera_t<WorldT, ViewT, N>::view_rect() {
    return m_view_rect;
}

template <typename WorldT, typename ViewT, std::size_t N>
const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N>& camera_t<WorldT, ViewT, N>::world_rect() const {
    return m_world_rect;
}

template <typename WorldT, typename ViewT, std::size_t N>
const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N>& camera_t<WorldT, ViewT, N>::view_rect() const {
    return m_view_rect;
}

template <typename WorldT, typename ViewT, std::size_t N>
m03ginwy24ng8o487c4beoms6l_vector::vector_t<ViewT, N> camera_t<WorldT, ViewT, N>::to_view(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<WorldT, N>& world_position) const {
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<ViewT, N> result;
    for (std::size_t i = 0; i < N; ++i) {
        const auto& camera_world_interval = m_world_rect[i];
        const auto& camera_view_interval = m_view_rect[i];
        const auto camera_view_interval_length = camera_view_interval.length();
        const auto camera_world_length = camera_world_interval.length();
        if (camera_world_length == 0) {
            throw std::runtime_error(std::format("camera_t<{}, {}, {}>::to_view: camera world rectangle has zero length in dimension {}", typeid(WorldT).name(), typeid(ViewT).name(), N, i));
        }
        result[i] = static_cast<ViewT>(camera_view_interval[0] + (world_position[i] - camera_world_interval[0]) * camera_view_interval_length / camera_world_length);
    }
    return result;
}

template <typename WorldT, typename ViewT, std::size_t N>
m03ginwy24ng8o487c4beoms6l_vector::vector_t<WorldT, N> camera_t<WorldT, ViewT, N>::to_world(const m03ginwy24ng8o487c4beoms6l_vector::vector_t<ViewT, N>& view_position) const {
    m03ginwy24ng8o487c4beoms6l_vector::vector_t<WorldT, N> result;
    for (std::size_t i = 0; i < N; ++i) {
        const auto& camera_view_interval = m_view_rect[i];
        const auto& camera_world_interval = m_world_rect[i];
        const auto camera_world_interval_length = camera_world_interval.length();
        const auto camera_view_length = camera_view_interval.length();
        if (camera_view_length == 0) {
            throw std::runtime_error(std::format("camera_t<{}, {}, {}>::to_world: camera view rectangle has zero length in dimension {}", typeid(WorldT).name(), typeid(ViewT).name(), N, i));
        }
        result[i] = static_cast<WorldT>(camera_world_interval[0] + (view_position[i] - camera_view_interval[0]) * camera_world_interval_length / camera_view_length);
    }
    return result;
}

template <typename WorldT, typename ViewT, std::size_t N>
m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N> camera_t<WorldT, ViewT, N>::to_view(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N>& world_rect) const {
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N> result;
    for (std::size_t i = 0; i < N; ++i) {
        const auto& world_interval = world_rect[i];
        const auto& camera_world_interval = m_world_rect[i];
        const auto& camera_view_interval = m_view_rect[i];
        const auto camera_view_interval_length = camera_view_interval.length();
        const auto camera_world_length = camera_world_interval.length();
        if (camera_world_length == 0) {
            throw std::runtime_error(std::format("camera_t<{}, {}, {}>::to_view: camera world rectangle has zero length in dimension {}", typeid(WorldT).name(), typeid(ViewT).name(), N, i));
        }
        result[i] = {
            static_cast<ViewT>(camera_view_interval[0] + (world_interval[0] - camera_world_interval[0]) * camera_view_interval_length / camera_world_length),
            static_cast<ViewT>(camera_view_interval[1] + (world_interval[1] - camera_world_interval[1]) * camera_view_interval_length / camera_world_length)
        };
    }
    return result;
}

template <typename WorldT, typename ViewT, std::size_t N>
m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N> camera_t<WorldT, ViewT, N>::to_world(const m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<ViewT, N>& view_rect) const {
    m03gintxczohr63y44o77b4pyj_hyperrectangle::hyperrectangle_t<WorldT, N> result;
    for (std::size_t i = 0; i < N; ++i) {
        const auto& view_interval = view_rect[i];
        const auto& camera_view_interval = m_view_rect[i];
        const auto& camera_world_interval = m_world_rect[i];
        const auto camera_world_interval_length = camera_world_interval.length();
        const auto camera_view_length = camera_view_interval.length();
        if (camera_view_length == 0) {
            throw std::runtime_error(std::format("camera_t<{}, {}, {}>::to_world: camera view rectangle has zero length in dimension {}", typeid(WorldT).name(), typeid(ViewT).name(), N, i));
        }
        result[i] = {
            static_cast<WorldT>(camera_world_interval[0] + (view_interval[0] - camera_view_interval[0]) * camera_world_interval_length / camera_view_length),
            static_cast<WorldT>(camera_world_interval[1] + (view_interval[1] - camera_view_interval[1]) * camera_world_interval_length / camera_view_length)
        };
    }
    return result;
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

namespace std {

template <typename WorldT, typename ViewT, std::size_t N>
struct formatter<m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::camera_t<WorldT, ViewT, N>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid camera_t format specifier");
        }
        return it;
    }

    auto format(const m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer::camera_t<WorldT, ViewT, N>& camera, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "world_rect: {}", camera.world_rect());
        out = std::format_to(out, ", view_rect: {}", camera.view_rect());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_CAMERA_H
