#include "helpers.h"

#include <algorithm>
#include <bit>
#include <format>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

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
        if (output == outputs.end()) {
            throw std::invalid_argument(std::format("software shader link requires vertex output location {} with type {} for the fragment input", input.index, input.type));
        }
        if (output->type != input.type) {
            throw std::invalid_argument(std::format("software shader link has vertex output type {} at location {}; expected {} for the fragment input", output->type, input.index, input.type));
        }
    }

    std::map<std::tuple<int, std::uint32_t>, shader::shader_data_type_t> binding_types;
    const auto collect = [&binding_types](const shader::shader_interface_t& interface) {
        for (const auto& binding : interface.bindings()) {
            const auto key = std::tuple(binding_namespace(binding.type), binding.index);
            const auto [iterator, inserted] = binding_types.emplace(key, binding.type);
            if (!inserted && iterator->second != binding.type) {
                throw std::invalid_argument(std::format("software shader link has binding type {} at {}; expected {} from the other stage", binding.type, binding.index, iterator->second));
            }
        }
    };
    collect(vertex.interface());
    collect(fragment.interface());
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
    throw std::logic_error(std::format("software shader compiler could not resolve reflected location {} with type {}", index, type));
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

int binding_namespace(shader::shader_data_type_t type) {
    switch (type.category()) {
        case shader::shader_data_category_t::texture_2d: return 1;
        case shader::shader_data_category_t::sampler: return 2;
        default: return 0;
    }
}

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader
