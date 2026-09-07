#ifndef M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H
# define M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H

# include <array>
# include <chrono>
# include <concepts>
# include <cstddef>
# include <format>
# include <memory>
# include <optional>
# include <ostream>
# include <ratio>
# include <span>
# include <stdexcept>
# include <string>
# include <string_view>
# include <type_traits>
# include <typeinfo>
# include <utility>
# include <vector>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

// Report columns: metric, count, last, min, mean, max, total, age, data.
using report_row_t = std::array<std::string, 9>;

// Persistent heterogeneous tree nodes. Only the measurement handle is temporary.
struct metric_base_t {
    explicit metric_base_t(const std::type_info& type, metric_base_t* parent) noexcept;
    virtual ~metric_base_t();
    virtual std::string format() const = 0;
    void start() noexcept;
    void stop() noexcept;
    void append_report(std::vector<report_row_t>& rows, std::chrono::nanoseconds now, const std::string& prefix, std::string_view branch) const;

    const std::type_info* type;
    metric_base_t* parent;
    std::vector<std::unique_ptr<metric_base_t>> children;
    std::chrono::nanoseconds started {};
    std::chrono::nanoseconds elapsed {};
    std::chrono::nanoseconds minimum {};
    std::chrono::nanoseconds maximum {};
    std::chrono::nanoseconds completed {};
    std::chrono::duration<long double, std::nano> total {};
    std::size_t count = 0;
    int exceptions = 0;
    bool unwinding = false;
};

template <typename T>
struct stored_metric_t final : metric_base_t {
    explicit stored_metric_t(metric_base_t* parent) noexcept;
    std::string format() const override;

    static_assert(std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>>, "profiling requires an unqualified metric object type");
    static_assert(std::is_nothrow_destructible_v<T>, "profiling requires nonthrowing metric destruction");
    static_assert(std::formattable<const T, char>, "profiling requires a usable std::formatter for the const metric");
    std::optional<T> metrics;
};

// Owns storage and validates the active branch; the caller selects the parent.
struct storage_t {
    template <typename T, typename... Args>
    metric_base_t& start(metric_base_t* parent, Args&&... args);
    void stop(metric_base_t& metric);
    void report(std::ostream& out, std::chrono::nanoseconds now) const;
    void require_stopped() const;
    const metric_base_t* find(std::span<const std::type_info* const> path) const;

    std::vector<std::unique_ptr<metric_base_t>> roots;
    metric_base_t* active = nullptr;
    std::size_t count = 0;
    bool constructing = false;
};

// Production uses steady_clock; PROFILING_TEST_CLOCK substitutes the test clock.
std::chrono::nanoseconds clock_now() noexcept;
std::string format_elapsed(std::chrono::duration<long double, std::nano> elapsed);
std::string format_count(std::size_t count);
std::size_t report_width(std::string_view text) noexcept;

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t>;
template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::stored_metric_t<T>>;
template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::storage_t>;

} // namespace std

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

template <typename T>
stored_metric_t<T>::stored_metric_t(metric_base_t* parent) noexcept:
    metric_base_t(typeid(T), parent)
{
}

template <typename T>
std::string stored_metric_t<T>::format() const {
    return std::format("{}", *metrics);
}

template <typename T, typename... Args>
metric_base_t& storage_t::start(metric_base_t* parent, Args&&... args) {
    static_assert(std::is_nothrow_constructible_v<T, Args...>, "profiling requires nonthrowing metric construction from the supplied arguments");
    if (constructing || active != parent) {
        throw std::logic_error("metric creation requires the active leaf as parent, or an idle profiler for a root");
    }
    auto& siblings = parent ? parent->children : roots;
    for (const auto& metric : siblings) {
        if (*metric->type == typeid(T)) {
            metric->start();
            active = metric.get();
            return *active;
        }
    }
    // Reserve before constructing producer data so publication cannot fail afterward.
    if (siblings.size() == siblings.capacity()) {
        siblings.reserve(siblings.empty() ? 4 : siblings.size() * 2);
    }
    auto stored_metric = std::make_unique<stored_metric_t<T>>(parent);
    constructing = true;
    stored_metric->metrics.emplace(std::forward<Args>(args)...);
    constructing = false;
    auto* destination = stored_metric.get();
    siblings.push_back(std::move(stored_metric));
    ++count;
    destination->start();
    active = destination;
    return *destination;
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
        out = std::format_to(out, "{{ completed: {} }}", metric.count != 0);
        return out;
    }
};

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::stored_metric_t<T>> : formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t> {};

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::storage_t> : formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t> {
    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::storage_t& storage, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ nodes: {} }}", storage.count);
        return out;
    }
};

} // namespace std

#endif // M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H
