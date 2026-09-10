#ifndef M03GE9IJ45DCZNRMNA12QOW5R5_FUNCTION_ID_FUNCTION_ID_H
# define M03GE9IJ45DCZNRMNA12QOW5R5_FUNCTION_ID_FUNCTION_ID_H

# include <chrono>
# include <cstddef>
# include <cstdint>
# include <format>
# include <functional>
# include <string>

namespace m03ge9ij45dcznrmna12qow5r5_function_id {

/**
 * @brief Identifies a function version by namespace, name and exact creation time.
 *
 * This mutable aggregate owns its strings and does not validate construction.
 * Equality and hashing include the full creation_time, while to_string() and
 * std::format emit `ns::name@seconds` with whole-second precision. Consequently,
 * a subsecond creation_time does not round-trip exactly and distinct IDs can
 * have identical text. No escaping is performed; a namespace containing `::`
 * or ending in `:` will not round-trip through the parser's first-separator rule.
 * For example, namespace `math:` and name `add` parse back as `math` and `:add`.
 *
 * @code{.cpp}
 * #include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>
 *
 * #include <cassert>
 * #include <chrono>
 *
 * int main() {
 *     using m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t;
 *     using namespace std::chrono_literals;
 *     auto function_id = function_id_t::from_string("math::add@42");
 *     assert(static_cast<bool>(function_id));
 *     function_id.creation_time += 999ms;
 *     assert(function_id_t::to_string(function_id) == "math::add@42");
 *     auto parsed_function_id = function_id_t::from_string(function_id_t::to_string(function_id));
 *     assert(parsed_function_id != function_id);
 * }
 * @endcode
 */
struct function_id_t {
    /// @brief Namespace component, compared literally and owned by this ID.
    std::string ns;
    /// @brief Function name within ns, compared literally and owned by this ID.
    std::string name;
    /// @brief Version timestamp on system_clock, retaining subsecond precision in memory.
    std::chrono::system_clock::time_point creation_time;

    /// @brief Compares all three fields, including the full timestamp precision.
    bool operator==(const function_id_t& other) const;

    /**
     * @brief Returns true exactly when ns and name are nonempty and creation_time is not the epoch.
     * This test does not check delimiters, filesystem suitability or timestamp range.
     */
    explicit operator bool() const;

    /**
     * @brief Formats ns::name followed by an at sign and seconds since the system_clock epoch.
     * duration_cast truncates fractional seconds toward zero. Fields are emitted
     * without validation or escaping; a default ID formats as `::@0`.
     */
    static std::string to_string(const function_id_t& function_id);
    /**
     * @brief Parses the first double colon, the last at sign and a whole-second timestamp.
     * The at sign must follow the double colon. Embedded at signs remain in name.
     * Timestamp conversion uses base-10 std::stoull, including its leading-space
     * and sign handling, and requires consumption of the entire suffix. Use
     * nonnegative seconds representable by system_clock; conversion to the clock's
     * duration is not range-checked. Parsing does not apply operator bool(): `::@0`
     * succeeds and returns a false-valued ID.
     * @throws std::runtime_error If separators are missing or in the wrong order.
     * @throws std::invalid_argument If the suffix has no number or has trailing characters.
     * @throws std::out_of_range If std::stoull cannot represent the number.
     */
    static function_id_t from_string(const std::string& str);
};

} // namespace m03ge9ij45dcznrmna12qow5r5_function_id

namespace std {

/// @brief Hashes namespace, name and the full creation timestamp for function ID containers.
template <>
struct hash<m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t> {
    size_t operator()(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& function_id) const noexcept {
        size_t h1 = std::hash<std::string>{}(function_id.ns);
        size_t h2 = std::hash<std::string>{}(function_id.name);
        size_t h3 = std::hash<int64_t>{}(function_id.creation_time.time_since_epoch().count());

        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

/// @brief Formats a function ID using function_id_t::to_string() and its whole-second precision.
template <>
struct formatter<m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    template <class FormatContext>
    auto format(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& function_id, FormatContext& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{}", m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t::to_string(function_id));
        return out;
    }
};

} // namespace std

#endif // M03GE9IJ45DCZNRMNA12QOW5R5_FUNCTION_ID_FUNCTION_ID_H
