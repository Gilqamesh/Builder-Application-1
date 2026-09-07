#include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

bool allocation_forbidden = false;
std::size_t clock_calls = 0;
std::int64_t tick = 0;
std::size_t constructions = 0;
std::size_t destructions = 0;
std::size_t formats = 0;
std::array<int, 16> destroyed {};

#ifdef PROFILING_TEST_CLOCK
// Link api.cpp without helpers.cpp for deterministic clock validation.
std::chrono::nanoseconds clock_now() noexcept {
    ++clock_calls;
    return std::chrono::nanoseconds(tick);
}
#endif

struct timing_t {};

struct nested_t {
    explicit nested_t(context_t& context) noexcept;
};

struct retained_t {
    explicit retained_t(int number) noexcept: m_number(number) { ++constructions; ++tick; }
    ~retained_t() { destroyed[destructions++ % destroyed.size()] = m_number; }
    retained_t(const retained_t&) = delete;
    retained_t(retained_t&&) = delete;
    int m_number;
};

struct forwarded_t {
    forwarded_t(int& borrowed, std::unique_ptr<int>&& owned) noexcept:
        m_borrowed(&borrowed), m_owned(std::move(owned)) {}
    int* m_borrowed;
    std::unique_ptr<int> m_owned;
};

struct alignas(256) aligned_t { int m_number = 42; };
struct oversized_t { std::array<std::byte, 8192> m_bytes {}; };
struct failing_t {};
struct missing_t {};

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<profiling::timing_t> : formatter<string_view> {
    auto format(const profiling::timing_t&, auto& ctx) const {
        return formatter<string_view>::format("timing", ctx);
    }
};

template <>
struct formatter<profiling::nested_t> : formatter<string_view> {
    auto format(const profiling::nested_t&, auto& ctx) const {
        return formatter<string_view>::format("nested construction", ctx);
    }
};

template <>
struct formatter<profiling::retained_t> : formatter<int> {
    auto format(const profiling::retained_t& metrics, auto& ctx) const {
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
struct formatter<profiling::oversized_t> : formatter<string_view> {
    auto format(const profiling::oversized_t&, auto& ctx) const {
        return formatter<string_view>::format("oversized_metric", ctx);
    }
};

template <>
struct formatter<profiling::failing_t> : formatter<string_view> {
    auto format(const profiling::failing_t&, auto& ctx) const {
        throw std::logic_error("payload formatter failed");
        return ctx.out();
    }
};

} // namespace std

void* operator new(std::size_t size) {
    if (profiling::allocation_forbidden) { std::abort(); }
    if (auto* memory = std::malloc(size == 0 ? 1 : size)) { return memory; }
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void* operator new(std::size_t size, std::align_val_t alignment) {
    if (profiling::allocation_forbidden) { std::abort(); }
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

nested_t::nested_t(context_t& context) noexcept {
    auto metric = context.metric<retained_t>(31);
}

void require(bool condition) {
    if (!condition) { throw std::runtime_error("profiling contract check failed"); }
}

template <typename F>
void rejects(F&& operation) {
    bool failed = false;
    try { operation(); } catch (const std::logic_error&) { failed = true; }
    require(failed);
}

void recurse(context_t& context, int depth) {
    auto metric = context.metric<retained_t>(depth);
    if (metric) { require(metric->m_number == depth); }
    if (depth != 0) { recurse(context, depth - 1); }
}

void test_capture() {
    static_assert(!std::is_copy_constructible_v<profiler_t> && !std::is_move_constructible_v<profiler_t>);
    static_assert(!std::is_copy_constructible_v<metric_t<retained_t>> && !std::is_move_constructible_v<metric_t<retained_t>>);
    static_assert(std::forward_iterator<records_t::iterator_t>);
    static_assert(std::ranges::forward_range<records_t>);
    std::array<std::byte, 4096> storage;
    profiler_t profiler(storage);
    auto context = profiler.context();
    rejects([&] { profiler.reset(); });
    rejects([&] { (void)profiler.records(); });
    rejects([&] { (void)profiler.omitted(); });
    rejects([&] { profiler.report(std::cout); });
    tick = 0;
    profiler.start();
    rejects([&] { profiler.start(); });
    const retained_t* retained = nullptr;
    const auto constructed = constructions;
    const auto destructed = destructions;
    const auto formatted = formats;
    tick = 10;
    {
        auto parent_metric = profiler.metric<timing_t>();
        require(bool(parent_metric) && !profiler.quiescent());
        rejects([&] { profiler.reset(); });
        rejects([&] { (void)profiler.records(); });
        rejects([&] { profiler.report(std::cout); });
        tick = 20;
        {
            allocation_forbidden = true;
            auto child_metric = context.metric<retained_t>(7);
            retained = child_metric.operator->();
            require(child_metric->m_number == 7);
            const auto& const_child_metric = child_metric;
            static_assert(std::same_as<decltype(const_child_metric.operator->()), const retained_t*>);
            tick = 50;
        }
        allocation_forbidden = false;
        require(constructions == constructed + 1 && destructions == destructed && formats == formatted);
        tick = 100;
    }
    const auto records = profiler.records();
    require(records.size() == 2 && !records[0].parent() && records[1].parent() == 0);
    require(records[0].metrics<timing_t>() && !records[0].metrics<retained_t>());
    require(records[1].metrics<retained_t>() == retained && retained->m_number == 7);
    require(records[0].elapsed() == *records[0].self() + records[1].elapsed());
#ifdef PROFILING_TEST_CLOCK
    require(records[0].start().count() == 10 && records[0].elapsed().count() == 90);
    // Payload construction advances the test clock before timing begins.
    require(records[1].start().count() == 21);
    require(records[1].elapsed().count() == 29 && records[0].self()->count() == 61);
#endif
    std::ostringstream report;
    profiler.report(report);
    require(formats == formatted + 1);
    require(report.str().find("timing inclusive=") != std::string::npos);
    require(report.str().find("  7 inclusive=") != std::string::npos);
    profiler.report(report);
    require(formats == formatted + 2 && destructions == destructed);
    // Retained pointers and a range snapshot remain usable as later_metric roots append.
    { auto later_metric = profiler.metric<timing_t>(); }
    require(retained->m_number == 7 && records.size() == 2);
    require(std::ranges::distance(records) == 2);
    profiler.reset();
    require(destructions == destructed + 1 && profiler.records().empty() && profiler.omitted() == 0);
    recurse(context, 3);
    recurse(context, 0);
    require(profiler.records().size() == 5);
    require(profiler.records()[3].depth() == 3 && !profiler.records()[4].parent());
    profiler.reset();
    try {
        auto parent_metric = profiler.metric<timing_t>();
        auto child_metric = context.metric<retained_t>(17);
        child_metric->m_number = 18;
        throw std::runtime_error("producer failed");
    } catch (const std::runtime_error&) {}
    require(profiler.records()[0].unwinding() && profiler.records()[1].unwinding());
    require(profiler.records()[1].metrics<retained_t>()->m_number == 18);
    profiler.reset();
    {
        auto closed_metric = profiler.metric<retained_t>(9);
        closed_metric.close();
        closed_metric.close();
        require(!closed_metric && profiler.quiescent());
        require(profiler.records()[0].metrics<retained_t>()->m_number == 9);
        profiler.reset(); // Destruction of the already closed_metric handle is a no-op.
    }
    {
        auto caught_metric = profiler.metric<timing_t>();
        try { throw 1; } catch (int) {}
    }
    require(!profiler.records()[0].unwinding());
    profiler.reset();
    {
        auto metric = profiler.metric<nested_t>(context);
    }
    require(profiler.records().size() == 2);
    require(profiler.records()[0].metrics<retained_t>()->m_number == 31);
    require(profiler.records()[1].metrics<nested_t>());
    require(!profiler.records()[0].parent() && !profiler.records()[1].parent());

}

void test_lifetimes_and_alignment() {
    std::array<std::byte, 4097> storage;
    int number = 3;
    auto owned = std::make_unique<int>(4);
    const auto destructed = destructions;
    {
        profiler_t profiler(std::span<std::byte>(storage).subspan(1));
        profiler.start();
        {
            allocation_forbidden = true;
            auto outer_metric = profiler.metric<retained_t>(21);
            auto inner_metric = profiler.metric<retained_t>(22);
            auto forwarded_metric = profiler.metric<forwarded_t>(number, std::move(owned));
            require(forwarded_metric->m_borrowed == &number && *forwarded_metric->m_owned == 4 && !owned);
            auto aligned_metric = profiler.metric<aligned_t>();
            require(reinterpret_cast<std::uintptr_t>(aligned_metric.operator->()) % alignof(aligned_t) == 0);
            allocation_forbidden = false;
        }
        require(destructions == destructed);
        const auto* aligned = profiler.records()[3].metrics<aligned_t>();
        require(aligned && aligned->m_number == 42);
        require(profiler.records()[2].metrics<forwarded_t>()->m_borrowed == &number);
        allocation_forbidden = true;
    }
    allocation_forbidden = false;
    require(destructions == destructed + 2);
    require(destroyed[destructed % destroyed.size()] == 22 && destroyed[(destructed + 1) % destroyed.size()] == 21);
}

void test_overflow() {
    std::array<std::byte, 2048> storage;
    profiler_t profiler(storage);
    auto context = profiler.context();
    profiler.start();
    const auto constructed = constructions;
    {
        auto parent_metric = profiler.metric<timing_t>();
        {
            auto oversized_metric = context.metric<oversized_t>();
            require(!oversized_metric);
            const auto calls = clock_calls;
            recurse(context, 3);
            require(constructions == constructed && clock_calls == calls);
            rejects([&] { profiler.reset(); });
            rejects([&] { (void)profiler.records(); });
            rejects([&] { context = context_t(); });
        }
        auto sibling_metric = context.metric<retained_t>(8);
        require(bool(sibling_metric));
    }
    require(profiler.records().size() == 2 && profiler.omitted() == 5);
    require(!profiler.records()[0].self() && profiler.records()[1].self().has_value());
    require(profiler.records()[1].parent() == 0);
    std::ostringstream report;
    profiler.report(report);
    require(report.str().find("self=unavailable") != std::string::npos);
    require(report.str().find("5 omitted, incomplete") != std::string::npos);
    profiler.reset();
    { auto metric = profiler.metric<timing_t>(); }
    require(profiler.omitted() == 0 && profiler.records()[0].self().has_value());

    profiler_t empty({});
    auto empty_context = empty.context();
    empty.start();
    const auto calls = clock_calls;
    recurse(empty_context, 4);
    require(empty.records().empty() && empty.omitted() == 5 && clock_calls == calls);
    {
        auto suppressed_metric = empty.metric<timing_t>();
        require(!suppressed_metric && !empty.quiescent());
        rejects([&] { empty.reset(); });
        rejects([&] { empty.report(report); });
    }
    require(empty.quiescent());

    // Exhaustion while unwinding must restore suppression and parent_metric state.
    profiler.reset();
    try {
        auto parent_metric = profiler.metric<timing_t>();
        auto suppressed_metric = profiler.metric<oversized_t>();
        auto descendant_metric = profiler.metric<retained_t>(9);
        throw 1;
    } catch (int) {}
    require(profiler.quiescent() && profiler.omitted() == 2 && profiler.records()[0].unwinding());
    { auto sibling_metric = profiler.metric<retained_t>(10); require(bool(sibling_metric)); }
    require(!profiler.records()[1].parent());
}

void test_contexts() {
    context_t unattached_context;
    const auto calls = clock_calls;
    const auto constructed = constructions;
    int arguments = 0;
    allocation_forbidden = true;
    {
        auto metric = unattached_context.metric<retained_t>(++arguments);
        require(!metric);
        metric.close();
    }
    allocation_forbidden = false;
    require(arguments == 1 && constructions == constructed && clock_calls == calls);
    std::array<std::byte, 1024> source_storage, destination_storage;
    profiler_t source_profiler(source_storage), destination_profiler(destination_storage);
    auto source_context = source_profiler.context();
    auto destination_context = destination_profiler.context();
    auto copied_context = source_context;
    source_profiler.start(); destination_profiler.start();
    {
        auto outer_metric = source_context.metric<timing_t>();
        rejects([&] { (void)source_profiler.context(); });
        rejects([&] { [[maybe_unused]] auto replacement_context = source_context; });
        rejects([&] { source_context = destination_context; });
        rejects([&] { destination_context = source_context; });
        auto child_metric = copied_context.metric<retained_t>(11);
        auto independent_metric = destination_context.metric<timing_t>();
    }
    require(source_profiler.records()[1].parent() == 0 && !destination_profiler.records()[0].parent());
    source_context = destination_context;
    { auto metric = source_context.metric<retained_t>(12); }
    require(source_profiler.records().size() == 2 && destination_profiler.records().size() == 2);
    source_context = context_t();
    { auto metric = source_context.metric<retained_t>(13); require(!metric); }
    source_profiler.reset();
    { auto metric = copied_context.metric<retained_t>(14); }
    require(source_profiler.records().size() == 1);
}

void test_report_failure() {
    std::array<std::byte, 1024> storage;
    profiler_t profiler(storage);
    profiler.start();
    { auto metric = profiler.metric<failing_t>(); }
    std::ostringstream out;
    rejects([&] { profiler.report(out); });
    require(profiler.records().size() == 1 && profiler.records()[0].metrics<failing_t>());
    rejects([&] { profiler.report(out); });
    profiler.reset();
    { auto metric = profiler.metric<retained_t>(29); }
    profiler.report(out);
    require(out.str().find("29 inclusive=") != std::string::npos);
    std::ostringstream failed;
    failed.setstate(std::ios::badbit);
    try { failed.exceptions(std::ios::badbit); } catch (const std::ios_base::failure&) {}
    bool rejected = false;
    try { profiler.report(failed); } catch (const std::ios_base::failure&) { rejected = true; }
    require(rejected && profiler.records()[0].metrics<retained_t>()->m_number == 29);
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

int main() {
#ifdef PROFILING_TEST_MISSING_FORMATTER
    // Must fail compilation even though this context can never record.
    profiling::context_t unattached;
    auto metric = unattached.metric<profiling::missing_t>();
#endif
    try {
        profiling::test_capture();
        profiling::test_lifetimes_and_alignment();
        profiling::test_overflow();
        profiling::test_contexts();
        profiling::test_report_failure();
        std::cout << "profiling public validation passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
