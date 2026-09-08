#include "helpers.h"
#include "invocation.h"
#include "software_shader.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#include <initializer_list>
#include <limits>
#include <map>
#include <set>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

struct prepared_invocation_t {
    std::span<const binding_value_t> bindings;
    std::span<const value_t> inputs;
    std::span<std::optional<value_t>> outputs;
};

enum class kernel_operation_t { input, uniform, output, component, construct, swizzle, negate, logical_not, absolute, square_root, floor, ceil, fract, sine, cosine, normalize, length, add, subtract, multiply, divide, modulo, equal, not_equal, less, less_equal, greater, greater_equal, dot, cross, power, reflect, minimum, maximum, step, clamp, mix, smoothstep, matrix_product };

struct kernel_arguments_t {
    const shader::shader_interface_t& interface;
    std::span<const value_t> slots;
    std::span<const operand_t> operands;
    const bindings_t* bindings;
    const prepared_invocation_t* prepared;
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
    std::size_t slot_count = 0;
    std::size_t local_count = 0;

private:
    void collect(const shader::shader_expression_node_t& expression);
    void collect(const shader::shader_block_t& block);
    void lower(const shader::shader_block_t& block);
    operand_t lower(const shader::shader_expression_node_t& expression);
    std::size_t slot();
    std::size_t emit(opcode_t opcode, std::optional<std::size_t> destination, std::span<const operand_t> inputs, std::optional<std::size_t> target = std::nullopt, std::size_t kernel_index = 0);
    std::size_t emit(opcode_t opcode, std::optional<std::size_t> destination, std::initializer_list<operand_t> inputs, std::optional<std::size_t> target = std::nullopt, std::size_t kernel_index = 0);
    operand_t compute(kernel_operation_t operation, std::span<const operand_t> inputs, std::initializer_list<shader::shader_data_type_t> types);
    operand_t compute(kernel_operation_t operation, std::initializer_list<operand_t> inputs, std::initializer_list<shader::shader_data_type_t> types);
    std::size_t interface_index(std::span<const shader::shader_interface_element_t> elements, std::uint32_t index, shader::shader_data_type_t type) const;

    const shader::shader_interface_t& m_interface;
    std::map<const shader::shader_local_node_t*, std::size_t> m_locals;
    std::set<const shader::shader_expression_node_t*> m_collected;
    std::map<const shader::shader_constant_node_t*, std::size_t> m_constants;
    std::vector<loop_t> m_loops;
    operand_t m_result {operand_kind_t::slot, 0};
};

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
int binding_namespace(shader::shader_data_type_t type);
template <typename IO>
void execute_stage(const stage_code_t& code, const bindings_t* bindings, IO& io, std::vector<value_t>& slots, std::vector<std::uint8_t>& local_initialized, const prepared_invocation_t* prepared);

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

namespace std {

template <>
struct formatter<m03gt1djvvy5atia5evkbg6rqy_software_shader::prepared_invocation_t> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const m03gt1djvvy5atia5evkbg6rqy_software_shader::prepared_invocation_t& invocation, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "inputs={} outputs={}", invocation.inputs.size(), invocation.outputs.size());
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
        out = std::format_to(out, "instructions={} slots={} locals={}", value.instructions.size(), value.slot_count, value.local_count);
        return out;
    }
};

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
        if (arguments.prepared) { return std::get<T>(arguments.prepared->inputs[arguments.operands[0].index]); }
        const auto location = arguments.interface.inputs()[arguments.operands[0].index].index;
        return arguments.vertex_io ? arguments.vertex_io->input<T>(location) : arguments.fragment_io->input<T>(location);
    } else if constexpr (Operation == kernel_operation_t::uniform) {
        if (arguments.prepared) { return std::get<T>(std::get<value_t>(arguments.prepared->bindings[arguments.operands[0].index])); }
        return arguments.bindings->uniform<T>(arguments.interface.bindings()[arguments.operands[0].index].index);
    } else if constexpr (Operation == kernel_operation_t::output) {
        const auto location = arguments.interface.outputs()[arguments.operands[0].index].index;
        const auto& output = read.template operator()<T>(1);
        if (arguments.prepared) {
            arguments.prepared->outputs[arguments.operands[0].index] = output;
        } else if (arguments.vertex_io) {
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
void execute_stage(const stage_code_t& code, const bindings_t* bindings, IO& io, std::vector<value_t>& slots, std::vector<std::uint8_t>& local_initialized, const prepared_invocation_t* prepared) {
    // Result invalidation precedes allocation, validation, and interpretation.
    io.clear_results();
    if (prepared) { std::ranges::fill(prepared->outputs, std::nullopt); }
    try {
        constexpr auto stage = std::same_as<IO, vertex_io_t> ? shader::shader_stage_t::vertex : shader::shader_stage_t::fragment;
        if (code.interface().stage() != stage) { throw std::invalid_argument("execution_context_t::execute stage does not match invocation IO"); }
        if (slots.size() < code.slot_count()) { slots.resize(code.slot_count()); }
        if (local_initialized.size() < code.local_count()) { local_initialized.resize(code.local_count()); }
        std::fill_n(local_initialized.begin(), code.local_count(), std::uint8_t(0));
        if (prepared) {
            if (prepared->inputs.size() != code.interface().inputs().size() || prepared->outputs.size() != code.interface().outputs().size()) {
                throw std::invalid_argument("prepared shader IO sizes do not match the reflected stage interface");
            }
        } else {
            io.reserve_outputs(code.interface().outputs().size());
            validate_inputs(code.interface(), io);
            validate_interface_bindings(code.interface(), *bindings);
        }
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
                    const auto result = entries[instruction.kernel_index].evaluate({code.interface(), slots, operands, bindings, prepared, vertex_io, fragment_io});
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
                    const auto& texture = prepared ? *std::get<const texture::texture_t*>(prepared->bindings[operands[0].index]) : bindings->texture(reflected[operands[0].index].index);
                    const auto& sampler = prepared ? *std::get<const texture::sampler_t*>(prepared->bindings[operands[1].index]) : bindings->sampler(reflected[operands[1].index].index);
                    const auto coordinates = std::get<shader::vector_t<float, 2>>(slots[operands[2].index]);
                    slots[*instruction.destination] = instruction.opcode == opcode_t::sample ? texture::sample(texture, sampler, coordinates) : texture::sample_lod(texture, sampler, coordinates, std::get<float>(slots[operands[3].index]));
                } break;
                case opcode_t::jump: { position = *instruction.target; } break;
                case opcode_t::jump_if_false: { if (!std::get<bool>(slots[operands[0].index])) { position = *instruction.target; } } break;
                case opcode_t::jump_if_true: { if (std::get<bool>(slots[operands[0].index])) { position = *instruction.target; } } break;
                case opcode_t::discard: { if constexpr (std::same_as<IO, fragment_io_t>) { io.discard(); if (prepared) { std::ranges::fill(prepared->outputs, std::nullopt); } complete = true; } else { throw std::logic_error("invalid compiled discard"); } } break;
                case opcode_t::finish: { complete = true; } break;
            }
        }
        if constexpr (std::same_as<IO, vertex_io_t>) {
            try { (void)io.position(); }
            catch (const std::logic_error&) { throw std::runtime_error("software shader vertex invocation completed without writing position"); }
        }
    } catch (...) {
        io.clear_results();
        if (prepared) { std::ranges::fill(prepared->outputs, std::nullopt); }
        throw;
    }
}


template <typename T>
constexpr std::size_t value_type_index() {
    return []<std::size_t... I>(std::index_sequence<I...>) {
        static_assert((std::same_as<T, std::variant_alternative_t<I, value_t>> || ...));
        return ((std::same_as<T, std::variant_alternative_t<I, value_t>> ? I : 0) + ...);
    }(std::make_index_sequence<std::variant_size_v<value_t>> {});
}

template <kernel_operation_t Operation, typename... T>
constexpr kernel_entry_t make_kernel() {
    static_assert(1 <= sizeof...(T) && sizeof...(T) <= 3);
    std::array<std::size_t, 3> types;
    types.fill(std::numeric_limits<std::size_t>::max());
    std::size_t index = 0;
    ((types[index++] = value_type_index<T>()), ...);
    return {Operation, types, evaluate_kernel<Operation, T...>};
}

compiler_t::compiler_t(const shader::shader_ast_t& ast):
    m_interface(ast.interface())
{
    collect(ast.root());
    local_count = slot_count;
    lower(ast.root());
    emit(opcode_t::finish, std::nullopt, {});
}

void compiler_t::visit(const shader::shader_constant_node_t& node) {
    const auto [entry, inserted] = m_constants.try_emplace(&node, constants.size());
    if (inserted) { constants.push_back(literal_value(node.value())); }
    const auto destination = slot();
    emit(opcode_t::constant, destination, {{operand_kind_t::constant, entry->second}});
    m_result = {operand_kind_t::slot, destination};
}

void compiler_t::visit(const shader::shader_input_node_t& node) {
    m_result = compute(kernel_operation_t::input, {{operand_kind_t::input, interface_index(m_interface.inputs(), node.location(), node.type())}}, {node.type()});
}

void compiler_t::visit(const shader::shader_uniform_node_t& node) {
    m_result = compute(kernel_operation_t::uniform, {{operand_kind_t::binding, interface_index(m_interface.bindings(), node.binding(), node.type())}}, {node.type()});
}

void compiler_t::visit(const shader::shader_resource_node_t& node) {
    m_result = {operand_kind_t::binding, interface_index(m_interface.bindings(), node.binding(), node.type())};
}

void compiler_t::visit(const shader::shader_builtin_node_t& node) {
    const auto destination = slot();
    emit(opcode_t::builtin, destination, {{operand_kind_t::builtin, static_cast<std::size_t>(node.builtin())}});
    m_result = {operand_kind_t::slot, destination};
}

void compiler_t::visit(const shader::shader_local_node_t& node) {
    const auto destination = slot();
    emit(opcode_t::read_local, destination, {{operand_kind_t::slot, m_locals.at(&node)}});
    m_result = {operand_kind_t::slot, destination};
}

void compiler_t::visit(const shader::shader_unary_node_t& node) {
    const auto input = lower(node.expression());
    m_result = compute(operation(node.operation()), {input}, {node.expression().type()});
}

void compiler_t::visit(const shader::shader_binary_node_t& node) {
    const auto left = lower(node.lhs());
    if (node.operation() == shader::shader_binary_operation_t::logical_and || node.operation() == shader::shader_binary_operation_t::logical_or) {
        const auto destination = slot();
        emit(opcode_t::copy, destination, {left});
        const auto branch = emit(node.operation() == shader::shader_binary_operation_t::logical_and ? opcode_t::jump_if_false : opcode_t::jump_if_true, std::nullopt, {left});
        const auto right = lower(node.rhs());
        emit(opcode_t::copy, destination, {right});
        instructions[branch].target = instructions.size();
        m_result = {operand_kind_t::slot, destination};
        return;
    }
    const auto right = lower(node.rhs());
    auto selected = operation(node.operation());
    if (selected == kernel_operation_t::multiply && node.lhs().type().category() == shader::shader_data_category_t::matrix && node.rhs().type().category() != shader::shader_data_category_t::scalar) {
        selected = kernel_operation_t::matrix_product;
    }
    switch (selected) {
        case kernel_operation_t::equal:
        case kernel_operation_t::not_equal:
        case kernel_operation_t::less:
        case kernel_operation_t::less_equal:
        case kernel_operation_t::greater:
        case kernel_operation_t::greater_equal:
        case kernel_operation_t::dot:
        case kernel_operation_t::cross:
        case kernel_operation_t::power:
        case kernel_operation_t::reflect: {
            m_result = compute(selected, {left, right}, {node.lhs().type()});
        } break;
        default: {
            m_result = compute(selected, {left, right}, {node.lhs().type(), node.rhs().type()});
        } break;
    }
}

void compiler_t::visit(const shader::shader_construct_node_t& node) {
    std::vector<operand_t> components;
    for (const auto* expression : node.operands()) {
        const auto input = lower(*expression);
        const auto type = expression->type();
        if (type.category() == shader::shader_data_category_t::scalar) {
            components.push_back(input);
        } else {
            const std::size_t count = std::size_t(type.rows()) * type.columns();
            for (std::size_t index = 0; index < count; ++index) {
                components.push_back(compute(kernel_operation_t::component, {input, {operand_kind_t::component, index}}, {type}));
            }
        }
    }
    m_result = compute(kernel_operation_t::construct, components, {node.type()});
}

void compiler_t::visit(const shader::shader_swizzle_node_t& node) {
    std::vector<operand_t> components {lower(node.expression())};
    for (const auto component : node.components()) { components.push_back({operand_kind_t::component, component}); }
    m_result = compute(kernel_operation_t::swizzle, components, {node.type(), node.expression().type()});
}

void compiler_t::visit(const shader::shader_call_node_t& node) {
    const auto expressions = node.operands();
    std::vector<operand_t> inputs;
    for (const auto* expression : expressions) { inputs.push_back(lower(*expression)); }
    switch (node.operation()) {
        case shader::shader_call_operation_t::clamp: {
            m_result = compute(kernel_operation_t::clamp, inputs, {expressions[0]->type(), expressions[1]->type(), expressions[2]->type()});
        } break;
        case shader::shader_call_operation_t::mix: {
            m_result = compute(kernel_operation_t::mix, inputs, {node.type()});
        } break;
        case shader::shader_call_operation_t::smoothstep: {
            m_result = compute(kernel_operation_t::smoothstep, {inputs[2], inputs[0], inputs[1]}, {expressions[2]->type(), expressions[0]->type(), expressions[1]->type()});
        } break;
        case shader::shader_call_operation_t::sample:
        case shader::shader_call_operation_t::sample_lod: {
            const auto destination = slot();
            emit(node.operation() == shader::shader_call_operation_t::sample ? opcode_t::sample : opcode_t::sample_lod, destination, inputs);
            m_result = {operand_kind_t::slot, destination};
        } break;
    }
}

void compiler_t::visit(const shader::shader_local_statement_t& node) {
    const auto initial = lower(node.initial());
    emit(opcode_t::initialize_local, m_locals.at(node.local_node()), {initial});
}

void compiler_t::visit(const shader::shader_assignment_statement_t& node) {
    const auto local = m_locals.at(node.local_node());
    // Preserve the existing check before evaluation of the assigned expression.
    emit(opcode_t::check_local, std::nullopt, {{operand_kind_t::slot, local}});
    const auto assigned = lower(node.value());
    emit(opcode_t::assign_local, local, {assigned});
}

void compiler_t::visit(const shader::shader_output_statement_t& node) {
    const auto input = lower(node.expression());
    switch (node.output()) {
        case shader::shader_output_t::position: { emit(opcode_t::position, std::nullopt, {input}); } break;
        case shader::shader_output_t::color: { emit(opcode_t::color, std::nullopt, {input}); } break;
        case shader::shader_output_t::location: {
            const auto output = interface_index(m_interface.outputs(), node.location(), node.expression().type());
            emit(opcode_t::kernel, std::nullopt, {{operand_kind_t::output, output}, input}, std::nullopt, select_kernel(kernel_operation_t::output, {node.expression().type()}));
        } break;
    }
}

void compiler_t::visit(const shader::shader_branch_statement_t& node) {
    const auto condition = lower(node.condition());
    const auto branch = emit(opcode_t::jump_if_false, std::nullopt, {condition});
    lower(node.true_block());
    const auto end = emit(opcode_t::jump, std::nullopt, {});
    instructions[branch].target = instructions.size();
    lower(node.false_block());
    instructions[end].target = instructions.size();
}

void compiler_t::visit(const shader::shader_loop_statement_t& node) {
    const auto start = instructions.size();
    const auto condition = lower(node.condition());
    const auto branch = emit(opcode_t::jump_if_false, std::nullopt, {condition});
    m_loops.push_back({start, {}});
    lower(node.body());
    emit(opcode_t::jump, std::nullopt, {}, start);
    instructions[branch].target = instructions.size();
    for (const auto index : m_loops.back().breaks) { instructions[index].target = instructions.size(); }
    m_loops.pop_back();
}

void compiler_t::visit(const shader::shader_break_statement_t&) {
    if (m_loops.empty()) { throw std::logic_error("software shader compiler encountered break outside a loop"); }
    m_loops.back().breaks.push_back(emit(opcode_t::jump, std::nullopt, {}));
}

void compiler_t::visit(const shader::shader_continue_statement_t&) {
    if (m_loops.empty()) { throw std::logic_error("software shader compiler encountered continue outside a loop"); }
    emit(opcode_t::jump, std::nullopt, {}, m_loops.back().condition);
}

void compiler_t::visit(const shader::shader_discard_statement_t&) {
    emit(opcode_t::discard, std::nullopt, {});
}

void compiler_t::collect(const shader::shader_expression_node_t& expression) {
    if (!m_collected.insert(&expression).second) { return; }
    if (const auto* local = dynamic_cast<const shader::shader_local_node_t*>(&expression)) {
        m_locals.emplace(local, slot());
    }
    for (const auto* operand : expression.operands()) { collect(*operand); }
}

void compiler_t::collect(const shader::shader_block_t& block) {
    for (const auto& statement : block.statements) {
        if (const auto* local = dynamic_cast<const shader::shader_local_statement_t*>(statement.get())) {
            collect(local->local()); collect(local->initial());
        } else if (const auto* assignment = dynamic_cast<const shader::shader_assignment_statement_t*>(statement.get())) {
            collect(assignment->local()); collect(assignment->value());
        } else if (const auto* output = dynamic_cast<const shader::shader_output_statement_t*>(statement.get())) {
            collect(output->expression());
        } else if (const auto* branch = dynamic_cast<const shader::shader_branch_statement_t*>(statement.get())) {
            collect(branch->condition()); collect(branch->true_block()); collect(branch->false_block());
        } else if (const auto* loop = dynamic_cast<const shader::shader_loop_statement_t*>(statement.get())) {
            collect(loop->condition()); collect(loop->body());
        }
    }
}

void compiler_t::lower(const shader::shader_block_t& block) {
    for (const auto& statement : block.statements) { statement->accept(*this); }
}

operand_t compiler_t::lower(const shader::shader_expression_node_t& expression) {
    // AST sharing is not value caching. Lower at each executed use site.
    expression.accept(*this);
    return m_result;
}

std::size_t compiler_t::slot() {
    return slot_count++;
}

std::size_t compiler_t::emit(opcode_t opcode, std::optional<std::size_t> destination, std::span<const operand_t> inputs, std::optional<std::size_t> target, std::size_t kernel_index) {
    const auto begin = operands.size();
    operands.insert(operands.end(), inputs.begin(), inputs.end());
    instructions.push_back({opcode, destination, begin, inputs.size(), target, kernel_index});
    return instructions.size() - 1;
}

std::size_t compiler_t::emit(opcode_t opcode, std::optional<std::size_t> destination, std::initializer_list<operand_t> inputs, std::optional<std::size_t> target, std::size_t kernel_index) {
    return emit(opcode, destination, std::span(inputs.begin(), inputs.size()), target, kernel_index);
}

operand_t compiler_t::compute(kernel_operation_t operation, std::span<const operand_t> inputs, std::initializer_list<shader::shader_data_type_t> types) {
    const auto destination = slot();
    emit(opcode_t::kernel, destination, inputs, std::nullopt, select_kernel(operation, types));
    return {operand_kind_t::slot, destination};
}

operand_t compiler_t::compute(kernel_operation_t operation, std::initializer_list<operand_t> inputs, std::initializer_list<shader::shader_data_type_t> types) {
    return compute(operation, std::span(inputs.begin(), inputs.size()), types);
}

std::size_t compiler_t::interface_index(std::span<const shader::shader_interface_element_t> elements, std::uint32_t index, shader::shader_data_type_t type) const {
    for (std::size_t i = 0; i < elements.size(); ++i) {
        if (elements[i].index == index && elements[i].type == type) { return i; }
    }
    throw std::logic_error("software shader compiler could not resolve a reflected interface entry");
}

stage_code_t::stage_code_t(const shader::shader_ast_t& ast):
    m_interface(ast.interface())
{
    compiler_t compiler(ast);
    m_instructions = std::move(compiler.instructions);
    m_operands = std::move(compiler.operands);
    m_constants = std::move(compiler.constants);
    m_slot_count = compiler.slot_count;
    m_local_count = compiler.local_count;
}

const shader::shader_interface_t& stage_code_t::interface() const { return m_interface; }
std::span<const instruction_t> stage_code_t::instructions() const { return m_instructions; }
std::span<const operand_t> stage_code_t::operands() const { return m_operands; }
std::span<const value_t> stage_code_t::constants() const { return m_constants; }
std::size_t stage_code_t::slot_count() const { return m_slot_count; }
std::size_t stage_code_t::local_count() const { return m_local_count; }
std::size_t stage_code_t::storage_bytes() const {
    // Owned payload, excluding allocator metadata and reflection's inaccessible spare capacity.
    return sizeof(*this) + m_instructions.capacity() * sizeof(instruction_t) + m_operands.capacity() * sizeof(operand_t) + m_constants.capacity() * sizeof(value_t) +
        (m_interface.inputs().size() + m_interface.outputs().size() + m_interface.bindings().size()) * sizeof(shader::shader_interface_element_t);
}

shader::matrix_t<float, 4, 4> identity_matrix() {
    return {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};
}

template <typename T, kernel_operation_t... Operations>
constexpr std::array<kernel_entry_t, sizeof...(Operations)> make_kernels() {
    return {make_kernel<Operations, T>()...};
}

template <typename T, typename U, kernel_operation_t... Operations>
constexpr std::array<kernel_entry_t, sizeof...(Operations)> make_binary_kernels() {
    return {make_kernel<Operations, T, U>()...};
}

template <typename T, typename U, typename V, kernel_operation_t... Operations>
constexpr std::array<kernel_entry_t, sizeof...(Operations)> make_ternary_kernels() {
    return {make_kernel<Operations, T, U, V>()...};
}

template <std::size_t... Counts>
constexpr auto join_kernels(const std::array<kernel_entry_t, Counts>&... families) {
    constexpr auto count = [] {
        std::size_t result = 0;
        for (const auto size : std::array<std::size_t, sizeof...(Counts)>{Counts...}) result += size;
        return result;
    }();
    std::array<kernel_entry_t, count> result;
    auto next = result.begin();
    for (const auto family : {std::span<const kernel_entry_t>(families)...}) {
        next = std::copy(family.begin(), family.end(), next);
    }
    return result;
}

std::span<const kernel_entry_t> kernels() {
    using enum kernel_operation_t;
    static const auto entries = join_kernels(
        make_kernels<bool, input, uniform, output, component, construct, equal, not_equal, logical_not>(),
        make_kernels<std::int32_t, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<std::int32_t, std::int32_t, add, subtract, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<std::int32_t, std::int32_t, std::int32_t, clamp>(),
        make_kernels<std::int32_t, less, less_equal, greater, greater_equal, absolute>(),
        make_kernels<std::uint32_t, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<std::uint32_t, std::uint32_t, add, subtract, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<std::uint32_t, std::uint32_t, std::uint32_t, clamp>(),
        make_kernels<std::uint32_t, less, less_equal, greater, greater_equal>(),
        make_kernels<float, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<float, float, add, subtract, divide, multiply, minimum, maximum>(),
        make_ternary_kernels<float, float, float, clamp>(),
        make_kernels<float, less, less_equal, greater, greater_equal, absolute, square_root, floor, ceil, fract, sine, cosine, power, reflect, mix>(),
        make_binary_kernels<float, float, step>(),
        make_ternary_kernels<float, float, float, smoothstep>(),
        make_kernels<shader::vector_t<bool, 2>, input, uniform, output, component, construct, equal, not_equal>(),
        make_binary_kernels<bool, shader::vector_t<bool, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<bool, 2>, shader::vector_t<bool, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<bool, 3>, shader::vector_t<bool, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<bool, 4>, shader::vector_t<bool, 2>, swizzle>(),
        make_kernels<shader::vector_t<bool, 3>, input, uniform, output, component, construct, equal, not_equal>(),
        make_binary_kernels<bool, shader::vector_t<bool, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<bool, 2>, shader::vector_t<bool, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<bool, 3>, shader::vector_t<bool, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<bool, 4>, shader::vector_t<bool, 3>, swizzle>(),
        make_kernels<shader::vector_t<bool, 4>, input, uniform, output, component, construct, equal, not_equal>(),
        make_binary_kernels<bool, shader::vector_t<bool, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<bool, 2>, shader::vector_t<bool, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<bool, 3>, shader::vector_t<bool, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<bool, 4>, shader::vector_t<bool, 4>, swizzle>(),
        make_kernels<shader::vector_t<std::int32_t, 2>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>, add>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 2>, std::int32_t, add>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>, subtract>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 2>, std::int32_t, subtract>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>, std::int32_t, clamp>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 2>, std::int32_t, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 2>, std::int32_t, shader::vector_t<std::int32_t, 2>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 2>, std::int32_t, std::int32_t, clamp>(),
        make_kernels<shader::vector_t<std::int32_t, 2>, absolute>(),
        make_binary_kernels<std::int32_t, shader::vector_t<std::int32_t, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 2>, swizzle>(),
        make_kernels<shader::vector_t<std::int32_t, 3>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>, add>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 3>, std::int32_t, add>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>, subtract>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 3>, std::int32_t, subtract>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>, std::int32_t, clamp>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 3>, std::int32_t, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 3>, std::int32_t, shader::vector_t<std::int32_t, 3>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 3>, std::int32_t, std::int32_t, clamp>(),
        make_kernels<shader::vector_t<std::int32_t, 3>, absolute>(),
        make_binary_kernels<std::int32_t, shader::vector_t<std::int32_t, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 3>, swizzle>(),
        make_kernels<shader::vector_t<std::int32_t, 4>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>, add>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 4>, std::int32_t, add>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>, subtract>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 4>, std::int32_t, subtract>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>, std::int32_t, clamp>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 4>, std::int32_t, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 4>, std::int32_t, shader::vector_t<std::int32_t, 4>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::int32_t, 4>, std::int32_t, std::int32_t, clamp>(),
        make_kernels<shader::vector_t<std::int32_t, 4>, absolute>(),
        make_binary_kernels<std::int32_t, shader::vector_t<std::int32_t, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>, swizzle>(),
        make_kernels<shader::vector_t<std::uint32_t, 2>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>, add>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 2>, std::uint32_t, add>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>, subtract>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 2>, std::uint32_t, subtract>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>, std::uint32_t, clamp>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 2>, std::uint32_t, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 2>, std::uint32_t, shader::vector_t<std::uint32_t, 2>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 2>, std::uint32_t, std::uint32_t, clamp>(),
        make_binary_kernels<std::uint32_t, shader::vector_t<std::uint32_t, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 2>, swizzle>(),
        make_kernels<shader::vector_t<std::uint32_t, 3>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>, add>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 3>, std::uint32_t, add>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>, subtract>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 3>, std::uint32_t, subtract>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>, std::uint32_t, clamp>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 3>, std::uint32_t, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 3>, std::uint32_t, shader::vector_t<std::uint32_t, 3>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 3>, std::uint32_t, std::uint32_t, clamp>(),
        make_binary_kernels<std::uint32_t, shader::vector_t<std::uint32_t, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 3>, swizzle>(),
        make_kernels<shader::vector_t<std::uint32_t, 4>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>, add>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 4>, std::uint32_t, add>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>, subtract>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 4>, std::uint32_t, subtract>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>, std::uint32_t, clamp>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 4>, std::uint32_t, divide, multiply, modulo, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 4>, std::uint32_t, shader::vector_t<std::uint32_t, 4>, clamp>(),
        make_ternary_kernels<shader::vector_t<std::uint32_t, 4>, std::uint32_t, std::uint32_t, clamp>(),
        make_binary_kernels<std::uint32_t, shader::vector_t<std::uint32_t, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>, swizzle>(),
        make_kernels<shader::vector_t<float, 2>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 2>, add>(),
        make_binary_kernels<shader::vector_t<float, 2>, float, add>(),
        make_binary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 2>, subtract>(),
        make_binary_kernels<shader::vector_t<float, 2>, float, subtract>(),
        make_binary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 2>, divide, multiply, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 2>, shader::vector_t<float, 2>, clamp>(),
        make_ternary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 2>, float, clamp>(),
        make_binary_kernels<shader::vector_t<float, 2>, float, divide, multiply, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<float, 2>, float, shader::vector_t<float, 2>, clamp>(),
        make_ternary_kernels<shader::vector_t<float, 2>, float, float, clamp>(),
        make_kernels<shader::vector_t<float, 2>, absolute, square_root, floor, ceil, fract, sine, cosine, power, reflect, mix, normalize, length, dot>(),
        make_binary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 2>, step>(),
        make_ternary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 2>, shader::vector_t<float, 2>, smoothstep>(),
        make_ternary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 2>, float, smoothstep>(),
        make_binary_kernels<float, shader::vector_t<float, 2>, step>(),
        make_ternary_kernels<shader::vector_t<float, 2>, float, shader::vector_t<float, 2>, smoothstep>(),
        make_ternary_kernels<shader::vector_t<float, 2>, float, float, smoothstep>(),
        make_binary_kernels<float, shader::vector_t<float, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 2>, swizzle>(),
        make_binary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 2>, swizzle>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, shader::vector_t<float, 2>, swizzle>(),
        make_kernels<shader::vector_t<float, 3>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 3>, add>(),
        make_binary_kernels<shader::vector_t<float, 3>, float, add>(),
        make_binary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 3>, subtract>(),
        make_binary_kernels<shader::vector_t<float, 3>, float, subtract>(),
        make_binary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 3>, divide, multiply, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 3>, shader::vector_t<float, 3>, clamp>(),
        make_ternary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 3>, float, clamp>(),
        make_binary_kernels<shader::vector_t<float, 3>, float, divide, multiply, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<float, 3>, float, shader::vector_t<float, 3>, clamp>(),
        make_ternary_kernels<shader::vector_t<float, 3>, float, float, clamp>(),
        make_kernels<shader::vector_t<float, 3>, absolute, square_root, floor, ceil, fract, sine, cosine, power, reflect, mix, normalize, length, dot, cross>(),
        make_binary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 3>, step>(),
        make_ternary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 3>, shader::vector_t<float, 3>, smoothstep>(),
        make_ternary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 3>, float, smoothstep>(),
        make_binary_kernels<float, shader::vector_t<float, 3>, step>(),
        make_ternary_kernels<shader::vector_t<float, 3>, float, shader::vector_t<float, 3>, smoothstep>(),
        make_ternary_kernels<shader::vector_t<float, 3>, float, float, smoothstep>(),
        make_binary_kernels<float, shader::vector_t<float, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 3>, swizzle>(),
        make_binary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 3>, swizzle>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, shader::vector_t<float, 3>, swizzle>(),
        make_kernels<shader::vector_t<float, 4>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 4>, add>(),
        make_binary_kernels<shader::vector_t<float, 4>, float, add>(),
        make_binary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 4>, subtract>(),
        make_binary_kernels<shader::vector_t<float, 4>, float, subtract>(),
        make_binary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 4>, divide, multiply, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 4>, shader::vector_t<float, 4>, clamp>(),
        make_ternary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 4>, float, clamp>(),
        make_binary_kernels<shader::vector_t<float, 4>, float, divide, multiply, minimum, maximum>(),
        make_ternary_kernels<shader::vector_t<float, 4>, float, shader::vector_t<float, 4>, clamp>(),
        make_ternary_kernels<shader::vector_t<float, 4>, float, float, clamp>(),
        make_kernels<shader::vector_t<float, 4>, absolute, square_root, floor, ceil, fract, sine, cosine, power, reflect, mix, normalize, length, dot>(),
        make_binary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 4>, step>(),
        make_ternary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 4>, shader::vector_t<float, 4>, smoothstep>(),
        make_ternary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 4>, float, smoothstep>(),
        make_binary_kernels<float, shader::vector_t<float, 4>, step>(),
        make_ternary_kernels<shader::vector_t<float, 4>, float, shader::vector_t<float, 4>, smoothstep>(),
        make_ternary_kernels<shader::vector_t<float, 4>, float, float, smoothstep>(),
        make_binary_kernels<float, shader::vector_t<float, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<float, 2>, shader::vector_t<float, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<float, 3>, shader::vector_t<float, 4>, swizzle>(),
        make_binary_kernels<shader::vector_t<float, 4>, shader::vector_t<float, 4>, swizzle>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, shader::vector_t<float, 4>, swizzle>(),
        make_kernels<shader::matrix_t<float, 2, 2>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>, add, subtract, divide>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, float, divide, multiply>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 2>, float, shader::matrix_t<float, 2, 2>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 2>, float, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>, step>(),
        make_binary_kernels<float, shader::matrix_t<float, 2, 2>, step>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, shader::vector_t<float, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 4>, matrix_product>(),
        make_kernels<shader::matrix_t<float, 2, 3>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>, add, subtract, divide>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 2, 3>, float, divide, multiply>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 3>, float, shader::matrix_t<float, 2, 3>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 3>, float, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>, step>(),
        make_binary_kernels<float, shader::matrix_t<float, 2, 3>, step>(),
        make_binary_kernels<shader::matrix_t<float, 2, 3>, shader::vector_t<float, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 3, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 3, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 3, 4>, matrix_product>(),
        make_kernels<shader::matrix_t<float, 2, 4>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>, add, subtract, divide>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 2, 4>, float, divide, multiply>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 4>, float, shader::matrix_t<float, 2, 4>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 2, 4>, float, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>, step>(),
        make_binary_kernels<float, shader::matrix_t<float, 2, 4>, step>(),
        make_binary_kernels<shader::matrix_t<float, 2, 4>, shader::vector_t<float, 4>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 4, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 4, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 4, 4>, matrix_product>(),
        make_kernels<shader::matrix_t<float, 3, 2>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>, add, subtract, divide>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 3, 2>, float, divide, multiply>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 2>, float, shader::matrix_t<float, 3, 2>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 2>, float, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>, step>(),
        make_binary_kernels<float, shader::matrix_t<float, 3, 2>, step>(),
        make_binary_kernels<shader::matrix_t<float, 3, 2>, shader::vector_t<float, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 2, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 2, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 2, 4>, matrix_product>(),
        make_kernels<shader::matrix_t<float, 3, 3>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>, add, subtract, divide>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 3, 3>, float, divide, multiply>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 3>, float, shader::matrix_t<float, 3, 3>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 3>, float, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>, step>(),
        make_binary_kernels<float, shader::matrix_t<float, 3, 3>, step>(),
        make_binary_kernels<shader::matrix_t<float, 3, 3>, shader::vector_t<float, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 4>, matrix_product>(),
        make_kernels<shader::matrix_t<float, 3, 4>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>, add, subtract, divide>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 3, 4>, float, divide, multiply>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 4>, float, shader::matrix_t<float, 3, 4>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 3, 4>, float, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>, step>(),
        make_binary_kernels<float, shader::matrix_t<float, 3, 4>, step>(),
        make_binary_kernels<shader::matrix_t<float, 3, 4>, shader::vector_t<float, 4>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 4, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 4, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 4, 4>, matrix_product>(),
        make_kernels<shader::matrix_t<float, 4, 2>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>, add, subtract, divide>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 4, 2>, float, divide, multiply>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 2>, float, shader::matrix_t<float, 4, 2>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 2>, float, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>, step>(),
        make_binary_kernels<float, shader::matrix_t<float, 4, 2>, step>(),
        make_binary_kernels<shader::matrix_t<float, 4, 2>, shader::vector_t<float, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 2, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 2, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 2, 4>, matrix_product>(),
        make_kernels<shader::matrix_t<float, 4, 3>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>, add, subtract, divide>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 4, 3>, float, divide, multiply>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 3>, float, shader::matrix_t<float, 4, 3>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 3>, float, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>, step>(),
        make_binary_kernels<float, shader::matrix_t<float, 4, 3>, step>(),
        make_binary_kernels<shader::matrix_t<float, 4, 3>, shader::vector_t<float, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 3, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 3, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 3, 4>, matrix_product>(),
        make_kernels<shader::matrix_t<float, 4, 4>, input, uniform, output, component, construct, equal, not_equal, negate>(),
        make_binary_kernels<shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>, add, subtract, divide>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 4, 4>, float, divide, multiply>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 4>, float, shader::matrix_t<float, 4, 4>, clamp>(),
        make_ternary_kernels<shader::matrix_t<float, 4, 4>, float, float, clamp>(),
        make_binary_kernels<shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>, step>(),
        make_binary_kernels<float, shader::matrix_t<float, 4, 4>, step>(),
        make_binary_kernels<shader::matrix_t<float, 4, 4>, shader::vector_t<float, 4>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 2>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 3>, matrix_product>(),
        make_binary_kernels<shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>, matrix_product>()
    );
    return entries;
}

std::size_t value_type_index(shader::shader_data_type_t type) {
    static constexpr auto types = []<std::size_t... I>(std::index_sequence<I...>) {
        return std::array {shader::shader_data_type<std::variant_alternative_t<I, value_t>>()...};
    }(std::make_index_sequence<std::variant_size_v<value_t>> {});
    const auto found = std::ranges::find(types, type);
    if (found == std::end(types)) { throw std::logic_error("software shader compiler encountered an unsupported value type"); }
    return std::size_t(found - std::begin(types));
}

std::size_t select_kernel(kernel_operation_t operation, std::initializer_list<shader::shader_data_type_t> types) {
    std::array<std::size_t, 3> indices;
    indices.fill(std::numeric_limits<std::size_t>::max());
    std::size_t i = 0;
    for (const auto type : types) { indices.at(i++) = value_type_index(type); }
    const auto entries = kernels();
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (entries[index].operation == operation && entries[index].types == indices) { return index; }
    }
    throw std::logic_error(std::format("software shader compiler has no kernel for operation {} and types {}, {}, {}", operation, indices[0], indices[1], indices[2]));
}

kernel_operation_t operation(shader::shader_unary_operation_t operation) {
    switch (operation) {
        case shader::shader_unary_operation_t::negate: return kernel_operation_t::negate;
        case shader::shader_unary_operation_t::logical_not: return kernel_operation_t::logical_not;
        case shader::shader_unary_operation_t::absolute: return kernel_operation_t::absolute;
        case shader::shader_unary_operation_t::square_root: return kernel_operation_t::square_root;
        case shader::shader_unary_operation_t::floor: return kernel_operation_t::floor;
        case shader::shader_unary_operation_t::ceil: return kernel_operation_t::ceil;
        case shader::shader_unary_operation_t::fract: return kernel_operation_t::fract;
        case shader::shader_unary_operation_t::sine: return kernel_operation_t::sine;
        case shader::shader_unary_operation_t::cosine: return kernel_operation_t::cosine;
        case shader::shader_unary_operation_t::normalize: return kernel_operation_t::normalize;
        case shader::shader_unary_operation_t::length: return kernel_operation_t::length;
        default: throw std::logic_error("software shader compiler encountered an unsupported operation");
    }
}

