#ifndef M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H
# define M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H

# include "helpers.h"

# include <chrono>
# include <concepts>
# include <cstddef>
# include <format>
# include <functional>
# include <memory>
# include <optional>
# include <ostream>
# include <stdexcept>
# include <type_traits>
# include <typeinfo>
# include <utility>
# include <vector>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

/**
 * @brief Times one measurement until stop() or destruction.
 *
 * The profiler outlives its active metrics. Default metrics are inactive.
 */
template <typename T>
class metric_t {
public:
    metric_t() noexcept;
    // Used by profiler_t::metric().
    template <typename... Args>
    explicit metric_t(stored_metric_t<T>& stored_metric, Args&&... args);
    ~metric_t();
    metric_t(const metric_t&) = delete;
    metric_t& operator=(const metric_t&) = delete;
    metric_t(metric_t&&) = delete;
    metric_t& operator=(metric_t&&) = delete;

    explicit operator bool() const noexcept;
    /**
     * @brief Invokes function immediately with T& while active; otherwise does nothing.
     *
     * The reference is borrowed for this invocation; the callable is not retained
     * and its exceptions propagate. Put metric-only work inside the callable:
     * capture expressions are evaluated even when inactive.
     */
    template <typename F>
        requires std::invocable<F, T&>
    void update(F&& function) noexcept(std::is_nothrow_invocable_v<F, T&>);
    /** @brief Stops timing once; subsequent updates, stops, and destruction do nothing. */
    void stop() noexcept;

private:
    static_assert(std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>>, "profiling requires an unqualified metric object type");
    static_assert(std::is_nothrow_destructible_v<T>, "profiling requires nonthrowing metric destruction");
    static_assert(std::formattable<const T, char>, "profiling requires a usable std::formatter for the const metric");

    stored_metric_t<T>* m_stored_metric = nullptr;
};

/**
 * @brief Retains the latest data and lifetime timing statistics for each metric type.
 *
 * Single-threaded and initially disabled. Enablement affects new measurements;
 * active measurements finish normally. Disabled creation skips lookup, allocation,
 * T construction, and clock reads; argument expressions still evaluate normally.
 * Storage grows internally; constructing a profiler allocates nothing.
 *
 * One measurement of each type may be active; different types may overlap.
 * Starting a type replaces its data before timing begins. Durations are inclusive
 * monotonic elapsed time. Stopping also records partial data on exception exits.
 *
 * Reads, reports, and destruction require all metrics stopped. Returned data
 * pointers expire at that type's next start or profiler destruction. Data borrowed
 * inside T must remain valid through deferred reporting.
 */
class profiler_t {
public:
    profiler_t() noexcept;
    ~profiler_t();
    profiler_t(const profiler_t&) = delete;
    profiler_t& operator=(const profiler_t&) = delete;
    profiler_t(profiler_t&&) = delete;
    profiler_t& operator=(profiler_t&&) = delete;

    bool& enabled() noexcept;
    const bool& enabled() const noexcept;
    /**
     * @brief Creates a measurement, rejecting an already-active type when enabled.
     *
     * T needs nonthrowing construction/destruction and a const std::formatter;
     * keep construction, destruction, and updates allocation-free. T need not move.
     * First enabled use may allocate; failure preserves previous results. Reuse
     * and stopping perform no profiler allocation, formatting, I/O, or locking.
     */
    template <typename T, typename... Args>
    metric_t<T> metric(Args&&... args);
    /**
     * @brief Reports latest data and count, last/mean/max/total duration, and age since completion.
     *
     * Timing includes every completion since construction, including exception exits.
     * Order is descending total duration, with registration order breaking ties.
     * Durations use SI prefixes from ns to Es and up to three decimal places.
     * Disabling and reporting preserve results, including when reporting fails.
     */
    void report(std::ostream& out) const;

    /** @brief Returns the latest completed T, or null when absent. */
    template <typename T>
    const T* metrics() const;
    /** @brief Returns T's latest duration, or no value when absent. */
    template <typename T>
    std::optional<std::chrono::nanoseconds> elapsed() const;
    /** @brief Returns whether T's latest completion occurred during unwinding, or no value when absent. */
    template <typename T>
    std::optional<bool> unwinding() const;
    std::size_t size() const;

private:
    const metric_base_t* find(const std::type_info& type) const;
    void require_stopped() const;

    std::vector<std::unique_ptr<metric_base_t>> m_metrics;
    bool m_enabled = false;
};

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t<T>>;

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t>;

} // namespace std

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

template <typename T>
metric_t<T>::metric_t() noexcept = default;

template <typename T>
template <typename... Args>
metric_t<T>::metric_t(stored_metric_t<T>& stored_metric, Args&&... args):
    m_stored_metric(&stored_metric)
{
    static_assert(std::is_nothrow_constructible_v<T, Args...>, "profiling requires nonthrowing metric construction from the supplied arguments");
    if (stored_metric.m_active) {
        throw std::logic_error("profiler_t::metric cannot start an already-active metric type");
    }
    // Block reads and same-type reentry during data destruction and construction.
    stored_metric.m_active = true;
    stored_metric.m_metrics.emplace(std::forward<Args>(args)...);
    stored_metric.start();
}

template <typename T>
metric_t<T>::~metric_t() {
    stop();
}

template <typename T>
metric_t<T>::operator bool() const noexcept {
    return m_stored_metric != nullptr;
}

template <typename T>
template <typename F>
    requires std::invocable<F, T&>
void metric_t<T>::update(F&& function) noexcept(std::is_nothrow_invocable_v<F, T&>) {
    if (m_stored_metric) {
        std::invoke(std::forward<F>(function), *m_stored_metric->m_metrics);
    }
}

template <typename T>
void metric_t<T>::stop() noexcept {
    if (auto* stored_metric = std::exchange(m_stored_metric, nullptr)) {
        stored_metric->stop();
    }
}

template <typename T, typename... Args>
metric_t<T> profiler_t::metric(Args&&... args) {
    static_assert(std::is_nothrow_constructible_v<T, Args...>, "profiling requires nonthrowing metric construction from the supplied arguments");
    if (!m_enabled) {
        return metric_t<T>();
    }
    for (const auto& metric : m_metrics) {
        if (*metric->m_type == typeid(T)) {
            return metric_t<T>(static_cast<stored_metric_t<T>&>(*metric), std::forward<Args>(args)...);
        }
    }
    if (m_metrics.capacity() == 0) {
        m_metrics.reserve(16);
    }
    auto stored_metric = std::make_unique<stored_metric_t<T>>();
    auto* destination = stored_metric.get();
    m_metrics.push_back(std::move(stored_metric));
    return metric_t<T>(*destination, std::forward<Args>(args)...);
}

template <typename T>
const T* profiler_t::metrics() const {
    static_assert(std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>>);
    const auto* metric = static_cast<const stored_metric_t<T>*>(find(typeid(T)));
    return metric && metric->m_count ? &*metric->m_metrics : nullptr;
}

template <typename T>
std::optional<std::chrono::nanoseconds> profiler_t::elapsed() const {
    const auto* metric = find(typeid(T));
    return metric && metric->m_count ? std::optional(metric->m_elapsed) : std::nullopt;
}

template <typename T>
std::optional<bool> profiler_t::unwinding() const {
    const auto* metric = find(typeid(T));
    return metric && metric->m_count ? std::optional(metric->m_unwinding) : std::nullopt;
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t<T>> : formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t> {
    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t<T>& metric, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ active: {} }}", static_cast<bool>(metric));
        return out;
    }
};

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t> : formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t> {
    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t& profiler, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ enabled: {} }}", profiler.enabled());
        return out;
    }
};

} // namespace std

#endif // M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H
