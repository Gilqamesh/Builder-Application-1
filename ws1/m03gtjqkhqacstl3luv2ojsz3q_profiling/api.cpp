#include "api.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

profiler_t::profiler_t() noexcept = default;

profiler_t::~profiler_t() {
    assert(std::none_of(m_metrics.begin(), m_metrics.end(), [](const auto& metric) { return metric->m_active; }));
}

bool& profiler_t::enabled() noexcept {
    return m_enabled;
}

const bool& profiler_t::enabled() const noexcept {
    return m_enabled;
}

void profiler_t::report(std::ostream& out) const {
    require_stopped();
    out << std::format("Recording: {}\n", m_enabled ? "enabled" : "disabled");
    if (m_metrics.empty()) {
        out << "No completed metrics.\n";
        return;
    }
    const auto now = clock_now();
    std::vector<const metric_base_t*> ordered;
    ordered.reserve(m_metrics.size());
    for (const auto& metric : m_metrics) {
        ordered.push_back(metric.get());
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto* left, const auto* right) {
        return right->m_total < left->m_total;
    });
    out << "Data: latest measurement; timing: since profiler construction; inclusive durations\n";
    for (const auto* metric : ordered) {
        metric->report(out);
        metric->report_timing(out, now);
    }
}

std::size_t profiler_t::size() const {
    require_stopped();
    return m_metrics.size();
}

const metric_base_t* profiler_t::find(const std::type_info& type) const {
    require_stopped();
    for (const auto& metric : m_metrics) {
        if (*metric->m_type == type) {
            return metric.get();
        }
    }
    return nullptr;
}

void profiler_t::require_stopped() const {
    for (const auto& metric : m_metrics) {
        if (metric->m_active) {
            throw std::logic_error("profiler_t read or report requires all metrics stopped");
        }
    }
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling
