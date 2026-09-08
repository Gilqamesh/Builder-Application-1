#include "helpers.h"

#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace m03gsy25j4v7nccgmsdov9ioft_shader {

[[noreturn]] void invalid(std::string_view message) {
    throw std::invalid_argument(std::format("invalid shader AST: {}", message));
}

[[noreturn]] void invalid_interface(std::string_view message) {
    throw std::invalid_argument(std::format("invalid shader interface: {}", message));
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
    invalid_interface(std::format("binding has unsupported shader type {}", type));
}

void canonicalize_locations(std::vector<shader_interface_element_t>& elements, std::string_view collection) {
    for (const auto& element : elements) {
        if (!valid(element.type) || !is_value(element.type)) {
            invalid_interface(std::format("{} location {} has invalid value type {}", collection, element.index, element.type));
        }
    }

    std::ranges::sort(elements, {}, &shader_interface_element_t::index);

    std::vector<shader_interface_element_t> canonical;
    canonical.reserve(elements.size());
    for (const auto& element : elements) {
        if (!canonical.empty() && canonical.back().index == element.index) {
            if (canonical.back().type != element.type || canonical.back().interpolation != element.interpolation) {
                invalid_interface(std::format("{} location {} conflicts: actual {}, expected {}", collection, element.index, element, canonical.back()));
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
            invalid_interface(std::format("binding {} has invalid type {}", element.index, element.type));
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
                invalid_interface(std::format("binding {} conflicts: actual {}, expected {}", element.index, element, canonical.back()));
            }
            continue;
        }
        canonical.push_back(element);
    }
    elements = std::move(canonical);
}

shader_analyzer_t::shader_analyzer_t(shader_stage_t stage, const shader_expression_nodes_t& expressions, const shader_block_t& root):
    m_stage(stage),
    m_expressions(expressions),
    m_root(root)
{
}

shader_interface_t shader_analyzer_t::analyze() {
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

void shader_analyzer_t::visit(const shader_constant_node_t& node) {
    if (!is_value(node.type()) || node.value().type() != node.type() || node.value().size() != components(node.type())) {
        invalid(std::format("constant has literal type {} with {} components; expected type {} with {} components", node.value().type(), node.value().size(), node.type(), components(node.type())));
    }
}

void shader_analyzer_t::visit(const shader_input_node_t& node) {
    if (!is_value(node.type())) {
        invalid("input is not a value type");
    }
    consistent(m_inputs, node.location(), node.type(), "input location has inconsistent types");
    const auto [iterator, inserted] = m_interpolations.emplace(node.location(), node.interpolation());
    if (!inserted && iterator->second != node.interpolation()) {
        invalid(std::format("input location {} has interpolation {}; expected {}", node.location(), node.interpolation(), iterator->second));
    }
}

void shader_analyzer_t::visit(const shader_uniform_node_t& node) {
    if (!is_value(node.type())) {
        invalid("uniform is not a value type");
    }
    consistent(m_uniforms, node.binding(), node.type(), "uniform binding has inconsistent types");
}

void shader_analyzer_t::visit(const shader_resource_node_t& node) {
    if (node.type().category() == shader_data_category_t::texture_2d) {
        consistent(m_textures, node.binding(), node.type(), "texture binding has inconsistent types");
    } else if (node.type().category() == shader_data_category_t::sampler) {
        consistent(m_samplers, node.binding(), node.type(), "sampler binding has inconsistent types");
    } else {
        invalid("resource has a non-resource type");
    }
}

void shader_analyzer_t::visit(const shader_builtin_node_t& node) {
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

void shader_analyzer_t::visit(const shader_local_node_t& node) {
    if (!is_value(node.type())) {
        invalid("local is not a value type");
    }
}

void shader_analyzer_t::visit(const shader_unary_node_t& node) {
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

void shader_analyzer_t::visit(const shader_binary_node_t& node) {
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

void shader_analyzer_t::visit(const shader_construct_node_t& node) {
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

void shader_analyzer_t::visit(const shader_swizzle_node_t& node) {
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

void shader_analyzer_t::visit(const shader_call_node_t& node) {
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

void shader_analyzer_t::visit(const shader_local_statement_t& statement) {
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

void shader_analyzer_t::visit(const shader_assignment_statement_t& statement) {
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

void shader_analyzer_t::visit(const shader_output_statement_t& statement) {
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

void shader_analyzer_t::visit(const shader_branch_statement_t& statement) {
    const auto& condition = owned(statement.condition_node());
    use(condition);
    if (!is_bool(condition.type())) {
        invalid("branch condition is not boolean");
    }
    validate_block(statement.true_block());
    validate_block(statement.false_block());
}

void shader_analyzer_t::visit(const shader_loop_statement_t& statement) {
    const auto& condition = owned(statement.condition_node());
    use(condition);
    if (!is_bool(condition.type())) {
        invalid("loop condition is not boolean");
    }
    ++m_loop_depth;
    validate_block(statement.body());
    --m_loop_depth;
}

void shader_analyzer_t::visit(const shader_break_statement_t&) {
    if (!m_loop_depth) {
        invalid("break appears outside a loop");
    }
}

void shader_analyzer_t::visit(const shader_continue_statement_t&) {
    if (!m_loop_depth) {
        invalid("continue appears outside a loop");
    }
}

void shader_analyzer_t::visit(const shader_discard_statement_t&) {
    if (m_stage != shader_stage_t::fragment) {
        invalid("discard appears outside a fragment shader");
    }
    m_discard = true;
}

bool shader_analyzer_t::same_or_scalar(shader_data_type_t value, shader_data_type_t other) {
    return value == other ||
        (value.category() == shader_data_category_t::vector &&
         other.category() == shader_data_category_t::scalar &&
         value.scalar() == other.scalar());
}

bool shader_analyzer_t::componentwise(shader_data_type_t lhs, shader_data_type_t rhs, shader_data_type_t result, bool matrix_scalar) {
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

bool shader_analyzer_t::multiply(shader_data_type_t lhs, shader_data_type_t rhs, shader_data_type_t result) {
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

void shader_analyzer_t::analyze(const shader_expression_node_t& expression) {
    if (!valid(expression.type())) {
        invalid(std::format("expression has invalid shader data type {}", expression.type()));
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

void shader_analyzer_t::validate_block(const shader_block_t& block) {
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

void shader_analyzer_t::validate_local_reads(const shader_expression_node_t& expression) {
    if (const auto* local = dynamic_cast<const shader_local_node_t*>(&expression); local && !visible(*local)) {
        invalid("local is used before declaration or outside its scope");
    }
    for (const auto* operand : expression.operands()) {
        validate_local_reads(owned(operand));
    }
}

void shader_analyzer_t::use(const shader_expression_node_t& expression) {
    owned(&expression);
    analyze(expression);
    validate_local_reads(expression);
}

bool shader_analyzer_t::visible(const shader_local_node_t& local) const {
    return std::ranges::find(m_visible_locals, &local) != m_visible_locals.end();
}

const shader_expression_node_t& shader_analyzer_t::owned(const shader_expression_node_t* expression) const {
    if (!expression || !m_owned.contains(expression)) {
        invalid("statement expression is outside the arena");
    }
    return *expression;
}

bool shader_analyzer_t::known(const shader_expression_node_t& expression) {
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

bool shader_analyzer_t::known(const shader_statement_node_t& statement) {
    return dynamic_cast<const shader_local_statement_t*>(&statement) ||
        dynamic_cast<const shader_assignment_statement_t*>(&statement) ||
        dynamic_cast<const shader_output_statement_t*>(&statement) ||
        dynamic_cast<const shader_branch_statement_t*>(&statement) ||
        dynamic_cast<const shader_loop_statement_t*>(&statement) ||
        dynamic_cast<const shader_break_statement_t*>(&statement) ||
        dynamic_cast<const shader_continue_statement_t*>(&statement) ||
        dynamic_cast<const shader_discard_statement_t*>(&statement);
}

shader_interface_t analyze_shader(
    shader_stage_t stage,
    const shader_expression_nodes_t& expressions,
    const shader_block_t& root
) {
    return shader_analyzer_t(stage, expressions, root).analyze();
}

} // namespace m03gsy25j4v7nccgmsdov9ioft_shader
