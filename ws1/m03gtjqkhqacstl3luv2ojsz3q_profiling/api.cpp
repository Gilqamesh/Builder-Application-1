#include "api.h"

#include <cassert>
#include <stdexcept>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

metric_base_t::metric_base_t(const std::type_info& type) noexcept:
    m_type(&type)
{
}

metric_base_t::~metric_base_t() = default;

profiler_t::profiler_t() {
    // Reserve only the pointer array. Typed storage is allocated on first use.
    m_metrics.reserve(16);
}

profiler_t::~profiler_t() {
    assert(quiescent());
    while (!m_metrics.empty()) {
        m_metrics.pop_back();
    }
}

bool profiler_t::quiescent() const noexcept {
    return m_active == 0;
}

std::size_t profiler_t::size() const {
    require_quiescent();
    return m_metrics.size();
}

void profiler_t::report(std::ostream& out) const {
    require_quiescent();
    for (const auto& metric : m_metrics) {
        metric->report(out);
    }
}

void profiler_t::begin() noexcept {
    ++m_active;
}

void profiler_t::end() noexcept {
    assert(m_active != 0);
    --m_active;
}

const metric_base_t* profiler_t::find(const std::type_info& type) const {
    require_quiescent();
    for (const auto& metric : m_metrics) {
        if (*metric->m_type == type) {
            return metric.get();
        }
    }
    return nullptr;
}

void profiler_t::require_quiescent() const {
    if (!quiescent()) {
        throw std::logic_error("profiler_t read or report requires no active metrics");
    }
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling
