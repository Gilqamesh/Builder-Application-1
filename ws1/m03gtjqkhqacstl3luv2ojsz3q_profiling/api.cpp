#include "api.h"

#include <exception>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

metric_t::metric_t() noexcept = default;

metric_t::metric_t(storage_t& storage, metric_base_t& metric) noexcept:
    m_storage(&storage),
    m_metric(&metric)
{
}

metric_t::~metric_t() {
    if (m_metric) {
        if (m_storage->constructing || m_storage->active != m_metric) {
            std::terminate();
        }
        stop();
    }
}

metric_t::operator bool() const noexcept {
    return m_metric != nullptr;
}

void metric_t::stop() {
    if (m_metric) {
        m_storage->stop(*m_metric);
        m_metric = nullptr;
    }
}

profiler_t::profiler_t() noexcept = default;

profiler_t::~profiler_t() {
    if (m_storage.active || m_storage.constructing) {
        std::terminate();
    }
}

bool& profiler_t::enabled() noexcept {
    return m_enabled;
}

const bool& profiler_t::enabled() const noexcept {
    return m_enabled;
}

void profiler_t::report(std::ostream& out) const {
    m_storage.require_stopped();
    if (m_storage.roots.empty()) {
        out << (m_enabled ? "No measurements.\n" : "Profiling disabled.\n");
        return;
    }
    m_storage.report(out, clock_now());
}

std::size_t profiler_t::size() const {
    m_storage.require_stopped();
    return m_storage.count;
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling
