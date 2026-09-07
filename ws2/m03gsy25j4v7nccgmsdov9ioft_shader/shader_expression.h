#ifndef M03GSY25J4V7NCCGMSDOV9IOFT_SHADER_SHADER_EXPRESSION_H
# define M03GSY25J4V7NCCGMSDOV9IOFT_SHADER_SHADER_EXPRESSION_H

# include <concepts>
# include <cstddef>
# include <cstdint>
# include <stdexcept>
# include <type_traits>
# include <utility>
# include <variant>
# include <vector>

# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>
# include <m03glv28yaiwc5hbnvz43r14zr_matrix/api.h>

namespace m03gsy25j4v7nccgmsdov9ioft_shader {

using m03ginwy24ng8o487c4beoms6l_vector::vector_t;
using m03glv28yaiwc5hbnvz43r14zr_matrix::matrix_t;

enum class shader_data_category_t {
    scalar,
    vector,
    matrix,
    texture_2d,
    sampler
};

enum class shader_scalar_type_t {
    none,
    boolean,
    signed_integer,
    unsigned_integer,
    floating_point
};

class shader_data_type_t {
public:
    constexpr shader_data_type_t(shader_data_category_t category, shader_scalar_type_t scalar, std::uint8_t rows = 0, std::uint8_t columns = 0):
        m_category(category),
        m_scalar(scalar),
        m_rows(rows),
        m_columns(columns)
    {
    }

    constexpr shader_data_category_t category() const { return m_category; }
    constexpr shader_scalar_type_t scalar() const { return m_scalar; }
    constexpr std::uint8_t rows() const { return m_rows; }
    constexpr std::uint8_t columns() const { return m_columns; }
    constexpr bool operator==(const shader_data_type_t&) const = default;

private:
    shader_data_category_t m_category;
    shader_scalar_type_t m_scalar;
    std::uint8_t m_rows;
    std::uint8_t m_columns;
};

struct shader_texture_2d_t {};
struct shader_sampler_t {};

template <typename T>
struct shader_type_traits {
    static constexpr bool valid = false;
    static constexpr bool scalar = false;
    static constexpr bool vector = false;
    static constexpr bool matrix = false;
    static constexpr bool value = false;
    static constexpr bool numeric = false;
    static constexpr bool integer = false;
    static constexpr bool resource = false;
};

template <typename T, shader_scalar_type_t Scalar, bool Numeric, bool Integer>
struct shader_scalar_type_traits {
    using scalar_type = T;
    static constexpr bool valid = true;
    static constexpr bool scalar = true;
    static constexpr bool vector = false;
    static constexpr bool matrix = false;
    static constexpr bool value = true;
    static constexpr bool numeric = Numeric;
    static constexpr bool integer = Integer;
    static constexpr bool resource = false;
    static constexpr shader_data_type_t data_type = {shader_data_category_t::scalar, Scalar};
};

template <>
struct shader_type_traits<bool> : shader_scalar_type_traits<bool, shader_scalar_type_t::boolean, false, false> {};
template <>
struct shader_type_traits<std::int32_t> : shader_scalar_type_traits<std::int32_t, shader_scalar_type_t::signed_integer, true, true> {};
template <>
struct shader_type_traits<std::uint32_t> : shader_scalar_type_traits<std::uint32_t, shader_scalar_type_t::unsigned_integer, true, true> {};
template <>
struct shader_type_traits<float> : shader_scalar_type_traits<float, shader_scalar_type_t::floating_point, true, false> {};

template <typename T, std::size_t N>
requires (shader_type_traits<T>::scalar && 2 <= N && N <= 4)
struct shader_type_traits<vector_t<T, N>> {
    using scalar_type = T;
    static constexpr bool valid = true;
    static constexpr bool scalar = false;
    static constexpr bool vector = true;
    static constexpr bool matrix = false;
    static constexpr bool value = true;
    static constexpr bool numeric = shader_type_traits<T>::numeric;
    static constexpr bool integer = shader_type_traits<T>::integer;
    static constexpr bool resource = false;
    static constexpr shader_data_type_t data_type = {
        shader_data_category_t::vector, shader_type_traits<T>::data_type.scalar(), static_cast<std::uint8_t>(N), 1
    };
};

template <std::size_t N, std::size_t M>
requires (2 <= N && N <= 4 && 2 <= M && M <= 4)
struct shader_type_traits<matrix_t<float, N, M>> {
    using scalar_type = float;
    static constexpr bool valid = true;
    static constexpr bool scalar = false;
    static constexpr bool vector = false;
    static constexpr bool matrix = true;
    static constexpr bool value = true;
    static constexpr bool numeric = true;
    static constexpr bool integer = false;
    static constexpr bool resource = false;
    static constexpr shader_data_type_t data_type = {
        shader_data_category_t::matrix, shader_scalar_type_t::floating_point, static_cast<std::uint8_t>(N), static_cast<std::uint8_t>(M)
    };
};

template <shader_data_category_t Category>
struct shader_resource_type_traits {
    using scalar_type = void;
    static constexpr bool valid = true;
    static constexpr bool scalar = false;
    static constexpr bool vector = false;
    static constexpr bool matrix = false;
    static constexpr bool value = false;
    static constexpr bool numeric = false;
    static constexpr bool integer = false;
    static constexpr bool resource = true;
    static constexpr shader_data_type_t data_type = {Category, shader_scalar_type_t::none};
};

template <>
struct shader_type_traits<shader_texture_2d_t> : shader_resource_type_traits<shader_data_category_t::texture_2d> {};
template <>
struct shader_type_traits<shader_sampler_t> : shader_resource_type_traits<shader_data_category_t::sampler> {};

template <typename T>
using shader_type_traits_t = shader_type_traits<std::remove_cvref_t<T>>;

template <typename T>
concept shader_type = shader_type_traits_t<T>::valid;
template <typename T>
concept shader_scalar = shader_type_traits_t<T>::scalar;
template <typename T>
concept shader_vector = shader_type_traits_t<T>::vector;
template <typename T>
concept shader_matrix = shader_type_traits_t<T>::matrix;
template <typename T>
concept shader_value = shader_type_traits_t<T>::value;
template <typename T>
concept shader_numeric_scalar = shader_type_traits_t<T>::scalar && shader_type_traits_t<T>::numeric;
template <typename T>
concept shader_numeric_vector = shader_type_traits_t<T>::vector && shader_type_traits_t<T>::numeric;
template <typename T>
concept shader_numeric_value = shader_type_traits_t<T>::value && shader_type_traits_t<T>::numeric;
template <typename T>
concept shader_integer_scalar = shader_type_traits_t<T>::scalar && shader_type_traits_t<T>::integer;
template <typename T>
concept shader_integer_vector = shader_type_traits_t<T>::vector && shader_type_traits_t<T>::integer;
template <typename T>
concept shader_integer_value = shader_type_traits_t<T>::value && shader_type_traits_t<T>::integer;
template <typename T>
concept shader_resource = shader_type_traits_t<T>::resource;

class shader_ast_builder_t;
class shader_expression_node_t;
class shader_local_node_t;

enum class shader_unary_operation_t { negate, logical_not, absolute, square_root, floor, ceil, fract, sine, cosine, normalize, length };
enum class shader_binary_operation_t {
    add, subtract, multiply, divide, modulo,
    equal, not_equal, less, less_equal, greater, greater_equal,
    logical_and, logical_or, dot, cross, power, reflect, minimum, maximum, step
};
enum class shader_call_operation_t { clamp, mix, smoothstep, sample, sample_lod };

template <shader_type T>
constexpr shader_data_type_t shader_data_type();

struct shader_boolean_components_t {
    std::vector<std::uint8_t> values;
};

using shader_literal_data_t = std::variant<shader_boolean_components_t, std::vector<std::int32_t>, std::vector<std::uint32_t>, std::vector<float>>;

class shader_literal_t {
public:
    template <shader_value T>
    explicit shader_literal_t(T value);

    shader_data_type_t type() const;
    const shader_literal_data_t& data() const;
    std::size_t size() const;

private:
    shader_data_type_t m_type;
    shader_literal_data_t m_data;
};

template <shader_type T>
class shader_expression_t {
public:
    using value_type = T;

public:
    shader_expression_t(shader_ast_builder_t* builder, const shader_expression_node_t* node);

    shader_ast_builder_t* builder() const;
    const shader_expression_node_t* node() const;

private:
    shader_ast_builder_t* m_builder;
    const shader_expression_node_t* m_node;
};

template <shader_value T>
class shader_local_t : public shader_expression_t<T> {
public:
    shader_local_t(shader_ast_builder_t* builder, const shader_local_node_t* node);
    const shader_local_node_t* node() const;

private:
    const shader_local_node_t* m_local;
};

template <typename T>
struct shader_operand_traits {
    using type = std::remove_cvref_t<T>;
    static constexpr bool valid = shader_value<type>;
    static constexpr bool handle = false;
};

template <shader_type T>
struct shader_operand_traits<shader_expression_t<T>> {
    using type = T;
    static constexpr bool valid = shader_value<T>;
    static constexpr bool handle = true;
};

template <shader_value T>
struct shader_operand_traits<shader_local_t<T>> {
    using type = T;
    static constexpr bool valid = true;
    static constexpr bool handle = true;
};

template <typename T>
using shader_operand_type_t = typename shader_operand_traits<std::remove_cvref_t<T>>::type;

template <typename T>
concept shader_operand = shader_operand_traits<std::remove_cvref_t<T>>::valid;

template <typename T>
concept shader_handle_operand = shader_operand<T> && shader_operand_traits<std::remove_cvref_t<T>>::handle;

template <typename L, typename R>
concept shader_binary_operands = shader_operand<L> && shader_operand<R> && (shader_handle_operand<L> || shader_handle_operand<R>);

template <shader_binary_operation_t Operation, typename L, typename R>
struct shader_binary_result;

template <shader_binary_operation_t Operation, shader_numeric_scalar T>
requires (
    Operation == shader_binary_operation_t::add || Operation == shader_binary_operation_t::subtract ||
    Operation == shader_binary_operation_t::multiply || Operation == shader_binary_operation_t::divide ||
    (Operation == shader_binary_operation_t::modulo && shader_integer_scalar<T>)
)
struct shader_binary_result<Operation, T, T> { using type = T; };

template <shader_binary_operation_t Operation, shader_numeric_scalar T, std::size_t N>
requires (
    2 <= N && N <= 4 &&
    (Operation == shader_binary_operation_t::add || Operation == shader_binary_operation_t::subtract ||
     Operation == shader_binary_operation_t::multiply || Operation == shader_binary_operation_t::divide ||
     (Operation == shader_binary_operation_t::modulo && shader_integer_scalar<T>))
)
struct shader_binary_result<Operation, vector_t<T, N>, vector_t<T, N>> { using type = vector_t<T, N>; };

template <shader_binary_operation_t Operation, shader_numeric_scalar T, std::size_t N>
requires (
    2 <= N && N <= 4 &&
    (Operation == shader_binary_operation_t::add || Operation == shader_binary_operation_t::subtract ||
     Operation == shader_binary_operation_t::multiply || Operation == shader_binary_operation_t::divide ||
     (Operation == shader_binary_operation_t::modulo && shader_integer_scalar<T>))
)
struct shader_binary_result<Operation, vector_t<T, N>, T> { using type = vector_t<T, N>; };

template <shader_binary_operation_t Operation, std::size_t N, std::size_t M>
requires (
    2 <= N && N <= 4 && 2 <= M && M <= 4 &&
    (Operation == shader_binary_operation_t::add || Operation == shader_binary_operation_t::subtract)
)
struct shader_binary_result<Operation, matrix_t<float, N, M>, matrix_t<float, N, M>> { using type = matrix_t<float, N, M>; };

template <shader_binary_operation_t Operation, std::size_t N, std::size_t M>
requires (
    2 <= N && N <= 4 && 2 <= M && M <= 4 &&
    (Operation == shader_binary_operation_t::multiply || Operation == shader_binary_operation_t::divide)
)
struct shader_binary_result<Operation, matrix_t<float, N, M>, float> { using type = matrix_t<float, N, M>; };

template <std::size_t N, std::size_t M>
requires (2 <= N && N <= 4 && 2 <= M && M <= 4)
struct shader_binary_result<shader_binary_operation_t::multiply, matrix_t<float, N, M>, vector_t<float, M>> { using type = vector_t<float, N>; };

template <std::size_t N, std::size_t M, std::size_t P>
requires (2 <= N && N <= 4 && 2 <= M && M <= 4 && 2 <= P && P <= 4)
struct shader_binary_result<shader_binary_operation_t::multiply, matrix_t<float, N, M>, matrix_t<float, M, P>> { using type = matrix_t<float, N, P>; };

template <shader_binary_operation_t Operation, typename L, typename R>
using shader_binary_result_t = typename shader_binary_result<Operation, shader_operand_type_t<L>, shader_operand_type_t<R>>::type;

template <shader_binary_operation_t Operation, typename L, typename R>
concept shader_binary_operable = shader_binary_operands<L, R> && requires {
    typename shader_binary_result<Operation, shader_operand_type_t<L>, shader_operand_type_t<R>>::type;
};

template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::add, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::add, L, R>> operator+(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::subtract, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::subtract, L, R>> operator-(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::multiply, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::multiply, L, R>> operator*(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::divide, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::divide, L, R>> operator/(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::modulo, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::modulo, L, R>> operator%(L&& lhs, R&& rhs);

template <shader_handle_operand T>
requires (shader_numeric_value<shader_operand_type_t<T>>)
shader_expression_t<shader_operand_type_t<T>> operator-(T&& expression);

shader_expression_t<bool> operator!(shader_expression_t<bool> expression);

template <shader_operand L, shader_operand R>
requires ((shader_handle_operand<L> || shader_handle_operand<R>) && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>)
shader_expression_t<bool> operator==(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires ((shader_handle_operand<L> || shader_handle_operand<R>) && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>)
shader_expression_t<bool> operator!=(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires (
    (shader_handle_operand<L> || shader_handle_operand<R>) &&
    shader_numeric_scalar<shader_operand_type_t<L>> && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>
)
shader_expression_t<bool> operator<(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires (
    (shader_handle_operand<L> || shader_handle_operand<R>) &&
    shader_numeric_scalar<shader_operand_type_t<L>> && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>
)
shader_expression_t<bool> operator<=(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires (
    (shader_handle_operand<L> || shader_handle_operand<R>) &&
    shader_numeric_scalar<shader_operand_type_t<L>> && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>
)
shader_expression_t<bool> operator>(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires (
    (shader_handle_operand<L> || shader_handle_operand<R>) &&
    shader_numeric_scalar<shader_operand_type_t<L>> && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>
)
shader_expression_t<bool> operator>=(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires ((shader_handle_operand<L> || shader_handle_operand<R>) && std::same_as<shader_operand_type_t<L>, bool> && std::same_as<shader_operand_type_t<R>, bool>)
shader_expression_t<bool> operator&&(L&& lhs, R&& rhs);
template <shader_operand L, shader_operand R>
requires ((shader_handle_operand<L> || shader_handle_operand<R>) && std::same_as<shader_operand_type_t<L>, bool> && std::same_as<shader_operand_type_t<R>, bool>)
shader_expression_t<bool> operator||(L&& lhs, R&& rhs);

template <std::size_t N>
shader_expression_t<float> dot(shader_expression_t<vector_t<float, N>> lhs, shader_expression_t<vector_t<float, N>> rhs);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> normalize(shader_expression_t<vector_t<float, N>> expression);
template <std::size_t N>
shader_expression_t<float> length(shader_expression_t<vector_t<float, N>> expression);
shader_expression_t<vector_t<float, 3>> cross(shader_expression_t<vector_t<float, 3>> lhs, shader_expression_t<vector_t<float, 3>> rhs);

shader_expression_t<float> abs(shader_expression_t<float> expression);
shader_expression_t<std::int32_t> abs(shader_expression_t<std::int32_t> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> abs(shader_expression_t<vector_t<float, N>> expression);
template <std::size_t N>
shader_expression_t<vector_t<std::int32_t, N>> abs(shader_expression_t<vector_t<std::int32_t, N>> expression);
shader_expression_t<float> sqrt(shader_expression_t<float> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> sqrt(shader_expression_t<vector_t<float, N>> expression);
shader_expression_t<float> pow(shader_expression_t<float> base, shader_expression_t<float> exponent);
shader_expression_t<float> pow(shader_expression_t<float> base, float exponent);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> pow(shader_expression_t<vector_t<float, N>> base, shader_expression_t<vector_t<float, N>> exponent);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> pow(shader_expression_t<vector_t<float, N>> base, vector_t<float, N> exponent);
shader_expression_t<float> floor(shader_expression_t<float> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> floor(shader_expression_t<vector_t<float, N>> expression);
shader_expression_t<float> ceil(shader_expression_t<float> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> ceil(shader_expression_t<vector_t<float, N>> expression);
shader_expression_t<float> fract(shader_expression_t<float> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> fract(shader_expression_t<vector_t<float, N>> expression);
shader_expression_t<float> sin(shader_expression_t<float> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> sin(shader_expression_t<vector_t<float, N>> expression);
shader_expression_t<float> cos(shader_expression_t<float> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> cos(shader_expression_t<vector_t<float, N>> expression);
shader_expression_t<float> reflect(shader_expression_t<float> incident, shader_expression_t<float> normal);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> reflect(shader_expression_t<vector_t<float, N>> incident, shader_expression_t<vector_t<float, N>> normal);
shader_expression_t<float> step(shader_expression_t<float> edge, shader_expression_t<float> expression);
shader_expression_t<float> step(float edge, shader_expression_t<float> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> step(shader_expression_t<vector_t<float, N>> edge, shader_expression_t<vector_t<float, N>> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> step(shader_expression_t<float> edge, shader_expression_t<vector_t<float, N>> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> step(float edge, shader_expression_t<vector_t<float, N>> expression);
shader_expression_t<float> smoothstep(shader_expression_t<float> edge0, shader_expression_t<float> edge1, shader_expression_t<float> expression);
shader_expression_t<float> smoothstep(float edge0, float edge1, shader_expression_t<float> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> smoothstep(
    shader_expression_t<vector_t<float, N>> edge0, shader_expression_t<vector_t<float, N>> edge1,
    shader_expression_t<vector_t<float, N>> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> smoothstep(shader_expression_t<float> edge0, shader_expression_t<float> edge1, shader_expression_t<vector_t<float, N>> expression);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> smoothstep(float edge0, float edge1, shader_expression_t<vector_t<float, N>> expression);

template <shader_numeric_scalar T>
shader_expression_t<T> min(shader_expression_t<T> lhs, shader_expression_t<T> rhs);
template <shader_numeric_scalar T>
shader_expression_t<T> min(shader_expression_t<T> lhs, T rhs);
template <shader_numeric_scalar T>
shader_expression_t<T> max(shader_expression_t<T> lhs, shader_expression_t<T> rhs);
template <shader_numeric_scalar T>
shader_expression_t<T> max(shader_expression_t<T> lhs, T rhs);
template <shader_numeric_scalar T>
shader_expression_t<T> clamp(shader_expression_t<T> expression, shader_expression_t<T> minimum, shader_expression_t<T> maximum);
template <shader_numeric_scalar T>
shader_expression_t<T> clamp(shader_expression_t<T> expression, T minimum, T maximum);
shader_expression_t<float> mix(shader_expression_t<float> lhs, shader_expression_t<float> rhs, shader_expression_t<float> factor);
shader_expression_t<float> mix(shader_expression_t<float> lhs, shader_expression_t<float> rhs, float factor);

template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> min(shader_expression_t<vector_t<T, N>> lhs, shader_expression_t<vector_t<T, N>> rhs);
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> min(shader_expression_t<vector_t<T, N>> lhs, T rhs);
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> max(shader_expression_t<vector_t<T, N>> lhs, shader_expression_t<vector_t<T, N>> rhs);
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> max(shader_expression_t<vector_t<T, N>> lhs, T rhs);
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> clamp(shader_expression_t<vector_t<T, N>> expression, shader_expression_t<vector_t<T, N>> minimum, shader_expression_t<vector_t<T, N>> maximum);
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> clamp(shader_expression_t<vector_t<T, N>> expression, T minimum, T maximum);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> mix(shader_expression_t<vector_t<float, N>> lhs, shader_expression_t<vector_t<float, N>> rhs, shader_expression_t<float> factor);
template <std::size_t N>
shader_expression_t<vector_t<float, N>> mix(shader_expression_t<vector_t<float, N>> lhs, shader_expression_t<vector_t<float, N>> rhs, float factor);

template <std::size_t... Components, shader_scalar T, std::size_t N>
requires (1 <= sizeof...(Components) && sizeof...(Components) <= 4 && ((Components < N) && ...))
auto swizzle(shader_expression_t<vector_t<T, N>> expression) -> shader_expression_t<std::conditional_t<sizeof...(Components) == 1, T, vector_t<T, sizeof...(Components)>>>;

shader_expression_t<vector_t<float, 4>> sample(
    shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler,
    shader_expression_t<vector_t<float, 2>> coordinates);
shader_expression_t<vector_t<float, 4>> sample(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, vector_t<float, 2> coordinates);

/** @brief Samples an explicit LOD in either shader stage; filtering and numeric validity belong to the sampler. */
shader_expression_t<vector_t<float, 4>> sample_lod(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, shader_expression_t<vector_t<float, 2>> coordinates, shader_expression_t<float> lod);
shader_expression_t<vector_t<float, 4>> sample_lod(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, shader_expression_t<vector_t<float, 2>> coordinates, float lod);
shader_expression_t<vector_t<float, 4>> sample_lod(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, vector_t<float, 2> coordinates, shader_expression_t<float> lod);
shader_expression_t<vector_t<float, 4>> sample_lod(shader_expression_t<shader_texture_2d_t> texture, shader_expression_t<shader_sampler_t> sampler, vector_t<float, 2> coordinates, float lod);

bool shader_expression_type_matches(const shader_expression_node_t* expression, shader_data_type_t type);
const shader_expression_node_t* shader_constant_expression(shader_ast_builder_t* builder, shader_literal_t literal);
const shader_expression_node_t* shader_unary_expression(
    shader_ast_builder_t* builder, shader_data_type_t type, shader_unary_operation_t operation,
    const shader_expression_node_t* expression);
const shader_expression_node_t* shader_binary_expression(
    shader_ast_builder_t* builder, shader_data_type_t type, shader_binary_operation_t operation,
    const shader_expression_node_t* lhs, const shader_expression_node_t* rhs);
const shader_expression_node_t* shader_call_expression(
    shader_ast_builder_t* builder, shader_data_type_t type, shader_call_operation_t operation,
    std::vector<const shader_expression_node_t*> arguments);
const shader_expression_node_t* shader_swizzle_expression(
    shader_ast_builder_t* builder, shader_data_type_t type, const shader_expression_node_t* expression,
    std::vector<std::uint8_t> components);

} // namespace m03gsy25j4v7nccgmsdov9ioft_shader

namespace m03gsy25j4v7nccgmsdov9ioft_shader {

template <shader_type T>
constexpr shader_data_type_t shader_data_type() {
    return shader_type_traits_t<T>::data_type;
}

template <shader_value T>
shader_literal_t::shader_literal_t(T value):
    m_type(shader_data_type<T>())
{
    using type = std::remove_cvref_t<T>;
    using scalar_type = typename shader_type_traits_t<type>::scalar_type;
    if constexpr (shader_scalar<type>) {
        if constexpr (std::same_as<scalar_type, bool>) {
            m_data = shader_boolean_components_t{{static_cast<std::uint8_t>(value)}};
        } else {
            m_data = std::vector<scalar_type>{value};
        }
    } else if constexpr (std::same_as<scalar_type, bool>) {
        shader_boolean_components_t components;
        for (bool component : value) {
            components.values.push_back(static_cast<std::uint8_t>(component));
        }
        m_data = std::move(components);
    } else {
        m_data = std::vector<scalar_type>(value.begin(), value.end());
    }
}

template <shader_type T>
shader_expression_t<T>::shader_expression_t(shader_ast_builder_t* builder, const shader_expression_node_t* node):
    m_builder(builder),
    m_node(node)
{
    if (!builder || !shader_expression_type_matches(node, shader_data_type<T>())) {
        throw std::invalid_argument("invalid shader expression handle");
    }
}

template <shader_type T>
shader_ast_builder_t* shader_expression_t<T>::builder() const {
    return m_builder;
}

template <shader_type T>
const shader_expression_node_t* shader_expression_t<T>::node() const {
    return m_node;
}

template <shader_value T>
const shader_local_node_t* shader_local_t<T>::node() const {
    return m_local;
}

template <shader_operand T>
shader_ast_builder_t* shader_builder_for(T&& operand) {
    if constexpr (shader_handle_operand<T>) return operand.builder();
    else return nullptr;
}

template <shader_operand L, shader_operand R>
shader_ast_builder_t& shader_builder_for(L&& lhs, R&& rhs) {
    auto* builder = shader_builder_for(lhs);
    if (!builder) {
        builder = shader_builder_for(rhs);
    }
    if (!builder) {
        throw std::invalid_argument("shader operation has no expression operand");
    }
    return *builder;
}

template <shader_operand T>
const shader_expression_node_t* shader_build_operand(shader_ast_builder_t& builder, T&& operand) {
    if constexpr (shader_handle_operand<T>) {
        if (operand.builder() != &builder) {
            throw std::invalid_argument("shader operands belong to different builders");
        }
        return operand.node();
    } else {
        return shader_constant_expression(&builder, shader_literal_t(std::forward<T>(operand)));
    }
}

template <shader_type T, shader_operand E>
shader_expression_t<T> shader_unary(shader_unary_operation_t operation, E&& expression) {
    auto* builder = shader_builder_for(expression);
    if (!builder) {
        throw std::invalid_argument("shader operation has no expression operand");
    }
    const auto* value = shader_build_operand(*builder, std::forward<E>(expression));
    return {builder, shader_unary_expression(builder, shader_data_type<T>(), operation, value)};
}

template <shader_type T, shader_operand L, shader_operand R>
shader_expression_t<T> shader_binary(shader_binary_operation_t operation, L&& lhs, R&& rhs) {
    auto& builder = shader_builder_for(lhs, rhs);
    const auto* left = shader_build_operand(builder, std::forward<L>(lhs));
    const auto* right = shader_build_operand(builder, std::forward<R>(rhs));
    return {&builder, shader_binary_expression(&builder, shader_data_type<T>(), operation, left, right)};
}

template <shader_type T, shader_operand... Ts>
shader_expression_t<T> shader_call(shader_call_operation_t operation, Ts&&... arguments) {
    shader_ast_builder_t* builder = nullptr;
    ((builder = builder ? builder : shader_builder_for(arguments)), ...);
    if (!builder) {
        throw std::invalid_argument("shader call has no expression argument");
    }
    std::vector<const shader_expression_node_t*> nodes;
    nodes.reserve(sizeof...(Ts));
    (nodes.push_back(shader_build_operand(*builder, std::forward<Ts>(arguments))), ...);
    return {builder, shader_call_expression(builder, shader_data_type<T>(), operation, std::move(nodes))};
}

template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::add, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::add, L, R>> operator+(L&& lhs, R&& rhs) { return shader_binary<shader_binary_result_t<shader_binary_operation_t::add, L, R>>(shader_binary_operation_t::add, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::subtract, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::subtract, L, R>> operator-(L&& lhs, R&& rhs) { return shader_binary<shader_binary_result_t<shader_binary_operation_t::subtract, L, R>>(shader_binary_operation_t::subtract, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::multiply, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::multiply, L, R>> operator*(L&& lhs, R&& rhs) { return shader_binary<shader_binary_result_t<shader_binary_operation_t::multiply, L, R>>(shader_binary_operation_t::multiply, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::divide, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::divide, L, R>> operator/(L&& lhs, R&& rhs) { return shader_binary<shader_binary_result_t<shader_binary_operation_t::divide, L, R>>(shader_binary_operation_t::divide, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires (shader_binary_operable<shader_binary_operation_t::modulo, L, R>)
shader_expression_t<shader_binary_result_t<shader_binary_operation_t::modulo, L, R>> operator%(L&& lhs, R&& rhs) { return shader_binary<shader_binary_result_t<shader_binary_operation_t::modulo, L, R>>(shader_binary_operation_t::modulo, std::forward<L>(lhs), std::forward<R>(rhs)); }

template <shader_handle_operand T>
requires (shader_numeric_value<shader_operand_type_t<T>>)
shader_expression_t<shader_operand_type_t<T>> operator-(T&& expression) { return shader_unary<shader_operand_type_t<T>>(shader_unary_operation_t::negate, std::forward<T>(expression)); }

template <shader_operand L, shader_operand R>
requires ((shader_handle_operand<L> || shader_handle_operand<R>) && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>)
shader_expression_t<bool> operator==(L&& lhs, R&& rhs) { return shader_binary<bool>(shader_binary_operation_t::equal, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires ((shader_handle_operand<L> || shader_handle_operand<R>) && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>)
shader_expression_t<bool> operator!=(L&& lhs, R&& rhs) { return shader_binary<bool>(shader_binary_operation_t::not_equal, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires (
    (shader_handle_operand<L> || shader_handle_operand<R>) &&
    shader_numeric_scalar<shader_operand_type_t<L>> && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>
)
shader_expression_t<bool> operator<(L&& lhs, R&& rhs) { return shader_binary<bool>(shader_binary_operation_t::less, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires (
    (shader_handle_operand<L> || shader_handle_operand<R>) &&
    shader_numeric_scalar<shader_operand_type_t<L>> && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>
)
shader_expression_t<bool> operator<=(L&& lhs, R&& rhs) { return shader_binary<bool>(shader_binary_operation_t::less_equal, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires (
    (shader_handle_operand<L> || shader_handle_operand<R>) &&
    shader_numeric_scalar<shader_operand_type_t<L>> && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>
)
shader_expression_t<bool> operator>(L&& lhs, R&& rhs) { return shader_binary<bool>(shader_binary_operation_t::greater, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires (
    (shader_handle_operand<L> || shader_handle_operand<R>) &&
    shader_numeric_scalar<shader_operand_type_t<L>> && std::same_as<shader_operand_type_t<L>, shader_operand_type_t<R>>
)
shader_expression_t<bool> operator>=(L&& lhs, R&& rhs) { return shader_binary<bool>(shader_binary_operation_t::greater_equal, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires ((shader_handle_operand<L> || shader_handle_operand<R>) && std::same_as<shader_operand_type_t<L>, bool> && std::same_as<shader_operand_type_t<R>, bool>)
shader_expression_t<bool> operator&&(L&& lhs, R&& rhs) { return shader_binary<bool>(shader_binary_operation_t::logical_and, std::forward<L>(lhs), std::forward<R>(rhs)); }
template <shader_operand L, shader_operand R>
requires ((shader_handle_operand<L> || shader_handle_operand<R>) && std::same_as<shader_operand_type_t<L>, bool> && std::same_as<shader_operand_type_t<R>, bool>)
shader_expression_t<bool> operator||(L&& lhs, R&& rhs) { return shader_binary<bool>(shader_binary_operation_t::logical_or, std::forward<L>(lhs), std::forward<R>(rhs)); }

template <std::size_t N>
shader_expression_t<float> dot(shader_expression_t<vector_t<float, N>> lhs, shader_expression_t<vector_t<float, N>> rhs) { return shader_binary<float>(shader_binary_operation_t::dot, lhs, rhs); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> normalize(shader_expression_t<vector_t<float, N>> expression) { return shader_unary<vector_t<float, N>>(shader_unary_operation_t::normalize, expression); }
template <std::size_t N>
shader_expression_t<float> length(shader_expression_t<vector_t<float, N>> expression) { return shader_unary<float>(shader_unary_operation_t::length, expression); }

template <std::size_t N>
shader_expression_t<vector_t<float, N>> abs(shader_expression_t<vector_t<float, N>> expression) { return shader_unary<vector_t<float, N>>(shader_unary_operation_t::absolute, expression); }
template <std::size_t N>
shader_expression_t<vector_t<std::int32_t, N>> abs(shader_expression_t<vector_t<std::int32_t, N>> expression) { return shader_unary<vector_t<std::int32_t, N>>(shader_unary_operation_t::absolute, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> sqrt(shader_expression_t<vector_t<float, N>> expression) { return shader_unary<vector_t<float, N>>(shader_unary_operation_t::square_root, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> pow(shader_expression_t<vector_t<float, N>> base, shader_expression_t<vector_t<float, N>> exponent) { return shader_binary<vector_t<float, N>>(shader_binary_operation_t::power, base, exponent); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> pow(shader_expression_t<vector_t<float, N>> base, vector_t<float, N> exponent) { return shader_binary<vector_t<float, N>>(shader_binary_operation_t::power, base, std::move(exponent)); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> floor(shader_expression_t<vector_t<float, N>> expression) { return shader_unary<vector_t<float, N>>(shader_unary_operation_t::floor, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> ceil(shader_expression_t<vector_t<float, N>> expression) { return shader_unary<vector_t<float, N>>(shader_unary_operation_t::ceil, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> fract(shader_expression_t<vector_t<float, N>> expression) { return shader_unary<vector_t<float, N>>(shader_unary_operation_t::fract, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> sin(shader_expression_t<vector_t<float, N>> expression) { return shader_unary<vector_t<float, N>>(shader_unary_operation_t::sine, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> cos(shader_expression_t<vector_t<float, N>> expression) { return shader_unary<vector_t<float, N>>(shader_unary_operation_t::cosine, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> reflect(shader_expression_t<vector_t<float, N>> incident, shader_expression_t<vector_t<float, N>> normal) { return shader_binary<vector_t<float, N>>(shader_binary_operation_t::reflect, incident, normal); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> step(shader_expression_t<vector_t<float, N>> edge, shader_expression_t<vector_t<float, N>> expression) { return shader_binary<vector_t<float, N>>(shader_binary_operation_t::step, edge, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> step(shader_expression_t<float> edge, shader_expression_t<vector_t<float, N>> expression) { return shader_binary<vector_t<float, N>>(shader_binary_operation_t::step, edge, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> step(float edge, shader_expression_t<vector_t<float, N>> expression) { return shader_binary<vector_t<float, N>>(shader_binary_operation_t::step, edge, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> smoothstep(shader_expression_t<vector_t<float, N>> edge0, shader_expression_t<vector_t<float, N>> edge1, shader_expression_t<vector_t<float, N>> expression) { return shader_call<vector_t<float, N>>(shader_call_operation_t::smoothstep, edge0, edge1, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> smoothstep(shader_expression_t<float> edge0, shader_expression_t<float> edge1, shader_expression_t<vector_t<float, N>> expression) { return shader_call<vector_t<float, N>>(shader_call_operation_t::smoothstep, edge0, edge1, expression); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> smoothstep(float edge0, float edge1, shader_expression_t<vector_t<float, N>> expression) { return shader_call<vector_t<float, N>>(shader_call_operation_t::smoothstep, edge0, edge1, expression); }

template <shader_numeric_scalar T>
shader_expression_t<T> min(shader_expression_t<T> lhs, shader_expression_t<T> rhs) { return shader_binary<T>(shader_binary_operation_t::minimum, lhs, rhs); }
template <shader_numeric_scalar T>
shader_expression_t<T> min(shader_expression_t<T> lhs, T rhs) { return shader_binary<T>(shader_binary_operation_t::minimum, lhs, rhs); }
template <shader_numeric_scalar T>
shader_expression_t<T> max(shader_expression_t<T> lhs, shader_expression_t<T> rhs) { return shader_binary<T>(shader_binary_operation_t::maximum, lhs, rhs); }
template <shader_numeric_scalar T>
shader_expression_t<T> max(shader_expression_t<T> lhs, T rhs) { return shader_binary<T>(shader_binary_operation_t::maximum, lhs, rhs); }
template <shader_numeric_scalar T>
shader_expression_t<T> clamp(shader_expression_t<T> expression, shader_expression_t<T> minimum, shader_expression_t<T> maximum) { return shader_call<T>(shader_call_operation_t::clamp, expression, minimum, maximum); }
template <shader_numeric_scalar T>
shader_expression_t<T> clamp(shader_expression_t<T> expression, T minimum, T maximum) { return shader_call<T>(shader_call_operation_t::clamp, expression, minimum, maximum); }

template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> min(shader_expression_t<vector_t<T, N>> lhs, shader_expression_t<vector_t<T, N>> rhs) { return shader_binary<vector_t<T, N>>(shader_binary_operation_t::minimum, lhs, rhs); }
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> min(shader_expression_t<vector_t<T, N>> lhs, T rhs) { return shader_binary<vector_t<T, N>>(shader_binary_operation_t::minimum, lhs, rhs); }
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> max(shader_expression_t<vector_t<T, N>> lhs, shader_expression_t<vector_t<T, N>> rhs) { return shader_binary<vector_t<T, N>>(shader_binary_operation_t::maximum, lhs, rhs); }
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> max(shader_expression_t<vector_t<T, N>> lhs, T rhs) { return shader_binary<vector_t<T, N>>(shader_binary_operation_t::maximum, lhs, rhs); }
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> clamp(shader_expression_t<vector_t<T, N>> expression, shader_expression_t<vector_t<T, N>> minimum, shader_expression_t<vector_t<T, N>> maximum) { return shader_call<vector_t<T, N>>(shader_call_operation_t::clamp, expression, minimum, maximum); }
template <shader_numeric_scalar T, std::size_t N>
shader_expression_t<vector_t<T, N>> clamp(shader_expression_t<vector_t<T, N>> expression, T minimum, T maximum) { return shader_call<vector_t<T, N>>(shader_call_operation_t::clamp, expression, minimum, maximum); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> mix(shader_expression_t<vector_t<float, N>> lhs, shader_expression_t<vector_t<float, N>> rhs, shader_expression_t<float> factor) { return shader_call<vector_t<float, N>>(shader_call_operation_t::mix, lhs, rhs, factor); }
template <std::size_t N>
shader_expression_t<vector_t<float, N>> mix(shader_expression_t<vector_t<float, N>> lhs, shader_expression_t<vector_t<float, N>> rhs, float factor) { return shader_call<vector_t<float, N>>(shader_call_operation_t::mix, lhs, rhs, factor); }

template <std::size_t... Components, shader_scalar T, std::size_t N>
requires (1 <= sizeof...(Components) && sizeof...(Components) <= 4 && ((Components < N) && ...))
auto swizzle(shader_expression_t<vector_t<T, N>> expression) -> shader_expression_t<std::conditional_t<sizeof...(Components) == 1, T, vector_t<T, sizeof...(Components)>>> {
    using result_type = std::conditional_t<sizeof...(Components) == 1, T, vector_t<T, sizeof...(Components)>>;
    return {expression.builder(), shader_swizzle_expression(expression.builder(), shader_data_type<result_type>(), expression.node(), {static_cast<std::uint8_t>(Components)...})};
}

} // namespace m03gsy25j4v7nccgmsdov9ioft_shader

#endif // M03GSY25J4V7NCCGMSDOV9IOFT_SHADER_SHADER_EXPRESSION_H
