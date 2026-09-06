#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_RASTER_FIXTURES_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_RASTER_FIXTURES_H

# include <array>

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

using clip_position_fixture_t = std::array<float, 4>;
using clip_triangle_fixture_t = std::array<clip_position_fixture_t, 3>;

constexpr std::array<float, 4> fractional {-0x1.815948p-1F, 0x1.a15948p-1F, -0x1.1f139p-1F, 0x1.fe2724p-2F};
constexpr std::array<float, 4> clipped {-0x1.133098p+1F, 0x1.1b3098p+1F, -0x1.14c7a6p+1F, 0x1.0cc7a6p+1F};
constexpr clip_triangle_fixture_t crossing {{
    {-0x1.f0499ap-1F, 0x1.eff334p-1F, -0x1.dc28f6p-2F, 1.0F},
    {-0x1.eef0fcp-1F, 0x1.f0542p-1F, -0x1.5d8064p+0F, 1.0F},
    {-0x1.f02666p-1F, 0x1.efd334p-1F, 0x1.0a3d7p-4F, 1.0F}
}};
constexpr clip_triangle_fixture_t concave {{
    {-0x1.f0799ap-1F, 0x1.efe000p-1F, -0x1.0a3d70p-1F, 1.0F},
    {-0x1.ef5940p-1F, 0x1.f07d48p-1F, -0x1.80c582p+0F, 1.0F},
    {-0x1.ef9666p-1F, 0x1.efb99ap-1F, 0x1.ac0832p-1F, 1.0F}
}};
constexpr std::array<clip_triangle_fixture_t, 3> collapsed {{
    {{{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}}},
    {{{-0.5F, 0, 0, 1}, {0, 0, 0, 1}, {0.5F, 0, 0, 1}}},
    {{{0, 0, 0, 1}, {0x1p-20F, 0, 0, 1}, {0, 0x1p-20F, 0, 1}}}
}};

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_RASTER_FIXTURES_H
