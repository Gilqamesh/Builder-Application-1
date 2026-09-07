#include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
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

struct counted_t {
    explicit counted_t(int number) noexcept: m_number(number) { ++live_metrics; tick += 5; }
    counted_t(counted_t&& other) noexcept: m_number(other.m_number) { ++live_metrics; tick += 7; }
    counted_t& operator=(counted_t&& other) noexcept { m_number = other.m_number; tick += 7; return *this; }
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
    observing_t(observing_t&& other) noexcept;
    observing_t& operator=(observing_t&& other) noexcept;
    ~observing_t();
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

observing_t::observing_t(profiler_t& profiler) noexcept: m_profiler(&profiler) {
    if (profiler.quiescent()) { std::abort(); }
}

observing_t::observing_t(observing_t&& other) noexcept: m_profiler(other.m_profiler) {
    if (m_profiler->quiescent()) { std::abort(); }
    other.m_profiler = nullptr;
}

observing_t& observing_t::operator=(observing_t&& other) noexcept {
    if (other.m_profiler->quiescent()) { std::abort(); }
    m_profiler = other.m_profiler;
    other.m_profiler = nullptr;
    return *this;
}

observing_t::~observing_t() = default;

void require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) { throw std::runtime_error(std::format("profiling contract check failed at line {}", location.line())); }
}

template <typename F>
void rejects(F&& operation) {
    bool failed = false;
    try { operation(); } catch (const std::logic_error&) { failed = true; }
    require(failed);
}

void test_replacement() {
    static_assert(!std::is_copy_constructible_v<profiler_t> && !std::is_move_constructible_v<profiler_t>);
    static_assert(!std::is_copy_constructible_v<metric_t<counted_t>> && !std::is_move_constructible_v<metric_t<counted_t>>);
    profiler_t profiler;
    require(profiler.quiescent() && profiler.size() == 0);
    require(!profiler.metrics<counted_t>() && !profiler.elapsed<counted_t>() && !profiler.unwinding<counted_t>());
    tick = 10;
    const auto formatted = formats;
    {
        auto metric = profiler.metric<counted_t>(7);
        require(!profiler.quiescent() && live_metrics == 1);
        static_assert(std::same_as<decltype(std::as_const(metric).operator->()), const counted_t*>);
        rejects([&] { (void)profiler.metrics<counted_t>(); });
        rejects([&] { (void)profiler.elapsed<counted_t>(); });
        rejects([&] { (void)profiler.unwinding<counted_t>(); });
        rejects([&] { (void)profiler.size(); });
        rejects([&] { profiler.report(std::cout); });
        tick = 50;
        allocation_forbidden = true;
        metric.stop();
        require(!metric && profiler.quiescent() && live_metrics == 1 && formats == formatted);
        const auto calls = clock_calls;
        metric.stop();
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
    allocation_forbidden = true;
    tick = 100;
    {
        auto metric = profiler.metric<counted_t>(20);
        require(live_metrics == 2);
        metric->m_number = 21;
        tick = 125;
    }
    allocation_forbidden = false;
    require(allocations == count && profiler.size() == 1 && live_metrics == 1);
    require(profiler.metrics<counted_t>()->m_number == 21);
#ifdef PROFILING_TEST_CLOCK
    require(profiler.elapsed<counted_t>()->count() == 20);
#endif
    std::ostringstream report;
    profiler.report(report);
    require(formats == formatted + 1);
    require(report.str().starts_with("21 elapsed=") && report.str().find("7 elapsed=") == std::string::npos);
    profiler.report(report);
    require(formats == formatted + 2);
    require(std::format("{}", profiler).find("quiescent: true") != std::string::npos);
}

void recurse(profiler_t& profiler, int depth) {
    auto metric = profiler.metric<counted_t>(depth);
    if (depth != 0) { recurse(profiler, depth - 1); }
}

void test_independent_completion() {
    profiler_t profiler;
    recurse(profiler, 4);
    require(profiler.size() == 1 && profiler.metrics<counted_t>()->m_number == 4);
    {
        auto first_metric = profiler.metric<counted_t>(11);
        auto last_metric = profiler.metric<counted_t>(22);
        first_metric.stop();
        require(!profiler.quiescent());
        last_metric.stop();
        require(profiler.quiescent());
    }
    require(profiler.metrics<counted_t>()->m_number == 22);
    {
        auto outer_metric = profiler.metric<timing_t>();
        auto inner_metric = profiler.metric<counted_t>(33);
        outer_metric.stop();
        inner_metric.stop();
    }
    require(profiler.size() == 2 && profiler.metrics<counted_t>()->m_number == 33);
    try {
        auto metric = profiler.metric<counted_t>(44);
        throw 1;
    } catch (int) {}
    require(profiler.quiescent() && profiler.unwinding<counted_t>() == true);
    require(profiler.metrics<counted_t>()->m_number == 44);
    {
        auto metric = profiler.metric<counted_t>(55);
        try { throw 1; } catch (int) {}
    }
    require(profiler.unwinding<counted_t>() == false);
    { auto metric = profiler.metric<observing_t>(profiler); }
    { auto metric = profiler.metric<observing_t>(profiler); }
}

template <std::size_t... N>
void grow(profiler_t& profiler, std::index_sequence<N...>) {
    (profiler.metric<numbered_t<N>>().stop(), ...);
}

void test_storage() {
    profiler_t profiler;
    { auto metric = profiler.metric<aligned_t>(); }
    const auto* aligned = profiler.metrics<aligned_t>();
    require(reinterpret_cast<std::uintptr_t>(aligned) % alignof(aligned_t) == 0);
    {
        auto metric = profiler.metric<counted_t>(3);
        auto* current = metric.operator->();
        grow(profiler, std::make_index_sequence<64>());
        require(metric.operator->() == current && current->m_number == 3);
    }
    require(profiler.size() == 66 && profiler.metrics<aligned_t>() == aligned);
    require(aligned->m_number == 42 && profiler.metrics<counted_t>()->m_number == 3);
    const auto count = allocations;
    allocation_forbidden = true;
    grow(profiler, std::make_index_sequence<64>());
    allocation_forbidden = false;
    require(allocations == count);
    int borrowed = 2;
    auto owned = std::make_unique<int>(5);
    {
        auto metric = profiler.metric<forwarded_t>(borrowed, std::move(owned));
        require(!owned && metric->m_borrowed == &borrowed && *metric->m_owned == 5);
    }
    require(profiler.metrics<forwarded_t>()->m_borrowed == &borrowed);
    require(*profiler.metrics<forwarded_t>()->m_owned == 5);
    owned = std::make_unique<int>(6);
    { auto metric = profiler.metric<forwarded_t>(borrowed, std::move(owned)); }
    require(*profiler.metrics<forwarded_t>()->m_owned == 6);
}

void test_allocation_failure() {
    allocations_before_failure = 0;
    bool failed = false;
    try { profiler_t profiler; } catch (const std::bad_alloc&) { failed = true; }
    allocations_before_failure = -1;
    require(failed);
    profiler_t profiler;
    { auto metric = profiler.metric<counted_t>(8); }
    const auto calls = clock_calls;
    allocations_before_failure = 0;
    failed = false;
    try { auto metric = profiler.metric<timing_t>(); } catch (const std::bad_alloc&) { failed = true; }
    allocations_before_failure = -1;
    require(failed && profiler.quiescent() && profiler.size() == 1 && clock_calls == calls);
    require(profiler.metrics<counted_t>()->m_number == 8 && !profiler.metrics<timing_t>());
    { auto metric = profiler.metric<timing_t>(); }
    require(profiler.size() == 2);
}

void test_inactive() {
    const auto calls = clock_calls;
    const auto count = allocations;
    allocation_forbidden = true;
    {
        metric_t<counted_t> metric;
        require(!metric && live_metrics == 0);
        metric.stop();
        metric.stop();
    }
    allocation_forbidden = false;
    require(clock_calls == calls && allocations == count && live_metrics == 0);
}

void test_reporting_failure() {
    profiler_t profiler;
    { auto metric = profiler.metric<failing_t>(); }
    std::ostringstream report;
    rejects([&] { profiler.report(report); });
    rejects([&] { profiler.report(report); });
    require(profiler.size() == 1 && profiler.metrics<failing_t>());
    profiler_t working_profiler;
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
    profiling::metric_t<profiling::missing_t> metric;
#endif
#ifdef PROFILING_TEST_THROWING_CONSTRUCTOR
    profiling::profiler_t profiler;
    auto metric = profiler.metric<profiling::throwing_constructor_t>();
#endif
#ifdef PROFILING_TEST_THROWING_MOVE
    profiling::metric_t<profiling::throwing_move_t> metric;
#endif
#ifdef PROFILING_TEST_THROWING_ASSIGNMENT
    profiling::metric_t<profiling::throwing_assignment_t> metric;
#endif
#ifdef PROFILING_TEST_THROWING_DESTRUCTOR
    profiling::metric_t<profiling::throwing_destructor_t> metric;
#endif
    try {
        profiling::test_replacement();
        profiling::require(profiling::live_metrics == 0);
        profiling::test_independent_completion();
        profiling::test_storage();
        profiling::test_allocation_failure();
        profiling::test_inactive();
        profiling::test_reporting_failure();
        profiling::require(profiling::live_metrics == 0);
        std::cout << "profiling public validation passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
