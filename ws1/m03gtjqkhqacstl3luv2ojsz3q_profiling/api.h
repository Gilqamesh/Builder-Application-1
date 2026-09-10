#ifndef M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H
# define M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H

# include "helpers.h"

# include <array>
# include <chrono>
# include <concepts>
# include <cstddef>
# include <format>
# include <functional>
# include <optional>
# include <ostream>
# include <stdexcept>
# include <tuple>
# include <type_traits>
# include <typeinfo>
# include <utility>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

/**
 * @brief Measures one tree node until stop() or destruction and opens explicit children.
 *
 * The owning profiler outlives its metrics. Metrics are single-threaded and must
 * finish in reverse start order. Explicit misuse is rejected without stopping;
 * destruction with active children terminates. Exception exits retain updates.
 * Default and disabled metrics are inactive and produce inactive children.
 */
class metric_t {
public:
    metric_t() noexcept;
    // Internal factory construction; storage.start() has already started this node.
    metric_t(storage_t& storage, metric_base_t& metric) noexcept;
    ~metric_t();
    metric_t(const metric_t&) = delete;
    metric_t& operator=(const metric_t&) = delete;
    metric_t(metric_t&&) = delete;
    metric_t& operator=(metric_t&&) = delete;

    /** @brief Returns whether this metric still has an active recorded measurement. */
    explicit operator bool() const noexcept;
    /**
     * @brief Opens a child identified by this node and T, inheriting recording from this measurement.
     *
     * Requires this measurement to be the active leaf. A stopped recorded metric
     * cannot open children. Type, construction, and allocation rules match profiler_t::metric().
     */
    template <typename T, typename... Args>
    metric_t metric(Args&&... args);
    /**
     * @brief Invokes function with the stored T& while active, rejecting a mismatched type.
     *
     * The reference is borrowed for this invocation and exceptions propagate.
     * Put metric-only work inside the callable; captures still evaluate normally.
     * Inactive and stopped metrics skip the callable and the runtime type check.
     */
    template <typename T, typename F>
    requires std::invocable<F, T&>
    void update(F&& function);
    /** @brief Finishes a leaf measurement; subsequent updates, stops, and destruction do nothing. */
    void stop();

private:
    storage_t* m_storage = nullptr;
    metric_base_t* m_metric = nullptr;
};

/**
 * @brief Owns persistent metric data and inclusive timing statistics for each metric path.
 *
 * Single-threaded and initially enabled. Root creation samples enablement;
 * descendants inherit it, so recording changes affect subsequent roots.
 * Disabled creation skips lookup, allocation, T construction, and clock reads;
 * argument expressions still evaluate. Construction allocates nothing.
 *
 * Roots and siblings execute sequentially. Every child finishes before its parent.
 * A type under different parents has independent data; repeated calls on the same
 * path reuse data. Recursive types create distinct nodes at each depth.
 *
 * Reads and reports require all metrics stopped. Returned data pointers remain
 * valid until profiler destruction; access requires all metrics stopped. Data
 * borrowed inside T must remain valid through deferred use.
 *
 * Keep application work outside update() callbacks so disabling profiling does
 * not disable that work. The same metric type can label a root and its child;
 * their counters are independent because their complete paths differ.
 *
 * @code{.cpp}
 * #include <m03gtjqkhqacstl3luv2ojsz3q_profiling/api.h>
 *
 * #include <cstddef>
 * #include <format>
 * #include <iostream>
 * #include <string_view>
 *
 * namespace app {
 * struct work_t { std::size_t completed = 0; };
 * } // namespace app
 *
 * template <>
 * struct std::formatter<app::work_t> : std::formatter<std::string_view> {
 *     auto format(const app::work_t& work, auto& ctx) const {
 *         auto out = ctx.out();
 *         out = std::format_to(out, "work completed={}", work.completed);
 *         return out;
 *     }
 * };
 *
 * int main() {
 *     std::size_t application_steps = 0;
 *     std::size_t updates = 0;
 *     m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t profiler;
 *     const auto process = [&] {
 *         auto root = profiler.metric<app::work_t>();
 *         {
 *             auto child = root.metric<app::work_t>();
 *             ++application_steps; // Application work always executes.
 *             child.update<app::work_t>([&](app::work_t& work) {
 *                 ++work.completed;
 *                 ++updates;
 *             });
 *         } // Child finishes before root.
 *         root.update<app::work_t>([&](app::work_t& work) {
 *             ++work.completed;
 *             ++updates;
 *         });
 *     };
 *     process();
 *     profiler.enabled() = false;
 *     process(); // Inactive root/child skip both update callbacks.
 *     profiler.report(std::cout); // All scopes finished; recorded results remain.
 *     return application_steps == 2 && updates == 2 ? 0 : 1;
 * }
 * @endcode
 */
class profiler_t {
public:
    profiler_t() noexcept;
    ~profiler_t();
    profiler_t(const profiler_t&) = delete;
    profiler_t& operator=(const profiler_t&) = delete;
    profiler_t(profiler_t&&) = delete;
    profiler_t& operator=(profiler_t&&) = delete;

    /** @brief Borrows the flag sampled when opening subsequent root measurements. */
    bool& enabled() noexcept;
    /** @brief Reads the flag without changing existing measurements or stored results. */
    const bool& enabled() const noexcept;
    /**
     * @brief Opens a root measurement when enabled and idle.
     *
     * T needs nonthrowing construction/destruction and a const std::formatter;
     * keep construction, destruction, and updates allocation-free. T need not move.
     * Arguments construct T only on first use of this path. First use may allocate;
     * allocation failure preserves completed results and the active branch.
     * Producer construction must not reenter the profiler; reads and new
     * measurements are rejected during construction.
     * Reuse and valid stopping perform no profiler allocation, formatting, I/O, or locking.
     */
    template <typename T, typename... Args>
    metric_t metric(Args&&... args);
    /**
     * @brief Reports nodes before their children, with siblings in first-use order.
     *
     * One line per metric, with tree connectors and aligned count, last/min/mean/max/
     * total duration, age since last completion, and data columns. Statistics include
     * exception exits and every completion since profiler construction. Durations
     * use SI prefixes from ns to Es and up to three decimal places.
     * The formatter's first whitespace-delimited word labels the metric; remaining
     * text is data. Line breaks and tabs become spaces. Labels use single-column
     * UTF-8 characters for alignment. Columns expand without truncating or wrapping
     * output. The latest exception exit adds [unwinding].
     * Nonempty reports start with the column headings. Empty reports say
     * "No measurements." or "Profiling disabled." according to enablement.
     * Reporting preserves results even on failure. Measurements on the same path
     * accumulate; repeated reports do not reset results. First-use tree order is
     * preserved rather than chronological event order. An active measurement
     * causes std::logic_error. Allocation and metric formatter exceptions
     * propagate; stream failures follow out's exception settings. Output may
     * be partial.
     */
    void report(std::ostream& out) const;

    /** @brief Returns data at the complete root-to-leaf type path, or null when absent. */
    template <typename T, typename... Path>
    const std::tuple_element_t<sizeof...(Path), std::tuple<T, Path...>>* metrics() const;
    /** @brief Returns the path's latest inclusive duration, or no value when absent. */
    template <typename T, typename... Path>
    std::optional<std::chrono::nanoseconds> elapsed() const;
    /** @brief Returns whether the path's latest completion occurred during unwinding, or no value when absent. */
    template <typename T, typename... Path>
    std::optional<bool> unwinding() const;
    /** @brief Counts stored nodes across the tree. */
    std::size_t size() const;

private:
    template <typename T, typename... Path>
    const metric_base_t* find() const;

    storage_t m_storage;
    bool m_enabled = true;
};

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t>;
template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t>;

} // namespace std

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

template <typename T, typename... Args>
metric_t metric_t::metric(Args&&... args) {
    if (!m_storage) {
        return metric_t();
    }
    if (!m_metric) {
        throw std::logic_error("metric_t::metric cannot open a child after stop");
    }
    return metric_t(*m_storage, m_storage->start<T>(m_metric, std::forward<Args>(args)...));
}

template <typename T, typename F>
    requires std::invocable<F, T&>
void metric_t::update(F&& function) {
    static_assert(std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>>);
    if (m_metric) {
        if (*m_metric->type != typeid(T)) {
            throw std::logic_error("metric_t::update type does not match this metric's stored data");
        }
        std::invoke(std::forward<F>(function), *static_cast<stored_metric_t<T>*>(m_metric)->metrics);
    }
}

template <typename T, typename... Args>
metric_t profiler_t::metric(Args&&... args) {
    if (!m_enabled) {
        return metric_t();
    }
    return metric_t(m_storage, m_storage.start<T>(nullptr, std::forward<Args>(args)...));
}

template <typename T, typename... Path>
const std::tuple_element_t<sizeof...(Path), std::tuple<T, Path...>>* profiler_t::metrics() const {
    using leaf_t = std::tuple_element_t<sizeof...(Path), std::tuple<T, Path...>>;
    const auto* metric = static_cast<const stored_metric_t<leaf_t>*>(find<T, Path...>());
    return metric ? &*metric->metrics : nullptr;
}

template <typename T, typename... Path>
std::optional<std::chrono::nanoseconds> profiler_t::elapsed() const {
    const auto* metric = find<T, Path...>();
    return metric ? std::optional(metric->elapsed) : std::nullopt;
}

template <typename T, typename... Path>
std::optional<bool> profiler_t::unwinding() const {
    const auto* metric = find<T, Path...>();
    return metric ? std::optional(metric->unwinding) : std::nullopt;
}

template <typename T, typename... Path>
const metric_base_t* profiler_t::find() const {
    static_assert(std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>>);
    static_assert(((std::is_object_v<Path> && std::same_as<Path, std::remove_cv_t<Path>>) && ...));
    const std::array path {&typeid(T), &typeid(Path)...};
    return m_storage.find(path);
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t> : formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t> {
    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t& metric, auto& ctx) const {
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
