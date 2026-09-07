#include "shader_builder.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace m03gsy25j4v7nccgmsdov9ioft_shader {

namespace {

[[noreturn]] void invalid(std::string_view message) {
    throw std::invalid_argument(std::string("invalid shader AST: ") + std::string(message));
}

[[noreturn]] void invalid_interface(std::string_view message) {
    throw std::invalid_argument(std::string("invalid shader interface: ") + std::string(message));
}

bool is_value(shader_data_type_t type) {
    return type.category() == shader_data_category_t::scalar ||
        type.category() == shader_data_category_t::vector ||
        type.category() == shader_data_category_t::matrix;
}

bool is_numeric(shader_data_type_t type) {
    return is_value(type) && type.scalar() != shader_scalar_type_t::none && type.scalar() != shader_scalar_type_t::boolean;
}

bool is_float(shader_data_type_t type) {
    return is_value(type) && type.scalar() == shader_scalar_type_t::floating_point;
}

bool is_bool(shader_data_type_t type) {
    return type == shader_data_type<bool>();
}

bool valid(shader_data_type_t type) {
    const auto scalar = type.scalar() == shader_scalar_type_t::boolean ||
        type.scalar() == shader_scalar_type_t::signed_integer ||
        type.scalar() == shader_scalar_type_t::unsigned_integer ||
        type.scalar() == shader_scalar_type_t::floating_point;
    switch (type.category()) {
        case shader_data_category_t::scalar: return scalar && !type.rows() && !type.columns();
        case shader_data_category_t::vector: return scalar && 2 <= type.rows() && type.rows() <= 4 && type.columns() == 1;
        case shader_data_category_t::matrix:
            return type.scalar() == shader_scalar_type_t::floating_point &&
                2 <= type.rows() && type.rows() <= 4 &&
                2 <= type.columns() && type.columns() <= 4;
        case shader_data_category_t::texture_2d:
        case shader_data_category_t::sampler: return type.scalar() == shader_scalar_type_t::none && !type.rows() && !type.columns();
        default: return false;
    }
}

std::size_t components(shader_data_type_t type) {
    if (type.category() == shader_data_category_t::scalar) {
        return 1;
    }
    if (type.category() == shader_data_category_t::vector) {
        return type.rows();
    }
    if (type.category() == shader_data_category_t::matrix) {
        return type.rows() * type.columns();
    }
    return 0;
}

template <typename Map>
void consistent(Map& bindings, std::uint32_t binding, shader_data_type_t type, std::string_view message) {
    const auto [iterator, inserted] = bindings.emplace(binding, type);
    if (!inserted && iterator->second != type) {
        invalid(message);
    }
}

int binding_namespace(shader_data_type_t type) {
    if (is_value(type)) {
        return 0;
    }
    if (type == shader_data_type<shader_texture_2d_t>()) {
        return 1;
    }
    if (type == shader_data_type<shader_sampler_t>()) {
        return 2;
    }
    invalid_interface("binding has an unsupported shader type");
}

void canonicalize_locations(std::vector<shader_interface_element_t>& elements, std::string_view collection) {
    for (const auto& element : elements) {
        if (!valid(element.type) || !is_value(element.type)) {
            invalid_interface(std::string(collection) + " element is not a shader value type");
        }
    }

    std::ranges::sort(elements, {}, &shader_interface_element_t::index);

    std::vector<shader_interface_element_t> canonical;
    canonical.reserve(elements.size());
    for (const auto& element : elements) {
        if (!canonical.empty() && canonical.back().index == element.index) {
            if (canonical.back().type != element.type || canonical.back().interpolation != element.interpolation) {
                invalid_interface(std::string(collection) + " location has inconsistent types");
            }
            continue;
        }
        canonical.push_back(element);
    }
    elements = std::move(canonical);
}

void canonicalize_bindings(std::vector<shader_interface_element_t>& elements) {
    for (const auto& element : elements) {
        if (!valid(element.type)) {
            invalid_interface("binding has an invalid shader type");
        }
        (void)binding_namespace(element.type);
    }

    std::ranges::sort(elements, [](const auto& lhs, const auto& rhs) {
        return std::tuple(binding_namespace(lhs.type), lhs.index) <
            std::tuple(binding_namespace(rhs.type), rhs.index);
    });

    std::vector<shader_interface_element_t> canonical;
    canonical.reserve(elements.size());
    for (const auto& element : elements) {
        if (!canonical.empty() &&
            binding_namespace(canonical.back().type) == binding_namespace(element.type) &&
            canonical.back().index == element.index) {
            if (canonical.back().type != element.type || canonical.back().interpolation != element.interpolation) {
                invalid_interface("binding has inconsistent types");
            }
            continue;
        }
        canonical.push_back(element);
    }
    elements = std::move(canonical);
}

template <typename Map>
std::vector<shader_interface_element_t> interface_elements(const Map& elements) {
    std::vector<shader_interface_element_t> result;
    result.reserve(elements.size());
    for (const auto& [index, type] : elements) {
        result.push_back({index, type});
    }
    return result;
}

class shader_analyzer_t final : public shader_ast_visitor_t {
public:
    shader_analyzer_t(shader_stage_t stage, const shader_expression_nodes_t& expressions, const shader_block_t& root):
        m_stage(stage),
        m_expressions(expressions),
        m_root(root)
    {
    }

    shader_interface_t analyze() {
        if (m_stage != shader_stage_t::vertex && m_stage != shader_stage_t::fragment) {
            invalid("unknown shader stage");
        }
        for (const auto& expression : m_expressions) {
            if (!expression || !m_owned.insert(expression.get()).second) {
                invalid("null or duplicate expression arena entry");
            }
        }
        validate_block(m_root);
        if (m_stage == shader_stage_t::vertex && !m_position) {
            invalid("vertex shader does not write position");
        }
        if (m_stage == shader_stage_t::fragment && m_outputs.empty() && !m_color && !m_discard) {
            invalid("fragment shader has no output");
        }

        auto bindings = interface_elements(m_uniforms);
        auto textures = interface_elements(m_textures);
        auto samplers = interface_elements(m_samplers);
        bindings.insert(bindings.end(), textures.begin(), textures.end());
        bindings.insert(bindings.end(), samplers.begin(), samplers.end());
        auto inputs = interface_elements(m_inputs);
        for (auto& input : inputs) {
            input.interpolation = m_interpolations.at(input.index);
        }
        return {
            m_stage,
            std::move(inputs),
            interface_elements(m_outputs),
            std::move(bindings)
        };
    }

    void visit(const shader_constant_node_t& node) override {
        if (!is_value(node.type()) || node.value().type() != node.type() || node.value().size() != components(node.type())) {
            invalid("constant value does not match its type");
        }
    }

    void visit(const shader_input_node_t& node) override {
        if (!is_value(node.type())) {
            invalid("input is not a value type");
        }
        consistent(m_inputs, node.location(), node.type(), "input location has inconsistent types");
        const auto [iterator, inserted] = m_interpolations.emplace(node.location(), node.interpolation());
        if (!inserted && iterator->second != node.interpolation()) {
            invalid("input location has conflicting interpolation declarations");
        }
    }

    void visit(const shader_uniform_node_t& node) override {
        if (!is_value(node.type())) {
            invalid("uniform is not a value type");
        }
        consistent(m_uniforms, node.binding(), node.type(), "uniform binding has inconsistent types");
    }

    void visit(const shader_resource_node_t& node) override {
        if (node.type().category() == shader_data_category_t::texture_2d) {
            consistent(m_textures, node.binding(), node.type(), "texture binding has inconsistent types");
        } else if (node.type().category() == shader_data_category_t::sampler) {
            consistent(m_samplers, node.binding(), node.type(), "sampler binding has inconsistent types");
        } else {
            invalid("resource has a non-resource type");
        }
    }

    void visit(const shader_builtin_node_t& node) override {
        switch (node.builtin()) {
            case shader_builtin_t::vertex_index:
            case shader_builtin_t::instance_index: {
                if (m_stage != shader_stage_t::vertex || node.type() != shader_data_type<std::int32_t>()) {
                    invalid("invalid vertex builtin");
                }
            } break;
            case shader_builtin_t::object_to_world:
            case shader_builtin_t::world_to_clip: {
                if (m_stage != shader_stage_t::vertex || node.type() != shader_data_type<matrix_t<float, 4, 4>>()) {
                    invalid("invalid vertex matrix builtin");
                }
            } break;
            case shader_builtin_t::fragment_coordinate: {
                if (m_stage != shader_stage_t::fragment || node.type() != shader_data_type<vector_t<float, 4>>()) {
                    invalid("invalid fragment-coordinate builtin");
                }
            } break;
            case shader_builtin_t::front_facing: {
                if (m_stage != shader_stage_t::fragment || node.type() != shader_data_type<bool>()) {
                    invalid("invalid front-facing builtin");
                }
            } break;
            default: {
                invalid("unknown builtin");
            }
        }
    }

    void visit(const shader_local_node_t& node) override {
        if (!is_value(node.type())) {
            invalid("local is not a value type");
        }
    }

    void visit(const shader_unary_node_t& node) override {
        const auto input = node.expression().type();
        const auto result = node.type();
        switch (node.operation()) {
            case shader_unary_operation_t::negate:
                if (!is_numeric(input) || result != input) {
                    invalid("invalid negation");
                }
                break;
            case shader_unary_operation_t::logical_not:
                if (!is_bool(input) || !is_bool(result)) {
                    invalid("invalid logical not");
                }
                break;
            case shader_unary_operation_t::absolute:
                if (!is_numeric(input) || input.category() == shader_data_category_t::matrix ||
                    input.scalar() == shader_scalar_type_t::unsigned_integer || result != input) {
                    invalid("invalid absolute value");
                }
                break;
            case shader_unary_operation_t::square_root:
            case shader_unary_operation_t::floor:
            case shader_unary_operation_t::ceil:
            case shader_unary_operation_t::fract:
            case shader_unary_operation_t::sine:
            case shader_unary_operation_t::cosine:
                if (!is_float(input) || input.category() == shader_data_category_t::matrix || result != input) {
                    invalid("invalid floating-point intrinsic");
                }
                break;
            case shader_unary_operation_t::normalize:
                if (!is_float(input) || input.category() != shader_data_category_t::vector || result != input) {
                    invalid("invalid normalize");
                }
                break;
            case shader_unary_operation_t::length:
                if (!is_float(input) || input.category() != shader_data_category_t::vector || result != shader_data_type<float>()) {
                    invalid("invalid length");
                }
                break;
            default: invalid("unknown unary operation");
        }
    }

    void visit(const shader_binary_node_t& node) override {
        const auto lhs = node.lhs().type();
        const auto rhs = node.rhs().type();
        const auto result = node.type();
        switch (node.operation()) {
            case shader_binary_operation_t::add:
            case shader_binary_operation_t::subtract:
                if (!componentwise(lhs, rhs, result, false)) {
                    invalid("invalid additive operation");
                }
                break;
            case shader_binary_operation_t::multiply:
                if (!multiply(lhs, rhs, result)) {
                    invalid("invalid multiplication");
                }
                break;
            case shader_binary_operation_t::divide:
                if (!componentwise(lhs, rhs, result)) {
                    invalid("invalid division");
                }
                break;
            case shader_binary_operation_t::modulo:
                if (!is_numeric(lhs) || lhs.scalar() == shader_scalar_type_t::floating_point ||
                    result != lhs || !same_or_scalar(lhs, rhs)) {
                    invalid("invalid modulo");
                }
                break;
            case shader_binary_operation_t::equal:
            case shader_binary_operation_t::not_equal:
                if (lhs != rhs || !is_value(lhs) || !is_bool(result)) {
                    invalid("invalid equality comparison");
                }
                break;
            case shader_binary_operation_t::less:
            case shader_binary_operation_t::less_equal:
            case shader_binary_operation_t::greater:
            case shader_binary_operation_t::greater_equal:
                if (lhs != rhs || !is_numeric(lhs) || lhs.category() != shader_data_category_t::scalar || !is_bool(result)) {
                    invalid("invalid ordered comparison");
                }
                break;
            case shader_binary_operation_t::logical_and:
            case shader_binary_operation_t::logical_or:
                if (!is_bool(lhs) || !is_bool(rhs) || !is_bool(result)) {
                    invalid("invalid logical operation");
                }
                break;
            case shader_binary_operation_t::dot:
                if (lhs != rhs || !is_float(lhs) || lhs.category() != shader_data_category_t::vector || result != shader_data_type<float>()) {
                    invalid("invalid dot product");
                }
                break;
            case shader_binary_operation_t::cross:
                if (lhs != shader_data_type<vector_t<float, 3>>() || rhs != lhs || result != lhs) {
                    invalid("invalid cross product");
                }
                break;
            case shader_binary_operation_t::power:
            case shader_binary_operation_t::reflect:
                if (lhs != rhs || !is_float(lhs) || lhs.category() == shader_data_category_t::matrix || result != lhs) {
                    invalid("invalid floating-point binary intrinsic");
                }
                break;
            case shader_binary_operation_t::minimum:
            case shader_binary_operation_t::maximum:
                if (!is_numeric(lhs) || lhs.category() == shader_data_category_t::matrix ||
                    !is_numeric(rhs) || result != lhs || !same_or_scalar(lhs, rhs)) {
                    invalid("invalid min/max");
                }
                break;
            case shader_binary_operation_t::step:
                if (!is_float(lhs) || !is_float(rhs) || result != rhs || !same_or_scalar(rhs, lhs)) {
                    invalid("invalid step");
                }
                break;
            default: invalid("unknown binary operation");
        }
    }

    void visit(const shader_construct_node_t& node) override {
        if (!is_value(node.type()) || node.operands().empty()) {
            invalid("invalid construction");
        }
        std::size_t count = 0;
        for (const auto* operand : node.operands()) {
            if (!is_value(operand->type()) || operand->type().scalar() != node.type().scalar()) {
                invalid("construction operand type mismatch");
            }
            count += components(operand->type());
        }
        if (count != components(node.type())) {
            invalid("construction component count mismatch");
        }
    }

    void visit(const shader_swizzle_node_t& node) override {
        const auto input = node.expression().type();
        if (input.category() != shader_data_category_t::vector || node.components().empty() || node.components().size() > 4) {
            invalid("invalid swizzle");
        }
        if (std::ranges::any_of(node.components(), [input](auto component) { return component >= input.rows(); })) {
            invalid("swizzle component is out of range");
        }
        if (node.type().scalar() != input.scalar() || components(node.type()) != node.components().size()) {
            invalid("swizzle result type mismatch");
        }
    }

    void visit(const shader_call_node_t& node) override {
        const auto arguments = node.operands();
        switch (node.operation()) {
            case shader_call_operation_t::clamp:
                if (arguments.size() != 3 || !is_numeric(node.type()) || arguments[0]->type() != node.type() ||
                    !same_or_scalar(node.type(), arguments[1]->type()) || !same_or_scalar(node.type(), arguments[2]->type())) {
                    invalid("invalid clamp");
                }
                break;
            case shader_call_operation_t::mix:
                if (arguments.size() != 3 || !is_float(node.type()) || node.type().category() == shader_data_category_t::matrix ||
                    arguments[0]->type() != node.type() || arguments[1]->type() != node.type() ||
                    arguments[2]->type() != shader_data_type<float>()) {
                    invalid("invalid mix");
                }
                break;
            case shader_call_operation_t::smoothstep:
                if (arguments.size() != 3 || !is_float(node.type()) || node.type().category() == shader_data_category_t::matrix ||
                    arguments[2]->type() != node.type() || !same_or_scalar(node.type(), arguments[0]->type()) ||
                    !same_or_scalar(node.type(), arguments[1]->type())) {
                    invalid("invalid smoothstep");
                }
                break;
            case shader_call_operation_t::sample:
            case shader_call_operation_t::sample_lod:
                if (arguments.size() != (node.operation() == shader_call_operation_t::sample ? 3U : 4U) ||
                    (node.operation() == shader_call_operation_t::sample_lod && arguments[3]->type() != shader_data_type<float>()) ||
                    arguments[0]->type() != shader_data_type<shader_texture_2d_t>() ||
                    arguments[1]->type() != shader_data_type<shader_sampler_t>() ||
                    arguments[2]->type() != shader_data_type<vector_t<float, 2>>() ||
                    node.type() != shader_data_type<vector_t<float, 4>>()) {
                    invalid("invalid texture sample");
                }
                break;
            default: invalid("unknown call operation");
        }
    }

    void visit(const shader_local_statement_t& statement) override {
        const auto& local = owned(statement.local_node());
        const auto& initial = owned(statement.initial_node());
        use(initial);
        if (!valid(local.type()) || !is_value(local.type())) {
            invalid("local target is not a shader value type");
        }
        if (local.type() != initial.type()) {
            invalid("local initializer type mismatch");
        }
        if (!m_declared_locals.insert(statement.local_node()).second) {
            invalid("local is declared more than once");
        }
        m_visible_locals.push_back(statement.local_node());
    }

    void visit(const shader_assignment_statement_t& statement) override {
        const auto& local = owned(statement.local_node());
        const auto& value = owned(statement.value_node());
        if (!valid(local.type()) || !is_value(local.type())) {
            invalid("assignment target is not a shader value type");
        }
        if (!visible(statement.local())) {
            invalid("assignment targets an out-of-scope local");
        }
        use(value);
        if (local.type() != value.type()) {
            invalid("assignment type mismatch");
        }
    }

    void visit(const shader_output_statement_t& statement) override {
        const auto& expression = owned(statement.expression_node());
        use(expression);
        if (!is_value(expression.type())) {
            invalid("output is not a value type");
        }
        if (statement.output() == shader_output_t::position) {
            if (m_stage != shader_stage_t::vertex || expression.type() != shader_data_type<vector_t<float, 4>>()) {
                invalid("invalid position output");
            }
            m_position = true;
        } else if (statement.output() == shader_output_t::color) {
            if (m_stage != shader_stage_t::fragment || expression.type() != shader_data_type<vector_t<float, 4>>()) {
                invalid("invalid color output");
            }
            m_color = true;
        } else if (statement.output() == shader_output_t::location) {
            consistent(m_outputs, statement.location(), expression.type(), "output location has inconsistent types");
        } else {
            invalid("unknown output semantic");
        }
    }

    void visit(const shader_branch_statement_t& statement) override {
        const auto& condition = owned(statement.condition_node());
        use(condition);
        if (!is_bool(condition.type())) {
            invalid("branch condition is not boolean");
        }
        validate_block(statement.true_block());
        validate_block(statement.false_block());
    }

    void visit(const shader_loop_statement_t& statement) override {
        const auto& condition = owned(statement.condition_node());
        use(condition);
        if (!is_bool(condition.type())) {
            invalid("loop condition is not boolean");
        }
        ++m_loop_depth;
        validate_block(statement.body());
        --m_loop_depth;
    }

    void visit(const shader_break_statement_t&) override {
        if (!m_loop_depth) {
            invalid("break appears outside a loop");
        }
    }

    void visit(const shader_continue_statement_t&) override {
        if (!m_loop_depth) {
            invalid("continue appears outside a loop");
        }
    }

    void visit(const shader_discard_statement_t&) override {
        if (m_stage != shader_stage_t::fragment) {
            invalid("discard appears outside a fragment shader");
        }
        m_discard = true;
    }

private:
    static bool same_or_scalar(shader_data_type_t value, shader_data_type_t other) {
        return value == other ||
            (value.category() == shader_data_category_t::vector &&
             other.category() == shader_data_category_t::scalar &&
             value.scalar() == other.scalar());
    }

    static bool componentwise(shader_data_type_t lhs, shader_data_type_t rhs, shader_data_type_t result, bool matrix_scalar = true) {
        if (!is_numeric(lhs) || !is_numeric(rhs) || lhs.scalar() != rhs.scalar() || result != lhs) {
            return false;
        }
        if (lhs == rhs) {
            return true;
        }
        return (lhs.category() == shader_data_category_t::vector ||
                (matrix_scalar && lhs.category() == shader_data_category_t::matrix)) &&
            rhs.category() == shader_data_category_t::scalar;
    }

    static bool multiply(shader_data_type_t lhs, shader_data_type_t rhs, shader_data_type_t result) {
        if (!is_numeric(lhs) || !is_numeric(rhs) || lhs.scalar() != rhs.scalar()) {
            return false;
        }
        if (lhs.category() == shader_data_category_t::matrix && rhs.category() == shader_data_category_t::vector) {
            return lhs.columns() == rhs.rows() && result.category() == shader_data_category_t::vector &&
                result.scalar() == lhs.scalar() && result.rows() == lhs.rows();
        }
        if (lhs.category() == shader_data_category_t::matrix && rhs.category() == shader_data_category_t::matrix) {
            return lhs.columns() == rhs.rows() && result.category() == shader_data_category_t::matrix &&
                result.scalar() == lhs.scalar() && result.rows() == lhs.rows() && result.columns() == rhs.columns();
        }
        return componentwise(lhs, rhs, result);
    }

    void analyze(const shader_expression_node_t& expression) {
        if (!valid(expression.type())) {
            invalid("invalid shader data type");
        }
        if (!known(expression)) {
            invalid("unknown expression structure");
        }
        const auto [iterator, inserted] = m_visiting.emplace(&expression, false);
        if (!inserted) {
            if (!iterator->second) {
                invalid("expression dependency cycle");
            }
            return;
        }
        for (const auto* operand : expression.operands()) {
            if (!operand || !m_owned.contains(operand)) {
                invalid("expression operand is outside the arena");
            }
            analyze(*operand);
        }
        expression.accept(*this);
        iterator->second = true;
    }

    void validate_block(const shader_block_t& block) {
        const auto scope = m_visible_locals.size();
        for (const auto& statement : block.statements) {
            if (!statement) {
                invalid("null statement");
            }
            if (!known(*statement)) {
                invalid("unknown statement structure");
            }
            statement->accept(*this);
        }
        m_visible_locals.resize(scope);
    }

    void validate_local_reads(const shader_expression_node_t& expression) {
        if (const auto* local = dynamic_cast<const shader_local_node_t*>(&expression); local && !visible(*local)) {
            invalid("local is used before declaration or outside its scope");
        }
        for (const auto* operand : expression.operands()) {
            validate_local_reads(owned(operand));
        }
    }

    void use(const shader_expression_node_t& expression) {
        owned(&expression);
        analyze(expression);
        validate_local_reads(expression);
    }

    bool visible(const shader_local_node_t& local) const {
        return std::ranges::find(m_visible_locals, &local) != m_visible_locals.end();
    }

    const shader_expression_node_t& owned(const shader_expression_node_t* expression) const {
        if (!expression || !m_owned.contains(expression)) {
            invalid("statement expression is outside the arena");
        }
        return *expression;
    }

    static bool known(const shader_expression_node_t& expression) {
        return dynamic_cast<const shader_constant_node_t*>(&expression) ||
            dynamic_cast<const shader_input_node_t*>(&expression) ||
            dynamic_cast<const shader_uniform_node_t*>(&expression) ||
            dynamic_cast<const shader_resource_node_t*>(&expression) ||
            dynamic_cast<const shader_builtin_node_t*>(&expression) ||
            dynamic_cast<const shader_local_node_t*>(&expression) ||
            dynamic_cast<const shader_unary_node_t*>(&expression) ||
            dynamic_cast<const shader_binary_node_t*>(&expression) ||
            dynamic_cast<const shader_construct_node_t*>(&expression) ||
            dynamic_cast<const shader_swizzle_node_t*>(&expression) ||
            dynamic_cast<const shader_call_node_t*>(&expression);
    }

    static bool known(const shader_statement_node_t& statement) {
        return dynamic_cast<const shader_local_statement_t*>(&statement) ||
            dynamic_cast<const shader_assignment_statement_t*>(&statement) ||
            dynamic_cast<const shader_output_statement_t*>(&statement) ||
            dynamic_cast<const shader_branch_statement_t*>(&statement) ||
            dynamic_cast<const shader_loop_statement_t*>(&statement) ||
            dynamic_cast<const shader_break_statement_t*>(&statement) ||
            dynamic_cast<const shader_continue_statement_t*>(&statement) ||
            dynamic_cast<const shader_discard_statement_t*>(&statement);
    }

    shader_stage_t m_stage;
    const shader_expression_nodes_t& m_expressions;
    const shader_block_t& m_root;
    std::unordered_set<const shader_expression_node_t*> m_owned;
    std::unordered_map<const shader_expression_node_t*, bool> m_visiting;
    std::unordered_set<const shader_local_node_t*> m_declared_locals;
    std::vector<const shader_local_node_t*> m_visible_locals;
    std::unordered_map<std::uint32_t, shader_data_type_t> m_inputs;
    std::unordered_map<std::uint32_t, interpolation_t> m_interpolations;
    std::unordered_map<std::uint32_t, shader_data_type_t> m_uniforms;
    std::unordered_map<std::uint32_t, shader_data_type_t> m_textures;
    std::unordered_map<std::uint32_t, shader_data_type_t> m_samplers;
    std::unordered_map<std::uint32_t, shader_data_type_t> m_outputs;
    std::size_t m_loop_depth = 0;
    bool m_position = false;
    bool m_color = false;
    bool m_discard = false;
};

shader_interface_t analyze_shader(
    shader_stage_t stage,
    const shader_expression_nodes_t& expressions,
    const shader_block_t& root
) {
    return shader_analyzer_t(stage, expressions, root).analyze();
}

} // namespace

shader_interface_t::shader_interface_t(
    shader_stage_t stage,
    std::vector<shader_interface_element_t> inputs,
    std::vector<shader_interface_element_t> outputs,
    std::vector<shader_interface_element_t> bindings
):
    m_stage(stage),
    m_inputs(std::move(inputs)),
    m_outputs(std::move(outputs)),
    m_bindings(std::move(bindings))
{
    if (m_stage != shader_stage_t::vertex && m_stage != shader_stage_t::fragment) {
        invalid_interface("unknown shader stage");
    }
    const auto validate_interpolation = [](const auto& elements, bool fragment_inputs) {
        for (const auto& element : elements) {
            switch (element.interpolation) {
                case interpolation_t::perspective: { } break;
                case interpolation_t::noperspective:
                case interpolation_t::flat: {
                    if (!fragment_inputs) {
                        invalid_interface("interpolation is only selectable on fragment inputs");
                    }
                } break;
                default: { invalid_interface("unknown interpolation mode"); } break;
            }
        }
    };
    validate_interpolation(m_inputs, stage == shader_stage_t::fragment);
    validate_interpolation(m_outputs, false);
    validate_interpolation(m_bindings, false);
    canonicalize_locations(m_inputs, "input");
    canonicalize_locations(m_outputs, "output");
    canonicalize_bindings(m_bindings);
}

shader_stage_t shader_interface_t::stage() const { return m_stage; }
std::span<const shader_interface_element_t> shader_interface_t::inputs() const { return m_inputs; }
std::span<const shader_interface_element_t> shader_interface_t::outputs() const { return m_outputs; }
std::span<const shader_interface_element_t> shader_interface_t::bindings() const { return m_bindings; }

bool shader_expression_type_matches(const shader_expression_node_t* expression, shader_data_type_t type) {
    return expression && expression->type() == type;
}

shader_data_type_t shader_literal_t::type() const { return m_type; }
const shader_literal_data_t& shader_literal_t::data() const { return m_data; }
std::size_t shader_literal_t::size() const {
    return std::visit([](const auto& data) -> std::size_t {
        using type = std::remove_cvref_t<decltype(data)>;
        if constexpr (std::same_as<type, shader_boolean_components_t>) {
            return data.values.size();
        } else {
            return data.size();
        }
    }, m_data);
}

const shader_expression_node_t* shader_constant_expression(shader_ast_builder_t* builder, shader_literal_t literal) {
    if (!builder) {
        throw std::invalid_argument("null shader builder");
    }
    return builder->expression(std::make_unique<shader_constant_node_t>(std::move(literal)));
}

const shader_expression_node_t* shader_unary_expression(
    shader_ast_builder_t* builder, shader_data_type_t type, shader_unary_operation_t operation,
    const shader_expression_node_t* expression
) {
    if (!builder) {
        throw std::invalid_argument("null shader builder");
    }
    return builder->expression(std::make_unique<shader_unary_node_t>(type, operation, expression));
}

const shader_expression_node_t* shader_binary_expression(
    shader_ast_builder_t* builder, shader_data_type_t type, shader_binary_operation_t operation,
    const shader_expression_node_t* lhs, const shader_expression_node_t* rhs
) {
    if (!builder) {
        throw std::invalid_argument("null shader builder");
    }
    return builder->expression(std::make_unique<shader_binary_node_t>(type, operation, lhs, rhs));
}

const shader_expression_node_t* shader_call_expression(
    shader_ast_builder_t* builder, shader_data_type_t type, shader_call_operation_t operation,
    std::vector<const shader_expression_node_t*> arguments
) {
    if (!builder) {
        throw std::invalid_argument("null shader builder");
    }
    return builder->expression(std::make_unique<shader_call_node_t>(type, operation, std::move(arguments)));
}

const shader_expression_node_t* shader_swizzle_expression(
    shader_ast_builder_t* builder, shader_data_type_t type, const shader_expression_node_t* expression,
    std::vector<std::uint8_t> components
) {
    if (!builder) {
        throw std::invalid_argument("null shader builder");
    }
    return builder->expression(std::make_unique<shader_swizzle_node_t>(type, expression, std::move(components)));
}

shader_expression_node_t::shader_expression_node_t(shader_data_type_t type, std::vector<const shader_expression_node_t*> operands):
    m_type(type),
    m_operands(std::move(operands))
{
    if (std::ranges::any_of(m_operands, [](const auto* operand) { return !operand; })) {
        throw std::invalid_argument("null shader expression operand");
    }
}

shader_data_type_t shader_expression_node_t::type() const { return m_type; }
std::span<const shader_expression_node_t* const> shader_expression_node_t::operands() const { return m_operands; }
shader_constant_node_t::shader_constant_node_t(shader_literal_t value):
    shader_expression_node_t(value.type()),
    m_value(std::move(value))
{
}
const shader_literal_t& shader_constant_node_t::value() const { return m_value; }
void shader_constant_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_input_node_t::shader_input_node_t(shader_data_type_t type, std::uint32_t location, interpolation_t interpolation):
    shader_expression_node_t(type),
    m_location(location),
    m_interpolation(interpolation)
{
}
std::uint32_t shader_input_node_t::location() const { return m_location; }
interpolation_t shader_input_node_t::interpolation() const { return m_interpolation; }
void shader_input_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_uniform_node_t::shader_uniform_node_t(shader_data_type_t type, std::uint32_t binding):
    shader_expression_node_t(type),
    m_binding(binding)
{
}
std::uint32_t shader_uniform_node_t::binding() const { return m_binding; }
void shader_uniform_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_resource_node_t::shader_resource_node_t(shader_data_type_t type, std::uint32_t binding):
    shader_expression_node_t(type),
    m_binding(binding)
{
}
std::uint32_t shader_resource_node_t::binding() const { return m_binding; }
void shader_resource_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_builtin_node_t::shader_builtin_node_t(shader_data_type_t type, shader_builtin_t builtin):
    shader_expression_node_t(type),
    m_builtin(builtin)
{
}
shader_builtin_t shader_builtin_node_t::builtin() const { return m_builtin; }
void shader_builtin_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_local_node_t::shader_local_node_t(shader_data_type_t type):
    shader_expression_node_t(type)
{
}
void shader_local_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_unary_node_t::shader_unary_node_t(shader_data_type_t type, shader_unary_operation_t operation, const shader_expression_node_t* expression):
    shader_expression_node_t(type, {expression}),
    m_operation(operation)
{
}
shader_unary_operation_t shader_unary_node_t::operation() const { return m_operation; }
const shader_expression_node_t& shader_unary_node_t::expression() const { return *operands()[0]; }
void shader_unary_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_binary_node_t::shader_binary_node_t(shader_data_type_t type, shader_binary_operation_t operation, const shader_expression_node_t* lhs, const shader_expression_node_t* rhs):
    shader_expression_node_t(type, {lhs, rhs}),
    m_operation(operation)
{
}
shader_binary_operation_t shader_binary_node_t::operation() const { return m_operation; }
const shader_expression_node_t& shader_binary_node_t::lhs() const { return *operands()[0]; }
const shader_expression_node_t& shader_binary_node_t::rhs() const { return *operands()[1]; }
void shader_binary_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_construct_node_t::shader_construct_node_t(shader_data_type_t type, std::vector<const shader_expression_node_t*> expressions):
    shader_expression_node_t(type, std::move(expressions))
{
}
void shader_construct_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_swizzle_node_t::shader_swizzle_node_t(shader_data_type_t type, const shader_expression_node_t* expression, std::vector<std::uint8_t> components):
    shader_expression_node_t(type, {expression}),
    m_components(std::move(components))
{
}
const shader_expression_node_t& shader_swizzle_node_t::expression() const { return *operands()[0]; }
const std::vector<std::uint8_t>& shader_swizzle_node_t::components() const { return m_components; }
void shader_swizzle_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_call_node_t::shader_call_node_t(shader_data_type_t type, shader_call_operation_t operation, std::vector<const shader_expression_node_t*> arguments):
    shader_expression_node_t(type, std::move(arguments)),
    m_operation(operation)
{
}
shader_call_operation_t shader_call_node_t::operation() const { return m_operation; }
void shader_call_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_local_statement_t::shader_local_statement_t(const shader_local_node_t* local, const shader_expression_node_t* initial):
    m_local(local),
    m_initial(initial)
{
    if (!local || !initial) {
        throw std::invalid_argument("null local declaration operand");
    }
}
const shader_local_node_t* shader_local_statement_t::local_node() const { return m_local; }
const shader_expression_node_t* shader_local_statement_t::initial_node() const { return m_initial; }
const shader_local_node_t& shader_local_statement_t::local() const { return *m_local; }
const shader_expression_node_t& shader_local_statement_t::initial() const { return *m_initial; }
void shader_local_statement_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_assignment_statement_t::shader_assignment_statement_t(const shader_local_node_t* local, const shader_expression_node_t* value):
    m_local(local),
    m_value(value)
{
    if (!local || !value) {
        throw std::invalid_argument("null assignment operand");
    }
}
const shader_local_node_t* shader_assignment_statement_t::local_node() const { return m_local; }
const shader_expression_node_t* shader_assignment_statement_t::value_node() const { return m_value; }
const shader_local_node_t& shader_assignment_statement_t::local() const { return *m_local; }
const shader_expression_node_t& shader_assignment_statement_t::value() const { return *m_value; }
void shader_assignment_statement_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_output_statement_t::shader_output_statement_t(shader_output_t output, std::uint32_t location, const shader_expression_node_t* expression):
    m_output(output),
    m_location(location),
    m_expression(expression)
{
    if (!expression) {
        throw std::invalid_argument("null shader output expression");
    }
}
shader_output_t shader_output_statement_t::output() const { return m_output; }
std::uint32_t shader_output_statement_t::location() const { return m_location; }
const shader_expression_node_t* shader_output_statement_t::expression_node() const { return m_expression; }
const shader_expression_node_t& shader_output_statement_t::expression() const { return *m_expression; }
void shader_output_statement_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_branch_statement_t::shader_branch_statement_t(const shader_expression_node_t* condition, shader_block_t true_block, shader_block_t false_block):
    m_condition(condition),
    m_true_block(std::move(true_block)),
    m_false_block(std::move(false_block))
{
    if (!condition) {
        throw std::invalid_argument("null branch condition");
    }
}
const shader_expression_node_t* shader_branch_statement_t::condition_node() const { return m_condition; }
const shader_expression_node_t& shader_branch_statement_t::condition() const { return *m_condition; }
const shader_block_t& shader_branch_statement_t::true_block() const { return m_true_block; }
const shader_block_t& shader_branch_statement_t::false_block() const { return m_false_block; }
void shader_branch_statement_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_loop_statement_t::shader_loop_statement_t(const shader_expression_node_t* condition, shader_block_t body):
    m_condition(condition),
    m_body(std::move(body))
{
    if (!condition) {
        throw std::invalid_argument("null loop condition");
    }
}
const shader_expression_node_t* shader_loop_statement_t::condition_node() const { return m_condition; }
const shader_expression_node_t& shader_loop_statement_t::condition() const { return *m_condition; }
const shader_block_t& shader_loop_statement_t::body() const { return m_body; }
void shader_loop_statement_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }
void shader_break_statement_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }
void shader_continue_statement_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }
void shader_discard_statement_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_ast_t::shader_ast_t(shader_stage_t stage, shader_expression_nodes_t expressions, shader_block_t root):
    m_stage(stage),
    m_expressions(std::move(expressions)),
    m_root(std::move(root)),
    m_interface(analyze_shader(m_stage, m_expressions, m_root))
{
}
shader_stage_t shader_ast_t::stage() const { return m_stage; }
const shader_block_t& shader_ast_t::root() const { return m_root; }
const shader_interface_t& shader_ast_t::interface() const { return m_interface; }

shader_ast_builder_t::shader_ast_builder_t(shader_stage_t stage):
    m_stage(stage),
    m_current_block(&m_root)
{
}

shader_ast_t shader_ast_builder_t::finalize() && {
    if (m_current_block != &m_root || m_loop_depth) {
        throw std::logic_error("shader builder has unfinished control flow");
    }
    return {m_stage, std::move(m_expressions), std::move(m_root)};
}

bool shader_ast_builder_t::owns(const shader_expression_node_t* expression) const {
    return expression && std::ranges::any_of(m_expressions, [expression](const auto& owned) { return owned.get() == expression; });
}

void shader_ast_builder_t::break_loop() {
    if (!m_loop_depth) {
        throw std::logic_error("shader break appears outside a loop");
    }
    statement(std::make_unique<shader_break_statement_t>());
}

void shader_ast_builder_t::continue_loop() {
    if (!m_loop_depth) {
        throw std::logic_error("shader continue appears outside a loop");
    }
    statement(std::make_unique<shader_continue_statement_t>());
}

vertex_shader_ast_builder_t::vertex_shader_ast_builder_t():
    shader_ast_builder_t(shader_stage_t::vertex)
{
}
shader_expression_t<std::int32_t> vertex_shader_ast_builder_t::vertex_index() { return builtin<std::int32_t>(shader_builtin_t::vertex_index); }
shader_expression_t<std::int32_t> vertex_shader_ast_builder_t::instance_index() { return builtin<std::int32_t>(shader_builtin_t::instance_index); }
shader_expression_t<matrix_t<float, 4, 4>> vertex_shader_ast_builder_t::object_to_world() { return builtin<matrix_t<float, 4, 4>>(shader_builtin_t::object_to_world); }
shader_expression_t<matrix_t<float, 4, 4>> vertex_shader_ast_builder_t::world_to_clip() { return builtin<matrix_t<float, 4, 4>>(shader_builtin_t::world_to_clip); }
void vertex_shader_ast_builder_t::position(shader_expression_t<vector_t<float, 4>> expression) { statement(std::make_unique<shader_output_statement_t>(shader_output_t::position, 0, require(expression))); }
void vertex_shader_ast_builder_t::position(vector_t<float, 4> value) { position(constant(std::move(value))); }

fragment_shader_ast_builder_t::fragment_shader_ast_builder_t():
    shader_ast_builder_t(shader_stage_t::fragment)
{
}
shader_expression_t<vector_t<float, 4>> fragment_shader_ast_builder_t::fragment_coordinate() { return builtin<vector_t<float, 4>>(shader_builtin_t::fragment_coordinate); }
shader_expression_t<bool> fragment_shader_ast_builder_t::front_facing() { return builtin<bool>(shader_builtin_t::front_facing); }
void fragment_shader_ast_builder_t::color(shader_expression_t<vector_t<float, 4>> expression) { statement(std::make_unique<shader_output_statement_t>(shader_output_t::color, 0, require(expression))); }
void fragment_shader_ast_builder_t::color(vector_t<float, 4> value) { color(constant(std::move(value))); }
void fragment_shader_ast_builder_t::discard() { statement(std::make_unique<shader_discard_statement_t>()); }

shader_expression_t<bool> operator!(shader_expression_t<bool> expression) { return shader_unary<bool>(shader_unary_operation_t::logical_not, expression); }
shader_expression_t<vector_t<float, 3>> cross(shader_expression_t<vector_t<float, 3>> lhs, shader_expression_t<vector_t<float, 3>> rhs) { return shader_binary<vector_t<float, 3>>(shader_binary_operation_t::cross, lhs, rhs); }
shader_expression_t<float> abs(shader_expression_t<float> expression) { return shader_unary<float>(shader_unary_operation_t::absolute, expression); }
shader_expression_t<std::int32_t> abs(shader_expression_t<std::int32_t> expression) { return shader_unary<std::int32_t>(shader_unary_operation_t::absolute, expression); }
shader_expression_t<float> sqrt(shader_expression_t<float> expression) { return shader_unary<float>(shader_unary_operation_t::square_root, expression); }
shader_expression_t<float> pow(shader_expression_t<float> base, shader_expression_t<float> exponent) { return shader_binary<float>(shader_binary_operation_t::power, base, exponent); }
shader_expression_t<float> pow(shader_expression_t<float> base, float exponent) { return pow(base, base.builder()->constant(exponent)); }
shader_expression_t<float> floor(shader_expression_t<float> expression) { return shader_unary<float>(shader_unary_operation_t::floor, expression); }
shader_expression_t<float> ceil(shader_expression_t<float> expression) { return shader_unary<float>(shader_unary_operation_t::ceil, expression); }
shader_expression_t<float> fract(shader_expression_t<float> expression) { return shader_unary<float>(shader_unary_operation_t::fract, expression); }
shader_expression_t<float> sin(shader_expression_t<float> expression) { return shader_unary<float>(shader_unary_operation_t::sine, expression); }
shader_expression_t<float> cos(shader_expression_t<float> expression) { return shader_unary<float>(shader_unary_operation_t::cosine, expression); }
shader_expression_t<float> reflect(shader_expression_t<float> incident, shader_expression_t<float> normal) { return shader_binary<float>(shader_binary_operation_t::reflect, incident, normal); }
shader_expression_t<float> step(shader_expression_t<float> edge, shader_expression_t<float> expression) { return shader_binary<float>(shader_binary_operation_t::step, edge, expression); }
shader_expression_t<float> step(float edge, shader_expression_t<float> expression) { return step(expression.builder()->constant(edge), expression); }
shader_expression_t<float> smoothstep(shader_expression_t<float> edge0, shader_expression_t<float> edge1, shader_expression_t<float> expression) { return shader_call<float>(shader_call_operation_t::smoothstep, edge0, edge1, expression); }
shader_expression_t<float> smoothstep(float edge0, float edge1, shader_expression_t<float> expression) { return smoothstep(expression.builder()->constant(edge0), expression.builder()->constant(edge1), expression); }
shader_expression_t<float> mix(shader_expression_t<float> lhs, shader_expression_t<float> rhs, shader_expression_t<float> factor) { return shader_call<float>(shader_call_operation_t::mix, lhs, rhs, factor); }
shader_expression_t<float> mix(shader_expression_t<float> lhs, shader_expression_t<float> rhs, float factor) { return mix(lhs, rhs, lhs.builder()->constant(factor)); }
shader_expression_t<vector_t<float, 4>> sample(
    shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler,
    shader_expression_t<vector_t<float, 2>> coordinates
) {
    return texture.builder()->expression<vector_t<float, 4>>(std::make_unique<shader_call_node_t>(
        shader_data_type<vector_t<float, 4>>(), shader_call_operation_t::sample,
        std::vector<const shader_expression_node_t*>{texture.node(), sampler.node(), coordinates.node()}
    ));
}
shader_expression_t<vector_t<float, 4>> sample(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, vector_t<float, 2> coordinates) { return sample(texture, sampler, texture.builder()->constant(std::move(coordinates))); }

shader_expression_t<vector_t<float, 4>> sample_lod(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, shader_expression_t<vector_t<float, 2>> coordinates, shader_expression_t<float> lod) {
    return texture.builder()->expression<vector_t<float, 4>>(std::make_unique<shader_call_node_t>(
        shader_data_type<vector_t<float, 4>>(), shader_call_operation_t::sample_lod,
        std::vector<const shader_expression_node_t*>{texture.node(), sampler.node(), coordinates.node(), lod.node()}
    ));
}
shader_expression_t<vector_t<float, 4>> sample_lod(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, shader_expression_t<vector_t<float, 2>> coordinates, float lod) {
    return sample_lod(texture, sampler, coordinates, texture.builder()->constant(lod));
}
shader_expression_t<vector_t<float, 4>> sample_lod(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, vector_t<float, 2> coordinates, shader_expression_t<float> lod) {
    return sample_lod(texture, sampler, texture.builder()->constant(coordinates), lod);
}
shader_expression_t<vector_t<float, 4>> sample_lod(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, vector_t<float, 2> coordinates, float lod) {
    return sample_lod(texture, sampler, texture.builder()->constant(coordinates), texture.builder()->constant(lod));
}

} // namespace m03gsy25j4v7nccgmsdov9ioft_shader
