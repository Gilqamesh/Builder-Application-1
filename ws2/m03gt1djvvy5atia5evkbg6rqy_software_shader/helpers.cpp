#include "helpers.h"

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

compiler_t::compiler_t(const shader::shader_ast_t& ast):
    m_interface(ast.interface())
{
    collect(ast.root());
    local_count = slot_types.size();
    lower(ast.root());
    emit(opcode_t::finish, std::nullopt, {});
}

void compiler_t::visit(const shader::shader_constant_node_t& node) {
    const auto [entry, inserted] = m_constants.try_emplace(&node, constants.size());
    if (inserted) { constants.push_back(literal_value(node.value())); }
    const auto destination = slot(node.type());
    emit(opcode_t::constant, destination, {{operand_kind_t::constant, entry->second}});
    m_result = {operand_kind_t::slot, destination};
}

void compiler_t::visit(const shader::shader_input_node_t& node) {
    m_result = compute(kernel_operation_t::input, node.type(), {{operand_kind_t::input, interface_index(m_interface.inputs(), node.location(), node.type())}}, {node.type()});
}

void compiler_t::visit(const shader::shader_uniform_node_t& node) {
    m_result = compute(kernel_operation_t::uniform, node.type(), {{operand_kind_t::binding, interface_index(m_interface.bindings(), node.binding(), node.type())}}, {node.type()});
}

void compiler_t::visit(const shader::shader_resource_node_t& node) {
    m_result = {operand_kind_t::binding, interface_index(m_interface.bindings(), node.binding(), node.type())};
}

void compiler_t::visit(const shader::shader_builtin_node_t& node) {
    const auto destination = slot(node.type());
    emit(opcode_t::builtin, destination, {{operand_kind_t::builtin, static_cast<std::size_t>(node.builtin())}});
    m_result = {operand_kind_t::slot, destination};
}

void compiler_t::visit(const shader::shader_local_node_t& node) {
    const auto destination = slot(node.type());
    emit(opcode_t::read_local, destination, {{operand_kind_t::slot, m_locals.at(&node)}});
    m_result = {operand_kind_t::slot, destination};
}

void compiler_t::visit(const shader::shader_unary_node_t& node) {
    const auto input = lower(node.expression());
    m_result = compute(operation(node.operation()), node.type(), {input}, {node.expression().type()});
}

void compiler_t::visit(const shader::shader_binary_node_t& node) {
    const auto left = lower(node.lhs());
    if (node.operation() == shader::shader_binary_operation_t::logical_and || node.operation() == shader::shader_binary_operation_t::logical_or) {
        const auto destination = slot(node.type());
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
            m_result = compute(selected, node.type(), {left, right}, {node.lhs().type()});
        } break;
        default: {
            m_result = compute(selected, node.type(), {left, right}, {node.lhs().type(), node.rhs().type()});
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
                components.push_back(compute(kernel_operation_t::component, {shader::shader_data_category_t::scalar, type.scalar()}, {input, {operand_kind_t::component, index}}, {type}));
            }
        }
    }
    m_result = compute(kernel_operation_t::construct, node.type(), components, {node.type()});
}

void compiler_t::visit(const shader::shader_swizzle_node_t& node) {
    std::vector<operand_t> components {lower(node.expression())};
    for (const auto component : node.components()) { components.push_back({operand_kind_t::component, component}); }
    m_result = compute(kernel_operation_t::swizzle, node.type(), components, {node.type(), node.expression().type()});
}

void compiler_t::visit(const shader::shader_call_node_t& node) {
    const auto expressions = node.operands();
    std::vector<operand_t> inputs;
    for (const auto* expression : expressions) { inputs.push_back(lower(*expression)); }
    switch (node.operation()) {
        case shader::shader_call_operation_t::clamp: {
            m_result = compute(kernel_operation_t::clamp, node.type(), inputs, {expressions[0]->type(), expressions[1]->type(), expressions[2]->type()});
        } break;
        case shader::shader_call_operation_t::mix: {
            m_result = compute(kernel_operation_t::mix, node.type(), inputs, {node.type()});
        } break;
        case shader::shader_call_operation_t::smoothstep: {
            m_result = compute(kernel_operation_t::smoothstep, node.type(), {inputs[2], inputs[0], inputs[1]}, {expressions[2]->type(), expressions[0]->type(), expressions[1]->type()});
        } break;
        case shader::shader_call_operation_t::sample:
        case shader::shader_call_operation_t::sample_lod: {
            const auto destination = slot(node.type());
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
        m_locals.emplace(local, slot(expression.type()));
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

std::size_t compiler_t::slot(shader::shader_data_type_t type) {
    slot_types.push_back(type);
    return slot_types.size() - 1;
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

operand_t compiler_t::compute(kernel_operation_t operation, shader::shader_data_type_t result_type, std::span<const operand_t> inputs, std::initializer_list<shader::shader_data_type_t> types) {
    const auto destination = slot(result_type);
    emit(opcode_t::kernel, destination, inputs, std::nullopt, select_kernel(operation, types));
    return {operand_kind_t::slot, destination};
}

operand_t compiler_t::compute(kernel_operation_t operation, shader::shader_data_type_t result_type, std::initializer_list<operand_t> inputs, std::initializer_list<shader::shader_data_type_t> types) {
    return compute(operation, result_type, std::span(inputs.begin(), inputs.size()), types);
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
    m_slot_types = std::move(compiler.slot_types);
    m_local_count = compiler.local_count;
}

const shader::shader_interface_t& stage_code_t::interface() const { return m_interface; }
std::span<const instruction_t> stage_code_t::instructions() const { return m_instructions; }
std::span<const operand_t> stage_code_t::operands() const { return m_operands; }
std::span<const value_t> stage_code_t::constants() const { return m_constants; }
std::span<const shader::shader_data_type_t> stage_code_t::slot_types() const { return m_slot_types; }
std::size_t stage_code_t::local_count() const { return m_local_count; }
std::size_t stage_code_t::storage_bytes() const {
    // Owned payload, excluding allocator metadata and reflection's inaccessible spare capacity.
    return sizeof(*this) + m_instructions.capacity() * sizeof(instruction_t) + m_operands.capacity() * sizeof(operand_t) + m_constants.capacity() * sizeof(value_t) + m_slot_types.capacity() * sizeof(shader::shader_data_type_t) +
        (m_interface.inputs().size() + m_interface.outputs().size() + m_interface.bindings().size()) * sizeof(shader::shader_interface_element_t);
}

shader::matrix_t<float, 4, 4> identity_matrix() {
    return {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};
}

std::span<const kernel_entry_t> kernels() {
    // Numeric instruction indices reference this immutable table. Selection occurs during compilation.
    static const kernel_entry_t entries[] {
        {kernel_operation_t::input, {0, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, bool>},
        {kernel_operation_t::uniform, {0, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, bool>},
        {kernel_operation_t::output, {0, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, bool>},
        {kernel_operation_t::component, {0, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, bool>},
        {kernel_operation_t::construct, {0, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, bool>},
        {kernel_operation_t::equal, {0, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, bool>},
        {kernel_operation_t::not_equal, {0, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, bool>},
        {kernel_operation_t::logical_not, {0, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::logical_not, bool>},
        {kernel_operation_t::input, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, std::int32_t>},
        {kernel_operation_t::uniform, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, std::int32_t>},
        {kernel_operation_t::output, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, std::int32_t>},
        {kernel_operation_t::component, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, std::int32_t>},
        {kernel_operation_t::construct, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, std::int32_t>},
        {kernel_operation_t::equal, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, std::int32_t>},
        {kernel_operation_t::not_equal, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, std::int32_t>},
        {kernel_operation_t::negate, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, std::int32_t>},
        {kernel_operation_t::add, {1, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, std::int32_t, std::int32_t>},
        {kernel_operation_t::subtract, {1, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, std::int32_t, std::int32_t>},
        {kernel_operation_t::divide, {1, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, std::int32_t, std::int32_t>},
        {kernel_operation_t::multiply, {1, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, std::int32_t, std::int32_t>},
        {kernel_operation_t::modulo, {1, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, std::int32_t, std::int32_t>},
        {kernel_operation_t::minimum, {1, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, std::int32_t, std::int32_t>},
        {kernel_operation_t::maximum, {1, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, std::int32_t, std::int32_t>},
        {kernel_operation_t::clamp, {1, 1, 1}, evaluate_kernel<kernel_operation_t::clamp, std::int32_t, std::int32_t, std::int32_t>},
        {kernel_operation_t::less, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::less, std::int32_t>},
        {kernel_operation_t::less_equal, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::less_equal, std::int32_t>},
        {kernel_operation_t::greater, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::greater, std::int32_t>},
        {kernel_operation_t::greater_equal, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::greater_equal, std::int32_t>},
        {kernel_operation_t::absolute, {1, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::absolute, std::int32_t>},
        {kernel_operation_t::input, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, std::uint32_t>},
        {kernel_operation_t::uniform, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, std::uint32_t>},
        {kernel_operation_t::output, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, std::uint32_t>},
        {kernel_operation_t::component, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, std::uint32_t>},
        {kernel_operation_t::construct, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, std::uint32_t>},
        {kernel_operation_t::equal, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, std::uint32_t>},
        {kernel_operation_t::not_equal, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, std::uint32_t>},
        {kernel_operation_t::negate, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, std::uint32_t>},
        {kernel_operation_t::add, {2, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::subtract, {2, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::divide, {2, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::multiply, {2, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::modulo, {2, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::minimum, {2, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::maximum, {2, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::clamp, {2, 2, 2}, evaluate_kernel<kernel_operation_t::clamp, std::uint32_t, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::less, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::less, std::uint32_t>},
        {kernel_operation_t::less_equal, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::less_equal, std::uint32_t>},
        {kernel_operation_t::greater, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::greater, std::uint32_t>},
        {kernel_operation_t::greater_equal, {2, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::greater_equal, std::uint32_t>},
        {kernel_operation_t::input, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, float>},
        {kernel_operation_t::uniform, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, float>},
        {kernel_operation_t::output, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, float>},
        {kernel_operation_t::component, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, float>},
        {kernel_operation_t::construct, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, float>},
        {kernel_operation_t::equal, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, float>},
        {kernel_operation_t::not_equal, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, float>},
        {kernel_operation_t::negate, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, float>},
        {kernel_operation_t::add, {3, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, float, float>},
        {kernel_operation_t::subtract, {3, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, float, float>},
        {kernel_operation_t::divide, {3, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, float, float>},
        {kernel_operation_t::multiply, {3, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, float, float>},
        {kernel_operation_t::minimum, {3, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, float, float>},
        {kernel_operation_t::maximum, {3, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, float, float>},
        {kernel_operation_t::clamp, {3, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, float, float, float>},
        {kernel_operation_t::less, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::less, float>},
        {kernel_operation_t::less_equal, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::less_equal, float>},
        {kernel_operation_t::greater, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::greater, float>},
        {kernel_operation_t::greater_equal, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::greater_equal, float>},
        {kernel_operation_t::absolute, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::absolute, float>},
        {kernel_operation_t::square_root, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::square_root, float>},
        {kernel_operation_t::floor, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::floor, float>},
        {kernel_operation_t::ceil, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::ceil, float>},
        {kernel_operation_t::fract, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::fract, float>},
        {kernel_operation_t::sine, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::sine, float>},
        {kernel_operation_t::cosine, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::cosine, float>},
        {kernel_operation_t::power, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::power, float>},
        {kernel_operation_t::reflect, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::reflect, float>},
        {kernel_operation_t::mix, {3, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::mix, float>},
        {kernel_operation_t::step, {3, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, float>},
        {kernel_operation_t::smoothstep, {3, 3, 3}, evaluate_kernel<kernel_operation_t::smoothstep, float, float, float>},
        {kernel_operation_t::input, {4, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<bool, 2>>},
        {kernel_operation_t::uniform, {4, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<bool, 2>>},
        {kernel_operation_t::output, {4, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<bool, 2>>},
        {kernel_operation_t::component, {4, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<bool, 2>>},
        {kernel_operation_t::construct, {4, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<bool, 2>>},
        {kernel_operation_t::equal, {4, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<bool, 2>>},
        {kernel_operation_t::not_equal, {4, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<bool, 2>>},
        {kernel_operation_t::swizzle, {0, 4, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, bool, shader::vector_t<bool, 2>>},
        {kernel_operation_t::swizzle, {4, 4, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<bool, 2>, shader::vector_t<bool, 2>>},
        {kernel_operation_t::swizzle, {5, 4, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<bool, 3>, shader::vector_t<bool, 2>>},
        {kernel_operation_t::swizzle, {6, 4, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<bool, 4>, shader::vector_t<bool, 2>>},
        {kernel_operation_t::input, {5, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<bool, 3>>},
        {kernel_operation_t::uniform, {5, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<bool, 3>>},
        {kernel_operation_t::output, {5, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<bool, 3>>},
        {kernel_operation_t::component, {5, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<bool, 3>>},
        {kernel_operation_t::construct, {5, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<bool, 3>>},
        {kernel_operation_t::equal, {5, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<bool, 3>>},
        {kernel_operation_t::not_equal, {5, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<bool, 3>>},
        {kernel_operation_t::swizzle, {0, 5, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, bool, shader::vector_t<bool, 3>>},
        {kernel_operation_t::swizzle, {4, 5, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<bool, 2>, shader::vector_t<bool, 3>>},
        {kernel_operation_t::swizzle, {5, 5, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<bool, 3>, shader::vector_t<bool, 3>>},
        {kernel_operation_t::swizzle, {6, 5, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<bool, 4>, shader::vector_t<bool, 3>>},
        {kernel_operation_t::input, {6, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<bool, 4>>},
        {kernel_operation_t::uniform, {6, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<bool, 4>>},
        {kernel_operation_t::output, {6, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<bool, 4>>},
        {kernel_operation_t::component, {6, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<bool, 4>>},
        {kernel_operation_t::construct, {6, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<bool, 4>>},
        {kernel_operation_t::equal, {6, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<bool, 4>>},
        {kernel_operation_t::not_equal, {6, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<bool, 4>>},
        {kernel_operation_t::swizzle, {0, 6, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, bool, shader::vector_t<bool, 4>>},
        {kernel_operation_t::swizzle, {4, 6, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<bool, 2>, shader::vector_t<bool, 4>>},
        {kernel_operation_t::swizzle, {5, 6, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<bool, 3>, shader::vector_t<bool, 4>>},
        {kernel_operation_t::swizzle, {6, 6, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<bool, 4>, shader::vector_t<bool, 4>>},
        {kernel_operation_t::input, {7, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::uniform, {7, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::output, {7, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::component, {7, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::construct, {7, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::equal, {7, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::not_equal, {7, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::negate, {7, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::add, {7, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::add, {7, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::int32_t, 2>, std::int32_t>},
        {kernel_operation_t::subtract, {7, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::subtract, {7, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::int32_t, 2>, std::int32_t>},
        {kernel_operation_t::divide, {7, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::multiply, {7, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::modulo, {7, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::minimum, {7, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::maximum, {7, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::clamp, {7, 7, 7}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::clamp, {7, 7, 1}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>, std::int32_t>},
        {kernel_operation_t::divide, {7, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::int32_t, 2>, std::int32_t>},
        {kernel_operation_t::multiply, {7, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::int32_t, 2>, std::int32_t>},
        {kernel_operation_t::modulo, {7, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::int32_t, 2>, std::int32_t>},
        {kernel_operation_t::minimum, {7, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::int32_t, 2>, std::int32_t>},
        {kernel_operation_t::maximum, {7, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::int32_t, 2>, std::int32_t>},
        {kernel_operation_t::clamp, {7, 1, 7}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 2>, std::int32_t, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::clamp, {7, 1, 1}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 2>, std::int32_t, std::int32_t>},
        {kernel_operation_t::absolute, {7, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::absolute, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::swizzle, {1, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, std::int32_t, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::swizzle, {7, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::swizzle, {8, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::swizzle, {9, 7, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 2>>},
        {kernel_operation_t::input, {8, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::uniform, {8, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::output, {8, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::component, {8, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::construct, {8, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::equal, {8, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::not_equal, {8, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::negate, {8, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::add, {8, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::add, {8, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::int32_t, 3>, std::int32_t>},
        {kernel_operation_t::subtract, {8, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::subtract, {8, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::int32_t, 3>, std::int32_t>},
        {kernel_operation_t::divide, {8, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::multiply, {8, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::modulo, {8, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::minimum, {8, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::maximum, {8, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::clamp, {8, 8, 8}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::clamp, {8, 8, 1}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>, std::int32_t>},
        {kernel_operation_t::divide, {8, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::int32_t, 3>, std::int32_t>},
        {kernel_operation_t::multiply, {8, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::int32_t, 3>, std::int32_t>},
        {kernel_operation_t::modulo, {8, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::int32_t, 3>, std::int32_t>},
        {kernel_operation_t::minimum, {8, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::int32_t, 3>, std::int32_t>},
        {kernel_operation_t::maximum, {8, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::int32_t, 3>, std::int32_t>},
        {kernel_operation_t::clamp, {8, 1, 8}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 3>, std::int32_t, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::clamp, {8, 1, 1}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 3>, std::int32_t, std::int32_t>},
        {kernel_operation_t::absolute, {8, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::absolute, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::swizzle, {1, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, std::int32_t, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::swizzle, {7, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::swizzle, {8, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::swizzle, {9, 8, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 3>>},
        {kernel_operation_t::input, {9, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::uniform, {9, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::output, {9, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::component, {9, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::construct, {9, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::equal, {9, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::not_equal, {9, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::negate, {9, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::add, {9, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::add, {9, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::int32_t, 4>, std::int32_t>},
        {kernel_operation_t::subtract, {9, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::subtract, {9, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::int32_t, 4>, std::int32_t>},
        {kernel_operation_t::divide, {9, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::multiply, {9, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::modulo, {9, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::minimum, {9, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::maximum, {9, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::clamp, {9, 9, 9}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::clamp, {9, 9, 1}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>, std::int32_t>},
        {kernel_operation_t::divide, {9, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::int32_t, 4>, std::int32_t>},
        {kernel_operation_t::multiply, {9, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::int32_t, 4>, std::int32_t>},
        {kernel_operation_t::modulo, {9, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::int32_t, 4>, std::int32_t>},
        {kernel_operation_t::minimum, {9, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::int32_t, 4>, std::int32_t>},
        {kernel_operation_t::maximum, {9, 1, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::int32_t, 4>, std::int32_t>},
        {kernel_operation_t::clamp, {9, 1, 9}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 4>, std::int32_t, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::clamp, {9, 1, 1}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::int32_t, 4>, std::int32_t, std::int32_t>},
        {kernel_operation_t::absolute, {9, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::absolute, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::swizzle, {1, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, std::int32_t, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::swizzle, {7, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::int32_t, 2>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::swizzle, {8, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::int32_t, 3>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::swizzle, {9, 9, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::int32_t, 4>, shader::vector_t<std::int32_t, 4>>},
        {kernel_operation_t::input, {10, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::uniform, {10, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::output, {10, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::component, {10, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::construct, {10, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::equal, {10, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::not_equal, {10, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::negate, {10, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::add, {10, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::add, {10, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::uint32_t, 2>, std::uint32_t>},
        {kernel_operation_t::subtract, {10, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::subtract, {10, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::uint32_t, 2>, std::uint32_t>},
        {kernel_operation_t::divide, {10, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::multiply, {10, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::modulo, {10, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::minimum, {10, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::maximum, {10, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::clamp, {10, 10, 10}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::clamp, {10, 10, 2}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>, std::uint32_t>},
        {kernel_operation_t::divide, {10, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::uint32_t, 2>, std::uint32_t>},
        {kernel_operation_t::multiply, {10, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::uint32_t, 2>, std::uint32_t>},
        {kernel_operation_t::modulo, {10, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::uint32_t, 2>, std::uint32_t>},
        {kernel_operation_t::minimum, {10, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::uint32_t, 2>, std::uint32_t>},
        {kernel_operation_t::maximum, {10, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::uint32_t, 2>, std::uint32_t>},
        {kernel_operation_t::clamp, {10, 2, 10}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 2>, std::uint32_t, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::clamp, {10, 2, 2}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 2>, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::swizzle, {2, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, std::uint32_t, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::swizzle, {10, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::swizzle, {11, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::swizzle, {12, 10, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 2>>},
        {kernel_operation_t::input, {11, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::uniform, {11, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::output, {11, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::component, {11, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::construct, {11, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::equal, {11, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::not_equal, {11, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::negate, {11, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::add, {11, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::add, {11, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::uint32_t, 3>, std::uint32_t>},
        {kernel_operation_t::subtract, {11, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::subtract, {11, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::uint32_t, 3>, std::uint32_t>},
        {kernel_operation_t::divide, {11, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::multiply, {11, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::modulo, {11, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::minimum, {11, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::maximum, {11, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::clamp, {11, 11, 11}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::clamp, {11, 11, 2}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>, std::uint32_t>},
        {kernel_operation_t::divide, {11, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::uint32_t, 3>, std::uint32_t>},
        {kernel_operation_t::multiply, {11, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::uint32_t, 3>, std::uint32_t>},
        {kernel_operation_t::modulo, {11, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::uint32_t, 3>, std::uint32_t>},
        {kernel_operation_t::minimum, {11, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::uint32_t, 3>, std::uint32_t>},
        {kernel_operation_t::maximum, {11, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::uint32_t, 3>, std::uint32_t>},
        {kernel_operation_t::clamp, {11, 2, 11}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 3>, std::uint32_t, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::clamp, {11, 2, 2}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 3>, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::swizzle, {2, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, std::uint32_t, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::swizzle, {10, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::swizzle, {11, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::swizzle, {12, 11, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 3>>},
        {kernel_operation_t::input, {12, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::uniform, {12, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::output, {12, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::component, {12, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::construct, {12, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::equal, {12, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::not_equal, {12, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::negate, {12, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::add, {12, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::add, {12, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<std::uint32_t, 4>, std::uint32_t>},
        {kernel_operation_t::subtract, {12, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::subtract, {12, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<std::uint32_t, 4>, std::uint32_t>},
        {kernel_operation_t::divide, {12, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::multiply, {12, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::modulo, {12, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::minimum, {12, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::maximum, {12, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::clamp, {12, 12, 12}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::clamp, {12, 12, 2}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>, std::uint32_t>},
        {kernel_operation_t::divide, {12, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<std::uint32_t, 4>, std::uint32_t>},
        {kernel_operation_t::multiply, {12, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<std::uint32_t, 4>, std::uint32_t>},
        {kernel_operation_t::modulo, {12, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::modulo, shader::vector_t<std::uint32_t, 4>, std::uint32_t>},
        {kernel_operation_t::minimum, {12, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<std::uint32_t, 4>, std::uint32_t>},
        {kernel_operation_t::maximum, {12, 2, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<std::uint32_t, 4>, std::uint32_t>},
        {kernel_operation_t::clamp, {12, 2, 12}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 4>, std::uint32_t, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::clamp, {12, 2, 2}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<std::uint32_t, 4>, std::uint32_t, std::uint32_t>},
        {kernel_operation_t::swizzle, {2, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, std::uint32_t, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::swizzle, {10, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::uint32_t, 2>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::swizzle, {11, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::uint32_t, 3>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::swizzle, {12, 12, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<std::uint32_t, 4>, shader::vector_t<std::uint32_t, 4>>},
        {kernel_operation_t::input, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<float, 2>>},
        {kernel_operation_t::uniform, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<float, 2>>},
        {kernel_operation_t::output, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<float, 2>>},
        {kernel_operation_t::component, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<float, 2>>},
        {kernel_operation_t::construct, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<float, 2>>},
        {kernel_operation_t::equal, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<float, 2>>},
        {kernel_operation_t::not_equal, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<float, 2>>},
        {kernel_operation_t::negate, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::vector_t<float, 2>>},
        {kernel_operation_t::add, {13, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::add, {13, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<float, 2>, float>},
        {kernel_operation_t::subtract, {13, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::subtract, {13, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<float, 2>, float>},
        {kernel_operation_t::divide, {13, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::multiply, {13, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::minimum, {13, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::maximum, {13, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::clamp, {13, 13, 13}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 2>, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::clamp, {13, 13, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 2>, shader::vector_t<float, 2>, float>},
        {kernel_operation_t::divide, {13, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<float, 2>, float>},
        {kernel_operation_t::multiply, {13, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<float, 2>, float>},
        {kernel_operation_t::minimum, {13, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<float, 2>, float>},
        {kernel_operation_t::maximum, {13, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<float, 2>, float>},
        {kernel_operation_t::clamp, {13, 3, 13}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 2>, float, shader::vector_t<float, 2>>},
        {kernel_operation_t::clamp, {13, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 2>, float, float>},
        {kernel_operation_t::absolute, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::absolute, shader::vector_t<float, 2>>},
        {kernel_operation_t::square_root, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::square_root, shader::vector_t<float, 2>>},
        {kernel_operation_t::floor, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::floor, shader::vector_t<float, 2>>},
        {kernel_operation_t::ceil, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::ceil, shader::vector_t<float, 2>>},
        {kernel_operation_t::fract, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::fract, shader::vector_t<float, 2>>},
        {kernel_operation_t::sine, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::sine, shader::vector_t<float, 2>>},
        {kernel_operation_t::cosine, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::cosine, shader::vector_t<float, 2>>},
        {kernel_operation_t::power, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::power, shader::vector_t<float, 2>>},
        {kernel_operation_t::reflect, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::reflect, shader::vector_t<float, 2>>},
        {kernel_operation_t::mix, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::mix, shader::vector_t<float, 2>>},
        {kernel_operation_t::normalize, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::normalize, shader::vector_t<float, 2>>},
        {kernel_operation_t::length, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::length, shader::vector_t<float, 2>>},
        {kernel_operation_t::dot, {13, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::dot, shader::vector_t<float, 2>>},
        {kernel_operation_t::step, {13, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::smoothstep, {13, 13, 13}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 2>, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::smoothstep, {13, 13, 3}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 2>, shader::vector_t<float, 2>, float>},
        {kernel_operation_t::step, {3, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::vector_t<float, 2>>},
        {kernel_operation_t::smoothstep, {13, 3, 13}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 2>, float, shader::vector_t<float, 2>>},
        {kernel_operation_t::smoothstep, {13, 3, 3}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 2>, float, float>},
        {kernel_operation_t::swizzle, {3, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, float, shader::vector_t<float, 2>>},
        {kernel_operation_t::swizzle, {13, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<float, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::swizzle, {14, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<float, 3>, shader::vector_t<float, 2>>},
        {kernel_operation_t::swizzle, {15, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<float, 4>, shader::vector_t<float, 2>>},
        {kernel_operation_t::swizzle, {16, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::matrix_t<float, 2, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::input, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<float, 3>>},
        {kernel_operation_t::uniform, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<float, 3>>},
        {kernel_operation_t::output, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<float, 3>>},
        {kernel_operation_t::component, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<float, 3>>},
        {kernel_operation_t::construct, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<float, 3>>},
        {kernel_operation_t::equal, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<float, 3>>},
        {kernel_operation_t::not_equal, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<float, 3>>},
        {kernel_operation_t::negate, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::vector_t<float, 3>>},
        {kernel_operation_t::add, {14, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::add, {14, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<float, 3>, float>},
        {kernel_operation_t::subtract, {14, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::subtract, {14, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<float, 3>, float>},
        {kernel_operation_t::divide, {14, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::multiply, {14, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::minimum, {14, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::maximum, {14, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::clamp, {14, 14, 14}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 3>, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::clamp, {14, 14, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 3>, shader::vector_t<float, 3>, float>},
        {kernel_operation_t::divide, {14, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<float, 3>, float>},
        {kernel_operation_t::multiply, {14, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<float, 3>, float>},
        {kernel_operation_t::minimum, {14, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<float, 3>, float>},
        {kernel_operation_t::maximum, {14, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<float, 3>, float>},
        {kernel_operation_t::clamp, {14, 3, 14}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 3>, float, shader::vector_t<float, 3>>},
        {kernel_operation_t::clamp, {14, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 3>, float, float>},
        {kernel_operation_t::absolute, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::absolute, shader::vector_t<float, 3>>},
        {kernel_operation_t::square_root, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::square_root, shader::vector_t<float, 3>>},
        {kernel_operation_t::floor, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::floor, shader::vector_t<float, 3>>},
        {kernel_operation_t::ceil, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::ceil, shader::vector_t<float, 3>>},
        {kernel_operation_t::fract, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::fract, shader::vector_t<float, 3>>},
        {kernel_operation_t::sine, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::sine, shader::vector_t<float, 3>>},
        {kernel_operation_t::cosine, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::cosine, shader::vector_t<float, 3>>},
        {kernel_operation_t::power, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::power, shader::vector_t<float, 3>>},
        {kernel_operation_t::reflect, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::reflect, shader::vector_t<float, 3>>},
        {kernel_operation_t::mix, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::mix, shader::vector_t<float, 3>>},
        {kernel_operation_t::normalize, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::normalize, shader::vector_t<float, 3>>},
        {kernel_operation_t::length, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::length, shader::vector_t<float, 3>>},
        {kernel_operation_t::dot, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::dot, shader::vector_t<float, 3>>},
        {kernel_operation_t::cross, {14, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::cross, shader::vector_t<float, 3>>},
        {kernel_operation_t::step, {14, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::smoothstep, {14, 14, 14}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 3>, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::smoothstep, {14, 14, 3}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 3>, shader::vector_t<float, 3>, float>},
        {kernel_operation_t::step, {3, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::vector_t<float, 3>>},
        {kernel_operation_t::smoothstep, {14, 3, 14}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 3>, float, shader::vector_t<float, 3>>},
        {kernel_operation_t::smoothstep, {14, 3, 3}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 3>, float, float>},
        {kernel_operation_t::swizzle, {3, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, float, shader::vector_t<float, 3>>},
        {kernel_operation_t::swizzle, {13, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<float, 2>, shader::vector_t<float, 3>>},
        {kernel_operation_t::swizzle, {14, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<float, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::swizzle, {15, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<float, 4>, shader::vector_t<float, 3>>},
        {kernel_operation_t::swizzle, {16, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::matrix_t<float, 2, 2>, shader::vector_t<float, 3>>},
        {kernel_operation_t::input, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::vector_t<float, 4>>},
        {kernel_operation_t::uniform, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::vector_t<float, 4>>},
        {kernel_operation_t::output, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::vector_t<float, 4>>},
        {kernel_operation_t::component, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::vector_t<float, 4>>},
        {kernel_operation_t::construct, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::vector_t<float, 4>>},
        {kernel_operation_t::equal, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::vector_t<float, 4>>},
        {kernel_operation_t::not_equal, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::vector_t<float, 4>>},
        {kernel_operation_t::negate, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::vector_t<float, 4>>},
        {kernel_operation_t::add, {15, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::add, {15, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::vector_t<float, 4>, float>},
        {kernel_operation_t::subtract, {15, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::subtract, {15, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::vector_t<float, 4>, float>},
        {kernel_operation_t::divide, {15, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::multiply, {15, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::minimum, {15, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::maximum, {15, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::clamp, {15, 15, 15}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 4>, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::clamp, {15, 15, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 4>, shader::vector_t<float, 4>, float>},
        {kernel_operation_t::divide, {15, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::vector_t<float, 4>, float>},
        {kernel_operation_t::multiply, {15, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::vector_t<float, 4>, float>},
        {kernel_operation_t::minimum, {15, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::minimum, shader::vector_t<float, 4>, float>},
        {kernel_operation_t::maximum, {15, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::maximum, shader::vector_t<float, 4>, float>},
        {kernel_operation_t::clamp, {15, 3, 15}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 4>, float, shader::vector_t<float, 4>>},
        {kernel_operation_t::clamp, {15, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::vector_t<float, 4>, float, float>},
        {kernel_operation_t::absolute, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::absolute, shader::vector_t<float, 4>>},
        {kernel_operation_t::square_root, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::square_root, shader::vector_t<float, 4>>},
        {kernel_operation_t::floor, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::floor, shader::vector_t<float, 4>>},
        {kernel_operation_t::ceil, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::ceil, shader::vector_t<float, 4>>},
        {kernel_operation_t::fract, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::fract, shader::vector_t<float, 4>>},
        {kernel_operation_t::sine, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::sine, shader::vector_t<float, 4>>},
        {kernel_operation_t::cosine, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::cosine, shader::vector_t<float, 4>>},
        {kernel_operation_t::power, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::power, shader::vector_t<float, 4>>},
        {kernel_operation_t::reflect, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::reflect, shader::vector_t<float, 4>>},
        {kernel_operation_t::mix, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::mix, shader::vector_t<float, 4>>},
        {kernel_operation_t::normalize, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::normalize, shader::vector_t<float, 4>>},
        {kernel_operation_t::length, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::length, shader::vector_t<float, 4>>},
        {kernel_operation_t::dot, {15, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::dot, shader::vector_t<float, 4>>},
        {kernel_operation_t::step, {15, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::smoothstep, {15, 15, 15}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 4>, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::smoothstep, {15, 15, 3}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 4>, shader::vector_t<float, 4>, float>},
        {kernel_operation_t::step, {3, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::vector_t<float, 4>>},
        {kernel_operation_t::smoothstep, {15, 3, 15}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 4>, float, shader::vector_t<float, 4>>},
        {kernel_operation_t::smoothstep, {15, 3, 3}, evaluate_kernel<kernel_operation_t::smoothstep, shader::vector_t<float, 4>, float, float>},
        {kernel_operation_t::swizzle, {3, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, float, shader::vector_t<float, 4>>},
        {kernel_operation_t::swizzle, {13, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<float, 2>, shader::vector_t<float, 4>>},
        {kernel_operation_t::swizzle, {14, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<float, 3>, shader::vector_t<float, 4>>},
        {kernel_operation_t::swizzle, {15, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::vector_t<float, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::swizzle, {16, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::swizzle, shader::matrix_t<float, 2, 2>, shader::vector_t<float, 4>>},
        {kernel_operation_t::input, {16, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::uniform, {16, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::output, {16, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::component, {16, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::construct, {16, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::equal, {16, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::not_equal, {16, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::negate, {16, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::add, {16, 16, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::subtract, {16, 16, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::divide, {16, 16, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::clamp, {16, 16, 16}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::clamp, {16, 16, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>, float>},
        {kernel_operation_t::divide, {16, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 2, 2>, float>},
        {kernel_operation_t::multiply, {16, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::matrix_t<float, 2, 2>, float>},
        {kernel_operation_t::clamp, {16, 3, 16}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 2>, float, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::clamp, {16, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 2>, float, float>},
        {kernel_operation_t::step, {16, 16, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::step, {3, 16, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::matrix_product, {16, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::matrix_product, {16, 16, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::matrix_product, {16, 17, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::matrix_product, {16, 18, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 2>, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::input, {17, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::uniform, {17, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::output, {17, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::component, {17, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::construct, {17, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::equal, {17, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::not_equal, {17, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::negate, {17, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::add, {17, 17, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::subtract, {17, 17, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::divide, {17, 17, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::clamp, {17, 17, 17}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::clamp, {17, 17, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>, float>},
        {kernel_operation_t::divide, {17, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 2, 3>, float>},
        {kernel_operation_t::multiply, {17, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::matrix_t<float, 2, 3>, float>},
        {kernel_operation_t::clamp, {17, 3, 17}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 3>, float, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::clamp, {17, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 3>, float, float>},
        {kernel_operation_t::step, {17, 17, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::step, {3, 17, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::matrix_product, {17, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::matrix_product, {17, 19, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::matrix_product, {17, 20, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::matrix_product, {17, 21, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 3>, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::input, {18, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::uniform, {18, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::output, {18, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::component, {18, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::construct, {18, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::equal, {18, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::not_equal, {18, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::negate, {18, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::add, {18, 18, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::subtract, {18, 18, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::divide, {18, 18, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::clamp, {18, 18, 18}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::clamp, {18, 18, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>, float>},
        {kernel_operation_t::divide, {18, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 2, 4>, float>},
        {kernel_operation_t::multiply, {18, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::matrix_t<float, 2, 4>, float>},
        {kernel_operation_t::clamp, {18, 3, 18}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 4>, float, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::clamp, {18, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 2, 4>, float, float>},
        {kernel_operation_t::step, {18, 18, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::step, {3, 18, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::matrix_product, {18, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::matrix_product, {18, 22, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::matrix_product, {18, 23, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::matrix_product, {18, 24, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 2, 4>, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::input, {19, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::uniform, {19, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::output, {19, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::component, {19, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::construct, {19, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::equal, {19, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::not_equal, {19, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::negate, {19, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::add, {19, 19, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::subtract, {19, 19, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::divide, {19, 19, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::clamp, {19, 19, 19}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::clamp, {19, 19, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>, float>},
        {kernel_operation_t::divide, {19, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 3, 2>, float>},
        {kernel_operation_t::multiply, {19, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::matrix_t<float, 3, 2>, float>},
        {kernel_operation_t::clamp, {19, 3, 19}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 2>, float, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::clamp, {19, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 2>, float, float>},
        {kernel_operation_t::step, {19, 19, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::step, {3, 19, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::matrix_product, {19, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::matrix_product, {19, 16, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::matrix_product, {19, 17, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::matrix_product, {19, 18, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 2>, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::input, {20, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::uniform, {20, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::output, {20, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::component, {20, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::construct, {20, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::equal, {20, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::not_equal, {20, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::negate, {20, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::add, {20, 20, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::subtract, {20, 20, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::divide, {20, 20, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::clamp, {20, 20, 20}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::clamp, {20, 20, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>, float>},
        {kernel_operation_t::divide, {20, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 3, 3>, float>},
        {kernel_operation_t::multiply, {20, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::matrix_t<float, 3, 3>, float>},
        {kernel_operation_t::clamp, {20, 3, 20}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 3>, float, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::clamp, {20, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 3>, float, float>},
        {kernel_operation_t::step, {20, 20, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::step, {3, 20, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::matrix_product, {20, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::matrix_product, {20, 19, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::matrix_product, {20, 20, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::matrix_product, {20, 21, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 3>, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::input, {21, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::uniform, {21, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::output, {21, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::component, {21, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::construct, {21, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::equal, {21, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::not_equal, {21, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::negate, {21, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::add, {21, 21, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::subtract, {21, 21, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::divide, {21, 21, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::clamp, {21, 21, 21}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::clamp, {21, 21, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>, float>},
        {kernel_operation_t::divide, {21, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 3, 4>, float>},
        {kernel_operation_t::multiply, {21, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::matrix_t<float, 3, 4>, float>},
        {kernel_operation_t::clamp, {21, 3, 21}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 4>, float, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::clamp, {21, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 3, 4>, float, float>},
        {kernel_operation_t::step, {21, 21, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::step, {3, 21, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::matrix_product, {21, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::matrix_product, {21, 22, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::matrix_product, {21, 23, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::matrix_product, {21, 24, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 3, 4>, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::input, {22, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::uniform, {22, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::output, {22, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::component, {22, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::construct, {22, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::equal, {22, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::not_equal, {22, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::negate, {22, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::add, {22, 22, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::subtract, {22, 22, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::divide, {22, 22, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::clamp, {22, 22, 22}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::clamp, {22, 22, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>, float>},
        {kernel_operation_t::divide, {22, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 4, 2>, float>},
        {kernel_operation_t::multiply, {22, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::matrix_t<float, 4, 2>, float>},
        {kernel_operation_t::clamp, {22, 3, 22}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 2>, float, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::clamp, {22, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 2>, float, float>},
        {kernel_operation_t::step, {22, 22, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::step, {3, 22, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::matrix_product, {22, 13, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 2>, shader::vector_t<float, 2>>},
        {kernel_operation_t::matrix_product, {22, 16, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 2, 2>>},
        {kernel_operation_t::matrix_product, {22, 17, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 2, 3>>},
        {kernel_operation_t::matrix_product, {22, 18, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 2>, shader::matrix_t<float, 2, 4>>},
        {kernel_operation_t::input, {23, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::uniform, {23, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::output, {23, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::component, {23, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::construct, {23, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::equal, {23, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::not_equal, {23, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::negate, {23, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::add, {23, 23, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::subtract, {23, 23, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::divide, {23, 23, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::clamp, {23, 23, 23}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::clamp, {23, 23, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>, float>},
        {kernel_operation_t::divide, {23, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 4, 3>, float>},
        {kernel_operation_t::multiply, {23, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::matrix_t<float, 4, 3>, float>},
        {kernel_operation_t::clamp, {23, 3, 23}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 3>, float, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::clamp, {23, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 3>, float, float>},
        {kernel_operation_t::step, {23, 23, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::step, {3, 23, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::matrix_product, {23, 14, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 3>, shader::vector_t<float, 3>>},
        {kernel_operation_t::matrix_product, {23, 19, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 3, 2>>},
        {kernel_operation_t::matrix_product, {23, 20, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 3, 3>>},
        {kernel_operation_t::matrix_product, {23, 21, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 3>, shader::matrix_t<float, 3, 4>>},
        {kernel_operation_t::input, {24, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::input, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::uniform, {24, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::uniform, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::output, {24, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::output, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::component, {24, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::component, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::construct, {24, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::construct, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::equal, {24, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::equal, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::not_equal, {24, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::not_equal, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::negate, {24, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::negate, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::add, {24, 24, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::add, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::subtract, {24, 24, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::subtract, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::divide, {24, 24, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::clamp, {24, 24, 24}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::clamp, {24, 24, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>, float>},
        {kernel_operation_t::divide, {24, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::divide, shader::matrix_t<float, 4, 4>, float>},
        {kernel_operation_t::multiply, {24, 3, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::multiply, shader::matrix_t<float, 4, 4>, float>},
        {kernel_operation_t::clamp, {24, 3, 24}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 4>, float, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::clamp, {24, 3, 3}, evaluate_kernel<kernel_operation_t::clamp, shader::matrix_t<float, 4, 4>, float, float>},
        {kernel_operation_t::step, {24, 24, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::step, {3, 24, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::step, float, shader::matrix_t<float, 4, 4>>},
        {kernel_operation_t::matrix_product, {24, 15, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 4>, shader::vector_t<float, 4>>},
        {kernel_operation_t::matrix_product, {24, 22, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 2>>},
        {kernel_operation_t::matrix_product, {24, 23, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 3>>},
        {kernel_operation_t::matrix_product, {24, 24, std::numeric_limits<std::size_t>::max()}, evaluate_kernel<kernel_operation_t::matrix_product, shader::matrix_t<float, 4, 4>, shader::matrix_t<float, 4, 4>>},
    };
    return entries;
}

std::size_t value_type_index(shader::shader_data_type_t type) {
    static const shader::shader_data_type_t types[] {
        shader::shader_data_type<bool>(),
        shader::shader_data_type<std::int32_t>(),
        shader::shader_data_type<std::uint32_t>(),
        shader::shader_data_type<float>(),
        shader::shader_data_type<shader::vector_t<bool, 2>>(),
        shader::shader_data_type<shader::vector_t<bool, 3>>(),
        shader::shader_data_type<shader::vector_t<bool, 4>>(),
        shader::shader_data_type<shader::vector_t<std::int32_t, 2>>(),
        shader::shader_data_type<shader::vector_t<std::int32_t, 3>>(),
        shader::shader_data_type<shader::vector_t<std::int32_t, 4>>(),
        shader::shader_data_type<shader::vector_t<std::uint32_t, 2>>(),
        shader::shader_data_type<shader::vector_t<std::uint32_t, 3>>(),
        shader::shader_data_type<shader::vector_t<std::uint32_t, 4>>(),
        shader::shader_data_type<shader::vector_t<float, 2>>(),
        shader::shader_data_type<shader::vector_t<float, 3>>(),
        shader::shader_data_type<shader::vector_t<float, 4>>(),
        shader::shader_data_type<shader::matrix_t<float, 2, 2>>(),
        shader::shader_data_type<shader::matrix_t<float, 2, 3>>(),
        shader::shader_data_type<shader::matrix_t<float, 2, 4>>(),
        shader::shader_data_type<shader::matrix_t<float, 3, 2>>(),
        shader::shader_data_type<shader::matrix_t<float, 3, 3>>(),
        shader::shader_data_type<shader::matrix_t<float, 3, 4>>(),
        shader::shader_data_type<shader::matrix_t<float, 4, 2>>(),
        shader::shader_data_type<shader::matrix_t<float, 4, 3>>(),
        shader::shader_data_type<shader::matrix_t<float, 4, 4>>(),
    };
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

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader
