#include "helpers.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <format>
#include <string_view>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

metric_base_t::metric_base_t(const std::type_info& type) noexcept:
    m_type(&type)
{
}

metric_base_t::~metric_base_t() = default;

void metric_base_t::start() noexcept {
    m_exceptions = std::uncaught_exceptions();
    m_start = clock_now();
}

void metric_base_t::stop() noexcept {
    m_completed = clock_now();
    m_elapsed = m_completed - m_start;
    m_total += m_elapsed;
    m_max = std::max(m_max, m_elapsed);
    ++m_count;
    m_unwinding = m_exceptions < std::uncaught_exceptions();
    m_active = false;
}

void metric_base_t::report_timing(std::ostream& out, std::chrono::nanoseconds now) const {
    out << std::format(
        "  count={} last={} mean={} max={} total={} age={}",
        m_count,
        format_elapsed(m_elapsed),
        format_elapsed(m_total / m_count),
        format_elapsed(m_max),
        format_elapsed(m_total),
        format_elapsed(now - m_completed)
    );
    if (m_unwinding) {
        out << " unwinding";
    }
    out << '\n';
}

#ifndef PROFILING_TEST_CLOCK
std::chrono::nanoseconds clock_now() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
}
#endif

std::string format_elapsed(std::chrono::duration<long double, std::nano> elapsed) {
    static constexpr std::array<std::string_view, 10> units {"ns", "us", "ms", "s", "ks", "Ms", "Gs", "Ts", "Ps", "Es"};
    auto count = elapsed.count();
    std::size_t unit = 0;
    while (unit + 1 < units.size() && 1000.0L <= std::abs(count)) {
        count /= 1000.0L;
        ++unit;
    }
    count = std::round(count * 1000.0L) / 1000.0L;
    // Rounding at a boundary should produce 1 s, for example, rather than 1000 ms.
    if (unit + 1 < units.size() && 1000.0L <= std::abs(count)) {
        count /= 1000.0L;
        ++unit;
    }
    auto text = std::format("{:.3f}", count);
    while (text.back() == '0') {
        text.pop_back();
    }
    if (text.back() == '.') {
        text.pop_back();
    }
    return std::format("{} {}", text, units[unit]);
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling
