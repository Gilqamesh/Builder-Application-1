#include "api.h"
#include "helpers.h"

#include <cassert>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

struct entry_t {
    entry_t* m_next = nullptr;
    entry_t* m_previous = nullptr;
    entry_t* m_parent = nullptr;
    std::size_t m_index = 0;
    std::size_t m_depth = 0;
    std::chrono::nanoseconds m_start {};
    std::chrono::nanoseconds m_elapsed {};
    std::chrono::nanoseconds m_children_elapsed {};
    bool m_children_complete = true;
    bool m_unwinding = false;
    void* m_metrics = nullptr;
    const std::type_info* m_type = nullptr;
    void (*m_destroy)(void*) noexcept = nullptr;
    void (*m_format)(std::ostream&, const void*) = nullptr;
};

record_t::record_t(const entry_t& entry) noexcept:
    m_entry(&entry)
{
}

std::optional<std::size_t> record_t::parent() const noexcept {
    if (m_entry->m_parent) {
        return m_entry->m_parent->m_index;
    }
    return std::nullopt;
}

std::size_t record_t::depth() const noexcept {
    return m_entry->m_depth;
}

std::chrono::nanoseconds record_t::start() const noexcept {
    return m_entry->m_start;
}

std::chrono::nanoseconds record_t::elapsed() const noexcept {
    return m_entry->m_elapsed;
}

std::optional<std::chrono::nanoseconds> record_t::self() const noexcept {
    if (!m_entry->m_children_complete) {
        return std::nullopt;
    }
    return m_entry->m_elapsed - m_entry->m_children_elapsed;
}

bool record_t::unwinding() const noexcept {
    return m_entry->m_unwinding;
}

void record_t::report(std::ostream& out) const {
    out << std::string(depth() * 2, ' ');
    m_entry->m_format(out, m_entry->m_metrics);
    out << std::format(" inclusive={} ns self=", elapsed().count());
    if (const auto duration = self()) {
        out << std::format("{} ns", duration->count());
    } else {
        out << "unavailable";
    }
    if (unwinding()) {
        out << " unwinding";
    }
    out << '\n';
}

const void* record_t::metrics(const std::type_info& type) const noexcept {
    return type == *m_entry->m_type ? m_entry->m_metrics : nullptr;
}

records_t::iterator_t::iterator_t() noexcept:
    m_entry(nullptr), m_remaining(0)
{
}

records_t::iterator_t::iterator_t(const entry_t* entry, std::size_t remaining) noexcept:
    m_entry(entry), m_remaining(remaining)
{
}

record_t records_t::iterator_t::operator*() const noexcept {
    assert(m_entry);
    return record_t(*m_entry);
}

records_t::iterator_t& records_t::iterator_t::operator++() noexcept {
    assert(m_entry);
    --m_remaining;
    m_entry = m_remaining == 0 ? nullptr : m_entry->m_next;
    return *this;
}

records_t::iterator_t records_t::iterator_t::operator++(int) noexcept {
    const auto previous = *this;
    ++*this;
    return previous;
}

bool records_t::iterator_t::operator==(const iterator_t& other) const noexcept {
    return m_entry == other.m_entry && m_remaining == other.m_remaining;
}

records_t::records_t(const entry_t* first, std::size_t size) noexcept:
    m_first(first), m_size(size)
{
}

records_t::iterator_t records_t::begin() const noexcept {
    return iterator_t(m_first, m_size);
}

records_t::iterator_t records_t::end() const noexcept {
    return iterator_t();
}

std::size_t records_t::size() const noexcept {
    return m_size;
}

bool records_t::empty() const noexcept {
    return m_size == 0;
}

record_t records_t::operator[](std::size_t index) const noexcept {
    assert(index < m_size);
    auto it = begin();
    while (index != 0) {
        ++it;
        --index;
    }
    return *it;
}

context_t::context_t() noexcept:
    m_profiler(nullptr)
{
}

context_t::context_t(profiler_t& profiler):
    m_profiler(&profiler)
{
    if (!profiler.quiescent()) {
        throw std::logic_error("context_t attachment requires no active scopes");
    }
}

context_t::context_t(const context_t& other):
    m_profiler(other.m_profiler)
{
    if (m_profiler && !m_profiler->quiescent()) {
        throw std::logic_error("context_t attachment requires no active scopes");
    }
}

context_t& context_t::operator=(const context_t& other) {
    if ((m_profiler && !m_profiler->quiescent()) || (other.m_profiler && !other.m_profiler->quiescent())) {
        throw std::logic_error("context_t replacement requires no active scopes in either collector");
    }
    m_profiler = other.m_profiler;
    return *this;
}

profiler_t::profiler_t(std::span<std::byte> storage) noexcept:
    m_storage(storage)
{
}

profiler_t::~profiler_t() {
    assert(quiescent());
    release();
}

void profiler_t::start() {
    if (m_started) {
        throw std::logic_error("profiler_t::start requires a profiler that has not started");
    }
    m_started = true;
    m_origin = clock_now();
}

void profiler_t::reset() {
    require_capture();
    release();
    m_origin = clock_now();
}

context_t profiler_t::context() {
    return context_t(*this);
}

bool profiler_t::quiescent() const noexcept {
    return m_depth == 0 && m_pending == 0;
}

records_t profiler_t::records() const {
    require_capture();
    return records_t(m_first, m_count);
}

std::size_t profiler_t::omitted() const {
    require_capture();
    return m_omitted;
}

void profiler_t::report(std::ostream& out) const {
    require_capture();
    for (const auto record : records()) {
        record.report(out);
    }
    out << std::format("capture: {} records, {} omitted, {}\n", m_count, m_omitted, m_omitted == 0 ? "complete" : "incomplete");
}

void profiler_t::end(entry_t* entry, bool unwinding) noexcept {
    assert(m_depth != 0);
    --m_depth;
    if (!entry) {
        assert(m_suppressed != 0);
        --m_suppressed;
        return;
    }
    assert(m_suppressed == 0 && m_parent == entry);
    entry->m_elapsed = clock_now() - m_origin - entry->m_start;
    entry->m_unwinding = unwinding;
    m_parent = entry->m_parent;
    if (m_parent) {
        m_parent->m_children_elapsed += entry->m_elapsed;
    }
}

entry_t* profiler_t::reserve(std::size_t size, std::size_t alignment, const std::type_info& type,
    void (*destroy)(void*) noexcept, void (*format)(std::ostream&, const void*)) noexcept {
    assert(m_started);
    // Reserve both pieces transactionally: an oversized payload does not consume
    // metadata or padding, so a later sibling can still fit.
    void* metadata = m_storage.empty() ? nullptr : m_storage.data() + m_used;
    std::size_t available = m_storage.size() - m_used;
    void* metrics = nullptr;
    if (m_suppressed == 0 && std::align(alignof(entry_t), sizeof(entry_t), metadata, available)) {
        metrics = static_cast<std::byte*>(metadata) + sizeof(entry_t);
        available -= sizeof(entry_t);
        if (!std::align(alignment, size, metrics, available)) {
            metrics = nullptr;
        }
    }
    if (!metrics) {
        ++m_depth;
        if (m_suppressed == 0 && m_parent) {
            m_parent->m_children_complete = false;
        }
        ++m_suppressed;
        if (m_omitted != std::numeric_limits<std::size_t>::max()) {
            ++m_omitted;
        }
        return nullptr;
    }
    auto* entry = std::construct_at(static_cast<entry_t*>(metadata));
    entry->m_metrics = metrics;
    entry->m_type = &type;
    entry->m_destroy = destroy;
    entry->m_format = format;
    m_used = static_cast<std::byte*>(metrics) - m_storage.data() + size;
    ++m_pending;
    return entry;
}

void* profiler_t::payload(entry_t* entry) const noexcept {
    return entry->m_metrics;
}

void profiler_t::begin(entry_t* entry) noexcept {
    assert(m_pending != 0);
    --m_pending;
    // Publish only after construction succeeds. Constructor-created scopes have
    // already opened and closed under the prior parent, matching lexical order.
    entry->m_previous = m_last;
    entry->m_parent = m_parent;
    entry->m_index = m_count++;
    entry->m_depth = m_depth++;
    entry->m_start = clock_now() - m_origin;
    if (m_last) {
        m_last->m_next = entry;
    } else {
        m_first = entry;
    }
    m_last = entry;
    m_parent = entry;
}

void profiler_t::require_capture() const {
    if (!m_started || !quiescent()) {
        throw std::logic_error("profiler_t operation requires start and no active scopes");
    }
}

void profiler_t::release() noexcept {
    // Reverse publication order lets a later payload borrow an earlier payload
    // through its own destruction, as with ordinary nested object lifetimes.
    while (m_last) {
        auto* previous = m_last->m_previous;
        m_last->m_destroy(m_last->m_metrics);
        std::destroy_at(m_last);
        m_last = previous;
    }
    m_first = nullptr;
    m_parent = nullptr;
    m_used = 0;
    m_count = 0;
    m_omitted = 0;
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling
