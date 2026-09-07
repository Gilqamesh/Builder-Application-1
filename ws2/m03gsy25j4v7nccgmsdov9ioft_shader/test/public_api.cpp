#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;
namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;

namespace {

using vector2f_t = shader::vector_t<float, 2>;
using vector3f_t = shader::vector_t<float, 3>;
using vector4f_t = shader::vector_t<float, 4>;
using matrix2x3f_t = shader::matrix_t<float, 2, 3>;
using matrix2x4f_t = shader::matrix_t<float, 2, 4>;
using matrix3x4f_t = shader::matrix_t<float, 3, 4>;
using matrix4f_t = shader::matrix_t<float, 4, 4>;

void expect_element(
    const shader::shader_interface_element_t& element,
    std::uint32_t index,
    shader::shader_data_type_t type
) {
    test::expect(std::equal_to<>(), element.index, index);
    test::expect(std::identity(), element.type == type);
}



void test_explicit_lod_ast_validation() {
    for (int invalid = 0; invalid < 3; ++invalid) {
        shader::fragment_shader_ast_builder_t fragment;
        const auto texture = fragment.resource<shader::shader_texture_2d_t>(0);
        const auto sampler = fragment.resource<shader::shader_sampler_t>(0);
        const auto uv = fragment.constant(vector2f_t({0.5F,0.5F}));
        const auto lod = fragment.constant(std::int32_t(1));
        std::vector<const shader::shader_expression_node_t*> operands {texture.node(),sampler.node(),uv.node()};
        if (invalid != 0) { operands.push_back(invalid == 1 ? lod.node() : uv.node()); }
        const auto expression = fragment.expression<vector4f_t>(std::make_unique<shader::shader_call_node_t>(
            shader::shader_data_type<vector4f_t>(),shader::shader_call_operation_t::sample_lod,std::move(operands)));
        fragment.color(expression);
        test::expect_throws([&]{(void)std::move(fragment).finalize();});
    }
    shader::fragment_shader_ast_builder_t fragment, foreign;
    const auto texture = fragment.resource<shader::shader_texture_2d_t>(0);
    const auto sampler = fragment.resource<shader::shader_sampler_t>(0);
    test::expect_throws([&]{
        fragment.color(shader::sample_lod(texture,sampler,vector2f_t({0,0}),foreign.constant(1.0F)));
        (void)std::move(fragment).finalize();
    });
    shader::vertex_shader_ast_builder_t vertex;
    vertex.position(vector4f_t({0,0,0,1}));
    const auto image = vertex.resource<shader::shader_texture_2d_t>(0);
    const auto filtering = vertex.resource<shader::shader_sampler_t>(0);
    const auto coordinates = vertex.constant(vector2f_t({0,0}));
    vertex.output(0,shader::sample_lod(image,filtering,coordinates,vertex.constant(0.5F)));
    vertex.output(1,shader::sample_lod(image,filtering,coordinates,0.5F));
    vertex.output(2,shader::sample_lod(image,filtering,vector2f_t({0,0}),vertex.constant(0.5F)));
    vertex.output(3,shader::sample_lod(image,filtering,vector2f_t({0,0}),0.5F));
    const auto ast=std::move(vertex).finalize();
    test::expect(std::equal_to<>(),ast.interface().bindings().size(),std::size_t(2));
}

void test_interpolation_metadata() {
    for (auto mode : {shader::interpolation_t::perspective, shader::interpolation_t::noperspective, shader::interpolation_t::flat}) {
        shader::fragment_shader_ast_builder_t fragment;
        fragment.output(0, fragment.input<vector4f_t>(7, mode));
        const auto ast = std::move(fragment).finalize();
        test::expect(std::identity(), ast.interface().inputs()[0].interpolation == mode);
        test::expect(std::identity(), !std::format("{}", mode).empty());
    }
    for (auto mode : {shader::interpolation_t::noperspective, shader::interpolation_t::flat, static_cast<shader::interpolation_t>(99)}) {
        shader::vertex_shader_ast_builder_t vertex;
        vertex.position(vertex.input<vector4f_t>(0, mode));
        test::expect_throws([&] { (void)std::move(vertex).finalize(); });
    }
    shader::fragment_shader_ast_builder_t conflict;
    conflict.output(0, conflict.input<float>(3, shader::interpolation_t::flat));
    conflict.output(1, conflict.input<float>(3, shader::interpolation_t::noperspective));
    test::expect_throws([&] { (void)std::move(conflict).finalize(); });
    test::expect_throws([&] {
        (void)shader::shader_interface_t(shader::shader_stage_t::fragment,
            {{0, shader::shader_data_type<float>(), shader::interpolation_t::flat},
             {0, shader::shader_data_type<float>(), shader::interpolation_t::perspective}}, {}, {});
    });
    test::expect_throws([&] {
        (void)shader::shader_interface_t(shader::shader_stage_t::fragment,
            {{0, shader::shader_data_type<float>(), static_cast<shader::interpolation_t>(99)}}, {}, {});
    });
}

void test_repeated_branch_outputs() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector4f_t>(0);
    const auto condition = vertex.input<bool>(1);
    const auto first = vertex.input<vector4f_t>(2);
    const auto second = vertex.input<vector4f_t>(3);

    vertex.position(position);
    vertex.position(position);
    vertex.branch(
        condition,
        [&] { vertex.output(4, first); },
        [&] { vertex.output(4, second); }
    );

    const auto ast = std::move(vertex).finalize();
    const auto& interface = ast.interface();

    test::expect(std::identity(), interface.stage() == shader::shader_stage_t::vertex);
    test::expect(std::equal_to<>(), interface.inputs().size(), std::size_t(4));
    expect_element(interface.inputs()[0], 0, shader::shader_data_type<vector4f_t>());
    expect_element(interface.inputs()[1], 1, shader::shader_data_type<bool>());
    expect_element(interface.inputs()[2], 2, shader::shader_data_type<vector4f_t>());
    expect_element(interface.inputs()[3], 3, shader::shader_data_type<vector4f_t>());
    test::expect(std::equal_to<>(), interface.outputs().size(), std::size_t(1));
    expect_element(interface.outputs()[0], 4, shader::shader_data_type<vector4f_t>());
    test::expect(std::identity(), interface.bindings().empty());
}

void test_dead_expressions_are_inert() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector4f_t>(0);
    const auto lhs = vertex.constant(1.0F);
    const auto rhs = vertex.constant(2.0F);

    [[maybe_unused]] const auto invalid = vertex.expression<float>(
        std::make_unique<shader::shader_binary_node_t>(
            shader::shader_data_type<float>(),
            shader::shader_binary_operation_t::logical_and,
            lhs.node(),
            rhs.node()
        )
    );
    [[maybe_unused]] const auto dead_float = vertex.uniform<float>(7);
    [[maybe_unused]] const auto dead_integer = vertex.uniform<std::int32_t>(7);
    [[maybe_unused]] const auto dead_input = vertex.input<float>(99);
    [[maybe_unused]] const auto dead_local = vertex.expression<float>(
        std::make_unique<shader::shader_local_node_t>(shader::shader_data_type<float>())
    );

    vertex.position(position);
    const auto ast = std::move(vertex).finalize();

    test::expect(std::equal_to<>(), ast.interface().inputs().size(), std::size_t(1));
    expect_element(ast.interface().inputs()[0], 0, shader::shader_data_type<vector4f_t>());
    test::expect(std::identity(), ast.interface().bindings().empty());
}

void test_binding_namespaces() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto position = vertex.input<vector4f_t>(5);
    const auto coordinates = vertex.input<vector2f_t>(2);
    const auto uniform = vertex.uniform<float>(0);
    const auto texture = vertex.resource<shader::shader_texture_2d_t>(0);
    const auto sampler = vertex.resource<shader::shader_sampler_t>(0);

    vertex.position(position);
    vertex.output(0, uniform);
    vertex.output(1, shader::sample(texture, sampler, coordinates));

    const auto ast = std::move(vertex).finalize();
    const auto bindings = ast.interface().bindings();

    test::expect(std::equal_to<>(), bindings.size(), std::size_t(3));
    expect_element(bindings[0], 0, shader::shader_data_type<float>());
    expect_element(bindings[1], 0, shader::shader_data_type<shader::shader_texture_2d_t>());
    expect_element(bindings[2], 0, shader::shader_data_type<shader::shader_sampler_t>());
}

void test_interface_value_validation() {
    shader::shader_interface_t interface(
        shader::shader_stage_t::fragment,
        {
            {3, shader::shader_data_type<float>()},
            {1, shader::shader_data_type<bool>()},
            {3, shader::shader_data_type<float>()}
        },
        {{0, shader::shader_data_type<vector4f_t>()}},
        {
            {0, shader::shader_data_type<shader::shader_sampler_t>()},
            {0, shader::shader_data_type<float>()},
            {0, shader::shader_data_type<float>()},
            {0, shader::shader_data_type<shader::shader_texture_2d_t>()}
        }
    );

    test::expect(std::equal_to<>(), interface.inputs().size(), std::size_t(2));
    expect_element(interface.inputs()[0], 1, shader::shader_data_type<bool>());
    expect_element(interface.inputs()[1], 3, shader::shader_data_type<float>());
    test::expect(std::equal_to<>(), interface.bindings().size(), std::size_t(3));
    expect_element(interface.bindings()[0], 0, shader::shader_data_type<float>());
    expect_element(interface.bindings()[1], 0, shader::shader_data_type<shader::shader_texture_2d_t>());
    expect_element(interface.bindings()[2], 0, shader::shader_data_type<shader::shader_sampler_t>());

    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] shader::shader_interface_t invalid(
            shader::shader_stage_t::vertex,
            {{0, shader::shader_data_type<shader::shader_texture_2d_t>()}},
            {},
            {}
        );
    });
    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] shader::shader_interface_t invalid(
            shader::shader_stage_t::fragment,
            {{0, shader::shader_data_type<float>()}, {0, shader::shader_data_type<bool>()}},
            {},
            {}
        );
    });
    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] shader::shader_interface_t invalid(
            static_cast<shader::shader_stage_t>(99),
            {},
            {},
            {}
        );
    });
    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] shader::shader_interface_t invalid(
            shader::shader_stage_t::fragment,
            {},
            {{0, shader::shader_data_type<shader::shader_texture_2d_t>()}},
            {}
        );
    });
    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] shader::shader_interface_t invalid(
            shader::shader_stage_t::fragment,
            {},
            {},
            {
                {0, shader::shader_data_type<float>()},
                {0, shader::shader_data_type<std::int32_t>()}
            }
        );
    });
    test::expect_throws<std::invalid_argument>([] {
        [[maybe_unused]] shader::shader_interface_t invalid(
            shader::shader_stage_t::fragment,
            {},
            {},
            {{0, {shader::shader_data_category_t::scalar, shader::shader_scalar_type_t::none}}}
        );
    });
}

void test_local_visibility_is_checked_per_use() {
    test::expect_throws<std::invalid_argument>([] {
        shader::vertex_shader_ast_builder_t vertex;
        const auto position = vertex.input<vector4f_t>(0);
        const auto condition = vertex.input<bool>(1);
        std::optional<shader::shader_expression_t<float>> escaped;

        vertex.position(position);
        vertex.branch(condition, [&] {
            const auto local = vertex.local(1.0F);
            const auto expression = local + 1.0F;
            vertex.output(0, expression);
            escaped = expression;
        });
        vertex.output(1, *escaped);

        [[maybe_unused]] const auto ast = std::move(vertex).finalize();
    });
}

void test_incompatible_output_writes_are_rejected() {
    test::expect_throws<std::invalid_argument>([] {
        shader::vertex_shader_ast_builder_t vertex;
        vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
        vertex.output(0, 1.0F);
        vertex.output(0, std::int32_t(1));
        [[maybe_unused]] const auto ast = std::move(vertex).finalize();
    });
}

void test_fragment_color_is_a_special_output() {
    shader::fragment_shader_ast_builder_t fragment;
    fragment.color(vector4f_t({1.0F, 0.5F, 0.25F, 1.0F}));

    const auto ast = std::move(fragment).finalize();
    test::expect(std::identity(), ast.interface().outputs().empty());

    test::expect_throws<std::invalid_argument>([] {
        shader::shader_expression_nodes_t expressions;
        shader::shader_block_t root;
        auto expression = std::make_unique<shader::shader_constant_node_t>(vector4f_t({1.0F, 1.0F, 1.0F, 1.0F}));
        const auto* expression_ptr = expression.get();
        expressions.push_back(std::move(expression));
        root.statements.push_back(std::make_unique<shader::shader_output_statement_t>(shader::shader_output_t::color, 0, expression_ptr));
        [[maybe_unused]] shader::shader_ast_t ast(shader::shader_stage_t::vertex, std::move(expressions), std::move(root));
    });
}

void test_null_arena_entry_is_rejected() {
    test::expect_throws<std::invalid_argument>([] {
        shader::shader_expression_nodes_t expressions;
        expressions.push_back(nullptr);
        [[maybe_unused]] shader::shader_ast_t ast(
            shader::shader_stage_t::vertex,
            std::move(expressions),
            {}
        );
    });
}

void test_matrix_multiplication_validation() {
    test::expect_throws<std::invalid_argument>([] {
        shader::vertex_shader_ast_builder_t vertex;
        const auto lhs = vertex.input<matrix2x3f_t>(0);
        const auto rhs = vertex.input<matrix2x3f_t>(1);
        const auto product = vertex.expression<matrix2x3f_t>(
            std::make_unique<shader::shader_binary_node_t>(
                shader::shader_data_type<matrix2x3f_t>(),
                shader::shader_binary_operation_t::multiply,
                lhs.node(),
                rhs.node()
            )
        );
        vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
        vertex.output(0, product);
        [[maybe_unused]] const auto ast = std::move(vertex).finalize();
    });

    shader::vertex_shader_ast_builder_t vertex;
    const auto matrix = vertex.input<matrix2x3f_t>(0);
    const auto rhs = vertex.input<matrix3x4f_t>(1);
    const auto vector = vertex.input<vector3f_t>(2);
    const auto scalar = vertex.input<float>(3);
    vertex.position(vector4f_t({0.0F, 0.0F, 0.0F, 1.0F}));
    vertex.output(0, matrix * rhs);
    vertex.output(1, matrix * vector);
    vertex.output(2, matrix * scalar);

    const auto ast = std::move(vertex).finalize();
    const auto outputs = ast.interface().outputs();
    test::expect(std::equal_to<>(), outputs.size(), std::size_t(3));
    expect_element(outputs[0], 0, shader::shader_data_type<matrix2x4f_t>());
    expect_element(outputs[1], 1, shader::shader_data_type<vector2f_t>());
    expect_element(outputs[2], 2, shader::shader_data_type<matrix2x3f_t>());
}

void test_vertex_matrix_builtins() {
    shader::vertex_shader_ast_builder_t vertex;
    const auto local = vertex.constant(vector4f_t({1.0F, 2.0F, 3.0F, 1.0F}));
    vertex.position(vertex.world_to_clip() * vertex.object_to_world() * local);
    vertex.output(7, vertex.object_to_world());

    const auto ast = std::move(vertex).finalize();
    test::expect(std::identity(), ast.interface().inputs().empty());
    test::expect(std::identity(), ast.interface().bindings().empty());
    test::expect(std::equal_to<>(), ast.interface().outputs().size(), std::size_t(1));
    expect_element(ast.interface().outputs()[0], 7, shader::shader_data_type<matrix4f_t>());
}

} // namespace

int main() {
    return test::run([] {
        test_explicit_lod_ast_validation();
        test_interpolation_metadata();
        test_repeated_branch_outputs();
        test_dead_expressions_are_inert();
        test_binding_namespaces();
        test_interface_value_validation();
        test_local_visibility_is_checked_per_use();
        test_incompatible_output_writes_are_rejected();
        test_fragment_color_is_a_special_output();
        test_null_arena_entry_is_rejected();
        test_matrix_multiplication_validation();
        test_vertex_matrix_builtins();
    });
}
