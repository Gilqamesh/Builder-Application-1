#ifndef M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_HELPERS_H
# define M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_HELPERS_H

# include "value.h"

# include <algorithm>
# include <cstddef>
# include <cstdint>
# include <format>
# include <optional>
# include <span>
# include <stdexcept>
# include <string_view>
# include <type_traits>
# include <utility>
# include <vector>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

class bindings_t;
class vertex_io_t;
class fragment_io_t;

// Module-local executable representation; constructed only by lowering a validated AST.
enum class operand_kind_t { slot, constant, input, output, binding, builtin, component };
enum class opcode_t { constant, copy, check_local, initialize_local, assign_local, read_local, kernel, builtin, position, color, sample, sample_lod, jump, jump_if_false, jump_if_true, discard, finish };

struct operand_t {
    operand_kind_t kind;
    std::size_t index;
};

struct instruction_t {
    opcode_t opcode;
    std::optional<std::size_t> destination;
    std::size_t operand_begin;
    std::size_t operand_count;
    std::optional<std::size_t> target;
    // Index into the immutable table of typed operations, used by kernel only.
    std::size_t kernel_index = 0;
};

class stage_code_t {
public:
    explicit stage_code_t(const shader::shader_ast_t& ast);

    const shader::shader_interface_t& interface() const;
    std::span<const instruction_t> instructions() const;
    std::span<const operand_t> operands() const;
    std::span<const value_t> constants() const;
    std::size_t slot_count() const;
    std::size_t local_count() const;
    std::size_t storage_bytes() const;

private:
    shader::shader_interface_t m_interface;
    std::vector<instruction_t> m_instructions;
    std::vector<operand_t> m_operands;
    std::vector<value_t> m_constants;
    std::size_t m_slot_count;
    std::size_t m_local_count;
};

shader::matrix_t<float, 4, 4> identity_matrix();
void validate_interface_bindings(const shader::shader_interface_t& interface, const bindings_t& bindings);
void validate_program_link(const shader::shader_ast_t& vertex, const shader::shader_ast_t& fragment);
void execute_stage(const stage_code_t& code, const bindings_t& bindings, vertex_io_t& io, std::vector<value_t>& slots, std::vector<std::uint8_t>& local_initialized);
void execute_stage(const stage_code_t& code, const bindings_t& bindings, fragment_io_t& io, std::vector<value_t>& slots, std::vector<std::uint8_t>& local_initialized);

template <typename T>
std::size_t find_location(std::span<T> values, std::uint32_t location);
template <shader::shader_value T>
void write_value(std::vector<std::pair<std::uint32_t, value_t>>& values, std::uint32_t location, T input);
template <shader::shader_value T>
std::optional<std::remove_cvref_t<T>> read_value(std::span<const std::pair<std::uint32_t, value_t>> values, std::uint32_t location, std::string_view name);

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::operand_kind_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::opcode_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::operand_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::instruction_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::stage_code_t>;

} // namespace std

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

template <typename T>
std::size_t find_location(std::span<T> values, std::uint32_t location) {
    const auto found = std::ranges::find(values, location, &std::pair<std::uint32_t, value_t>::first);
    return static_cast<std::size_t>(found - values.begin());
}

template <shader::shader_value T>
void write_value(std::vector<std::pair<std::uint32_t, value_t>>& values, std::uint32_t location, T input) {
    const auto index = find_location(std::span(values), location);
    if (index == values.size()) {
        values.emplace_back(location, value_t(std::move(input)));
    } else {
        values[index].second = std::move(input);
    }
}

template <shader::shader_value T>
std::optional<std::remove_cvref_t<T>> read_value(std::span<const std::pair<std::uint32_t, value_t>> values, std::uint32_t location, std::string_view name) {
    const auto index = find_location(values, location);
    if (index == values.size()) {
        return std::nullopt;
    }
    const auto* result = std::get_if<std::remove_cvref_t<T>>(&values[index].second);
    if (!result) {
        throw std::invalid_argument(std::format("software shader {} location {} has the wrong type", name, location));
    }
    return *result;
}

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::operand_kind_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::operand_kind_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "{}", static_cast<int>(value));
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::opcode_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::opcode_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "{}", static_cast<int>(value));
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::operand_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::operand_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "{}:{}", value.kind, value.index);
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::instruction_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::instruction_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "opcode={} operands={}+{} kernel={}", value.opcode, value.operand_begin, value.operand_count, value.kernel_index);
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::stage_code_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::stage_code_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "instructions={} operands={} slots={} locals={} storage_bytes={}", value.instructions().size(), value.operands().size(), value.slot_count(), value.local_count(), value.storage_bytes());
        return out;
    }
};

} // namespace std

#endif // M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_HELPERS_H
