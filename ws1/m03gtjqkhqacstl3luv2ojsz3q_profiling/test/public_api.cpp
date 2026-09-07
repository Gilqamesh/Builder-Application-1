#include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <exception>
#include <iostream>
#include <memory>
#include <new>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <sys/wait.h>
#include <unistd.h>

namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

bool allocation_forbidden = false;
std::ptrdiff_t allocations_before_failure = -1;
std::size_t allocations = 0;
std::size_t clock_calls = 0;
std::int64_t tick = 0;
std::size_t live_metrics = 0;
std::size_t formats = 0;

#ifdef PROFILING_TEST_CLOCK
std::chrono::nanoseconds clock_now() noexcept {
    ++clock_calls;
    return std::chrono::nanoseconds(tick);
}
#endif

struct timing_t {};
struct multiline_t {};

struct counted_t {
    explicit counted_t(int number) noexcept: m_number(number) { ++live_metrics; tick += 5; }
    counted_t(const counted_t&) = delete;
    counted_t& operator=(const counted_t&) = delete;
    ~counted_t() { --live_metrics; tick += 3; }
    int m_number;
};

struct forwarded_t {
    forwarded_t(int& borrowed, std::unique_ptr<int>&& owned) noexcept:
        m_borrowed(&borrowed), m_owned(std::move(owned)) {}
    int* m_borrowed;
    std::unique_ptr<int> m_owned;
};

struct alignas(256) aligned_t { int m_number = 42; };
struct failing_t {};
struct missing_t {};
struct throwing_constructor_t { throwing_constructor_t() noexcept(false) {} };
struct throwing_move_t {
    throwing_move_t() = default;
    throwing_move_t(throwing_move_t&&) noexcept(false) {}
    throwing_move_t& operator=(throwing_move_t&&) noexcept = default;
};
struct throwing_assignment_t {
    throwing_assignment_t() = default;
    throwing_assignment_t(throwing_assignment_t&&) noexcept = default;
    throwing_assignment_t& operator=(throwing_assignment_t&&) noexcept(false) { return *this; }
};
struct throwing_destructor_t { ~throwing_destructor_t() noexcept(false) {} };

template <std::size_t N>
struct numbered_t {};

struct observing_t {
    explicit observing_t(profiler_t& profiler) noexcept;
    profiler_t* m_profiler;
};

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<profiling::timing_t> : formatter<string_view> {
    auto format(const profiling::timing_t&, auto& ctx) const {
        return formatter<string_view>::format("timing", ctx);
    }
};

template <>
struct formatter<profiling::multiline_t> : formatter<string_view> {
    auto format(const profiling::multiline_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "renderer.very_long_rasterization_metric\nsecond\tthird\r\nfourth\n");
        return out;
    }
};

template <>
struct formatter<profiling::counted_t> : formatter<int> {
    auto format(const profiling::counted_t& metrics, auto& ctx) const {
        ++profiling::formats;
        return formatter<int>::format(metrics.m_number, ctx);
    }
};

template <>
struct formatter<profiling::forwarded_t> : formatter<int> {
    auto format(const profiling::forwarded_t& metrics, auto& ctx) const {
        return formatter<int>::format(*metrics.m_borrowed + *metrics.m_owned, ctx);
    }
};

template <>
struct formatter<profiling::aligned_t> : formatter<int> {
    auto format(const profiling::aligned_t& metrics, auto& ctx) const {
        return formatter<int>::format(metrics.m_number, ctx);
    }
};

template <>
struct formatter<profiling::failing_t> : formatter<string_view> {
    auto format(const profiling::failing_t&, auto& ctx) const {
        throw std::logic_error("metric formatter failed");
        return ctx.out();
    }
};

template <std::size_t N>
struct formatter<profiling::numbered_t<N>> : formatter<size_t> {
    auto format(const profiling::numbered_t<N>&, auto& ctx) const {
        return formatter<size_t>::format(N, ctx);
    }
};

template <>
struct formatter<profiling::observing_t> : formatter<string_view> {
    auto format(const profiling::observing_t&, auto& ctx) const {
        return formatter<string_view>::format("observing", ctx);
    }
};

template <>
struct formatter<profiling::throwing_constructor_t> : formatter<string_view> {
    auto format(const profiling::throwing_constructor_t&, auto& ctx) const {
        return formatter<string_view>::format("invalid", ctx);
    }
};

template <>
struct formatter<profiling::throwing_move_t> : formatter<string_view> {
    auto format(const profiling::throwing_move_t&, auto& ctx) const {
        return formatter<string_view>::format("invalid", ctx);
    }
};

template <>
struct formatter<profiling::throwing_assignment_t> : formatter<string_view> {
    auto format(const profiling::throwing_assignment_t&, auto& ctx) const {
        return formatter<string_view>::format("invalid", ctx);
    }
};

template <>
struct formatter<profiling::throwing_destructor_t> : formatter<string_view> {
    auto format(const profiling::throwing_destructor_t&, auto& ctx) const {
        return formatter<string_view>::format("invalid", ctx);
    }
};

} // namespace std

void* operator new(std::size_t size) {
    if (profiling::allocation_forbidden) { std::abort(); }
    if (profiling::allocations_before_failure == 0) { throw std::bad_alloc(); }
    if (0 < profiling::allocations_before_failure) { --profiling::allocations_before_failure; }
    ++profiling::allocations;
    if (auto* memory = std::malloc(size == 0 ? 1 : size)) { return memory; }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void* operator new(std::size_t size, std::align_val_t alignment) {
    if (profiling::allocation_forbidden) { std::abort(); }
    if (profiling::allocations_before_failure == 0) { throw std::bad_alloc(); }
    if (0 < profiling::allocations_before_failure) { --profiling::allocations_before_failure; }
    ++profiling::allocations;
    void* memory = nullptr;
    if (posix_memalign(&memory, static_cast<std::size_t>(alignment), size == 0 ? 1 : size) == 0) { return memory; }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size, std::align_val_t alignment) { return ::operator new(size, alignment); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete(void* memory, std::align_val_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::align_val_t) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t, std::align_val_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept { std::free(memory); }

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

void require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) { throw std::runtime_error(std::format("profiling contract check failed at line {}", location.line())); }
}

template <typename F>
void rejects(F&& operation) {
    bool failed = false;
    try { operation(); } catch (const std::logic_error&) { failed = true; }
    require(failed);
}

observing_t::observing_t(profiler_t& profiler) noexcept: m_profiler(&profiler) {
    rejects([&] { (void)profiler.size(); });
    rejects([&] { auto metric = profiler.metric<observing_t>(profiler); });
}

void test_persistence() {
    static_assert(!std::is_copy_constructible_v<profiler_t> && !std::is_move_constructible_v<profiler_t>);
    static_assert(!std::is_copy_constructible_v<metric_t> && !std::is_move_constructible_v<metric_t>);
    profiler_t profiler;
    require(profiler.enabled() && profiler.size() == 0);
    require(!profiler.metrics<counted_t>() && !profiler.elapsed<counted_t>() && !profiler.unwinding<counted_t>());
    tick = 10;
    const auto formatted = formats;
    {
        auto metric = profiler.metric<counted_t>(7);
        require(live_metrics == 1);
        profiler.enabled() = false;
        bool updated = false;
        metric.update<counted_t>([&updated](const counted_t& metric) {
            require(metric.m_number == 7);
            updated = true;
        });
        require(updated && metric);
        rejects([&] { (void)profiler.metrics<counted_t>(); });
        rejects([&] { (void)profiler.elapsed<counted_t>(); });
        rejects([&] { (void)profiler.unwinding<counted_t>(); });
        rejects([&] { (void)profiler.size(); });
        rejects([&] { profiler.report(std::cout); });
        tick = 50;
        allocation_forbidden = true;
        metric.stop();
        require(!metric && live_metrics == 1 && formats == formatted);
        const auto calls = clock_calls;
        metric.stop();
        metric.update<counted_t>([](counted_t&) { std::abort(); });
        require(clock_calls == calls);
        allocation_forbidden = false;
        tick = 500;
    }
    require(profiler.size() == 1 && profiler.metrics<counted_t>()->m_number == 7);
    require(profiler.unwinding<counted_t>() == false);
#ifdef PROFILING_TEST_CLOCK
    require(profiler.elapsed<counted_t>()->count() == 35);
#endif
    const auto count = allocations;
    const auto calls = clock_calls;
    allocation_forbidden = true;
    {
        auto metric = profiler.metric<counted_t>(99);
        metric.update<counted_t>([](counted_t&) { std::abort(); });
        metric.stop();
    }
    require(clock_calls == calls && allocations == count);
    require(profiler.metrics<counted_t>()->m_number == 7);
    const auto* stored = profiler.metrics<counted_t>();
    profiler.enabled() = true;
    tick = 100;
    {
        auto metric = profiler.metric<counted_t>(20);
        require(live_metrics == 1 && tick == 100);
        metric.update<counted_t>([stored](counted_t& metric) {
            require(&metric == stored && metric.m_number == 7);
            ++metric.m_number;
        });
        tick = 125;
    }
    allocation_forbidden = false;
    require(allocations == count && profiler.size() == 1 && live_metrics == 1);
    require(profiler.metrics<counted_t>() == stored && stored->m_number == 8);
#ifdef PROFILING_TEST_CLOCK
    require(profiler.elapsed<counted_t>()->count() == 25);
#endif
    std::ostringstream report;
    profiler.report(report);
    require(formats == formatted + 1);
    const auto report_text = report.str();
    require(report_text.starts_with("Metric") && report_text.find("\n8 ") != std::string::npos);
    require(std::count(report_text.begin(), report_text.end(), '\n') == 2);
    profiler.report(report);
    require(formats == formatted + 2);
    require(std::format("{}", profiler).find("enabled: true") != std::string::npos);
}

void test_lifetimes() {
    profiler_t profiler;
    {
        auto outer_metric = profiler.metric<timing_t>();
        auto inner_metric = outer_metric.metric<counted_t>(11);
        rejects([&] { outer_metric.stop(); });
        rejects([&] { auto sibling = outer_metric.metric<numbered_t<1>>(); });
        rejects([&] { auto root = profiler.metric<numbered_t<1>>(); });
        rejects([&] { inner_metric.update<timing_t>([](timing_t&) { std::abort(); }); });
        inner_metric.update<counted_t>([](counted_t& counted) { ++counted.m_number; });
        inner_metric.stop();
        rejects([&] { auto child = inner_metric.metric<timing_t>(); });
        auto next_metric = outer_metric.metric<counted_t>(99);
        inner_metric.stop();
        rejects([&] { auto child = inner_metric.metric<timing_t>(); });
        next_metric.update<counted_t>([](const counted_t& counted) { require(counted.m_number == 12); });
    }
    require(profiler.size() == 2);
    require(profiler.metrics<timing_t, counted_t>()->m_number == 12);
    require(!profiler.metrics<counted_t>());
    try {
        auto outer_metric = profiler.metric<timing_t>();
        auto inner_metric = outer_metric.metric<counted_t>(99);
        inner_metric.update<counted_t>([](counted_t& counted) {
            counted.m_number = 44;
            throw 1;
        });
    } catch (int) {}
    require(profiler.unwinding<timing_t>() == true);
    require(profiler.unwinding<timing_t, counted_t>() == true);
    require(profiler.metrics<timing_t, counted_t>()->m_number == 44);
    { auto metric = profiler.metric<observing_t>(profiler); }
    { auto metric = profiler.metric<observing_t>(profiler); }
}

void test_paths() {
    profiler_t profiler;
    {
        auto frame = profiler.metric<timing_t>();
        auto draw = frame.metric<numbered_t<0>>();
        auto counter = draw.metric<counted_t>(7);
        counter.update<counted_t>([](counted_t& counted) { ++counted.m_number; });
    }
    {
        auto frame = profiler.metric<numbered_t<1>>();
        auto draw = frame.metric<numbered_t<0>>();
        auto counter = draw.metric<counted_t>(20);
        counter.update<counted_t>([](counted_t& counted) { counted.m_number += 3; });
    }
    {
        auto frame = profiler.metric<timing_t>();
        auto draw = frame.metric<numbered_t<0>>();
        auto counter = draw.metric<counted_t>(999);
        counter.update<counted_t>([](counted_t& counted) { ++counted.m_number; });
    }
    require(profiler.size() == 6 && live_metrics == 2);
    const auto* first = profiler.metrics<timing_t, numbered_t<0>, counted_t>();
    const auto* second = profiler.metrics<numbered_t<1>, numbered_t<0>, counted_t>();
    require(first != second && first->m_number == 9 && second->m_number == 23);
    require(!profiler.metrics<timing_t, counted_t>());
    require(!profiler.elapsed<numbered_t<2>, counted_t>());
    {
        auto outer = profiler.metric<timing_t>();
        auto inner = outer.metric<timing_t>();
        auto deepest = inner.metric<timing_t>();
    }
    require(profiler.size() == 8 && profiler.metrics<timing_t, timing_t, timing_t>());
}

void test_enablement() {
    profiler_t profiler;
    {
        auto root = profiler.metric<timing_t>();
        profiler.enabled() = false;
        auto child = root.metric<counted_t>(7);
        require(static_cast<bool>(child));
        child.update<counted_t>([](counted_t& counted) { ++counted.m_number; });
    }
    require(profiler.metrics<timing_t, counted_t>()->m_number == 8);
    const auto count = allocations;
    const auto calls = clock_calls;
    allocation_forbidden = true;
    {
        auto root = profiler.metric<timing_t>();
        profiler.enabled() = true;
        auto child = root.metric<counted_t>(9);
        auto grandchild = child.metric<numbered_t<1>>();
        require(!root && !child && !grandchild);
        child.update<counted_t>([](counted_t&) { std::abort(); });
    }
    allocation_forbidden = false;
    require(count == allocations && calls == clock_calls && profiler.size() == 2);
    {
        auto root = profiler.metric<timing_t>();
        auto child = root.metric<counted_t>(9);
        child.update<counted_t>([](counted_t& counted) { ++counted.m_number; });
    }
    require(profiler.metrics<timing_t, counted_t>()->m_number == 9);
}

template <typename F>
void terminates(F&& operation) {
    const auto pid = fork();
    require(0 <= pid);
    if (pid == 0) {
        std::set_terminate([] { std::_Exit(73); });
        operation();
        std::_Exit(0);
    }
    int status = 0;
    require(waitpid(pid, &status, 0) == pid);
    require(WIFEXITED(status) && WEXITSTATUS(status) == 73);
}

void test_destruction() {
    terminates([] {
        profiler_t profiler;
        auto outer = profiler.metric<timing_t>();
        auto inner = outer.metric<counted_t>(1);
        outer.~metric_t();
    });
    terminates([] {
        profiler_t profiler;
        auto metric = profiler.metric<timing_t>();
        profiler.~profiler_t();
    });
}

template <typename Parent, std::size_t... N>
void grow(Parent& parent, std::index_sequence<N...>) {
    (parent.template metric<numbered_t<N>>().stop(), ...);
}

void test_storage() {
    static_assert(!std::is_move_constructible_v<counted_t>);
    static_assert(sizeof(metric_t) <= 2 * sizeof(void*));
    profiler_t profiler;
    profiler.enabled() = true;
    { auto metric = profiler.metric<throwing_move_t>(); }
    { auto metric = profiler.metric<throwing_assignment_t>(); }
    { auto metric = profiler.metric<aligned_t>(); }
    const auto* aligned = profiler.metrics<aligned_t>();
    require(reinterpret_cast<std::uintptr_t>(aligned) % alignof(aligned_t) == 0);
    {
        auto metric = profiler.metric<counted_t>(3);
        metric.update<counted_t>([&](counted_t& counted) {
            const auto* current = &counted;
            grow(metric, std::make_index_sequence<64>());
            metric.update<counted_t>([current](const counted_t& counted) {
                require(&counted == current && counted.m_number == 3);
            });
        });
    }
    require(profiler.size() == 68 && profiler.metrics<aligned_t>() == aligned);
    require(aligned->m_number == 42 && profiler.metrics<counted_t>()->m_number == 3);
    const auto count = allocations;
    allocation_forbidden = true;
    { auto metric = profiler.metric<counted_t>(99); grow(metric, std::make_index_sequence<64>()); }
    allocation_forbidden = false;
    require(allocations == count);
    int borrowed = 2;
    auto owned = std::make_unique<int>(5);
    {
        auto metric = profiler.metric<forwarded_t>(borrowed, std::move(owned));
        metric.update<forwarded_t>([&](const forwarded_t& metric) {
            require(!owned && metric.m_borrowed == &borrowed && *metric.m_owned == 5);
        });
    }
    require(profiler.metrics<forwarded_t>()->m_borrowed == &borrowed);
    require(*profiler.metrics<forwarded_t>()->m_owned == 5);
    owned = std::make_unique<int>(6);
    { auto metric = profiler.metric<forwarded_t>(borrowed, std::move(owned)); }
    require(owned && *owned == 6 && *profiler.metrics<forwarded_t>()->m_owned == 5);
}

void test_allocation_failure() {
    profiler_t profiler;
    profiler.enabled() = true;
    bool failed = false;
    const auto initial_calls = clock_calls;
    for (const auto permitted : std::array{0, 1}) {
        allocations_before_failure = permitted;
        failed = false;
        try { auto metric = profiler.metric<counted_t>(8); } catch (const std::bad_alloc&) { failed = true; }
        allocations_before_failure = -1;
        require(failed && profiler.size() == 0);
        require(clock_calls == initial_calls && live_metrics == 0);
    }
    { auto metric = profiler.metric<counted_t>(8); }
    const auto calls = clock_calls;
    allocations_before_failure = 0;
    failed = false;
    try { auto metric = profiler.metric<timing_t>(); } catch (const std::bad_alloc&) { failed = true; }
    allocations_before_failure = -1;
    require(failed && profiler.size() == 1 && clock_calls == calls);
    require(profiler.metrics<counted_t>()->m_number == 8 && !profiler.metrics<timing_t>());
    { auto metric = profiler.metric<timing_t>(); }
    require(profiler.size() == 2);
}

void test_inactive() {
    const auto calls = clock_calls;
    const auto count = allocations;
    allocation_forbidden = true;
    {
        profiler_t profiler;
        require(std::as_const(profiler).enabled() && profiler.size() == 0);
        profiler.enabled() = false;
        metric_t metric;
        auto disabled_metric = profiler.metric<counted_t>(17);
        require(!metric && !disabled_metric && live_metrics == 0);
        profiler.enabled() = true;
        metric.update<counted_t>([](counted_t&) { std::abort(); });
        disabled_metric.update<counted_t>([](counted_t&) { std::abort(); });
        static_assert(!noexcept(metric.update<counted_t>([](counted_t&) noexcept {})));
        static_assert(!noexcept(metric.update<counted_t>([](counted_t&) {})));
        metric.stop();
        metric.stop();
        disabled_metric.stop();
        disabled_metric.update<counted_t>([](counted_t&) { std::abort(); });
        require(profiler.size() == 0);
    }
    allocation_forbidden = false;
    require(clock_calls == calls && allocations == count && live_metrics == 0);
}

void test_reporting() {
    profiler_t profiler;
    std::ostringstream empty;
    profiler.report(empty);
    require(empty.str() == "No measurements.\n");
    profiler.enabled() = false;
    std::ostringstream disabled;
    profiler.report(disabled);
    require(disabled.str() == "Profiling disabled.\n");
#ifdef PROFILING_TEST_CLOCK
    tick = 100;
    profiler.enabled() = true;
    {
        auto metric = profiler.metric<timing_t>();
        tick = 120;
    }
    {
        auto metric = profiler.metric<timing_t>();
        tick = 160;
    }
    {
        auto metric = profiler.metric<numbered_t<7>>();
        tick = 220;
    }
    try {
        auto metric = profiler.metric<counted_t>(42); // Construction ends at 225.
        tick = 325;
        throw 1;
    } catch (int) {}
    profiler.enabled() = false;
    tick = 400;
    const auto calls = clock_calls;
    std::ostringstream report;
    profiler.report(report);
    const std::string expected =
        "Metric  Count    Last     Min    Mean     Max   Total     Age  Data\n"
        "timing      2   40 ns   20 ns   30 ns   40 ns   60 ns  240 ns\n"
        "7           1   60 ns   60 ns   60 ns   60 ns   60 ns  180 ns\n"
        "42          1  100 ns  100 ns  100 ns  100 ns  100 ns   75 ns  [unwinding]\n";
    require(report.str() == expected && clock_calls == calls + 1);
    tick = 410;
    std::ostringstream later_report;
    profiler.report(later_report);
    require(later_report.str().find("85 ns  [unwinding]") != std::string::npos);
    require(profiler.elapsed<counted_t>()->count() == 100 && profiler.metrics<counted_t>()->m_number == 42);
    profiler.enabled() = true;
    { auto metric = profiler.metric<timing_t>(); } // Zero duration still completes.
    std::ostringstream resumed_report;
    profiler.report(resumed_report);
    const auto resumed = resumed_report.str();
    const auto timing = resumed.substr(resumed.find('\n') + 1);
    std::istringstream fields(timing);
    std::string name, unit;
    std::size_t count = 0;
    long double last = 0, minimum = 0, mean = 0, maximum = 0, total = 0, age = 0;
    fields >> name >> count >> last >> unit >> minimum >> unit >> mean >> unit >> maximum >> unit >> total >> unit >> age >> unit;
    require(bool(fields) && name == "timing" && count == 3 && last == 0 && minimum == 0);
    require(mean == 20 && maximum == 40 && total == 60 && age == 0);
#endif
}

void test_tree_report() {
#ifdef PROFILING_TEST_CLOCK
    profiler_t profiler;
    tick = 0;
    {
        auto root = profiler.metric<timing_t>();
        tick = 10;
        {
            auto child = root.metric<numbered_t<1>>();
            tick = 12;
            { auto grandchild = child.metric<numbered_t<5>>(); tick = 14; }
            tick = 20;
        }
        tick = 30;
        { auto child = root.metric<numbered_t<2>>(); tick = 40; }
        tick = 50;
        { auto child = root.metric<numbered_t<1>>(); tick = 60; }
        tick = 70;
    }
    tick = 80;
    {
        auto root = profiler.metric<numbered_t<3>>();
        tick = 90;
        { auto child = root.metric<numbered_t<1>>(); tick = 100; }
        tick = 110;
    }
    tick = 120;
    std::ostringstream report;
    profiler.report(report);
    const std::string expected =
        "Metric   Count   Last    Min   Mean    Max  Total     Age  Data\n"
        "timing       1  70 ns  70 ns  70 ns  70 ns  70 ns   50 ns\n"
        "├─ 1         2  10 ns  10 ns  10 ns  10 ns  20 ns   60 ns\n"
        "│  └─ 5      1   2 ns   2 ns   2 ns   2 ns   2 ns  106 ns\n"
        "└─ 2         1  10 ns  10 ns  10 ns  10 ns  10 ns   80 ns\n"
        "3            1  30 ns  30 ns  30 ns  30 ns  30 ns   10 ns\n"
        "└─ 1         1  10 ns  10 ns  10 ns  10 ns  10 ns   20 ns\n";
    require(report.str() == expected && profiler.size() == 6);
#endif
}

void test_multiline_report() {
    profiler_t profiler;
    {
        auto parent = profiler.metric<timing_t>();
        auto child = parent.metric<multiline_t>();
    }
    std::ostringstream report;
    profiler.report(report);
    const auto text = report.str();
    require(std::count(text.begin(), text.end(), '\n') == 3);
    require(text.starts_with("Metric") && text.find("\n└─ renderer.very_long_rasterization_metric ") != std::string::npos);
    require(text.find("second third  fourth\n") != std::string::npos);
    require(text.find('\t') == std::string::npos && text.find('\r') == std::string::npos);
}

void test_child_allocation_failure() {
    profiler_t profiler;
    {
        auto root = profiler.metric<timing_t>();
        for (int permitted : {0, 1}) {
            allocations_before_failure = permitted;
            bool failed = false;
            try { auto child = root.metric<counted_t>(8); } catch (const std::bad_alloc&) { failed = true; }
            allocations_before_failure = -1;
            require(failed && live_metrics == 0 && root);
        }
        auto child = root.metric<counted_t>(8);
        child.update<counted_t>([](counted_t& counted) { ++counted.m_number; });
    }
    require(profiler.size() == 2 && profiler.metrics<timing_t, counted_t>()->m_number == 9);
}

void test_reporting_failure() {
    profiler_t profiler;
    profiler.enabled() = true;
    { auto metric = profiler.metric<failing_t>(); }
    std::ostringstream report;
    rejects([&] { profiler.report(report); });
    rejects([&] { profiler.report(report); });
    require(profiler.size() == 1 && profiler.metrics<failing_t>());
    profiler_t working_profiler;
    working_profiler.enabled() = true;
    { auto metric = working_profiler.metric<counted_t>(29); }
    std::ostringstream failed;
    failed.setstate(std::ios::badbit);
    try { failed.exceptions(std::ios::badbit); } catch (const std::ios_base::failure&) {}
    bool rejected = false;
    try { working_profiler.report(failed); } catch (const std::ios_base::failure&) { rejected = true; }
    require(rejected && working_profiler.metrics<counted_t>()->m_number == 29);
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

int main() {
#ifdef PROFILING_TEST_MISSING_FORMATTER
    profiling::profiler_t profiler;
    auto metric = profiler.metric<profiling::missing_t>();
#endif
#ifdef PROFILING_TEST_THROWING_CONSTRUCTOR
    profiling::profiler_t profiler;
    auto metric = profiler.metric<profiling::throwing_constructor_t>();
#endif
#ifdef PROFILING_TEST_THROWING_DESTRUCTOR
    profiling::profiler_t profiler;
    auto metric = profiler.metric<profiling::throwing_destructor_t>();
#endif
    try {
        profiling::test_persistence();
        profiling::require(profiling::live_metrics == 0);
        profiling::test_lifetimes();
        profiling::test_paths();
        profiling::test_enablement();
        profiling::test_destruction();
        profiling::test_tree_report();
        profiling::test_multiline_report();
        profiling::test_child_allocation_failure();
        profiling::test_storage();
        profiling::test_allocation_failure();
        profiling::test_inactive();
        profiling::test_reporting();
        profiling::test_reporting_failure();
        profiling::require(profiling::live_metrics == 0);
        std::cout << "profiling public validation passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
