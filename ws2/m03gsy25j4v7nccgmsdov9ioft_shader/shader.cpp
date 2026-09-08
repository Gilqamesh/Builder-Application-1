#include "shader_builder.h"
#include "helpers.h"

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace m03gsy25j4v7nccgmsdov9ioft_shader {

shader_data_type_t shader_literal_t::type() const { return m_type; }

const shader_literal_data_t& shader_literal_t::data() const { return m_data; }

std::size_t shader_literal_t::size() const {
    return std::visit([](const auto& data) -> std::size_t {
        using type_t = std::remove_cvref_t<decltype(data)>;
        if constexpr (std::same_as<type_t, shader_boolean_components_t>) {
            return data.values.size();
        } else {
            return data.size();
        }
    }, m_data);
}

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

bool shader_expression_type_matches(const shader_expression_node_t* expression, shader_data_type_t type) {
    return expression && expression->type() == type;
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

shader_data_type_t shader_expression_node_t::type() const { return m_type; }

std::span<const shader_expression_node_t* const> shader_expression_node_t::operands() const { return m_operands; }

shader_expression_node_t::shader_expression_node_t(shader_data_type_t type, std::vector<const shader_expression_node_t*> operands):
    m_type(type),
    m_operands(std::move(operands))
{
    if (std::ranges::any_of(m_operands, [](const auto* operand) { return !operand; })) {
        throw std::invalid_argument("null shader expression operand");
    }
}

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

shader_ast_t shader_ast_builder_t::finalize() && {
    if (m_current_block != &m_root || m_loop_depth) {
        throw std::logic_error("shader builder has unfinished control flow");
    }
    return {m_stage, std::move(m_expressions), std::move(m_root)};
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

shader_ast_builder_t::shader_ast_builder_t(shader_stage_t stage):
    m_stage(stage),
    m_current_block(&m_root)
{
}

bool shader_ast_builder_t::owns(const shader_expression_node_t* expression) const {
    return expression && std::ranges::any_of(m_expressions, [expression](const auto& owned) { return owned.get() == expression; });
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

shader_constant_node_t::shader_constant_node_t(shader_literal_t value):
    shader_expression_node_t(value.type()),
    m_value(std::move(value))
{
}

const shader_literal_t& shader_constant_node_t::value() const { return m_value; }

void shader_constant_node_t::accept(shader_ast_visitor_t& visitor) const { visitor.visit(*this); }

shader_input_node_t::shader_input_node_t(shader_data_type_t type, std::uint32_t location):
    shader_input_node_t(type, location, interpolation_t::perspective)
{
}

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

} // namespace m03gsy25j4v7nccgmsdov9ioft_shader
