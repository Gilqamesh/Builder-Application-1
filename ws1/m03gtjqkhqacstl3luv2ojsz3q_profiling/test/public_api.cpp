#include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace profiling = m03gtjqkhqacstl3luv2ojsz3q_profiling;

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

bool allocation_forbidden = false;

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

void* operator new(std::size_t size) {
    if (profiling::allocation_forbidden) {
        std::abort();
    }
    if (auto* memory = std::malloc(size == 0 ? 1 : size)) {
        return memory;
    }
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

struct test_clock_t {
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<test_clock_t>;
    static constexpr bool is_steady = true;
    static inline std::int64_t tick = 0;
    static inline std::size_t calls = 0;

    static time_point now() noexcept {
        ++calls;
        return time_point(duration(tick));
    }
};

// Intentionally lacks a formatter and cannot be constructed.
struct unavailable_t {
    unavailable_t() = delete;
};

void require(bool condition) {
    if (!condition) {
        throw std::runtime_error("profiling contract check failed");
    }
}

template <typename F>
void rejects(F&& operation) {
    bool failed = false;
    try { operation(); } catch (const std::logic_error&) { failed = true; }
    require(failed);
}

using test_profiler_t = profiling::profiler_t<std::size_t, profiling::text_report_t, test_clock_t>;

void recurse(test_profiler_t& profiler, profiling::region_id_t region, std::size_t depth) {
    auto scope = profiler.scope<std::size_t>(region);
    scope.metrics() = depth;
    if (depth != 0) {
        recurse(profiler, region, depth - 1);
    }
}

void test_capture() {
    std::array<test_profiler_t::record_type_t, 12> storage;
    test_profiler_t profiler(storage);
    std::string name = "frame";
    const auto frame_region = profiler.register_region(name);
    name.assign("destroyed caller name");
    const auto child_region = profiler.register_region("child");
    rejects([&] { profiler.reset(); });
    rejects([&] { (void)profiler.records(); });
    test_clock_t::tick = 0;
    profiler.start();
    rejects([&] { profiler.start(); });
    rejects([&] { profiler.register_region("late"); });
    require(profiler.regions()[frame_region.m_index] == "frame");
    const auto* registered_name = profiler.regions()[frame_region.m_index].data();
    test_clock_t::tick = 10;
    {
        auto frame = profiler.scope(frame_region);
        rejects([&] { profiler.reset(); });
        rejects([&] { (void)profiler.records(); });
        test_clock_t::tick = 20;
        {
            allocation_forbidden = true;
            auto child = profiler.scope<std::size_t>(child_region);
            child.metrics() = 7;
            test_clock_t::tick = 50;
        }
        allocation_forbidden = false;
        test_clock_t::tick = 100;
    }
    const auto records = profiler.records();
    require(records.size() == 2 && !records[0].m_parent && records[1].m_parent == 0);
    require(records[0].m_start.count() == 10 && records[0].m_elapsed.count() == 90);
    require(records[1].m_elapsed.count() == 30 && records[0].self()->count() == 60);
    require(!records[0].m_metrics && *records[1].m_metrics == 7);
    std::ostringstream report;
    profiler.report(report);
    require(report.str().find("frame inclusive=90 ns self=60 ns") != std::string::npos);
    require(report.str().find("  child inclusive=30 ns self=30 ns 7") != std::string::npos);
    profiler.reset();
    require(profiler.records().empty() && profiler.omitted() == 0);
    require(profiler.regions()[frame_region.m_index].data() == registered_name);
    recurse(profiler, child_region, 3);
    recurse(profiler, child_region, 0);
    require(profiler.records().size() == 5);
    require(profiler.records()[3].m_depth == 3 && !profiler.records()[4].m_parent);
    profiler.reset();
    try {
        auto parent = profiler.scope(frame_region);
        auto child = profiler.scope<std::size_t>(child_region);
        child.metrics() = 17;
        throw std::runtime_error("producer failed");
    } catch (const std::runtime_error&) {}
    require(profiler.records()[0].m_unwinding && profiler.records()[1].m_unwinding);
    require(*profiler.records()[1].m_metrics == 17);
    profiler.reset();
    {
        auto closed = profiler.scope<std::size_t>(child_region);
        closed.metrics() = 9;
        closed.close();
        closed.close();
        require(profiler.records().size() == 1 && *profiler.records()[0].m_metrics == 9);
    }
    profiler.reset();
    {
        auto caught = profiler.scope(frame_region);
        try { throw 1; } catch (int) {}
    }
    require(!profiler.records()[0].m_unwinding);
}

void test_overflow() {
    std::array<test_profiler_t::record_type_t, 2> storage;
    test_profiler_t profiler(storage);
    const auto region = profiler.register_region("recursive");
    profiler.start();
    recurse(profiler, region, 3);
    require(profiler.records().size() == 2 && profiler.omitted() == 2);
    require(profiler.records()[0].self().has_value());
    require(!profiler.records()[1].self());
    require(profiler.records()[1].m_parent == 0);
    std::ostringstream report;
    profiler.report(report);
    require(report.str().find("self=unavailable") != std::string::npos);
    require(report.str().find("2 omitted, incomplete") != std::string::npos);
    profiler.reset();
    { auto scope = profiler.scope(region); }
    require(profiler.omitted() == 0 && profiler.records()[0].self().has_value());

    test_profiler_t empty({});
    const auto empty_region = empty.register_region("empty");
    empty.start();
    const auto calls = test_clock_t::calls;
    recurse(empty, empty_region, 4);
    require(empty.records().empty() && empty.omitted() == 5);
    require(test_clock_t::calls == calls);
}

struct failing_report_t {
    template <typename Metrics>
    void operator()(std::ostream&, std::span<const profiling::record_t<Metrics>>, std::span<const std::string>, std::size_t) const {
        throw std::logic_error("report destination failed");
    }
};

void test_report_failure() {
    using report_profiler_t = profiling::profiler_t<std::size_t, failing_report_t>;
    std::array<report_profiler_t::record_type_t, 1> storage;
    report_profiler_t profiler(storage);
    const auto region = profiler.register_region("report");
    profiler.start();
    {
        auto scope = profiler.scope<std::size_t>(region);
        scope.metrics() = 42;
        rejects([&] { profiler.report(std::cout); });
    }
    rejects([&] { profiler.report(std::cout); });
    require(profiler.records().size() == 1 && *profiler.records()[0].m_metrics == 42);
}

void test_timing_only() {
    using timing_profiler_t = profiling::profiler_t<>;
    std::array<timing_profiler_t::record_type_t, 1> storage;
    timing_profiler_t profiler(storage);
    const auto region = profiler.register_region("timing");
    profiler.start();
    { auto scope = profiler.scope(region); }
    require(!profiler.records()[0].m_metrics);
    std::ostringstream out;
    profiler.report(out);
    require(out.str().find("timing inclusive=") != std::string::npos);
}

void test_disabled_and_independent() {
    using disabled_t = profiling::profiler_t<unavailable_t, void, test_clock_t>;
    static_assert(std::is_empty_v<disabled_t> && std::is_empty_v<profiling::disabled_scope_t>);
    disabled_t disabled;
    const auto calls = test_clock_t::calls;
    allocation_forbidden = true;
    const auto region = disabled.register_region("disabled");
    disabled.start();
    { [[maybe_unused]] auto scope = disabled.scope<unavailable_t>(region); }
    disabled.reset();
    allocation_forbidden = false;
    require(test_clock_t::calls == calls);

    std::array<test_profiler_t::record_type_t, 1> first_storage, second_storage;
    test_profiler_t first(first_storage), second(second_storage);
    const auto first_region = first.register_region("first");
    const auto second_region = second.register_region("second");
    first.start(); second.start();
    {
        auto outer = first.scope(first_region);
        auto independent = second.scope(second_region);
    }
    require(!first.records()[0].m_parent && !second.records()[0].m_parent);
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

int main() {
    try {
        profiling::test_capture();
        profiling::test_overflow();
        profiling::test_disabled_and_independent();
        profiling::test_timing_only();
        profiling::test_report_failure();
        std::cout << "profiling public validation passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
