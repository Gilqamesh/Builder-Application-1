#ifndef M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H
# define M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H

# include <cassert>
# include <chrono>
# include <concepts>
# include <cstddef>
# include <exception>
# include <format>
# include <limits>
# include <optional>
# include <ostream>
# include <span>
# include <stdexcept>
# include <string>
# include <string_view>
# include <type_traits>
# include <utility>
# include <variant>
# include <vector>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

/** @brief Identifies registered metadata within one profiler's lifetime. */
struct region_id_t {
    const void* m_owner = nullptr;
    std::size_t m_index = 0;
};

/**
 * @brief Stores one scope; its index in the capture is its record identity.
 *
 * Durations are inclusive, monotonic elapsed time in nanoseconds, not CPU usage.
 * Parents precede children. Metrics are committed when the scope closes, including
 * partial metrics during unwinding. Timing-only scopes have no metrics value.
 * An incomplete child set makes self time unavailable, preserving inclusive time.
 */
template <typename Metrics>
struct record_t {
    std::optional<std::size_t> m_parent;
    std::size_t m_region = 0;
    std::size_t m_depth = 0;
    std::chrono::nanoseconds m_start {};
    std::chrono::nanoseconds m_elapsed {};
    std::chrono::nanoseconds m_children_elapsed {};
    bool m_children_complete = true;
    bool m_unwinding = false;
    std::optional<Metrics> m_metrics;

    std::optional<std::chrono::nanoseconds> self() const noexcept;
};

/** @brief Writes hierarchy and timing, delegating typed payloads to std::formatter. */
struct text_report_t {
    template <typename Metrics>
    void operator()(std::ostream& out, std::span<const record_t<Metrics>> records, std::span<const std::string> regions, std::size_t omitted) const;
};

template <typename Metrics = std::monostate, typename Report = text_report_t, typename Clock = std::chrono::steady_clock>
class profiler_t;

/**
 * @brief Accumulates a local payload and closes a synchronous scope on destruction.
 *
 * Guards cannot be copied or moved and must close in reverse construction order.
 * The profiler and its storage outlive the guard. Payload construction, movement,
 * destruction, and producer counter updates must not allocate or throw.
 */
template <typename Metrics, typename Report, typename Clock, typename T>
class scope_t {
public:
    static constexpr bool enabled = true;
    scope_t(profiler_t<Metrics, Report, Clock>& profiler, region_id_t region) noexcept;
    ~scope_t();
    /** @brief Closes once; later close calls and destruction do nothing. */
    void close() noexcept;
    scope_t(const scope_t&) = delete;
    scope_t& operator=(const scope_t&) = delete;
    scope_t(scope_t&&) = delete;
    scope_t& operator=(scope_t&&) = delete;

    /** @brief Accesses the local payload before close; later changes are not recorded. */
    T& metrics() noexcept;
    const T& metrics() const noexcept;

private:
    profiler_t<Metrics, Report, Clock>& m_profiler;
    T m_metrics {};
    std::optional<std::size_t> m_record;
    int m_exceptions;
    bool m_closed = false;
};

/**
 * @brief Borrows fixed-capacity record storage and owns registered region names.
 *
 * Register every application and producer region, then call start() before opening
 * scopes. start() freezes metadata for this profiler's lifetime. Region IDs remain
 * stable across reset(); copied names remain valid throughout capture and reporting.
 *
 * The caller provides exclusively borrowed storage that outlives the profiler.
 * Do not inspect or mutate that storage while recording. records() borrows it until
 * reset or reuse; region views remain valid after start until profiler destruction.
 * One thread owns all operations; scopes are synchronous and strictly nested.
 *
 * Recording performs no allocation, formatting, I/O, or locking. Metrics is a
 * producer value or variant of values with nonthrowing move construction and
 * destruction; scoped payloads also require nonthrowing default construction.
 * Producers guarantee these operations do not allocate. Report policy invocation
 * occurs only in report(). Every payload alternative except timing-only
 * std::monostate requires std::formatter.
 * A steady, nonthrowing Clock supports deterministic measurement tests.
 *
 * Exhaustion omits the new scope and all its descendants, counting each omission.
 * Inclusive parent durations still include omitted work; self time is unavailable
 * for parents missing direct-child timings. The omission count saturates rather than wraps.
 */
template <typename Metrics, typename Report, typename Clock>
class profiler_t {
public:
    static constexpr bool enabled = true;
    using record_type_t = record_t<Metrics>;

    explicit profiler_t(std::span<record_type_t> storage, Report report = {});
    profiler_t(const profiler_t&) = delete;
    profiler_t& operator=(const profiler_t&) = delete;
    profiler_t(profiler_t&&) = delete;
    profiler_t& operator=(profiler_t&&) = delete;

    /** @brief Copies a region name during setup; registration after start fails. */
    region_id_t register_region(std::string_view name);
    /** @brief Freezes registration and starts the first capture; repeated start fails. */
    void start();
    /** @brief Discards records and restarts elapsed time, preserving frozen registration. */
    void reset();

    /**
     * @brief Opens a scope with default-constructed T, or no payload for std::monostate.
     *
     * Requires start(), a region belonging to this profiler, and synchronous LIFO use.
     * Violating recording preconditions is a programming error checked by assertions.
     */
    template <typename T = std::monostate>
    scope_t<Metrics, Report, Clock, T> scope(region_id_t region) noexcept;

    /** @brief Returns finalized records; fails while scopes are active. */
    std::span<const record_type_t> records() const;
    std::span<const std::string> regions() const;
    std::size_t omitted() const;
    /** @brief Formats a quiescent capture; reporting errors propagate to the caller. */
    void report(std::ostream& out) const;

    // Ordinary pairing interface used by scope_t. Prefer scope() for lexical lifetimes.
    // begin/end have scope()'s preconditions; end must match the latest begin exactly.
    std::optional<std::size_t> begin(region_id_t region) noexcept;
    template <typename T>
    void end(std::optional<std::size_t> record, T&& metrics, bool unwinding) noexcept;

private:
    void require_quiescent() const;

    std::span<record_type_t> m_storage;
    Report m_report;
    std::vector<std::string> m_regions;
    typename Clock::time_point m_origin {};
    std::optional<std::size_t> m_parent;
    std::size_t m_count = 0;
    std::size_t m_depth = 0;
    std::size_t m_suppressed = 0;
    std::size_t m_omitted = 0;
    bool m_started = false;
};

struct disabled_scope_t {
    static constexpr bool enabled = false;
};

/** @brief Disables collection without constructing payloads or requiring formatters. */
template <typename Metrics, typename Clock>
class profiler_t<Metrics, void, Clock> {
public:
    static constexpr bool enabled = false;

    region_id_t register_region(std::string_view name) const noexcept;
    void start() const noexcept;
    void reset() const noexcept;
    template <typename T = std::monostate>
    disabled_scope_t scope(region_id_t region) const noexcept;
    void report(std::ostream& out) const noexcept;
};

using disabled_profiler_t = profiler_t<std::monostate, void>;

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::region_id_t>;

template <typename Metrics>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::record_t<Metrics>>;

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::text_report_t>;

template <typename Metrics, typename Report, typename Clock, typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::scope_t<Metrics, Report, Clock, T>>;

template <typename Metrics, typename Report, typename Clock>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t<Metrics, Report, Clock>>;

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::disabled_scope_t>;

} // namespace std

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

template <typename Metrics>
std::optional<std::chrono::nanoseconds> record_t<Metrics>::self() const noexcept {
    if (!m_children_complete) {
        return std::nullopt;
    }
    return m_elapsed - m_children_elapsed;
}

template <typename Metrics>
void text_report_t::operator()(std::ostream& out, std::span<const record_t<Metrics>> records, std::span<const std::string> regions, std::size_t omitted) const {
    for (const auto& record : records) {
        out << std::string(record.m_depth * 2, ' ') << regions[record.m_region];
        out << std::format(" inclusive={} ns self=", record.m_elapsed.count());
        if (const auto self = record.self()) {
            out << std::format("{} ns", self->count());
        } else {
            out << "unavailable";
        }
        if (record.m_unwinding) {
            out << " unwinding";
        }
        if (record.m_metrics) {
            const auto write = [&](const auto& metrics) {
                if constexpr (!std::same_as<std::remove_cvref_t<decltype(metrics)>, std::monostate>) {
                    out << std::format(" {}", metrics);
                }
            };
            if constexpr (requires { std::variant_size<Metrics>::value; }) {
                std::visit(write, *record.m_metrics);
            } else {
                write(*record.m_metrics);
            }
        }
        out << '\n';
    }
    out << std::format("capture: {} records, {} omitted, {}\n", records.size(), omitted, omitted == 0 ? "complete" : "incomplete");
}

