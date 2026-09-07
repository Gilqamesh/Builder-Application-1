#ifndef M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H
# define M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H

# include "helpers.h"

# include <cassert>
# include <chrono>
# include <concepts>
# include <cstddef>
# include <exception>
# include <format>
# include <memory>
# include <optional>
# include <ostream>
# include <type_traits>
# include <typeinfo>
# include <utility>
# include <vector>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

// Internal heterogeneous storage supporting profiler_t::metric<T>().
struct metric_base_t {
    explicit metric_base_t(const std::type_info& type) noexcept;
    virtual ~metric_base_t();
    virtual const void* metrics() const noexcept = 0;
    virtual void report(std::ostream& out) const = 0;

    const std::type_info* m_type;
    std::optional<std::chrono::nanoseconds> m_elapsed;
    bool m_unwinding = false;
};

template <typename T>
struct stored_metric_t final : metric_base_t {
    stored_metric_t() noexcept;
    const void* metrics() const noexcept override;
    void report(std::ostream& out) const override;

    std::optional<T> m_metrics;
};

class profiler_t;

/**
 * @brief Owns one measurement until stop() or destruction replaces the profiler's latest metric of its type.
 *
 * A default metric is inactive and constructs no T or clock observation. Active
 * metrics cannot be copied or moved and may stop independently of other metrics.
 * Mutable data access requires an active metric. The profiler outlives it.
 */
template <typename T>
class metric_t {
public:
    metric_t() noexcept;
    // Pairing constructor used by profiler_t::metric().
    template <typename... Args>
    metric_t(profiler_t& profiler, stored_metric_t<T>& stored_metric, Args&&... args) noexcept;
    ~metric_t();
    metric_t(const metric_t&) = delete;
    metric_t& operator=(const metric_t&) = delete;
    metric_t(metric_t&&) = delete;
    metric_t& operator=(metric_t&&) = delete;

    explicit operator bool() const noexcept;
    T* operator->() noexcept;
    const T* operator->() const noexcept;
    /** @brief Stops timing and replaces the stored metric once; subsequent stops and destruction do nothing. */
    void stop() noexcept;

private:
    static_assert(std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>>, "profiling requires an unqualified metric object type");
    static_assert(std::is_nothrow_move_constructible_v<T>, "profiling requires nonthrowing metric move construction");
    static_assert(std::is_nothrow_move_assignable_v<T>, "profiling requires nonthrowing metric move assignment");
    static_assert(std::is_nothrow_destructible_v<T>, "profiling requires nonthrowing metric destruction");
    static_assert(std::formattable<const T, char>, "profiling requires a usable std::formatter for the const metric");

    profiler_t* m_profiler = nullptr;
    stored_metric_t<T>* m_stored_metric = nullptr;
    std::optional<T> m_metrics;
    std::chrono::nanoseconds m_start {};
    int m_exceptions = 0;
};

/**
 * @brief Owns the latest completed metric of each concrete type and reports it on demand.
 *
 * Each profiler operates on one thread. Metric creation starts timing after lookup
 * and construction; completion replaces both data and inclusive monotonic elapsed
 * nanoseconds. The last completion of a type wins, including recursion and unwinding.
 * Types not measured again retain their previous data. There is no capture start or reset.
 *
 * Storage grows internally. Construction and first use of a type may allocate and
 * fail without discarding completed metrics. Reusing a type and stopping perform no
 * profiler-owned allocation. Recording performs no formatting, I/O, or locking.
 *
 * T has nonthrowing construction from supplied arguments, move construction, move
 * assignment, and destruction, and a usable const std::formatter. Producers keep
 * metric operations and counter updates allocation-free. Borrowed data inside T
 * remains valid through its deferred use. Moves must preserve those data lifetimes.
 *
 * Reads, reports, attachment changes, and destruction require no active metrics,
 * including construction and replacement. Returned data pointers expire on that
 * type's next replacement or profiler destruction. Report errors preserve metrics.
 */
class profiler_t {
public:
    profiler_t();
    ~profiler_t();
    profiler_t(const profiler_t&) = delete;
    profiler_t& operator=(const profiler_t&) = delete;
    profiler_t(profiler_t&&) = delete;
    profiler_t& operator=(profiler_t&&) = delete;

    bool quiescent() const noexcept;
    template <typename T, typename... Args>
    metric_t<T> metric(Args&&... args);
    /** @brief Returns the latest completed T, or null when none exists. */
    template <typename T>
    const T* metrics() const;
    /** @brief Returns the latest completed duration for T, or no value when none exists. */
    template <typename T>
    std::optional<std::chrono::nanoseconds> elapsed() const;
    /** @brief Returns whether T's latest completion occurred during exception unwinding, or no value when absent. */
    template <typename T>
    std::optional<bool> unwinding() const;
    std::size_t size() const;
    /**
     * @brief Reports completed metrics in first-registration order using their const formatters.
     *
     * Elapsed time uses SI prefixes in steps of 1000 from ns through Es, with up
     * to three decimal places. Rounding promotes a value that reaches the next unit.
     */
    void report(std::ostream& out) const;

    // Pairing operations used by metric_t, including its construction/replacement.
    void begin() noexcept;
    void end() noexcept;

private:
    const metric_base_t* find(const std::type_info& type) const;
    void require_quiescent() const;

    std::vector<std::unique_ptr<metric_base_t>> m_metrics;
    std::size_t m_active = 0;
};

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t>;

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::stored_metric_t<T>>;

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t<T>>;

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t>;

} // namespace std

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

template <typename T>
stored_metric_t<T>::stored_metric_t() noexcept:
    metric_base_t(typeid(T))
{
}

template <typename T>
const void* stored_metric_t<T>::metrics() const noexcept {
    return m_metrics ? &*m_metrics : nullptr;
}

template <typename T>
void stored_metric_t<T>::report(std::ostream& out) const {
    if (m_elapsed) {
        out << std::format("{} elapsed={}", *m_metrics, format_elapsed(*m_elapsed));
        if (m_unwinding) {
            out << " unwinding";
        }
        out << '\n';
    }
}

template <typename T>
metric_t<T>::metric_t() noexcept = default;

template <typename T>
template <typename... Args>
metric_t<T>::metric_t(profiler_t& profiler, stored_metric_t<T>& stored_metric, Args&&... args) noexcept:
    m_profiler(&profiler), m_stored_metric(&stored_metric)
{
    static_assert(std::is_nothrow_constructible_v<T, Args...>, "profiling requires nonthrowing metric construction from the supplied arguments");
    profiler.begin();
    m_metrics.emplace(std::forward<Args>(args)...);
    m_exceptions = std::uncaught_exceptions();
    m_start = clock_now();
}

template <typename T>
metric_t<T>::~metric_t() {
    stop();
}

template <typename T>
metric_t<T>::operator bool() const noexcept {
    return m_profiler != nullptr;
}

template <typename T>
T* metric_t<T>::operator->() noexcept {
    assert(m_profiler);
    return &*m_metrics;
}

template <typename T>
const T* metric_t<T>::operator->() const noexcept {
    assert(m_profiler);
    return &*m_metrics;
}

template <typename T>
void metric_t<T>::stop() noexcept {
    if (!m_profiler) {
        return;
    }
    const auto elapsed = clock_now() - m_start;
    const bool unwinding = m_exceptions < std::uncaught_exceptions();
    auto* profiler = std::exchange(m_profiler, nullptr);
    if (m_stored_metric->m_metrics) {
        *m_stored_metric->m_metrics = std::move(*m_metrics);
    } else {
        m_stored_metric->m_metrics.emplace(std::move(*m_metrics));
    }
    m_stored_metric->m_elapsed = elapsed;
    m_stored_metric->m_unwinding = unwinding;
    m_metrics.reset();
    m_stored_metric = nullptr;
    profiler->end();
}

template <typename T, typename... Args>
metric_t<T> profiler_t::metric(Args&&... args) {
    static_assert(std::is_nothrow_constructible_v<T, Args...>, "profiling requires nonthrowing metric construction from the supplied arguments");
    for (const auto& metric : m_metrics) {
        if (*metric->m_type == typeid(T)) {
            return metric_t<T>(*this, static_cast<stored_metric_t<T>&>(*metric), std::forward<Args>(args)...);
        }
    }
    auto stored_metric = std::make_unique<stored_metric_t<T>>();
    auto* destination = stored_metric.get();
    m_metrics.push_back(std::move(stored_metric));
    return metric_t<T>(*this, *destination, std::forward<Args>(args)...);
}

template <typename T>
const T* profiler_t::metrics() const {
    static_assert(std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>>);
    const auto* metric = find(typeid(T));
    return metric ? static_cast<const T*>(metric->metrics()) : nullptr;
}

template <typename T>
std::optional<std::chrono::nanoseconds> profiler_t::elapsed() const {
    const auto* metric = find(typeid(T));
    return metric ? metric->m_elapsed : std::nullopt;
}

template <typename T>
std::optional<bool> profiler_t::unwinding() const {
    const auto* metric = find(typeid(T));
    return metric && metric->m_elapsed ? std::optional(metric->m_unwinding) : std::nullopt;
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t& metric, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ completed: {} }}", metric.m_elapsed.has_value());
        return out;
    }
};

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::stored_metric_t<T>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::stored_metric_t<T>& metric, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ completed: {} }}", metric.m_elapsed.has_value());
        return out;
    }
};

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t<T>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t<T>& metric, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ active: {} }}", static_cast<bool>(metric));
        return out;
    }
};

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t& metric, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ quiescent: {} }}", metric.quiescent());
        return out;
    }
};

} // namespace std

#endif // M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H
