#include "software_shader.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

namespace {

shader::matrix_t<float, 4, 4> identity_matrix() {
    return {
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    };
}

class value_t {
public:
    explicit value_t(const shader::shader_literal_t& literal):
        m_type(literal.type()),
        m_data(literal.data())
    {
    }

    value_t(shader::shader_data_type_t type, shader::shader_literal_data_t data):
        m_type(type),
        m_data(std::move(data))
    {
    }

    shader::shader_data_type_t type() const {
        return m_type;
    }

    const shader::shader_literal_data_t& data() const {
        return m_data;
    }

private:
    shader::shader_data_type_t m_type;
    shader::shader_literal_data_t m_data;
};

template <typename T>
using component_t = std::conditional_t<std::same_as<T, bool>, std::uint8_t, T>;

template <typename T>
const std::vector<T>& components(const value_t& value) {
    if constexpr (std::same_as<T, std::uint8_t>) {
        return std::get<shader::shader_boolean_components_t>(value.data()).values;
    } else {
        return std::get<std::vector<T>>(value.data());
    }
}

template <typename T>
value_t make_value(shader::shader_data_type_t type, std::vector<T> values) {
    if constexpr (std::same_as<T, std::uint8_t>) {
        return value_t(type, shader::shader_boolean_components_t{std::move(values)});
    } else {
        return value_t(type, std::move(values));
    }
}

template <shader::shader_value T>
value_t make_value(T value) {
    return value_t(shader::shader_literal_t(std::move(value)));
}

template <shader::shader_value T>
std::remove_cvref_t<T> typed_value(const value_t& value) {
    using type = std::remove_cvref_t<T>;
    using scalar_type = typename shader::shader_type_traits_t<type>::scalar_type;
    using stored_type = component_t<scalar_type>;

    if (value.type() != shader::shader_data_type<type>()) {
        throw std::logic_error("software shader runtime value has the wrong type");
    }

    const auto& data = components<stored_type>(value);
    if constexpr (shader::shader_type_traits_t<type>::scalar) {
        return static_cast<type>(data[0]);
    } else {
        type result;
        auto output = result.begin();
        for (const auto component : data) {
            *output = static_cast<scalar_type>(component);
            ++output;
        }
        return result;
    }
}

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

template <typename T, typename Operation>
value_t unary_components(shader::shader_data_type_t result_type, const value_t& input, Operation operation) {
    const auto& source = components<T>(input);
    std::vector<T> result;
    result.reserve(source.size());
    for (const auto component : source) {
        result.push_back(operation(component));
    }
    return make_value(result_type, std::move(result));
}

template <typename T, typename Operation>
value_t binary_components(
    shader::shader_data_type_t result_type,
    const value_t& lhs,
    const value_t& rhs,
    Operation operation
) {
    const auto& left = components<T>(lhs);
    const auto& right = components<T>(rhs);
    std::vector<T> result;
    result.reserve(left.size());
    for (std::size_t i = 0; i < left.size(); ++i) {
        result.push_back(operation(left[i], right.size() == 1 ? right[0] : right[i]));
    }
    return make_value(result_type, std::move(result));
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
value_t arithmetic_components(
    shader::shader_binary_operation_t operation,
    shader::shader_data_type_t result_type,
    const value_t& lhs,
    const value_t& rhs
) {
    switch (operation) {
        case shader::shader_binary_operation_t::add:
            return binary_components<T>(result_type, lhs, rhs, [](T left, T right) {
                if constexpr (std::same_as<T, std::int32_t>) {
                    return signed_add(left, right);
                } else {
                    return static_cast<T>(left + right);
                }
            });
        case shader::shader_binary_operation_t::subtract:
            return binary_components<T>(result_type, lhs, rhs, [](T left, T right) {
                if constexpr (std::same_as<T, std::int32_t>) {
                    return signed_subtract(left, right);
                } else {
                    return static_cast<T>(left - right);
                }
            });
        case shader::shader_binary_operation_t::multiply:
            return binary_components<T>(result_type, lhs, rhs, [](T left, T right) {
                if constexpr (std::same_as<T, std::int32_t>) {
                    return signed_multiply(left, right);
                } else {
                    return static_cast<T>(left * right);
                }
            });
        case shader::shader_binary_operation_t::divide:
            return binary_components<T>(result_type, lhs, rhs, [](T left, T right) { return divide(left, right); });
        case shader::shader_binary_operation_t::modulo:
            return binary_components<T>(result_type, lhs, rhs, [](T left, T right) { return modulo(left, right); });
        case shader::shader_binary_operation_t::minimum:
            return binary_components<T>(result_type, lhs, rhs, [](T left, T right) { return std::min(left, right); });
        case shader::shader_binary_operation_t::maximum:
            return binary_components<T>(result_type, lhs, rhs, [](T left, T right) { return std::max(left, right); });
        default: throw std::logic_error("software shader selected a non-arithmetic operation");
    }
}

value_t arithmetic(
    shader::shader_binary_operation_t operation,
    shader::shader_data_type_t result_type,
    const value_t& lhs,
    const value_t& rhs
) {
    switch (lhs.type().scalar()) {
        case shader::shader_scalar_type_t::signed_integer:
            return arithmetic_components<std::int32_t>(operation, result_type, lhs, rhs);
        case shader::shader_scalar_type_t::unsigned_integer:
            return arithmetic_components<std::uint32_t>(operation, result_type, lhs, rhs);
        case shader::shader_scalar_type_t::floating_point:
            return arithmetic_components<float>(operation, result_type, lhs, rhs);
        default: throw std::logic_error("software shader arithmetic used a non-numeric value");
    }
}

value_t multiply_matrix(
    shader::shader_data_type_t result_type,
    const value_t& lhs,
    const value_t& rhs
) {
    const auto& left = components<float>(lhs);
    const auto& right = components<float>(rhs);
    const std::size_t rows = lhs.type().rows();
    const std::size_t inner = lhs.type().columns();

    if (rhs.type().category() == shader::shader_data_category_t::vector) {
        std::vector<float> result(rows, 0.0F);
        for (std::size_t row = 0; row < rows; ++row) {
            for (std::size_t i = 0; i < inner; ++i) {
                result[row] += left[row * inner + i] * right[i];
            }
        }
        return make_value(result_type, std::move(result));
    }

    const std::size_t columns = rhs.type().columns();
    std::vector<float> result(rows * columns, 0.0F);
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            for (std::size_t i = 0; i < inner; ++i) {
                result[row * columns + column] += left[row * inner + i] * right[i * columns + column];
            }
        }
    }
    return make_value(result_type, std::move(result));
}

template <typename T>
bool equal_components(const value_t& lhs, const value_t& rhs) {
    return components<T>(lhs) == components<T>(rhs);
}

bool equal(const value_t& lhs, const value_t& rhs) {
    switch (lhs.type().scalar()) {
        case shader::shader_scalar_type_t::boolean: return equal_components<std::uint8_t>(lhs, rhs);
        case shader::shader_scalar_type_t::signed_integer: return equal_components<std::int32_t>(lhs, rhs);
        case shader::shader_scalar_type_t::unsigned_integer: return equal_components<std::uint32_t>(lhs, rhs);
        case shader::shader_scalar_type_t::floating_point: return equal_components<float>(lhs, rhs);
        default: throw std::logic_error("software shader equality used an unsupported value");
    }
}

template <typename T>
bool compare_components(shader::shader_binary_operation_t operation, const value_t& lhs, const value_t& rhs) {
    const auto left = components<T>(lhs)[0];
    const auto right = components<T>(rhs)[0];
    switch (operation) {
        case shader::shader_binary_operation_t::less: return left < right;
        case shader::shader_binary_operation_t::less_equal: return left <= right;
        case shader::shader_binary_operation_t::greater: return right < left;
        case shader::shader_binary_operation_t::greater_equal: return right <= left;
        default: throw std::logic_error("software shader selected a non-comparison operation");
    }
}

bool compare(shader::shader_binary_operation_t operation, const value_t& lhs, const value_t& rhs) {
    switch (lhs.type().scalar()) {
        case shader::shader_scalar_type_t::signed_integer: return compare_components<std::int32_t>(operation, lhs, rhs);
        case shader::shader_scalar_type_t::unsigned_integer: return compare_components<std::uint32_t>(operation, lhs, rhs);
        case shader::shader_scalar_type_t::floating_point: return compare_components<float>(operation, lhs, rhs);
        default: throw std::logic_error("software shader comparison used a non-numeric value");
    }
}

value_t floating_binary(
    shader::shader_binary_operation_t operation,
    shader::shader_data_type_t result_type,
    const value_t& lhs,
    const value_t& rhs
) {
    switch (operation) {
        case shader::shader_binary_operation_t::power:
            return binary_components<float>(result_type, lhs, rhs, [](float left, float right) { return std::pow(left, right); });
        case shader::shader_binary_operation_t::step:
            return binary_components<float>(result_type, rhs, lhs, [](float expression, float edge) { return expression < edge ? 0.0F : 1.0F; });
        case shader::shader_binary_operation_t::reflect: {
            const auto& incident = components<float>(lhs);
            const auto& normal = components<float>(rhs);
            float projection = 0.0F;
            for (std::size_t i = 0; i < incident.size(); ++i) {
                projection += normal[i] * incident[i];
            }
            std::vector<float> result;
            result.reserve(incident.size());
            for (std::size_t i = 0; i < incident.size(); ++i) {
                result.push_back(incident[i] - 2.0F * projection * normal[i]);
            }
            return make_value(result_type, std::move(result));
        }
        default: throw std::logic_error("software shader selected an unsupported floating operation");
    }
}

value_t clamp_value(
    shader::shader_data_type_t result_type,
    const value_t& expression,
    const value_t& minimum,
    const value_t& maximum
) {
    switch (expression.type().scalar()) {
        case shader::shader_scalar_type_t::signed_integer:
            return binary_components<std::int32_t>(
                result_type,
                binary_components<std::int32_t>(result_type, expression, minimum, [](auto value, auto lower) { return std::max(value, lower); }),
                maximum,
                [](auto value, auto upper) { return std::min(value, upper); }
            );
        case shader::shader_scalar_type_t::unsigned_integer:
            return binary_components<std::uint32_t>(
                result_type,
                binary_components<std::uint32_t>(result_type, expression, minimum, [](auto value, auto lower) { return std::max(value, lower); }),
                maximum,
                [](auto value, auto upper) { return std::min(value, upper); }
            );
        case shader::shader_scalar_type_t::floating_point:
            return binary_components<float>(
                result_type,
                binary_components<float>(result_type, expression, minimum, [](auto value, auto lower) { return std::max(value, lower); }),
                maximum,
                [](auto value, auto upper) { return std::min(value, upper); }
            );
        default: throw std::logic_error("software shader clamp used a non-numeric value");
    }
}

value_t mix_value(
    shader::shader_data_type_t result_type,
    const value_t& lhs,
    const value_t& rhs,
    const value_t& factor
) {
    const float amount = components<float>(factor)[0];
    return binary_components<float>(result_type, lhs, rhs, [amount](float left, float right) {
        return left * (1.0F - amount) + right * amount;
    });
}

value_t smoothstep_value(
    shader::shader_data_type_t result_type,
    const value_t& edge0,
    const value_t& edge1,
    const value_t& expression
) {
    const auto& lower = components<float>(edge0);
    const auto& upper = components<float>(edge1);
    const auto& source = components<float>(expression);
    std::vector<float> result;
    result.reserve(source.size());
    for (std::size_t i = 0; i < source.size(); ++i) {
        const float minimum = lower.size() == 1 ? lower[0] : lower[i];
        const float maximum = upper.size() == 1 ? upper[0] : upper[i];
        const float factor = std::clamp((source[i] - minimum) / (maximum - minimum), 0.0F, 1.0F);
        result.push_back(factor * factor * (3.0F - 2.0F * factor));
    }
    return make_value(result_type, std::move(result));
}

using evaluation_t = std::variant<value_t, const texture::texture_t*, const texture::sampler_t*>;

value_t value(evaluation_t evaluation) {
    if (auto* result = std::get_if<value_t>(&evaluation)) {
        return std::move(*result);
    }
    throw std::logic_error("software shader expected a value expression");
}

const texture::texture_t& texture_value(const evaluation_t& evaluation) {
    const auto* resource = std::get_if<const texture::texture_t*>(&evaluation);
    if (!resource || !*resource) {
        throw std::logic_error("software shader expected a texture expression");
    }
    return **resource;
}

const texture::sampler_t& sampler_value(const evaluation_t& evaluation) {
    const auto* resource = std::get_if<const texture::sampler_t*>(&evaluation);
    if (!resource || !*resource) {
        throw std::logic_error("software shader expected a sampler expression");
    }
    return **resource;
}

enum class flow_t { none, break_loop, continue_loop, discard };

class interpreter_t final : public shader::shader_ast_visitor_t {
public:
    interpreter_t(const bindings_t& bindings, vertex_io_t& io):
        m_bindings(bindings),
        m_vertex_io(&io),
        m_fragment_io(nullptr)
    {
    }

    interpreter_t(const bindings_t& bindings, fragment_io_t& io):
        m_bindings(bindings),
        m_vertex_io(nullptr),
        m_fragment_io(&io)
    {
    }

    void execute(const shader::shader_block_t& block) {
        execute_block(block);
        if (m_flow == flow_t::break_loop || m_flow == flow_t::continue_loop) {
            throw std::logic_error("software shader loop control escaped its loop");
        }
    }

    void visit(const shader::shader_constant_node_t& node) override {
        m_evaluation = value_t(node.value());
    }

    void visit(const shader::shader_input_node_t& node) override {
        if (m_vertex_io) {
            m_evaluation = dispatch_value_type<value_t>(node.type(), [&]<typename T>() {
                return make_value(m_vertex_io->input<T>(node.location()));
            });
        } else {
            m_evaluation = dispatch_value_type<value_t>(node.type(), [&]<typename T>() {
                return make_value(m_fragment_io->input<T>(node.location()));
            });
        }
    }

    void visit(const shader::shader_uniform_node_t& node) override {
        m_evaluation = dispatch_value_type<value_t>(node.type(), [&]<typename T>() {
            return make_value(m_bindings.uniform<T>(node.binding()));
        });
    }

    void visit(const shader::shader_resource_node_t& node) override {
        if (node.type().category() == shader::shader_data_category_t::texture_2d) {
            m_evaluation = &m_bindings.texture(node.binding());
        } else if (node.type().category() == shader::shader_data_category_t::sampler) {
            m_evaluation = &m_bindings.sampler(node.binding());
        } else {
            throw std::logic_error("software shader resource has an unsupported type");
        }
    }

    void visit(const shader::shader_builtin_node_t& node) override {
        switch (node.builtin()) {
            case shader::shader_builtin_t::vertex_index: {
                m_evaluation = make_value(m_vertex_io->vertex_index());
            } break;
            case shader::shader_builtin_t::instance_index: {
                m_evaluation = make_value(m_vertex_io->instance_index());
            } break;
            case shader::shader_builtin_t::object_to_world: {
                m_evaluation = make_value(m_vertex_io->object_to_world());
            } break;
            case shader::shader_builtin_t::world_to_clip: {
                m_evaluation = make_value(m_vertex_io->world_to_clip());
            } break;
            case shader::shader_builtin_t::fragment_coordinate: {
                m_evaluation = make_value(m_fragment_io->fragment_coordinate());
            } break;
            case shader::shader_builtin_t::front_facing: {
                m_evaluation = make_value(m_fragment_io->front_facing());
            } break;
            default: {
                throw std::logic_error("software shader encountered an unsupported builtin");
            }
        }
    }

    void visit(const shader::shader_local_node_t& node) override {
        const auto iterator = m_locals.find(&node);
        if (iterator == m_locals.end()) {
            throw std::logic_error("software shader read an uninitialized local");
        }
        m_evaluation = iterator->second;
    }

    void visit(const shader::shader_unary_node_t& node) override {
        const auto input = value(evaluate(node.expression()));
        switch (node.operation()) {
            case shader::shader_unary_operation_t::negate:
                switch (input.type().scalar()) {
                    case shader::shader_scalar_type_t::signed_integer:
                        m_evaluation = unary_components<std::int32_t>(node.type(), input, signed_negate);
                        break;
                    case shader::shader_scalar_type_t::unsigned_integer:
                        m_evaluation = unary_components<std::uint32_t>(node.type(), input, [](std::uint32_t component) {
                            return std::uint32_t(0) - component;
                        });
                        break;
                    case shader::shader_scalar_type_t::floating_point:
                        m_evaluation = unary_components<float>(node.type(), input, [](float component) { return -component; });
                        break;
                    default: throw std::logic_error("software shader negated an unsupported value");
                }
                break;
            case shader::shader_unary_operation_t::logical_not:
                m_evaluation = make_value(!typed_value<bool>(input));
                break;
            case shader::shader_unary_operation_t::absolute:
                if (input.type().scalar() == shader::shader_scalar_type_t::signed_integer) {
                    m_evaluation = unary_components<std::int32_t>(node.type(), input, [](std::int32_t component) {
                        return component < 0 ? signed_negate(component) : component;
                    });
                } else {
                    m_evaluation = unary_components<float>(node.type(), input, [](float component) { return std::abs(component); });
                }
                break;
            case shader::shader_unary_operation_t::square_root:
                m_evaluation = unary_components<float>(node.type(), input, [](float component) { return std::sqrt(component); });
                break;
            case shader::shader_unary_operation_t::floor:
                m_evaluation = unary_components<float>(node.type(), input, [](float component) { return std::floor(component); });
                break;
            case shader::shader_unary_operation_t::ceil:
                m_evaluation = unary_components<float>(node.type(), input, [](float component) { return std::ceil(component); });
                break;
            case shader::shader_unary_operation_t::fract:
                m_evaluation = unary_components<float>(node.type(), input, [](float component) { return component - std::floor(component); });
                break;
            case shader::shader_unary_operation_t::sine:
                m_evaluation = unary_components<float>(node.type(), input, [](float component) { return std::sin(component); });
                break;
            case shader::shader_unary_operation_t::cosine:
                m_evaluation = unary_components<float>(node.type(), input, [](float component) { return std::cos(component); });
                break;
            case shader::shader_unary_operation_t::normalize: {
                const auto& source = components<float>(input);
                float squared_length = 0.0F;
                for (const auto component : source) {
                    squared_length += component * component;
                }
                const float length = std::sqrt(squared_length);
                m_evaluation = unary_components<float>(node.type(), input, [length](float component) { return component / length; });
                break;
            }
            case shader::shader_unary_operation_t::length: {
                float squared_length = 0.0F;
                for (const auto component : components<float>(input)) {
                    squared_length += component * component;
                }
                m_evaluation = make_value(std::sqrt(squared_length));
                break;
            }
            default: throw std::logic_error("software shader encountered an unsupported unary operation");
        }
    }

    void visit(const shader::shader_binary_node_t& node) override {
        auto lhs = value(evaluate(node.lhs()));

        if (node.operation() == shader::shader_binary_operation_t::logical_and && !typed_value<bool>(lhs)) {
            m_evaluation = make_value(false);
            return;
        }
        if (node.operation() == shader::shader_binary_operation_t::logical_or && typed_value<bool>(lhs)) {
            m_evaluation = make_value(true);
            return;
        }

        const auto rhs = value(evaluate(node.rhs()));
        switch (node.operation()) {
            case shader::shader_binary_operation_t::add:
            case shader::shader_binary_operation_t::subtract:
            case shader::shader_binary_operation_t::divide:
            case shader::shader_binary_operation_t::modulo:
            case shader::shader_binary_operation_t::minimum:
            case shader::shader_binary_operation_t::maximum:
                m_evaluation = arithmetic(node.operation(), node.type(), lhs, rhs);
                break;
            case shader::shader_binary_operation_t::multiply:
                if (lhs.type().category() == shader::shader_data_category_t::matrix &&
                    rhs.type().category() != shader::shader_data_category_t::scalar) {
                    m_evaluation = multiply_matrix(node.type(), lhs, rhs);
                } else {
                    m_evaluation = arithmetic(node.operation(), node.type(), lhs, rhs);
                }
                break;
            case shader::shader_binary_operation_t::equal:
                m_evaluation = make_value(equal(lhs, rhs));
                break;
            case shader::shader_binary_operation_t::not_equal:
                m_evaluation = make_value(!equal(lhs, rhs));
                break;
            case shader::shader_binary_operation_t::less:
            case shader::shader_binary_operation_t::less_equal:
            case shader::shader_binary_operation_t::greater:
            case shader::shader_binary_operation_t::greater_equal:
                m_evaluation = make_value(compare(node.operation(), lhs, rhs));
                break;
            case shader::shader_binary_operation_t::logical_and:
                m_evaluation = make_value(typed_value<bool>(rhs));
                break;
            case shader::shader_binary_operation_t::logical_or:
                m_evaluation = make_value(typed_value<bool>(rhs));
                break;
            case shader::shader_binary_operation_t::dot: {
                const auto& left = components<float>(lhs);
                const auto& right = components<float>(rhs);
                float result = 0.0F;
                for (std::size_t i = 0; i < left.size(); ++i) {
                    result += left[i] * right[i];
                }
                m_evaluation = make_value(result);
                break;
            }
            case shader::shader_binary_operation_t::cross: {
                const auto& left = components<float>(lhs);
                const auto& right = components<float>(rhs);
                m_evaluation = make_value(node.type(), std::vector<float>{
                    left[1] * right[2] - left[2] * right[1],
                    left[2] * right[0] - left[0] * right[2],
                    left[0] * right[1] - left[1] * right[0]
                });
                break;
            }
            case shader::shader_binary_operation_t::power:
            case shader::shader_binary_operation_t::reflect:
            case shader::shader_binary_operation_t::step:
                m_evaluation = floating_binary(node.operation(), node.type(), lhs, rhs);
                break;
            default: throw std::logic_error("software shader encountered an unsupported binary operation");
        }
    }

    void visit(const shader::shader_construct_node_t& node) override {
        switch (node.type().scalar()) {
            case shader::shader_scalar_type_t::boolean:
                m_evaluation = construct<std::uint8_t>(node);
                break;
            case shader::shader_scalar_type_t::signed_integer:
                m_evaluation = construct<std::int32_t>(node);
                break;
            case shader::shader_scalar_type_t::unsigned_integer:
                m_evaluation = construct<std::uint32_t>(node);
                break;
            case shader::shader_scalar_type_t::floating_point:
                m_evaluation = construct<float>(node);
                break;
            default: throw std::logic_error("software shader constructed an unsupported value");
        }
    }

    void visit(const shader::shader_swizzle_node_t& node) override {
        const auto input = value(evaluate(node.expression()));
        switch (node.type().scalar()) {
            case shader::shader_scalar_type_t::boolean:
                m_evaluation = swizzle<std::uint8_t>(node, input);
                break;
            case shader::shader_scalar_type_t::signed_integer:
                m_evaluation = swizzle<std::int32_t>(node, input);
                break;
            case shader::shader_scalar_type_t::unsigned_integer:
                m_evaluation = swizzle<std::uint32_t>(node, input);
                break;
            case shader::shader_scalar_type_t::floating_point:
                m_evaluation = swizzle<float>(node, input);
                break;
            default: throw std::logic_error("software shader swizzled an unsupported value");
        }
    }

    void visit(const shader::shader_call_node_t& node) override {
        const auto arguments = node.operands();
        switch (node.operation()) {
            case shader::shader_call_operation_t::clamp:
                m_evaluation = clamp_value(
                    node.type(),
                    value(evaluate(*arguments[0])),
                    value(evaluate(*arguments[1])),
                    value(evaluate(*arguments[2]))
                );
                break;
            case shader::shader_call_operation_t::mix:
                m_evaluation = mix_value(
                    node.type(),
                    value(evaluate(*arguments[0])),
                    value(evaluate(*arguments[1])),
                    value(evaluate(*arguments[2]))
                );
                break;
            case shader::shader_call_operation_t::smoothstep:
                m_evaluation = smoothstep_value(
                    node.type(),
                    value(evaluate(*arguments[0])),
                    value(evaluate(*arguments[1])),
                    value(evaluate(*arguments[2]))
                );
                break;
            case shader::shader_call_operation_t::sample:
            case shader::shader_call_operation_t::sample_lod: {
                const auto texture_resource = evaluate(*arguments[0]);
                const auto sampler_resource = evaluate(*arguments[1]);
                const auto coordinates = value(evaluate(*arguments[2]));
                if (node.operation() == shader::shader_call_operation_t::sample) {
                    m_evaluation = make_value(texture::sample(
                        texture_value(texture_resource), sampler_value(sampler_resource),
                        typed_value<shader::vector_t<float, 2>>(coordinates)
                    ));
                } else {
                    const float lod = typed_value<float>(value(evaluate(*arguments[3])));
                    m_evaluation = make_value(texture::sample_lod(
                        texture_value(texture_resource), sampler_value(sampler_resource),
                        typed_value<shader::vector_t<float, 2>>(coordinates), lod
                    ));
                }
                break;
            }
            default: throw std::logic_error("software shader encountered an unsupported call operation");
        }
    }

    void visit(const shader::shader_local_statement_t& statement) override {
        m_locals.insert_or_assign(statement.local_node(), value(evaluate(statement.initial())));
    }

    void visit(const shader::shader_assignment_statement_t& statement) override {
        const auto iterator = m_locals.find(statement.local_node());
        if (iterator == m_locals.end()) {
            throw std::logic_error("software shader assigned an uninitialized local");
        }
        iterator->second = value(evaluate(statement.value()));
    }

    void visit(const shader::shader_output_statement_t& statement) override {
        const auto result = value(evaluate(statement.expression()));
        if (statement.output() == shader::shader_output_t::position) {
            m_vertex_io->position(typed_value<shader::vector_t<float, 4>>(result));
            return;
        }
        if (statement.output() == shader::shader_output_t::color) {
            m_fragment_io->color(typed_value<shader::vector_t<float, 4>>(result));
            return;
        }
        if (m_vertex_io) {
            dispatch_value_type<void>(result.type(), [&]<typename T>() {
                m_vertex_io->output(statement.location(), typed_value<T>(result));
            });
        } else {
            dispatch_value_type<void>(result.type(), [&]<typename T>() {
                m_fragment_io->output(statement.location(), typed_value<T>(result));
            });
        }
    }

    void visit(const shader::shader_branch_statement_t& statement) override {
        if (typed_value<bool>(value(evaluate(statement.condition())))) {
            execute_block(statement.true_block());
        } else {
            execute_block(statement.false_block());
        }
    }

    void visit(const shader::shader_loop_statement_t& statement) override {
        while (typed_value<bool>(value(evaluate(statement.condition())))) {
            execute_block(statement.body());
            if (m_flow == flow_t::discard) {
                return;
            }
            if (m_flow == flow_t::break_loop) {
                m_flow = flow_t::none;
                return;
            }
            if (m_flow == flow_t::continue_loop) {
                m_flow = flow_t::none;
            }
        }
    }

    void visit(const shader::shader_break_statement_t&) override {
        m_flow = flow_t::break_loop;
    }

    void visit(const shader::shader_continue_statement_t&) override {
        m_flow = flow_t::continue_loop;
    }

    void visit(const shader::shader_discard_statement_t&) override {
        m_fragment_io->discard();
        m_flow = flow_t::discard;
    }

private:
    evaluation_t evaluate(const shader::shader_expression_node_t& expression) {
        m_evaluation.reset();
        expression.accept(*this);
        if (!m_evaluation) {
            throw std::logic_error("software shader expression produced no value");
        }
        auto result = std::move(*m_evaluation);
        m_evaluation.reset();
        return result;
    }

    void execute_block(const shader::shader_block_t& block) {
        for (const auto& statement : block.statements) {
            statement->accept(*this);
            if (m_flow != flow_t::none) {
                return;
            }
        }
    }

    template <typename T>
    value_t construct(const shader::shader_construct_node_t& node) {
        std::vector<T> result;
        for (const auto* operand : node.operands()) {
            const auto argument = value(evaluate(*operand));
            const auto& data = components<T>(argument);
            result.insert(result.end(), data.begin(), data.end());
        }
        return make_value(node.type(), std::move(result));
    }

    template <typename T>
    value_t swizzle(const shader::shader_swizzle_node_t& node, const value_t& input) {
        const auto& source = components<T>(input);
        std::vector<T> result;
        result.reserve(node.components().size());
        for (const auto component : node.components()) {
            result.push_back(source[component]);
        }
        return make_value(node.type(), std::move(result));
    }

    const bindings_t& m_bindings;
    vertex_io_t* m_vertex_io;
    fragment_io_t* m_fragment_io;
    std::map<const shader::shader_local_node_t*, value_t> m_locals;
    std::optional<evaluation_t> m_evaluation;
    flow_t m_flow = flow_t::none;
};

template <typename IO>
void validate_inputs(const shader::shader_interface_t& interface, const IO& io) {
    for (const auto& input : interface.inputs()) {
        dispatch_value_type<void>(input.type, [&]<typename T>() {
            (void)io.template input<T>(input.index);
        });
    }
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

} // namespace

void bindings_t::texture(std::uint32_t binding, const texture::texture_t& texture) {
    m_textures.insert_or_assign(binding, std::cref(texture));
}

const texture::texture_t& bindings_t::texture(std::uint32_t binding) const {
    const auto iterator = m_textures.find(binding);
    if (iterator == m_textures.end()) {
        throw std::invalid_argument(std::format("software shader texture binding {} is missing", binding));
    }
    return iterator->second.get();
}

void bindings_t::clear_texture(std::uint32_t binding) {
    m_textures.erase(binding);
}

void bindings_t::sampler(std::uint32_t binding, const texture::sampler_t& sampler) {
    m_samplers.insert_or_assign(binding, std::cref(sampler));
}

const texture::sampler_t& bindings_t::sampler(std::uint32_t binding) const {
    const auto iterator = m_samplers.find(binding);
    if (iterator == m_samplers.end()) {
        throw std::invalid_argument(std::format("software shader sampler binding {} is missing", binding));
    }
    return iterator->second.get();
}

void bindings_t::clear_sampler(std::uint32_t binding) {
    m_samplers.erase(binding);
}

vertex_io_t::vertex_io_t(std::int32_t vertex_index, std::int32_t instance_index):
    m_vertex_index(vertex_index),
    m_instance_index(instance_index),
    m_object_to_world(identity_matrix()),
    m_world_to_clip(identity_matrix())
{
}

void vertex_io_t::reset(std::int32_t vertex_index, std::int32_t instance_index) {
    m_vertex_index = vertex_index;
    m_instance_index = instance_index;
    m_inputs.clear();
    clear_results();
}

std::int32_t vertex_io_t::vertex_index() const {
    return m_vertex_index;
}

std::int32_t vertex_io_t::instance_index() const {
    return m_instance_index;
}

void vertex_io_t::object_to_world(shader::matrix_t<float, 4, 4> matrix) {
    m_object_to_world = std::move(matrix);
}

shader::matrix_t<float, 4, 4> vertex_io_t::object_to_world() const {
    return m_object_to_world;
}

void vertex_io_t::world_to_clip(shader::matrix_t<float, 4, 4> matrix) {
    m_world_to_clip = std::move(matrix);
}

shader::matrix_t<float, 4, 4> vertex_io_t::world_to_clip() const {
    return m_world_to_clip;
}

void vertex_io_t::position(shader::vector_t<float, 4> position) {
    m_position = std::move(position);
}

shader::vector_t<float, 4> vertex_io_t::position() const {
    if (!m_position) {
        throw std::logic_error("software shader vertex position is unavailable");
    }
    return *m_position;
}

void vertex_io_t::clear_results() {
    m_outputs.clear();
    m_position.reset();
}

fragment_io_t::fragment_io_t(shader::vector_t<float, 4> fragment_coordinate, bool front_facing):
    m_fragment_coordinate(std::move(fragment_coordinate)),
    m_front_facing(front_facing),
    m_discarded(false)
{
}

void fragment_io_t::reset(shader::vector_t<float, 4> fragment_coordinate, bool front_facing) {
    m_fragment_coordinate = std::move(fragment_coordinate);
    m_front_facing = front_facing;
    m_inputs.clear();
    clear_results();
}

shader::vector_t<float, 4> fragment_io_t::fragment_coordinate() const {
    return m_fragment_coordinate;
}

bool fragment_io_t::front_facing() const {
    return m_front_facing;
}

void fragment_io_t::color(shader::vector_t<float, 4> color) {
    m_color = std::move(color);
}

std::optional<shader::vector_t<float, 4>> fragment_io_t::color() const {
    return m_color;
}

bool fragment_io_t::discarded() const {
    return m_discarded;
}

void fragment_io_t::discard() {
    m_outputs.clear();
    m_color.reset();
    m_discarded = true;
}

void fragment_io_t::clear_results() {
    m_outputs.clear();
    m_color.reset();
    m_discarded = false;
}

program_t::program_t(shader::shader_ast_t vertex, shader::shader_ast_t fragment):
    m_vertex(std::move(vertex)),
    m_fragment(std::move(fragment))
{
    validate_program_link(m_vertex, m_fragment);
}

const shader::shader_interface_t& program_t::vertex_interface() const {
    return m_vertex.interface();
}

const shader::shader_interface_t& program_t::fragment_interface() const {
    return m_fragment.interface();
}

void program_t::validate_bindings(const bindings_t& bindings) const {
    validate_interface_bindings(m_vertex.interface(), bindings);
    validate_interface_bindings(m_fragment.interface(), bindings);
}

void program_t::run(const bindings_t& bindings, vertex_io_t& io) const {
    io.clear_results();
    try {
        validate_inputs(m_vertex.interface(), io);
        validate_interface_bindings(m_vertex.interface(), bindings);
        interpreter_t(bindings, io).execute(m_vertex.root());
        try {
            (void)io.position();
        } catch (const std::logic_error&) {
            throw std::runtime_error("software shader vertex invocation completed without writing position");
        }
    } catch (...) {
        io.clear_results();
        throw;
    }
}

void program_t::run(const bindings_t& bindings, fragment_io_t& io) const {
    io.clear_results();
    try {
        validate_inputs(m_fragment.interface(), io);
        validate_interface_bindings(m_fragment.interface(), bindings);
        interpreter_t(bindings, io).execute(m_fragment.root());
    } catch (...) {
        io.clear_results();
        throw;
    }
}

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader
