#ifndef M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H
# define M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H

# include <cassert>
# include <chrono>
# include <concepts>
# include <cstddef>
# include <exception>
# include <format>
# include <iterator>
# include <memory>
# include <optional>
# include <ostream>
# include <span>
# include <type_traits>
# include <typeinfo>
# include <utility>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

// Opaque capture storage; defined only in the implementation.
struct entry_t;
class profiler_t;

/**
 * @brief Borrows one finalized record, including its retained, immutable payload.
 *
 * Views and payload pointers expire on reset or collector destruction. Read at
 * quiescent boundaries. Record indices identify occurrences, including recursion.
 * Durations are inclusive monotonic elapsed nanoseconds, not CPU usage.
 */
class record_t {
public:
    explicit record_t(const entry_t& entry) noexcept;

    std::optional<std::size_t> parent() const noexcept;
    std::size_t depth() const noexcept;
    std::chrono::nanoseconds start() const noexcept;
    std::chrono::nanoseconds elapsed() const noexcept;
    /** @brief Returns self time only when all direct-child timings were retained. */
    std::optional<std::chrono::nanoseconds> self() const noexcept;
    bool unwinding() const noexcept;
    /** @brief Returns the retained payload when its concrete type is T, otherwise null. */
    template <typename T>
    const T* metrics() const noexcept;
    void report(std::ostream& out) const;

private:
    const void* metrics(const std::type_info& type) const noexcept;

    const entry_t* m_entry;
};

/** @brief Borrows finalized records in opening order until reset or destruction. */
class records_t {
public:
    class iterator_t {
    public:
        using value_type = record_t;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept = std::forward_iterator_tag;

        iterator_t() noexcept;
        iterator_t(const entry_t* entry, std::size_t remaining) noexcept;
        record_t operator*() const noexcept;
        iterator_t& operator++() noexcept;
        iterator_t operator++(int) noexcept;
        bool operator==(const iterator_t& other) const noexcept;

    private:
        const entry_t* m_entry;
        std::size_t m_remaining;
    };

    records_t(const entry_t* first, std::size_t size) noexcept;
    iterator_t begin() const noexcept;
    iterator_t end() const noexcept;
    std::size_t size() const noexcept;
    bool empty() const noexcept;
    /** @brief Looks up an existing record by opening index in linear time. */
    record_t operator[](std::size_t index) const noexcept;

private:
    const entry_t* m_first;
    std::size_t m_size;
};

/**
 * @brief Owns closure of one synchronous measurement and provides nullable access to its metrics payload.
 *
 * Handles cannot be copied or moved and close in reverse opening order. A false
 * handle has no payload; an overflow-suppressed handle still owns exit bookkeeping.
 * Closing retains the payload for reporting and makes the handle false. Payload
 * mutation is confined to the active measurement. Destruction closes its timing
 * interval at scope exit; explicit close() may end it earlier. The collector and
 * its storage outlive all active handles.
 */
template <typename T>
class metric_t {
public:
    metric_t() noexcept;
    // Pairing constructor used by profiler_t::metric().
    metric_t(profiler_t& profiler, entry_t* entry, T* metrics) noexcept;
    ~metric_t();
    metric_t(const metric_t&) = delete;
    metric_t& operator=(const metric_t&) = delete;
    metric_t(metric_t&&) = delete;
    metric_t& operator=(metric_t&&) = delete;

    explicit operator bool() const noexcept;
    T* operator->() noexcept;
    const T* operator->() const noexcept;
    /** @brief Closes once; subsequent closure and destruction do nothing. */
    void close() noexcept;

private:
    profiler_t* m_profiler;
    entry_t* m_entry;
    T* m_metrics;
    int m_exceptions;
};

/**
 * @brief Borrows an optional collector for measurements with producer-owned metrics payloads.
 *
 * A default context is unattached: measurements read no clock, construct no payload,
 * and record nothing. Argument expressions still evaluate, and payload formatter
 * and construction requirements still apply at compilation. Copying or replacing
 * an attachment requires both collectors to have no active measurements; capture need
 * not have started. The collector outlives every use of an attached context.
 */
class context_t {
public:
    context_t() noexcept;
    explicit context_t(profiler_t& profiler);
    context_t(const context_t& other);
    context_t& operator=(const context_t& other);

    template <typename T, typename... Args>
    metric_t<T> metric(Args&&... args) const noexcept;

private:
    profiler_t* m_profiler;
};

/**
 * @brief Collects heterogeneous typed measurements in exclusively borrowed byte storage.
 *
 * Storage outlives the collector. Each collector operates on one thread with
 * synchronous, strictly nested measurements. Call start() once before attached recording;
 * reset() destroys retained payloads and begins another capture without changing
 * contexts. Reads, reports, reset, and attachment changes require quiescence.
 * Destruction requires no active measurements and destroys every retained payload.
 *
 * Recording performs no profiler-owned allocation, formatting, I/O, or locking.
 * T must be an unqualified object type with a usable const std::formatter and
 * nonthrowing construction from the supplied arguments and destruction. Producers
 * guarantee payload operations and counter updates do not allocate. Borrowed data
 * inside a payload remains valid through its deferred use in reporting.
 *
 * Payloads are constructed directly in aligned storage and never moved. Their
 * construction precedes linking and measurement timing. Exhaustion omits a measurement and all its
 * descendants without constructing their payloads or reading the clock. Each
 * omission is counted with saturation; inclusive parent time includes omitted work.
 * Report/formatter errors propagate without discarding the capture.
 */
class profiler_t {
public:
    explicit profiler_t(std::span<std::byte> storage) noexcept;
    ~profiler_t();
    profiler_t(const profiler_t&) = delete;
    profiler_t& operator=(const profiler_t&) = delete;
    profiler_t(profiler_t&&) = delete;
    profiler_t& operator=(profiler_t&&) = delete;

