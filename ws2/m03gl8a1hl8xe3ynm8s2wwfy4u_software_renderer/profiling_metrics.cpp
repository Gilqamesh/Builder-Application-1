#include "profiling_metrics.h"

namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

clear_metrics_t::clear_metrics_t(clear_target_t target) noexcept:
    m_target(target)
{
}

vertex_metrics_t::vertex_metrics_t(std::size_t expected) noexcept:
    m_expected(expected)
{
}

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
