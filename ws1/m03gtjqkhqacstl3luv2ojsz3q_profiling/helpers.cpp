#include "helpers.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <string_view>

namespace m03gtjqkhqacstl3luv2ojsz3q_profiling {

metric_base_t::metric_base_t(const std::type_info& type, metric_base_t* parent) noexcept:
    type(&type),
    parent(parent)
{
}

metric_base_t::~metric_base_t() = default;

void metric_base_t::start() noexcept {
    exceptions = std::uncaught_exceptions();
    started = clock_now();
}

void metric_base_t::stop() noexcept {
    completed = clock_now();
    elapsed = completed - started;
    total += elapsed;
    minimum = count == 0 ? elapsed : std::min(minimum, elapsed);
    maximum = std::max(maximum, elapsed);
    ++count;
    unwinding = exceptions < std::uncaught_exceptions();
}

void metric_base_t::append_report(std::vector<report_row_t>& rows, std::chrono::nanoseconds now, const std::string& prefix, std::string_view branch) const {
    auto text = format();
    // Whitespace can separate a label and data, but never split a report row.
    for (auto& character : text) {
        if (character == '\n' || character == '\r' || character == '\t' || character == '\v' || character == '\f') {
            character = ' ';
        }
    }
    const auto first = text.find_first_not_of(' ');
    text = first == std::string::npos ? "" : text.substr(first, text.find_last_not_of(' ') - first + 1);
    const auto separator = text.find(' ');
    const auto data_start = separator == std::string::npos ? separator : text.find_first_not_of(' ', separator);
    report_row_t row {
        prefix + std::string(branch) + text.substr(0, separator),
        format_count(count),
        format_elapsed(elapsed),
        format_elapsed(minimum),
        format_elapsed(total / count),
        format_elapsed(maximum),
        format_elapsed(total),
        format_elapsed(now - completed),
        data_start == std::string::npos ? "" : text.substr(data_start)
    };
    if (unwinding) {
        if (!row.back().empty()) {
            row.back() += ' ';
        }
        row.back() += "[unwinding]";
    }
    rows.push_back(std::move(row));
    const auto child_prefix = prefix + (branch.empty() ? "" : branch == "├─ " ? "│  " : "   ");
    for (std::size_t index = 0; index < children.size(); ++index) {
        children[index]->append_report(rows, now, child_prefix, index + 1 == children.size() ? "└─ " : "├─ ");
    }
}

void storage_t::stop(metric_base_t& metric) {
    if (constructing || active != &metric) {
        throw std::logic_error("metric_t::stop requires all children stopped");
    }
    metric.stop();
    active = metric.parent;
}

void storage_t::report(std::ostream& out, std::chrono::nanoseconds now) const {
    std::vector<report_row_t> rows;
    rows.reserve(count + 1);
    rows.push_back({"Metric", "Count", "Last", "Min", "Mean", "Max", "Total", "Age", "Data"});
    for (const auto& metric : roots) {
        metric->append_report(rows, now, "", "");
    }
    std::array<std::size_t, 8> widths {};
    for (const auto& row : rows) {
        for (std::size_t column = 0; column < widths.size(); ++column) {
            widths[column] = std::max(widths[column], report_width(row[column]));
        }
    }
    for (const auto& row : rows) {
        out << row[0] << std::string(widths[0] - report_width(row[0]), ' ');
        for (std::size_t column = 1; column < widths.size(); ++column) {
            out << std::string(2 + widths[column] - report_width(row[column]), ' ') << row[column];
        }
        if (!row.back().empty()) {
            out << "  " << row.back();
        }
        out << '\n';
    }
}

void storage_t::require_stopped() const {
    if (active || constructing) {
        throw std::logic_error("profiler_t read or report requires all metrics stopped");
    }
}

const metric_base_t* storage_t::find(std::span<const std::type_info* const> path) const {
    require_stopped();
    const auto* siblings = &roots;
    const metric_base_t* found = nullptr;
    for (const auto* type : path) {
        found = nullptr;
        for (const auto& metric : *siblings) {
            if (*metric->type == *type) {
                found = metric.get();
                siblings = &metric->children;
                break;
            }
        }
        if (!found) {
            break;
        }
    }
    return found;
}

#ifndef PROFILING_TEST_CLOCK
std::chrono::nanoseconds clock_now() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
}
#endif

std::string format_elapsed(std::chrono::duration<long double, std::nano> elapsed) {
    static constexpr std::array<std::string_view, 10> units {"ns", "us", "ms", "s", "ks", "Ms", "Gs", "Ts", "Ps", "Es"};
    auto count = elapsed.count();
    std::size_t unit = 0;
    while (unit + 1 < units.size() && 1000.0L <= std::abs(count)) {
        count /= 1000.0L;
        ++unit;
    }
    count = std::round(count * 1000.0L) / 1000.0L;
    // Rounding at a boundary should produce 1 s, for example, rather than 1000 ms.
    if (unit + 1 < units.size() && 1000.0L <= std::abs(count)) {
        count /= 1000.0L;
        ++unit;
    }
    auto text = std::format("{:.3f}", count);
    while (text.back() == '0') {
        text.pop_back();
    }
    if (text.back() == '.') {
        text.pop_back();
    }
    return std::format("{} {}", text, units[unit]);
}

std::string format_count(std::size_t count) {
    auto text = std::to_string(count);
    for (auto position = text.size(); 3 < position;) {
        position -= 3;
        text.insert(position, 1, ',');
    }
    return text;
}

std::size_t report_width(std::string_view text) noexcept {
    // Count UTF-8 code points so tree connectors occupy one column.
    return std::count_if(text.begin(), text.end(), [](unsigned char character) {
        return (character & 0xc0) != 0x80;
    });
}

} // namespace m03gtjqkhqacstl3luv2ojsz3q_profiling