kernel_operation_t operation(shader::shader_binary_operation_t operation) {
    switch (operation) {
        case shader::shader_binary_operation_t::add: return kernel_operation_t::add;
        case shader::shader_binary_operation_t::subtract: return kernel_operation_t::subtract;
        case shader::shader_binary_operation_t::multiply: return kernel_operation_t::multiply;
        case shader::shader_binary_operation_t::divide: return kernel_operation_t::divide;
        case shader::shader_binary_operation_t::modulo: return kernel_operation_t::modulo;
        case shader::shader_binary_operation_t::equal: return kernel_operation_t::equal;
        case shader::shader_binary_operation_t::not_equal: return kernel_operation_t::not_equal;
        case shader::shader_binary_operation_t::less: return kernel_operation_t::less;
        case shader::shader_binary_operation_t::less_equal: return kernel_operation_t::less_equal;
        case shader::shader_binary_operation_t::greater: return kernel_operation_t::greater;
        case shader::shader_binary_operation_t::greater_equal: return kernel_operation_t::greater_equal;
        case shader::shader_binary_operation_t::dot: return kernel_operation_t::dot;
        case shader::shader_binary_operation_t::cross: return kernel_operation_t::cross;
        case shader::shader_binary_operation_t::power: return kernel_operation_t::power;
        case shader::shader_binary_operation_t::reflect: return kernel_operation_t::reflect;
        case shader::shader_binary_operation_t::minimum: return kernel_operation_t::minimum;
        case shader::shader_binary_operation_t::maximum: return kernel_operation_t::maximum;
        case shader::shader_binary_operation_t::step: return kernel_operation_t::step;
        default: throw std::logic_error("software shader compiler encountered an unsupported operation");
    }
}

value_t literal_value(const shader::shader_literal_t& literal) {
    return dispatch_value_type<value_t>(literal.type(), [&]<typename T>() -> value_t {
        T result {};
        auto destination = value_components(result);
        if constexpr (std::same_as<scalar_t<T>, bool>) {
            const auto& components = std::get<shader::shader_boolean_components_t>(literal.data()).values;
            for (std::size_t i = 0; i < destination.size(); ++i) { destination[i] = components[i]; }
        } else {
            const auto& components = std::get<std::vector<scalar_t<T>>>(literal.data());
            for (std::size_t i = 0; i < destination.size(); ++i) { destination[i] = components[i]; }
        }
        return result;
    });
}

std::int32_t signed_add(std::int32_t lhs, std::int32_t rhs) {
    const auto result = std::bit_cast<std::uint32_t>(lhs) + std::bit_cast<std::uint32_t>(rhs);
    return std::bit_cast<std::int32_t>(result);
}

std::int32_t signed_subtract(std::int32_t lhs, std::int32_t rhs) {
    const auto result = std::bit_cast<std::uint32_t>(lhs) - std::bit_cast<std::uint32_t>(rhs);
    return std::bit_cast<std::int32_t>(result);
}

std::int32_t signed_multiply(std::int32_t lhs, std::int32_t rhs) {
    const auto result = std::bit_cast<std::uint32_t>(lhs) * std::bit_cast<std::uint32_t>(rhs);
    return std::bit_cast<std::int32_t>(result);
}

std::int32_t signed_negate(std::int32_t value) {
    const auto result = std::uint32_t(0) - std::bit_cast<std::uint32_t>(value);
    return std::bit_cast<std::int32_t>(result);
}

void validate_interface_bindings(const shader::shader_interface_t& interface, const bindings_t& bindings) {
    for (const auto& binding : interface.bindings()) {
        switch (binding.type.category()) {
            case shader::shader_data_category_t::texture_2d: {
                (void)bindings.texture(binding.index);
            } break;
            case shader::shader_data_category_t::sampler: {
                (void)bindings.sampler(binding.index);
            } break;
            default: {
                dispatch_value_type<void>(binding.type, [&]<typename T>() {
                    (void)bindings.uniform<T>(binding.index);
                });
            } break;
        }
    }
}

int binding_namespace(shader::shader_data_type_t type) {
    switch (type.category()) {
        case shader::shader_data_category_t::texture_2d: return 1;
        case shader::shader_data_category_t::sampler: return 2;
        default: return 0;
    }
}

void validate_program_link(const shader::shader_ast_t& vertex, const shader::shader_ast_t& fragment) {
    if (vertex.stage() != shader::shader_stage_t::vertex) {
        throw std::invalid_argument("software shader program requires a vertex AST first");
    }
    if (fragment.stage() != shader::shader_stage_t::fragment) {
        throw std::invalid_argument("software shader program requires a fragment AST second");
    }

    for (const auto& input : fragment.interface().inputs()) {
        const auto outputs = vertex.interface().outputs();
        const auto output = std::ranges::find(outputs, input.index, &shader::shader_interface_element_t::index);
        if (output == outputs.end() || output->type != input.type) {
            throw std::invalid_argument("software shader fragment input has no compatible vertex output");
        }
    }

    std::map<std::tuple<int, std::uint32_t>, shader::shader_data_type_t> binding_types;
    const auto collect = [&binding_types](const shader::shader_interface_t& interface) {
        for (const auto& binding : interface.bindings()) {
            const auto key = std::tuple(binding_namespace(binding.type), binding.index);
            const auto [iterator, inserted] = binding_types.emplace(key, binding.type);
            if (!inserted && iterator->second != binding.type) {
                throw std::invalid_argument("software shader binding has incompatible types across stages");
            }
        }
    };
    collect(vertex.interface());
    collect(fragment.interface());
}

program_t::execution_context_t::execution_context_t() = default;
std::size_t program_t::execution_context_t::slot_capacity() const { return m_slots.capacity(); }
std::size_t program_t::execution_context_t::local_capacity() const { return m_local_initialized.capacity(); }

void program_t::execution_context_t::run(const program_t& program, const bindings_t& bindings, vertex_io_t& io) {
    execute_stage(program.m_vertex, &bindings, io, m_slots, m_local_initialized, nullptr);
}

void program_t::execution_context_t::run(const program_t& program, const bindings_t& bindings, fragment_io_t& io) {
    execute_stage(program.m_fragment, &bindings, io, m_slots, m_local_initialized, nullptr);
}

program_t::execution_context_t::prepared_t::prepared_t() = default;

program_t::execution_context_t::prepared_t& program_t::execution_context_t::prepared_t::operator=(const prepared_t& other) {
    if (this != &other) {
        prepared_t copy(other);
        *this = std::move(copy);
    }
    return *this;
}

program_t::execution_context_t::prepared_t::prepared_t(prepared_t&& other) noexcept:
    m_program(std::exchange(other.m_program, nullptr)),
    m_vertex_bindings(std::move(other.m_vertex_bindings)),
    m_fragment_bindings(std::move(other.m_fragment_bindings))
{
}

program_t::execution_context_t::prepared_t& program_t::execution_context_t::prepared_t::operator=(prepared_t&& other) noexcept {
    if (this != &other) {
        m_program = std::exchange(other.m_program, nullptr);
        m_vertex_bindings = std::move(other.m_vertex_bindings);
        m_fragment_bindings = std::move(other.m_fragment_bindings);
    }
    return *this;
}

void program_t::execution_context_t::prepared_t::reset() {
    m_program = nullptr;
    m_vertex_bindings.clear();
    m_fragment_bindings.clear();
}

void program_t::execution_context_t::prepared_t::run(std::span<const value_t> inputs, std::span<std::optional<value_t>> outputs, vertex_io_t& io, execution_context_t& context) const {
    if (!m_program) {
        io.clear_results();
        std::ranges::fill(outputs, std::nullopt);
        throw std::logic_error("prepared shader run requires successful preparation");
    }
    const prepared_invocation_t invocation {m_vertex_bindings, inputs, outputs};
    execute_stage(m_program->m_vertex, nullptr, io, context.m_slots, context.m_local_initialized, &invocation);
}

void program_t::execution_context_t::prepared_t::run(std::span<const value_t> inputs, std::span<std::optional<value_t>> outputs, fragment_io_t& io, execution_context_t& context) const {
    if (!m_program) {
        io.clear_results();
        std::ranges::fill(outputs, std::nullopt);
        throw std::logic_error("prepared shader run requires successful preparation");
    }
    const prepared_invocation_t invocation {m_fragment_bindings, inputs, outputs};
    execute_stage(m_program->m_fragment, nullptr, io, context.m_slots, context.m_local_initialized, &invocation);
}

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader
