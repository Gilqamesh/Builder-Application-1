#include "helpers.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <string_view>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

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
