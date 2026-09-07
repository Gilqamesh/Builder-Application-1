#ifndef M03GSY25J4V7NCCGMSDOV9IOFT_SHADER_SHADER_BUILDER_H
# define M03GSY25J4V7NCCGMSDOV9IOFT_SHADER_SHADER_BUILDER_H

# include "shader_expression.h"

# include <concepts>
# include <cstdint>
# include <functional>
# include <format>
# include <memory>
# include <span>
# include <stdexcept>
# include <type_traits>
# include <utility>
# include <vector>

namespace m03gsy25j4v7nccgmsdov9ioft_shader {

enum class shader_stage_t { vertex, fragment };
enum class shader_builtin_t { vertex_index, instance_index, object_to_world, world_to_clip, fragment_coordinate, front_facing };
enum class shader_output_t { location, position, color };

/** @brief Selects fragment-input interpolation; other interface elements use perspective. */
enum class interpolation_t { perspective, noperspective, flat };

struct shader_interface_element_t {
    std::uint32_t index;
    shader_data_type_t type;
    interpolation_t interpolation = interpolation_t::perspective;
};

class shader_interface_t {
public:
    shader_interface_t(
        shader_stage_t stage,
        std::vector<shader_interface_element_t> inputs,
        std::vector<shader_interface_element_t> outputs,
        std::vector<shader_interface_element_t> bindings
    );

    shader_stage_t stage() const;
    std::span<const shader_interface_element_t> inputs() const;
    std::span<const shader_interface_element_t> outputs() const;
    std::span<const shader_interface_element_t> bindings() const;

private:
    shader_stage_t m_stage;
    std::vector<shader_interface_element_t> m_inputs;
    std::vector<shader_interface_element_t> m_outputs;
    std::vector<shader_interface_element_t> m_bindings;
};

class shader_ast_visitor_t;
class shader_constant_node_t;
class shader_input_node_t;
class shader_uniform_node_t;
class shader_resource_node_t;
class shader_builtin_node_t;
class shader_local_node_t;
class shader_unary_node_t;
class shader_binary_node_t;
class shader_construct_node_t;
class shader_swizzle_node_t;
class shader_call_node_t;
class shader_local_statement_t;
class shader_assignment_statement_t;
class shader_output_statement_t;
class shader_branch_statement_t;
class shader_loop_statement_t;
class shader_break_statement_t;
class shader_continue_statement_t;
class shader_discard_statement_t;

class shader_expression_node_t {
public:
    virtual ~shader_expression_node_t() = default;

    shader_data_type_t type() const;
    std::span<const shader_expression_node_t* const> operands() const;
    virtual void accept(shader_ast_visitor_t& visitor) const = 0;

protected:
    explicit shader_expression_node_t(shader_data_type_t type, std::vector<const shader_expression_node_t*> operands = {});

private:
    shader_data_type_t m_type;
    std::vector<const shader_expression_node_t*> m_operands;
};

using shader_expression_nodes_t = std::vector<std::unique_ptr<const shader_expression_node_t>>; // Immutable computations, not cached runtime values.

template <typename T>
concept shader_expression_structure =
    std::same_as<T, shader_constant_node_t> || std::same_as<T, shader_input_node_t> || std::same_as<T, shader_uniform_node_t> || std::same_as<T, shader_resource_node_t> ||
    std::same_as<T, shader_builtin_node_t> || std::same_as<T, shader_local_node_t> || std::same_as<T, shader_unary_node_t> || std::same_as<T, shader_binary_node_t> ||
    std::same_as<T, shader_construct_node_t> || std::same_as<T, shader_swizzle_node_t> || std::same_as<T, shader_call_node_t>;

class shader_statement_node_t {
public:
    virtual ~shader_statement_node_t() = default;
    virtual void accept(shader_ast_visitor_t& visitor) const = 0;
};

using shader_statement_nodes_t = std::vector<std::unique_ptr<const shader_statement_node_t>>;

class shader_block_t {
public:
    shader_statement_nodes_t statements;
};

template <typename T>
concept shader_statement_structure =
    std::same_as<T, shader_local_statement_t> || std::same_as<T, shader_assignment_statement_t> ||
    std::same_as<T, shader_output_statement_t> || std::same_as<T, shader_branch_statement_t> ||
    std::same_as<T, shader_loop_statement_t> || std::same_as<T, shader_break_statement_t> ||
    std::same_as<T, shader_continue_statement_t> || std::same_as<T, shader_discard_statement_t>;

class shader_ast_t {
public:
    shader_ast_t(shader_stage_t stage, shader_expression_nodes_t expressions, shader_block_t root);
    shader_ast_t(shader_ast_t&&) noexcept = default;
    shader_ast_t& operator=(shader_ast_t&&) noexcept = default;

    shader_stage_t stage() const;
    const shader_block_t& root() const;
    const shader_interface_t& interface() const;

private:
    shader_stage_t m_stage;
    shader_expression_nodes_t m_expressions;
    shader_block_t m_root;
    shader_interface_t m_interface;
};

class shader_ast_builder_t {
public:
    shader_ast_builder_t(const shader_ast_builder_t&) = delete;
    shader_ast_builder_t& operator=(const shader_ast_builder_t&) = delete;
    shader_ast_builder_t(shader_ast_builder_t&&) = delete;
    shader_ast_builder_t& operator=(shader_ast_builder_t&&) = delete;

    shader_ast_t finalize() &&;

    template <shader_expression_structure Node>
    const shader_expression_node_t* expression(std::unique_ptr<Node> expression);

    template <shader_type T, shader_expression_structure Node>
    shader_expression_t<T> expression(std::unique_ptr<Node> expression);

    /** @brief Declares an input with perspective interpolation. */
    template <shader_value T>
    shader_expression_t<T> input(std::uint32_t location);

    /** @brief Declares an input; non-default interpolation is available only in fragment shaders. */
    template <shader_value T>
    shader_expression_t<T> input(std::uint32_t location, interpolation_t interpolation);

    template <shader_value T>
    void output(std::uint32_t location, shader_expression_t<T> expression);

    template <shader_value T>
    void output(std::uint32_t location, T value);

    template <shader_value T>
    shader_expression_t<T> uniform(std::uint32_t binding);

    template <shader_resource T>
    shader_expression_t<std::remove_cvref_t<T>> resource(std::uint32_t binding);

    template <shader_value T>
    shader_expression_t<std::remove_cvref_t<T>> constant(T value);

    template <shader_value T, typename... Ts>
    requires (shader_operand<Ts> && ...)
    shader_expression_t<T> construct(Ts&&... expressions);

    template <shader_value T>
    shader_local_t<T> local(shader_expression_t<T> initial);

    template <shader_value T>
    shader_local_t<std::remove_cvref_t<T>> local(T initial);

    template <shader_value T>
    void assign(shader_local_t<T> local, shader_expression_t<T> value);

    template <shader_value T>
    void assign(shader_local_t<T> local, T value);

    template <typename Body>
    requires (std::invocable<Body>)
    void branch(shader_expression_t<bool> condition, Body&& body);

    template <typename TrueBody, typename FalseBody>
    requires (std::invocable<TrueBody> && std::invocable<FalseBody>)
    void branch(shader_expression_t<bool> condition, TrueBody&& true_body, FalseBody&& false_body);

    template <typename Body>
    requires (std::invocable<Body>)
    void loop(shader_expression_t<bool> condition, Body&& body);

    void break_loop();
    void continue_loop();

protected:
    explicit shader_ast_builder_t(shader_stage_t stage);

    template <shader_value T>
    shader_expression_t<T> builtin(shader_builtin_t builtin);

    template <shader_statement_structure Node>
    void statement(std::unique_ptr<Node> statement);

    template <shader_type T>
    const shader_expression_node_t* require(shader_expression_t<T> expression) const;

private:
    template <typename Body>
    shader_block_t block(Body&& body);

    template <shader_operand T>
    const shader_expression_node_t* operand(T&& operand);

    bool owns(const shader_expression_node_t* expression) const;

    shader_stage_t m_stage;
    shader_expression_nodes_t m_expressions;
    shader_block_t m_root;
    shader_block_t* m_current_block;
    std::size_t m_loop_depth = 0;
};

class vertex_shader_ast_builder_t : public shader_ast_builder_t {
public:
    vertex_shader_ast_builder_t();
    shader_expression_t<std::int32_t> vertex_index();
    shader_expression_t<std::int32_t> instance_index();

    /**
     * @brief Reads the backend-supplied homogeneous float object-to-world matrix.
     */
    shader_expression_t<matrix_t<float, 4, 4>> object_to_world();

    /**
     * @brief Reads the backend-supplied homogeneous float world-to-clip matrix.
     */
    shader_expression_t<matrix_t<float, 4, 4>> world_to_clip();
    void position(shader_expression_t<vector_t<float, 4>> expression);
    void position(vector_t<float, 4> value);
};

class fragment_shader_ast_builder_t : public shader_ast_builder_t {
public:
    fragment_shader_ast_builder_t();
    shader_expression_t<vector_t<float, 4>> fragment_coordinate();
    shader_expression_t<bool> front_facing();
    void color(shader_expression_t<vector_t<float, 4>> expression);
    void color(vector_t<float, 4> value);
    void discard();
};

class shader_ast_visitor_t {
public:
    virtual ~shader_ast_visitor_t() = default;

    virtual void visit(const shader_constant_node_t&) = 0;
    virtual void visit(const shader_input_node_t&) = 0;
    virtual void visit(const shader_uniform_node_t&) = 0;
    virtual void visit(const shader_resource_node_t&) = 0;
    virtual void visit(const shader_builtin_node_t&) = 0;
    virtual void visit(const shader_local_node_t&) = 0;
    virtual void visit(const shader_unary_node_t&) = 0;
    virtual void visit(const shader_binary_node_t&) = 0;
    virtual void visit(const shader_construct_node_t&) = 0;
    virtual void visit(const shader_swizzle_node_t&) = 0;
    virtual void visit(const shader_call_node_t&) = 0;
    virtual void visit(const shader_local_statement_t&) = 0;
    virtual void visit(const shader_assignment_statement_t&) = 0;
    virtual void visit(const shader_output_statement_t&) = 0;
    virtual void visit(const shader_branch_statement_t&) = 0;
    virtual void visit(const shader_loop_statement_t&) = 0;
    virtual void visit(const shader_break_statement_t&) = 0;
    virtual void visit(const shader_continue_statement_t&) = 0;
    virtual void visit(const shader_discard_statement_t&) = 0;
};

class shader_constant_node_t final : public shader_expression_node_t {
public:
    template <shader_value T>
    explicit shader_constant_node_t(T value);
    explicit shader_constant_node_t(shader_literal_t value);

    const shader_literal_t& value() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    shader_literal_t m_value;
};

class shader_input_node_t final : public shader_expression_node_t {
public:
    shader_input_node_t(shader_data_type_t type, std::uint32_t location);
    shader_input_node_t(shader_data_type_t type, std::uint32_t location, interpolation_t interpolation);
    std::uint32_t location() const;
    interpolation_t interpolation() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    std::uint32_t m_location;
    interpolation_t m_interpolation;
};

class shader_uniform_node_t final : public shader_expression_node_t {
public:
    shader_uniform_node_t(shader_data_type_t type, std::uint32_t binding);
    std::uint32_t binding() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    std::uint32_t m_binding;
};

class shader_resource_node_t final : public shader_expression_node_t {
public:
    shader_resource_node_t(shader_data_type_t type, std::uint32_t binding);
    std::uint32_t binding() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    std::uint32_t m_binding;
};

class shader_builtin_node_t final : public shader_expression_node_t {
public:
    shader_builtin_node_t(shader_data_type_t type, shader_builtin_t builtin);
    shader_builtin_t builtin() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    shader_builtin_t m_builtin;
};

class shader_local_node_t final : public shader_expression_node_t {
public:
    explicit shader_local_node_t(shader_data_type_t type);
    void accept(shader_ast_visitor_t& visitor) const override;
};

class shader_unary_node_t final : public shader_expression_node_t {
public:
    shader_unary_node_t(shader_data_type_t type, shader_unary_operation_t operation, const shader_expression_node_t* expression);
    shader_unary_operation_t operation() const;
    const shader_expression_node_t& expression() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    shader_unary_operation_t m_operation;
};

class shader_binary_node_t final : public shader_expression_node_t {
public:
    shader_binary_node_t(shader_data_type_t type, shader_binary_operation_t operation, const shader_expression_node_t* lhs, const shader_expression_node_t* rhs);
    shader_binary_operation_t operation() const;
    const shader_expression_node_t& lhs() const;
    const shader_expression_node_t& rhs() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    shader_binary_operation_t m_operation;
};

class shader_construct_node_t final : public shader_expression_node_t {
public:
    shader_construct_node_t(shader_data_type_t type, std::vector<const shader_expression_node_t*> expressions);
    void accept(shader_ast_visitor_t& visitor) const override;
};

class shader_swizzle_node_t final : public shader_expression_node_t {
public:
    shader_swizzle_node_t(shader_data_type_t type, const shader_expression_node_t* expression, std::vector<std::uint8_t> components);
    const shader_expression_node_t& expression() const;
    const std::vector<std::uint8_t>& components() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    std::vector<std::uint8_t> m_components;
};

class shader_call_node_t final : public shader_expression_node_t {
public:
    shader_call_node_t(shader_data_type_t type, shader_call_operation_t operation, std::vector<const shader_expression_node_t*> arguments);
    shader_call_operation_t operation() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    shader_call_operation_t m_operation;
};

class shader_local_statement_t final : public shader_statement_node_t {
public:
    shader_local_statement_t(const shader_local_node_t* local, const shader_expression_node_t* initial);
    const shader_local_node_t* local_node() const;
    const shader_expression_node_t* initial_node() const;
    const shader_local_node_t& local() const;
    const shader_expression_node_t& initial() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    const shader_local_node_t* m_local;
    const shader_expression_node_t* m_initial;
};

class shader_assignment_statement_t final : public shader_statement_node_t {
public:
    shader_assignment_statement_t(const shader_local_node_t* local, const shader_expression_node_t* value);
    const shader_local_node_t* local_node() const;
    const shader_expression_node_t* value_node() const;
    const shader_local_node_t& local() const;
    const shader_expression_node_t& value() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    const shader_local_node_t* m_local;
    const shader_expression_node_t* m_value;
};

class shader_output_statement_t final : public shader_statement_node_t {
public:
    shader_output_statement_t(shader_output_t output, std::uint32_t location, const shader_expression_node_t* expression);
    shader_output_t output() const;
    std::uint32_t location() const;
    const shader_expression_node_t* expression_node() const;
    const shader_expression_node_t& expression() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    shader_output_t m_output;
    std::uint32_t m_location;
    const shader_expression_node_t* m_expression;
};

class shader_branch_statement_t final : public shader_statement_node_t {
public:
    shader_branch_statement_t(const shader_expression_node_t* condition, shader_block_t true_block, shader_block_t false_block = {});
    const shader_expression_node_t* condition_node() const;
    const shader_expression_node_t& condition() const;
    const shader_block_t& true_block() const;
    const shader_block_t& false_block() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    const shader_expression_node_t* m_condition;
    shader_block_t m_true_block;
    shader_block_t m_false_block;
};

class shader_loop_statement_t final : public shader_statement_node_t {
public:
    shader_loop_statement_t(const shader_expression_node_t* condition, shader_block_t body);
    const shader_expression_node_t* condition_node() const;
    const shader_expression_node_t& condition() const;
    const shader_block_t& body() const;
    void accept(shader_ast_visitor_t& visitor) const override;

private:
    const shader_expression_node_t* m_condition;
    shader_block_t m_body;
};

class shader_break_statement_t final : public shader_statement_node_t {
public:
    void accept(shader_ast_visitor_t& visitor) const override;
};

class shader_continue_statement_t final : public shader_statement_node_t {
public:
    void accept(shader_ast_visitor_t& visitor) const override;
};

class shader_discard_statement_t final : public shader_statement_node_t {
public:
    void accept(shader_ast_visitor_t& visitor) const override;
};

} // namespace m03gsy25j4v7nccgmsdov9ioft_shader

namespace std {

template <>
struct formatter<m03gsy25j4v7nccgmsdov9ioft_shader::interpolation_t>;

} // namespace std

namespace m03gsy25j4v7nccgmsdov9ioft_shader {

template <shader_value T>
shader_constant_node_t::shader_constant_node_t(T value):
    shader_constant_node_t(shader_literal_t(std::move(value)))
{
}

template <shader_value T>
shader_local_t<T>::shader_local_t(shader_ast_builder_t* builder, const shader_local_node_t* node):
    shader_expression_t<T>(builder, node),
    m_local(node)
{
}

template <shader_expression_structure Node>
const shader_expression_node_t* shader_ast_builder_t::expression(std::unique_ptr<Node> expression) {
    if (!expression) {
        throw std::invalid_argument("null shader expression");
    }
    for (const auto* operand : expression->operands()) {
        if (!owns(operand)) {
            throw std::invalid_argument("shader expression belongs to another builder");
        }
    }
    const auto* node = expression.get();
    m_expressions.push_back(std::move(expression));
    return node;
}

template <shader_type T, shader_expression_structure Node>
shader_expression_t<T> shader_ast_builder_t::expression(std::unique_ptr<Node> expression) {
    if (!expression || expression->type() != shader_data_type<T>()) {
        throw std::invalid_argument("shader expression result type does not match its handle");
    }
    return {this, this->expression(std::move(expression))};
}

template <shader_value T>
shader_expression_t<T> shader_ast_builder_t::input(std::uint32_t location) {
    return input<T>(location, interpolation_t::perspective);
}

template <shader_value T>
shader_expression_t<T> shader_ast_builder_t::input(std::uint32_t location, interpolation_t interpolation) {
    return expression<T>(std::make_unique<shader_input_node_t>(shader_data_type<T>(), location, interpolation));
}

template <shader_value T>
void shader_ast_builder_t::output(std::uint32_t location, shader_expression_t<T> expression) {
    statement(std::make_unique<shader_output_statement_t>(shader_output_t::location, location, require(expression)));
}

template <shader_value T>
void shader_ast_builder_t::output(std::uint32_t location, T value) {
    output(location, constant(std::move(value)));
}

template <shader_value T>
shader_expression_t<T> shader_ast_builder_t::uniform(std::uint32_t binding) {
    return expression<T>(std::make_unique<shader_uniform_node_t>(shader_data_type<T>(), binding));
}

template <shader_resource T>
shader_expression_t<std::remove_cvref_t<T>> shader_ast_builder_t::resource(std::uint32_t binding) {
    using type = std::remove_cvref_t<T>;
    return expression<type>(std::make_unique<shader_resource_node_t>(shader_data_type<type>(), binding));
}

template <shader_value T>
shader_expression_t<std::remove_cvref_t<T>> shader_ast_builder_t::constant(T value) {
    using type = std::remove_cvref_t<T>;
    return expression<type>(std::make_unique<shader_constant_node_t>(std::move(value)));
}

template <shader_value T, typename... Ts>
requires (shader_operand<Ts> && ...)
shader_expression_t<T> shader_ast_builder_t::construct(Ts&&... expressions) {
    std::vector<const shader_expression_node_t*> operands;
    operands.reserve(sizeof...(Ts));
    (operands.push_back(operand(std::forward<Ts>(expressions))), ...);
    return expression<T>(std::make_unique<shader_construct_node_t>(shader_data_type<T>(), std::move(operands)));
}

template <shader_value T>
shader_local_t<T> shader_ast_builder_t::local(shader_expression_t<T> initial) {
    const auto* initial_node = require(initial);
    auto local = expression<T>(std::make_unique<shader_local_node_t>(shader_data_type<T>()));
    const auto* local_node = static_cast<const shader_local_node_t*>(local.node());
    statement(std::make_unique<shader_local_statement_t>(local_node, initial_node));
    return {this, local_node};
}

template <shader_value T>
shader_local_t<std::remove_cvref_t<T>> shader_ast_builder_t::local(T initial) {
    return local(constant(std::move(initial)));
}

template <shader_value T>
void shader_ast_builder_t::assign(shader_local_t<T> local, shader_expression_t<T> value) {
    statement(std::make_unique<shader_assignment_statement_t>(static_cast<const shader_local_node_t*>(require(local)), require(value)));
}

template <shader_value T>
void shader_ast_builder_t::assign(shader_local_t<T> local, T value) {
    assign(local, constant(std::move(value)));
}

template <typename Body>
requires (std::invocable<Body>)
void shader_ast_builder_t::branch(shader_expression_t<bool> condition, Body&& body) {
    statement(std::make_unique<shader_branch_statement_t>(require(condition), block(std::forward<Body>(body))));
}

template <typename TrueBody, typename FalseBody>
requires (std::invocable<TrueBody> && std::invocable<FalseBody>)
void shader_ast_builder_t::branch(shader_expression_t<bool> condition, TrueBody&& true_body, FalseBody&& false_body) {
    auto true_block = block(std::forward<TrueBody>(true_body));
    auto false_block = block(std::forward<FalseBody>(false_body));
    statement(std::make_unique<shader_branch_statement_t>(require(condition), std::move(true_block), std::move(false_block)));
}

template <typename Body>
requires (std::invocable<Body>)
void shader_ast_builder_t::loop(shader_expression_t<bool> condition, Body&& body) {
    ++m_loop_depth;
    try {
        auto loop_body = block(std::forward<Body>(body));
        --m_loop_depth;
        statement(std::make_unique<shader_loop_statement_t>(require(condition), std::move(loop_body)));
    } catch (...) {
        --m_loop_depth;
        throw;
    }
}

template <shader_value T>
shader_expression_t<T> shader_ast_builder_t::builtin(shader_builtin_t builtin) {
    return expression<T>(std::make_unique<shader_builtin_node_t>(shader_data_type<T>(), builtin));
}

template <shader_statement_structure Node>
void shader_ast_builder_t::statement(std::unique_ptr<Node> statement) {
    if (!statement) {
        throw std::invalid_argument("null shader statement");
    }
    m_current_block->statements.push_back(std::move(statement));
}

template <typename Body>
shader_block_t shader_ast_builder_t::block(Body&& body) {
    shader_block_t result;
    auto* previous = std::exchange(m_current_block, &result);
    try {
        std::invoke(std::forward<Body>(body));
        m_current_block = previous;
        return result;
    } catch (...) {
        m_current_block = previous;
        throw;
    }
}

template <shader_operand T>
const shader_expression_node_t* shader_ast_builder_t::operand(T&& value) {
    if constexpr (shader_handle_operand<T>) {
        return require(shader_expression_t<shader_operand_type_t<T>>(value));
    } else {
        return constant(std::forward<T>(value)).node();
    }
}

template <shader_type T>
const shader_expression_node_t* shader_ast_builder_t::require(shader_expression_t<T> expression) const {
    if (expression.builder() != this || !owns(expression.node())) {
        throw std::invalid_argument("shader expression belongs to another builder");
    }
    return expression.node();
}

} // namespace m03gsy25j4v7nccgmsdov9ioft_shader

namespace std {

template <>
struct formatter<m03gsy25j4v7nccgmsdov9ioft_shader::interpolation_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid interpolation_t format specifier");
        }
        return it;
    }
    auto format(m03gsy25j4v7nccgmsdov9ioft_shader::interpolation_t interpolation, auto& ctx) const {
        auto out = ctx.out();
        switch (interpolation) {
            case m03gsy25j4v7nccgmsdov9ioft_shader::interpolation_t::perspective: { out = std::format_to(out, "perspective"); } break;
            case m03gsy25j4v7nccgmsdov9ioft_shader::interpolation_t::noperspective: { out = std::format_to(out, "noperspective"); } break;
            case m03gsy25j4v7nccgmsdov9ioft_shader::interpolation_t::flat: { out = std::format_to(out, "flat"); } break;
            default: { out = std::format_to(out, "invalid({})", static_cast<int>(interpolation)); } break;
        }
        return out;
    }
};

} // namespace std

#endif // M03GSY25J4V7NCCGMSDOV9IOFT_SHADER_SHADER_BUILDER_H
