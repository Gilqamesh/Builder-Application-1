#ifndef M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H
# define M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H

# include <chrono>
# include <cstddef>
# include <format>
# include <optional>
# include <ostream>
# include <ratio>
# include <string>
# include <typeinfo>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

// Internal heterogeneous storage. Each T stays at a stable address as the pointer
// vector grows; first construction precedes timing and reuse preserves the data.
struct metric_base_t {
    explicit metric_base_t(const std::type_info& type) noexcept;
    virtual ~metric_base_t();
    virtual void report(std::ostream& out) const = 0;
    void start() noexcept;
    void stop() noexcept;
    void report_timing(std::ostream& out, std::chrono::nanoseconds now) const;

    const std::type_info* m_type;
    std::chrono::nanoseconds m_start {};
    std::chrono::nanoseconds m_elapsed {};
    std::chrono::nanoseconds m_max {};
    std::chrono::nanoseconds m_completed {};
    // Widen the total so repeated measurements do not overflow nanoseconds::rep.
    std::chrono::duration<long double, std::nano> m_total {};
    std::size_t m_count = 0;
    int m_exceptions = 0;
    bool m_active = false;
    bool m_unwinding = false;
};

template <typename T>
struct stored_metric_t final : metric_base_t {
    stored_metric_t() noexcept;
    void report(std::ostream& out) const override;

    std::optional<T> m_metrics;
};

// Production uses steady_clock; PROFILING_TEST_CLOCK substitutes the test clock.
std::chrono::nanoseconds clock_now() noexcept;
std::string format_elapsed(std::chrono::duration<long double, std::nano> elapsed);

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t>;

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::stored_metric_t<T>>;

} // namespace std

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

template <typename T>
stored_metric_t<T>::stored_metric_t() noexcept:
    metric_base_t(typeid(T))
{
}

template <typename T>
void stored_metric_t<T>::report(std::ostream& out) const {
    out << std::format("{}\n", *m_metrics);
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
        out = std::format_to(out, "{{ completed: {} }}", metric.m_count != 0);
        return out;
    }
};

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::stored_metric_t<T>> : formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_base_t> {};

} // namespace std

#endif // M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_HELPERS_H
