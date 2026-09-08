#ifndef M03GSY25J4V7NCCGMSDOV9IOFT_SHADER_HELPERS_H
# define M03GSY25J4V7NCCGMSDOV9IOFT_SHADER_HELPERS_H

# include "shader_builder.h"

# include <algorithm>
# include <cstddef>
# include <cstdint>
# include <format>
# include <string_view>
# include <unordered_map>
# include <unordered_set>
# include <vector>

namespace m03gsy25j4v7nccgmsdov9ioft_shader {

[[noreturn]] void invalid(std::string_view message);

[[noreturn]] void invalid_interface(std::string_view message);

bool is_value(shader_data_type_t type);

bool is_numeric(shader_data_type_t type);

bool is_float(shader_data_type_t type);

bool is_bool(shader_data_type_t type);

bool valid(shader_data_type_t type);

std::size_t components(shader_data_type_t type);

template <typename Map>
void consistent(Map& bindings, std::uint32_t binding, shader_data_type_t type, std::string_view message);

int binding_namespace(shader_data_type_t type);

void canonicalize_locations(std::vector<shader_interface_element_t>& elements, std::string_view collection);

void canonicalize_bindings(std::vector<shader_interface_element_t>& elements);

template <typename Map>
std::vector<shader_interface_element_t> interface_elements(const Map& elements);

class shader_analyzer_t final : public shader_ast_visitor_t {
public:
    shader_analyzer_t(shader_stage_t stage, const shader_expression_nodes_t& expressions, const shader_block_t& root);
    shader_interface_t analyze();
    void visit(const shader_constant_node_t& node) override;
    void visit(const shader_input_node_t& node) override;
    void visit(const shader_uniform_node_t& node) override;
    void visit(const shader_resource_node_t& node) override;
    void visit(const shader_builtin_node_t& node) override;
    void visit(const shader_local_node_t& node) override;
    void visit(const shader_unary_node_t& node) override;
    void visit(const shader_binary_node_t& node) override;
    void visit(const shader_construct_node_t& node) override;
    void visit(const shader_swizzle_node_t& node) override;
    void visit(const shader_call_node_t& node) override;
    void visit(const shader_local_statement_t& statement) override;
    void visit(const shader_assignment_statement_t& statement) override;
    void visit(const shader_output_statement_t& statement) override;
    void visit(const shader_branch_statement_t& statement) override;
    void visit(const shader_loop_statement_t& statement) override;
    void visit(const shader_break_statement_t&) override;
    void visit(const shader_continue_statement_t&) override;
    void visit(const shader_discard_statement_t&) override;
private:
    static bool same_or_scalar(shader_data_type_t value, shader_data_type_t other);
    static bool componentwise(shader_data_type_t lhs, shader_data_type_t rhs, shader_data_type_t result, bool matrix_scalar = true);
    static bool multiply(shader_data_type_t lhs, shader_data_type_t rhs, shader_data_type_t result);
    void analyze(const shader_expression_node_t& expression);
    void validate_block(const shader_block_t& block);
    void validate_local_reads(const shader_expression_node_t& expression);
    void use(const shader_expression_node_t& expression);
    bool visible(const shader_local_node_t& local) const;
    const shader_expression_node_t& owned(const shader_expression_node_t* expression) const;
    static bool known(const shader_expression_node_t& expression);
    static bool known(const shader_statement_node_t& statement);
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
);

} // namespace m03gsy25j4v7nccgmsdov9ioft_shader

namespace std {

template <>
struct formatter<m03gsy25j4v7nccgmsdov9ioft_shader::shader_analyzer_t>;

} // namespace std

namespace m03gsy25j4v7nccgmsdov9ioft_shader {

template <typename Map>
void consistent(Map& bindings, std::uint32_t binding, shader_data_type_t type, std::string_view message) {
    const auto [iterator, inserted] = bindings.emplace(binding, type);
    if (!inserted && iterator->second != type) {
        invalid(std::format("{} at {}: actual {}, expected {}", message, binding, type, iterator->second));
    }
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

} // namespace m03gsy25j4v7nccgmsdov9ioft_shader

namespace std {

template <>
struct formatter<m03gsy25j4v7nccgmsdov9ioft_shader::shader_analyzer_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto iterator = ctx.begin();
        if (iterator != ctx.end() && *iterator != '}') {
            throw std::format_error("invalid shader_analyzer_t format specifier");
        }
        return iterator;
    }
    auto format(const m03gsy25j4v7nccgmsdov9ioft_shader::shader_analyzer_t&, auto& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "shader_analyzer_t");
        return out;
    }
};

} // namespace std

#endif // M03GSY25J4V7NCCGMSDOV9IOFT_SHADER_HELPERS_H