    void start();
    void reset();
    context_t context();
    /** @brief Reports whether there is no measurement construction or active retained/suppressed measurement. */
    bool quiescent() const noexcept;
    /**
     * @brief Starts one measurement with a typed payload in capture storage, returning its nullable handle.
     *
     * Attached recording requires start() and synchronous LIFO use. Violating
     * recording preconditions is a programming error checked by assertions.
     */
    template <typename T, typename... Args>
    metric_t<T> metric(Args&&... args) noexcept;
    records_t records() const;
    std::size_t omitted() const;
    void report(std::ostream& out) const;

    // Pairing operation used by metric_t; closes the latest opening exactly once.
    void end(entry_t* entry, bool unwinding) noexcept;

private:
    entry_t* reserve(std::size_t size, std::size_t alignment, const std::type_info& type,
        void (*destroy)(void*) noexcept, void (*format)(std::ostream&, const void*)) noexcept;
    void* payload(entry_t* entry) const noexcept;
    void begin(entry_t* entry) noexcept;
    void require_capture() const;
    void release() noexcept;

    std::span<std::byte> m_storage;
    std::size_t m_used = 0;
    entry_t* m_first = nullptr;
    entry_t* m_last = nullptr;
    entry_t* m_parent = nullptr;
    std::chrono::nanoseconds m_origin {};
    std::size_t m_count = 0;
    std::size_t m_depth = 0;
    std::size_t m_pending = 0;
    std::size_t m_suppressed = 0;
    std::size_t m_omitted = 0;
    bool m_started = false;
};

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::record_t>;

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::records_t>;

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::records_t::iterator_t>;

template <typename T>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::metric_t<T>>;

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::context_t>;

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t>;

} // namespace std

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

template <typename T>
const T* record_t::metrics() const noexcept {
    static_assert(std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>>);
    return static_cast<const T*>(metrics(typeid(T)));
}

template <typename T>
metric_t<T>::metric_t() noexcept:
    m_profiler(nullptr), m_entry(nullptr), m_metrics(nullptr), m_exceptions(0)
{
}

template <typename T>
metric_t<T>::metric_t(profiler_t& profiler, entry_t* entry, T* metrics) noexcept:
    m_profiler(&profiler), m_entry(entry), m_metrics(metrics),
    m_exceptions(metrics ? std::uncaught_exceptions() : 0)
{
}

template <typename T>
metric_t<T>::~metric_t() {
    close();
}

template <typename T>
metric_t<T>::operator bool() const noexcept {
    return m_metrics != nullptr;
}

template <typename T>
T* metric_t<T>::operator->() noexcept {
    assert(m_metrics);
    return m_metrics;
}

template <typename T>
const T* metric_t<T>::operator->() const noexcept {
    assert(m_metrics);
    return m_metrics;
}

template <typename T>
void metric_t<T>::close() noexcept {
    if (auto* profiler = std::exchange(m_profiler, nullptr)) {
        profiler->end(m_entry, m_metrics && m_exceptions < std::uncaught_exceptions());
        m_entry = nullptr;
        m_metrics = nullptr;
    }
}

template <typename T, typename... Args>
metric_t<T> context_t::metric(Args&&... args) const noexcept {
    // Requirements intentionally instantiate even for an unattached context.
    if (m_profiler) {
        return m_profiler->metric<T>(std::forward<Args>(args)...);
    }
    return metric_t<T>();
}

template <typename T, typename... Args>
metric_t<T> profiler_t::metric(Args&&... args) noexcept {
    static_assert(std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>>, "profiling requires an unqualified payload object type");
    static_assert(std::is_nothrow_constructible_v<T, Args...>, "profiling requires nonthrowing payload construction from the supplied arguments");
    static_assert(std::is_nothrow_destructible_v<T>, "profiling requires nonthrowing payload destruction");
    static_assert(std::formattable<const T, char>, "profiling requires a usable std::formatter for the const payload");
    if constexpr (std::is_object_v<T> && std::same_as<T, std::remove_cv_t<T>> &&
        std::is_nothrow_constructible_v<T, Args...> && std::is_nothrow_destructible_v<T> && std::formattable<const T, char>) {
        auto* entry = reserve(sizeof(T), alignof(T), typeid(T),
            [](void* metrics) noexcept { std::destroy_at(static_cast<T*>(metrics)); },
            [](std::ostream& out, const void* metrics) { out << std::format("{}", *static_cast<const T*>(metrics)); });
        T* metrics = nullptr;
        if (entry) {
            metrics = std::construct_at(static_cast<T*>(payload(entry)), std::forward<Args>(args)...);
            begin(entry);
        }
        return metric_t<T>(*this, entry, metrics);
    } else {
        return metric_t<T>(); // Keep invalid types out of storage and formatting instantiations.
    }
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling

namespace std {
template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::record_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::record_t& record, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ elapsed_ns: {}, unwinding: {} }}", record.elapsed().count(), record.unwinding());
        return out;
    }
};

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::records_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::records_t& records, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ records: {} }}", records.size());
        return out;
    }
};

template <>
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::records_t::iterator_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::records_t::iterator_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "record_iterator");
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
struct formatter<m03gtjqkhqacstl3luv2ojsz3q_profiling::context_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid profiling format specifier");
        }
        return it;
    }

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::context_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "profiling_context");
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

    auto format(const m03gtjqkhqacstl3luv2ojsz3q_profiling::profiler_t& profiler, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{ quiescent: {} }}", profiler.quiescent());
        return out;
    }
};

} // namespace std

#endif // M03GTJQKHQACSTL3LUV2OJSZ3Q_PROFILING_API_H