template <typename Metrics, typename Report, typename Clock, typename T>
scope_t<Metrics, Report, Clock, T>::scope_t(profiler_t<Metrics, Report, Clock>& profiler, region_id_t region) noexcept:
    m_profiler(profiler),
    m_record(profiler.begin(region)),
    m_exceptions(std::uncaught_exceptions())
{
    static_assert(std::is_nothrow_default_constructible_v<T> && std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>);
}

template <typename Metrics, typename Report, typename Clock, typename T>
scope_t<Metrics, Report, Clock, T>::~scope_t() {
    close();
}

template <typename Metrics, typename Report, typename Clock, typename T>
void scope_t<Metrics, Report, Clock, T>::close() noexcept {
    if (!m_closed) {
        m_profiler.end(m_record, std::move(m_metrics), m_exceptions < std::uncaught_exceptions());
        m_closed = true;
    }
}

template <typename Metrics, typename Report, typename Clock, typename T>
T& scope_t<Metrics, Report, Clock, T>::metrics() noexcept {
    return m_metrics;
}

template <typename Metrics, typename Report, typename Clock, typename T>
const T& scope_t<Metrics, Report, Clock, T>::metrics() const noexcept {
    return m_metrics;
}

template <typename Metrics, typename Report, typename Clock>
profiler_t<Metrics, Report, Clock>::profiler_t(std::span<record_type_t> storage, Report report):
    m_storage(storage),
    m_report(std::move(report))
{
    static_assert(Clock::is_steady && noexcept(Clock::now()));
    static_assert(std::is_nothrow_move_constructible_v<Metrics> && std::is_nothrow_destructible_v<Metrics>);
    if constexpr (requires { std::variant_size<Metrics>::value; }) {
        []<std::size_t... I>(std::index_sequence<I...>) {
            static_assert(((std::same_as<std::variant_alternative_t<I, Metrics>, std::monostate> || std::formattable<std::variant_alternative_t<I, Metrics>, char>) && ...), "enabled profiling requires payload formatters");
        }(std::make_index_sequence<std::variant_size_v<Metrics>>{});
    } else {
        static_assert(std::same_as<Metrics, std::monostate> || std::formattable<Metrics, char>, "enabled profiling requires a payload formatter");
    }
}

template <typename Metrics, typename Report, typename Clock>
region_id_t profiler_t<Metrics, Report, Clock>::register_region(std::string_view name) {
    if (m_started) {
        throw std::logic_error("profiler_t::register_region requires setup before start");
    }
    const auto index = m_regions.size();
    m_regions.emplace_back(name);
    return {this, index};
}

template <typename Metrics, typename Report, typename Clock>
void profiler_t<Metrics, Report, Clock>::start() {
    if (m_started) {
        throw std::logic_error("profiler_t::start requires a profiler that has not started");
    }
    m_started = true;
    reset();
}

template <typename Metrics, typename Report, typename Clock>
void profiler_t<Metrics, Report, Clock>::reset() {
    require_quiescent();
    for (auto& record : m_storage.first(m_count)) {
        record.m_metrics.reset();
    }
    m_count = 0;
    m_omitted = 0;
    m_origin = Clock::now();
}

template <typename Metrics, typename Report, typename Clock>
template <typename T>
scope_t<Metrics, Report, Clock, T> profiler_t<Metrics, Report, Clock>::scope(region_id_t region) noexcept {
    return scope_t<Metrics, Report, Clock, T>(*this, region);
}

template <typename Metrics, typename Report, typename Clock>
std::span<const typename profiler_t<Metrics, Report, Clock>::record_type_t> profiler_t<Metrics, Report, Clock>::records() const {
    require_quiescent();
    return m_storage.first(m_count);
}

template <typename Metrics, typename Report, typename Clock>
std::span<const std::string> profiler_t<Metrics, Report, Clock>::regions() const {
    require_quiescent();
    return m_regions;
}

template <typename Metrics, typename Report, typename Clock>
std::size_t profiler_t<Metrics, Report, Clock>::omitted() const {
    require_quiescent();
    return m_omitted;
}

