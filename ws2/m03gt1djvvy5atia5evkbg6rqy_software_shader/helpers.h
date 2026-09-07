#ifndef M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_HELPERS_H
# define M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_HELPERS_H

# include "invocation.h"

# include <algorithm>
# include <array>
# include <bit>
# include <cmath>
# include <concepts>
# include <cstddef>
# include <cstdint>
# include <format>
# include <initializer_list>
# include <limits>
# include <map>
# include <optional>
# include <set>
# include <span>
# include <stdexcept>
# include <string_view>
# include <type_traits>
# include <utility>
# include <vector>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

// Module-local executable representation; constructed only by lowering a validated AST.
enum class operand_kind_t { slot, constant, input, output, binding, builtin, component };
enum class opcode_t { constant, copy, check_local, initialize_local, assign_local, read_local, kernel, builtin, position, color, sample, sample_lod, jump, jump_if_false, jump_if_true, discard, finish };
enum class kernel_operation_t { input, uniform, output, component, construct, swizzle, negate, logical_not, absolute, square_root, floor, ceil, fract, sine, cosine, normalize, length, add, subtract, multiply, divide, modulo, equal, not_equal, less, less_equal, greater, greater_equal, dot, cross, power, reflect, minimum, maximum, step, clamp, mix, smoothstep, matrix_product };

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

struct kernel_arguments_t {
    const shader::shader_interface_t& interface;
    std::span<const value_t> slots;
    std::span<const operand_t> operands;
    const bindings_t& bindings;
    vertex_io_t* vertex_io;
    fragment_io_t* fragment_io;
};

using kernel_t = value_t (*)(const kernel_arguments_t&);

struct kernel_entry_t {
    kernel_operation_t operation;
    std::array<std::size_t, 3> types;
    kernel_t evaluate;
};

struct loop_t {
    std::size_t condition;
    std::vector<std::size_t> breaks;
};

class compiler_t final : public shader::shader_ast_visitor_t {
public:
    explicit compiler_t(const shader::shader_ast_t& ast);
    void visit(const shader::shader_constant_node_t& node) override;
    void visit(const shader::shader_input_node_t& node) override;
    void visit(const shader::shader_uniform_node_t& node) override;
    void visit(const shader::shader_resource_node_t& node) override;
    void visit(const shader::shader_builtin_node_t& node) override;
    void visit(const shader::shader_local_node_t& node) override;
    void visit(const shader::shader_unary_node_t& node) override;
    void visit(const shader::shader_binary_node_t& node) override;
    void visit(const shader::shader_construct_node_t& node) override;
    void visit(const shader::shader_swizzle_node_t& node) override;
    void visit(const shader::shader_call_node_t& node) override;
    void visit(const shader::shader_local_statement_t& node) override;
    void visit(const shader::shader_assignment_statement_t& node) override;
    void visit(const shader::shader_output_statement_t& node) override;
    void visit(const shader::shader_branch_statement_t& node) override;
    void visit(const shader::shader_loop_statement_t& node) override;
    void visit(const shader::shader_break_statement_t& node) override;
    void visit(const shader::shader_continue_statement_t& node) override;
    void visit(const shader::shader_discard_statement_t& node) override;

    std::vector<instruction_t> instructions;
    std::vector<operand_t> operands;
    std::vector<value_t> constants;
    std::vector<shader::shader_data_type_t> slot_types;
    std::size_t local_count = 0;

private:
    void collect(const shader::shader_expression_node_t& expression);
    void collect(const shader::shader_block_t& block);
    void lower(const shader::shader_block_t& block);
    operand_t lower(const shader::shader_expression_node_t& expression);
    std::size_t slot(shader::shader_data_type_t type);
    std::size_t emit(opcode_t opcode, std::optional<std::size_t> destination, std::span<const operand_t> inputs, std::optional<std::size_t> target = std::nullopt, std::size_t kernel_index = 0);
    std::size_t emit(opcode_t opcode, std::optional<std::size_t> destination, std::initializer_list<operand_t> inputs, std::optional<std::size_t> target = std::nullopt, std::size_t kernel_index = 0);
    operand_t compute(kernel_operation_t operation, shader::shader_data_type_t result_type, std::span<const operand_t> inputs, std::initializer_list<shader::shader_data_type_t> types);
    operand_t compute(kernel_operation_t operation, shader::shader_data_type_t result_type, std::initializer_list<operand_t> inputs, std::initializer_list<shader::shader_data_type_t> types);
    std::size_t interface_index(std::span<const shader::shader_interface_element_t> elements, std::uint32_t index, shader::shader_data_type_t type) const;

    const shader::shader_interface_t& m_interface;
    std::map<const shader::shader_local_node_t*, std::size_t> m_locals;
    std::set<const shader::shader_expression_node_t*> m_collected;
    std::map<const shader::shader_constant_node_t*, std::size_t> m_constants;
    std::vector<loop_t> m_loops;
    operand_t m_result {operand_kind_t::slot, 0};
};

class stage_code_t {
public:
    explicit stage_code_t(const shader::shader_ast_t& ast);

    const shader::shader_interface_t& interface() const;
    std::span<const instruction_t> instructions() const;
    std::span<const operand_t> operands() const;
    std::span<const value_t> constants() const;
    std::span<const shader::shader_data_type_t> slot_types() const;
    std::size_t local_count() const;
    std::size_t storage_bytes() const;

private:
    shader::shader_interface_t m_interface;
    std::vector<instruction_t> m_instructions;
    std::vector<operand_t> m_operands;
    std::vector<value_t> m_constants;
    std::vector<shader::shader_data_type_t> m_slot_types;
    std::size_t m_local_count;
};

shader::matrix_t<float, 4, 4> identity_matrix();
std::span<const kernel_entry_t> kernels();
std::size_t value_type_index(shader::shader_data_type_t type);
std::size_t select_kernel(kernel_operation_t operation, std::initializer_list<shader::shader_data_type_t> types);
kernel_operation_t operation(shader::shader_unary_operation_t operation);
kernel_operation_t operation(shader::shader_binary_operation_t operation);
value_t literal_value(const shader::shader_literal_t& literal);
std::int32_t signed_add(std::int32_t lhs, std::int32_t rhs);
std::int32_t signed_subtract(std::int32_t lhs, std::int32_t rhs);
std::int32_t signed_multiply(std::int32_t lhs, std::int32_t rhs);
std::int32_t signed_negate(std::int32_t value);

template <typename R, typename F, typename Scalar>
R dispatch_vector(std::uint8_t rows, F& function);
template <typename R, typename F, std::size_t Rows>
R dispatch_matrix_columns(std::uint8_t columns, F& function);
template <typename R, typename F>
R dispatch_value_type(shader::shader_data_type_t type, F&& function);
template <typename T>
T divide(T lhs, T rhs);
template <typename T>
T modulo(T lhs, T rhs);
template <typename T>
using scalar_t = typename shader::shader_type_traits_t<T>::scalar_type;
template <typename T>
std::span<const scalar_t<T>> value_components(const T& input);
template <typename T>
std::span<scalar_t<T>> value_components(T& input);
template <kernel_operation_t Operation, typename T, typename U = T, typename V = U>
value_t evaluate_kernel(const kernel_arguments_t& arguments);
template <typename IO>
void validate_inputs(const shader::shader_interface_t& interface, const IO& io);
void validate_interface_bindings(const shader::shader_interface_t& interface, const bindings_t& bindings);
int binding_namespace(shader::shader_data_type_t type);
void validate_program_link(const shader::shader_ast_t& vertex, const shader::shader_ast_t& fragment);
template <typename IO>
void execute_stage(const stage_code_t& code, const bindings_t& bindings, IO& io, std::vector<value_t>& slots, std::vector<std::uint8_t>& local_initialized);

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::operand_kind_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::opcode_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::kernel_operation_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::operand_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::instruction_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::kernel_arguments_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::kernel_entry_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::loop_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::compiler_t>;

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::stage_code_t>;

} // namespace std

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

template <typename R, typename F, typename Scalar>
R dispatch_vector(std::uint8_t rows, F& function) {
    switch (rows) {
        case 2: return function.template operator()<shader::vector_t<Scalar, 2>>();
        case 3: return function.template operator()<shader::vector_t<Scalar, 3>>();
        case 4: return function.template operator()<shader::vector_t<Scalar, 4>>();
        default: throw std::invalid_argument("software shader encountered an unsupported vector type");
    }
}

template <typename R, typename F, std::size_t Rows>
R dispatch_matrix_columns(std::uint8_t columns, F& function) {
    switch (columns) {
        case 2: return function.template operator()<shader::matrix_t<float, Rows, 2>>();
        case 3: return function.template operator()<shader::matrix_t<float, Rows, 3>>();
        case 4: return function.template operator()<shader::matrix_t<float, Rows, 4>>();
        default: throw std::invalid_argument("software shader encountered an unsupported matrix type");
    }
}

template <typename R, typename F>
R dispatch_value_type(shader::shader_data_type_t type, F&& function) {
    auto& callable = function;
    switch (type.category()) {
        case shader::shader_data_category_t::scalar:
            switch (type.scalar()) {
                case shader::shader_scalar_type_t::boolean: return callable.template operator()<bool>();
                case shader::shader_scalar_type_t::signed_integer: return callable.template operator()<std::int32_t>();
                case shader::shader_scalar_type_t::unsigned_integer: return callable.template operator()<std::uint32_t>();
                case shader::shader_scalar_type_t::floating_point: return callable.template operator()<float>();
                default: throw std::invalid_argument("software shader encountered an unsupported scalar type");
            }
        case shader::shader_data_category_t::vector:
            switch (type.scalar()) {
                case shader::shader_scalar_type_t::boolean: return dispatch_vector<R, F, bool>(type.rows(), callable);
                case shader::shader_scalar_type_t::signed_integer: return dispatch_vector<R, F, std::int32_t>(type.rows(), callable);
                case shader::shader_scalar_type_t::unsigned_integer: return dispatch_vector<R, F, std::uint32_t>(type.rows(), callable);
                case shader::shader_scalar_type_t::floating_point: return dispatch_vector<R, F, float>(type.rows(), callable);
                default: throw std::invalid_argument("software shader encountered an unsupported vector type");
            }
        case shader::shader_data_category_t::matrix:
            if (type.scalar() != shader::shader_scalar_type_t::floating_point) {
                throw std::invalid_argument("software shader encountered an unsupported matrix scalar type");
            }
            switch (type.rows()) {
                case 2: return dispatch_matrix_columns<R, F, 2>(type.columns(), callable);
                case 3: return dispatch_matrix_columns<R, F, 3>(type.columns(), callable);
                case 4: return dispatch_matrix_columns<R, F, 4>(type.columns(), callable);
                default: throw std::invalid_argument("software shader encountered an unsupported matrix type");
            }
        default: throw std::invalid_argument("software shader encountered a resource where a value was required");
    }
}

template <typename T>
T divide(T lhs, T rhs) {
    if constexpr (std::integral<T>) {
        if (rhs == 0) {
            throw std::domain_error("integer division by zero in software shader");
        }
        if constexpr (std::signed_integral<T>) {
            if (lhs == std::numeric_limits<T>::min() && rhs == T(-1)) {
                return lhs;
            }
        }
    }
    return lhs / rhs;
}

template <typename T>
T modulo(T lhs, T rhs) {
    if constexpr (std::integral<T>) {
        if (rhs == 0) {
            throw std::domain_error("integer modulo by zero in software shader");
        }
        if constexpr (std::signed_integral<T>) {
            if (lhs == std::numeric_limits<T>::min() && rhs == T(-1)) {
                return 0;
            }
        }
        return lhs % rhs;
    } else {
        throw std::logic_error("floating-point modulo is not a software shader operation");
    }
}

template <typename T>
std::span<const scalar_t<T>> value_components(const T& input) {
    if constexpr (shader::shader_type_traits_t<T>::scalar) {
        return {&input, 1};
    } else {
        return {input.begin(), input.end()};
    }
}

template <typename T>
std::span<scalar_t<T>> value_components(T& input) {
    if constexpr (shader::shader_type_traits_t<T>::scalar) {
        return {&input, 1};
    } else {
        return {input.begin(), input.end()};
    }
}

template <kernel_operation_t Operation, typename T, typename U, typename V>
value_t evaluate_kernel(const kernel_arguments_t& arguments) {
    const auto read = [&]<typename R>(std::size_t index) -> const R& {
        return std::get<R>(arguments.slots[arguments.operands[index].index]);
    };
    if constexpr (Operation == kernel_operation_t::input) {
        const auto location = arguments.interface.inputs()[arguments.operands[0].index].index;
        return arguments.vertex_io ? arguments.vertex_io->input<T>(location) : arguments.fragment_io->input<T>(location);
    } else if constexpr (Operation == kernel_operation_t::uniform) {
        return arguments.bindings.uniform<T>(arguments.interface.bindings()[arguments.operands[0].index].index);
    } else if constexpr (Operation == kernel_operation_t::output) {
        const auto location = arguments.interface.outputs()[arguments.operands[0].index].index;
        const auto& output = read.template operator()<T>(1);
        if (arguments.vertex_io) {
            arguments.vertex_io->output(location, output);
        } else {
            arguments.fragment_io->output(location, output);
        }
        return false;
    } else if constexpr (Operation == kernel_operation_t::component) {
        return value_components(read.template operator()<T>(0))[arguments.operands[1].index];
    } else if constexpr (Operation == kernel_operation_t::construct) {
        T result {};
        auto destination = value_components(result);
        for (std::size_t i = 0; i < destination.size(); ++i) {
            destination[i] = read.template operator()<scalar_t<T>>(i);
        }
        return result;
    } else if constexpr (Operation == kernel_operation_t::swizzle) {
        const auto source = value_components(read.template operator()<U>(0));
        T result {};
        auto destination = value_components(result);
        for (std::size_t i = 0; i < destination.size(); ++i) {
            destination[i] = source[arguments.operands[i + 1].index];
        }
        return result;
    } else if constexpr (Operation == kernel_operation_t::logical_not) {
        return !read.template operator()<bool>(0);
    } else if constexpr (Operation == kernel_operation_t::equal || Operation == kernel_operation_t::not_equal) {
        const auto left = value_components(read.template operator()<T>(0));
        const auto right = value_components(read.template operator()<T>(1));
        const bool same = std::equal(left.begin(), left.end(), right.begin());
        return Operation == kernel_operation_t::equal ? same : !same;
    } else if constexpr (Operation == kernel_operation_t::less || Operation == kernel_operation_t::less_equal || Operation == kernel_operation_t::greater || Operation == kernel_operation_t::greater_equal) {
        const auto left = read.template operator()<T>(0);
        const auto right = read.template operator()<T>(1);
        if constexpr (Operation == kernel_operation_t::less) { return left < right; }
        if constexpr (Operation == kernel_operation_t::less_equal) { return left <= right; }
        if constexpr (Operation == kernel_operation_t::greater) { return right < left; }
        if constexpr (Operation == kernel_operation_t::greater_equal) { return right <= left; }
    } else if constexpr (Operation == kernel_operation_t::length || Operation == kernel_operation_t::normalize) {
        const auto source = value_components(read.template operator()<T>(0));
        float squared_length = 0;
        for (float component : source) { squared_length += component * component; }
        const float length = std::sqrt(squared_length);
        if constexpr (Operation == kernel_operation_t::length) {
            return length;
        } else {
            T result {};
            auto destination = value_components(result);
            for (std::size_t i = 0; i < destination.size(); ++i) { destination[i] = source[i] / length; }
            return result;
        }
    } else if constexpr (Operation == kernel_operation_t::dot) {
        const auto left = value_components(read.template operator()<T>(0));
        const auto right = value_components(read.template operator()<T>(1));
        float result = 0;
        for (std::size_t i = 0; i < left.size(); ++i) { result += left[i] * right[i]; }
        return result;
    } else if constexpr (Operation == kernel_operation_t::cross) {
        const auto& left = read.template operator()<T>(0);
        const auto& right = read.template operator()<T>(1);
        return T({left[1] * right[2] - left[2] * right[1], left[2] * right[0] - left[0] * right[2], left[0] * right[1] - left[1] * right[0]});
    } else if constexpr (Operation == kernel_operation_t::matrix_product) {
        constexpr auto left_type = shader::shader_data_type<T>();
        constexpr auto right_type = shader::shader_data_type<U>();
        constexpr std::size_t rows = left_type.rows();
        constexpr std::size_t inner = left_type.columns();
        constexpr bool vector = right_type.category() == shader::shader_data_category_t::vector;
        constexpr std::size_t columns = vector ? 1 : right_type.columns();
        using result_t = std::conditional_t<vector, shader::vector_t<float, rows>, shader::matrix_t<float, rows, columns>>;
        result_t result(0.0F);
        auto destination = value_components(result);
        const auto left = value_components(read.template operator()<T>(0));
        const auto right = value_components(read.template operator()<U>(1));
        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t column = 0; column < columns; ++column) {
                for (std::size_t i = 0; i < inner; ++i) {
                    destination[row * columns + column] += left[row * inner + i] * right[i * columns + column];
                }
            }
        }
        return result;
    } else if constexpr (Operation == kernel_operation_t::step) {
        const auto edges = value_components(read.template operator()<T>(0));
        const auto source = value_components(read.template operator()<U>(1));
        U result {};
        auto destination = value_components(result);
        for (std::size_t i = 0; i < destination.size(); ++i) { destination[i] = source[i] < edges[edges.size() == 1 ? 0 : i] ? 0.0F : 1.0F; }
        return result;
    } else if constexpr (Operation == kernel_operation_t::reflect) {
        const auto incident = value_components(read.template operator()<T>(0));
        const auto normal = value_components(read.template operator()<T>(1));
        float projection = 0;
        for (std::size_t i = 0; i < incident.size(); ++i) { projection += normal[i] * incident[i]; }
        T result {};
        auto destination = value_components(result);
        for (std::size_t i = 0; i < destination.size(); ++i) { destination[i] = incident[i] - 2.0F * projection * normal[i]; }
        return result;
    } else if constexpr (Operation == kernel_operation_t::clamp || Operation == kernel_operation_t::smoothstep) {
        const auto source = value_components(read.template operator()<T>(0));
        const auto lower = value_components(read.template operator()<U>(1));
        const auto upper = value_components(read.template operator()<V>(2));
        T result {};
        auto destination = value_components(result);
        for (std::size_t i = 0; i < destination.size(); ++i) {
            const auto minimum = lower[lower.size() == 1 ? 0 : i];
            const auto maximum = upper[upper.size() == 1 ? 0 : i];
            if constexpr (Operation == kernel_operation_t::clamp) {
                destination[i] = std::min(std::max(source[i], minimum), maximum);
            } else {
                const float factor = std::clamp((source[i] - minimum) / (maximum - minimum), 0.0F, 1.0F);
                destination[i] = factor * factor * (3.0F - 2.0F * factor);
            }
        }
        return result;
    } else if constexpr (Operation == kernel_operation_t::mix) {
        const auto left = value_components(read.template operator()<T>(0));
        const auto right = value_components(read.template operator()<T>(1));
        const float amount = read.template operator()<float>(2);
        T result {};
        auto destination = value_components(result);
        for (std::size_t i = 0; i < destination.size(); ++i) { destination[i] = left[i] * (1.0F - amount) + right[i] * amount; }
        return result;
    } else {
        const auto left = value_components(read.template operator()<T>(0));
        T result {};
        auto destination = value_components(result);
        for (std::size_t i = 0; i < destination.size(); ++i) {
            const auto component = left[i];
            if constexpr (Operation == kernel_operation_t::negate) {
                if constexpr (std::same_as<scalar_t<T>, std::int32_t>) { destination[i] = signed_negate(component); }
                else { destination[i] = -component; }
            } else if constexpr (Operation == kernel_operation_t::absolute) {
                if constexpr (std::same_as<scalar_t<T>, std::int32_t>) { destination[i] = component < 0 ? signed_negate(component) : component; }
                else { destination[i] = std::abs(component); }
            } else if constexpr (Operation == kernel_operation_t::square_root) { destination[i] = std::sqrt(component); }
            else if constexpr (Operation == kernel_operation_t::floor) { destination[i] = std::floor(component); }
            else if constexpr (Operation == kernel_operation_t::ceil) { destination[i] = std::ceil(component); }
            else if constexpr (Operation == kernel_operation_t::fract) { destination[i] = component - std::floor(component); }
            else if constexpr (Operation == kernel_operation_t::sine) { destination[i] = std::sin(component); }
            else if constexpr (Operation == kernel_operation_t::cosine) { destination[i] = std::cos(component); }
            else {
                const auto right = value_components(read.template operator()<U>(1));
                const auto other = right[right.size() == 1 ? 0 : i];
                if constexpr (Operation == kernel_operation_t::add) {
                    if constexpr (std::same_as<scalar_t<T>, std::int32_t>) { destination[i] = signed_add(component, other); }
                    else { destination[i] = component + other; }
                } else if constexpr (Operation == kernel_operation_t::subtract) {
                    if constexpr (std::same_as<scalar_t<T>, std::int32_t>) { destination[i] = signed_subtract(component, other); }
                    else { destination[i] = component - other; }
                } else if constexpr (Operation == kernel_operation_t::multiply) {
                    if constexpr (std::same_as<scalar_t<T>, std::int32_t>) { destination[i] = signed_multiply(component, other); }
                    else { destination[i] = component * other; }
                } else if constexpr (Operation == kernel_operation_t::divide) { destination[i] = divide(component, other); }
                else if constexpr (Operation == kernel_operation_t::modulo) { destination[i] = modulo(component, other); }
                else if constexpr (Operation == kernel_operation_t::minimum) { destination[i] = std::min(component, other); }
                else if constexpr (Operation == kernel_operation_t::maximum) { destination[i] = std::max(component, other); }
                else if constexpr (Operation == kernel_operation_t::power) { destination[i] = std::pow(component, other); }
            }
        }
        return result;
    }
}
template <typename IO>
void validate_inputs(const shader::shader_interface_t& interface, const IO& io) {
    for (const auto& input : interface.inputs()) {
        dispatch_value_type<void>(input.type, [&]<typename T>() {
            (void)io.template input<T>(input.index);
        });
    }
}


template <typename IO>
void execute_stage(const stage_code_t& code, const bindings_t& bindings, IO& io, std::vector<value_t>& slots, std::vector<std::uint8_t>& local_initialized) {
    // Result invalidation precedes allocation, validation, and interpretation.
    io.clear_results();
    try {
        constexpr auto stage = std::same_as<IO, vertex_io_t> ? shader::shader_stage_t::vertex : shader::shader_stage_t::fragment;
        if (code.interface().stage() != stage) { throw std::invalid_argument("execution_context_t::execute stage does not match invocation IO"); }
        if (slots.size() < code.slot_types().size()) { slots.resize(code.slot_types().size()); }
        if (local_initialized.size() < code.local_count()) { local_initialized.resize(code.local_count()); }
        std::fill_n(local_initialized.begin(), code.local_count(), std::uint8_t(0));
        io.reserve_outputs(code.interface().outputs().size());
        validate_inputs(code.interface(), io);
        validate_interface_bindings(code.interface(), bindings);
        vertex_io_t* vertex_io = nullptr;
        fragment_io_t* fragment_io = nullptr;
        if constexpr (std::same_as<IO, vertex_io_t>) { vertex_io = &io; }
        else { fragment_io = &io; }
        const auto entries = kernels();
        const auto instructions = code.instructions();
        const auto all_operands = code.operands();
        const auto constants = code.constants();
        std::size_t position = 0;
        bool complete = instructions.empty();
        while (!complete) {
            const auto& instruction = instructions[position++];
            const auto operands = all_operands.subspan(instruction.operand_begin, instruction.operand_count);
            switch (instruction.opcode) {
                case opcode_t::constant: { slots[*instruction.destination] = constants[operands[0].index]; } break;
                case opcode_t::copy: { slots[*instruction.destination] = slots[operands[0].index]; } break;
                case opcode_t::check_local: {
                    if (!local_initialized[operands[0].index]) { throw std::logic_error("software shader assigned an uninitialized local"); }
                } break;
                case opcode_t::initialize_local: {
                    slots[*instruction.destination] = slots[operands[0].index];
                    local_initialized[*instruction.destination] = 1;
                } break;
                case opcode_t::assign_local: {
                    if (!local_initialized[*instruction.destination]) { throw std::logic_error("software shader assigned an uninitialized local"); }
                    slots[*instruction.destination] = slots[operands[0].index];
                } break;
                case opcode_t::read_local: {
                    if (!local_initialized[operands[0].index]) { throw std::logic_error("software shader read an uninitialized local"); }
                    slots[*instruction.destination] = slots[operands[0].index];
                } break;
                case opcode_t::kernel: {
                    const auto result = entries[instruction.kernel_index].evaluate({code.interface(), slots, operands, bindings, vertex_io, fragment_io});
                    // Assignment establishes the active variant alternative after context reuse.
                    if (instruction.destination) { slots[*instruction.destination] = result; }
                } break;
                case opcode_t::builtin: {
                    switch (static_cast<shader::shader_builtin_t>(operands[0].index)) {
                        case shader::shader_builtin_t::vertex_index: { if constexpr (std::same_as<IO, vertex_io_t>) { slots[*instruction.destination] = io.vertex_index(); } else { throw std::logic_error("invalid compiled vertex operation"); } } break;
                        case shader::shader_builtin_t::instance_index: { if constexpr (std::same_as<IO, vertex_io_t>) { slots[*instruction.destination] = io.instance_index(); } else { throw std::logic_error("invalid compiled vertex operation"); } } break;
                        case shader::shader_builtin_t::object_to_world: { if constexpr (std::same_as<IO, vertex_io_t>) { slots[*instruction.destination] = io.object_to_world(); } else { throw std::logic_error("invalid compiled vertex operation"); } } break;
                        case shader::shader_builtin_t::world_to_clip: { if constexpr (std::same_as<IO, vertex_io_t>) { slots[*instruction.destination] = io.world_to_clip(); } else { throw std::logic_error("invalid compiled vertex operation"); } } break;
                        case shader::shader_builtin_t::fragment_coordinate: { if constexpr (std::same_as<IO, fragment_io_t>) { slots[*instruction.destination] = io.fragment_coordinate(); } else { throw std::logic_error("invalid compiled fragment operation"); } } break;
                        case shader::shader_builtin_t::front_facing: { if constexpr (std::same_as<IO, fragment_io_t>) { slots[*instruction.destination] = io.front_facing(); } else { throw std::logic_error("invalid compiled fragment operation"); } } break;
                    }
                } break;
                case opcode_t::position: { if constexpr (std::same_as<IO, vertex_io_t>) { io.position(std::get<shader::vector_t<float, 4>>(slots[operands[0].index])); } else { throw std::logic_error("invalid compiled stage output"); } } break;
                case opcode_t::color: { if constexpr (std::same_as<IO, fragment_io_t>) { io.color(std::get<shader::vector_t<float, 4>>(slots[operands[0].index])); } else { throw std::logic_error("invalid compiled stage output"); } } break;
                case opcode_t::sample:
                case opcode_t::sample_lod: {
                    const auto reflected = code.interface().bindings();
                    const auto& texture = bindings.texture(reflected[operands[0].index].index);
                    const auto& sampler = bindings.sampler(reflected[operands[1].index].index);
                    const auto coordinates = std::get<shader::vector_t<float, 2>>(slots[operands[2].index]);
                    slots[*instruction.destination] = instruction.opcode == opcode_t::sample ? texture::sample(texture, sampler, coordinates) : texture::sample_lod(texture, sampler, coordinates, std::get<float>(slots[operands[3].index]));
                } break;
                case opcode_t::jump: { position = *instruction.target; } break;
                case opcode_t::jump_if_false: { if (!std::get<bool>(slots[operands[0].index])) { position = *instruction.target; } } break;
                case opcode_t::jump_if_true: { if (std::get<bool>(slots[operands[0].index])) { position = *instruction.target; } } break;
                case opcode_t::discard: { if constexpr (std::same_as<IO, fragment_io_t>) { io.discard(); complete = true; } else { throw std::logic_error("invalid compiled discard"); } } break;
                case opcode_t::finish: { complete = true; } break;
            }
        }
        if constexpr (std::same_as<IO, vertex_io_t>) {
            try { (void)io.position(); }
            catch (const std::logic_error&) { throw std::runtime_error("software shader vertex invocation completed without writing position"); }
        }
    } catch (...) {
        io.clear_results();
        throw;
    }
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
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::kernel_operation_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::kernel_operation_t& value, auto& context) const {
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
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::kernel_arguments_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::kernel_arguments_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "slots={} operands={}", value.slots.size(), value.operands.size());
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::kernel_entry_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::kernel_entry_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "operation={}", value.operation);
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::loop_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::loop_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "condition={} breaks={}", value.condition, value.breaks.size());
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::compiler_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::compiler_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "instructions={} slots={} locals={}", value.instructions.size(), value.slot_types.size(), value.local_count);
        return out;
    }
};

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::stage_code_t> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::stage_code_t& value, auto& context) const {
        auto out = context.out();
        out = std::format_to(out, "instructions={} operands={} slots={} locals={} storage_bytes={}", value.instructions().size(), value.operands().size(), value.slot_types().size(), value.local_count(), value.storage_bytes());
        return out;
    }
};

} // namespace std

#endif // M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_HELPERS_H