template <typename Metrics, typename Report, typename Clock>
void profiler_t<Metrics, Report, Clock>::report(std::ostream& out) const {
    require_quiescent();
    m_report(out, records(), regions(), m_omitted);
}

template <typename Metrics, typename Report, typename Clock>
std::optional<std::size_t> profiler_t<Metrics, Report, Clock>::begin(region_id_t region) noexcept {
    assert(m_started && region.m_owner == this && region.m_index < m_regions.size());
    ++m_depth;
    if (m_suppressed != 0 || m_count == m_storage.size()) {
        if (m_suppressed == 0 && m_parent) {
            m_storage[*m_parent].m_children_complete = false;
        }
        ++m_suppressed;
        if (m_omitted != std::numeric_limits<std::size_t>::max()) {
            ++m_omitted;
        }
        return std::nullopt;
    }
    const auto index = m_count++;
    auto& record = m_storage[index];
    record.m_parent = m_parent;
    record.m_region = region.m_index;
    record.m_depth = m_depth - 1;
    record.m_start = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_origin);
    record.m_elapsed = {};
    record.m_children_elapsed = {};
    record.m_children_complete = true;
    record.m_unwinding = false;
    record.m_metrics.reset();
    m_parent = index;
    return index;
}

template <typename Metrics, typename Report, typename Clock>
template <typename T>
void profiler_t<Metrics, Report, Clock>::end(std::optional<std::size_t> record_index, T&& metrics, bool unwinding) noexcept {
    assert(m_depth != 0);
    --m_depth;
    if (!record_index) {
        assert(m_suppressed != 0);
        --m_suppressed;
        return;
    }
    assert(m_suppressed == 0 && m_parent == record_index);
    auto& record = m_storage[*record_index];
    record.m_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_origin) - record.m_start;
    record.m_unwinding = unwinding;
    if constexpr (!std::same_as<std::remove_cvref_t<T>, std::monostate>) {
        if constexpr (std::same_as<std::remove_cvref_t<T>, Metrics>) {
            record.m_metrics.emplace(std::forward<T>(metrics));
        } else {
            record.m_metrics.emplace(std::in_place_type<std::remove_cvref_t<T>>, std::forward<T>(metrics));
        }
    }
    m_parent = record.m_parent;
    if (m_parent) {
        m_storage[*m_parent].m_children_elapsed += record.m_elapsed;
    }
}

template <typename Metrics, typename Report, typename Clock>
void profiler_t<Metrics, Report, Clock>::require_quiescent() const {
    if (!m_started || m_depth != 0) {
        throw std::logic_error("profiler_t operation requires start and no active scopes");
    }
}

template <typename Metrics, typename Clock>
region_id_t profiler_t<Metrics, void, Clock>::register_region(std::string_view) const noexcept {
    return {};
}

template <typename Metrics, typename Clock>
void profiler_t<Metrics, void, Clock>::start() const noexcept {
}

template <typename Metrics, typename Clock>
void profiler_t<Metrics, void, Clock>::reset() const noexcept {
}

template <typename Metrics, typename Clock>
template <typename T>
disabled_scope_t profiler_t<Metrics, void, Clock>::scope(region_id_t) const noexcept {
    return {};
}

template <typename Metrics, typename Clock>
void profiler_t<Metrics, void, Clock>::report(std::ostream&) const noexcept {
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::region_id_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::region_id_t& value, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "region({})", value.m_index);
        return out;
    }
};

template <typename Metrics>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::record_t<Metrics>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::record_t<Metrics>& value, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ region: {}, elapsed_ns: {}, unwinding: {} }}", value.m_region, value.m_elapsed.count(), value.m_unwinding);
        return out;
    }
};

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::text_report_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::text_report_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "text_report");
        return out;
    }
};

template <typename Metrics, typename Report, typename Clock, typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::scope_t<Metrics, Report, Clock, T>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::scope_t<Metrics, Report, Clock, T>&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "scope");
        return out;
    }
};

template <typename Metrics, typename Report, typename Clock>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t<Metrics, Report, Clock>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t<Metrics, Report, Clock>&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ enabled: {} }}", m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t<Metrics, Report, Clock>::enabled);
        return out;
    }
};

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::disabled_scope_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::disabled_scope_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "disabled_scope");
        return out;
    }
};

} // namespace std

#endif // M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H
