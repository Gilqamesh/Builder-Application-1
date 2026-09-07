#ifndef M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_VALUE_H
# define M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_VALUE_H

# include <m03gsy25j4v7nccgmsdov9ioft_shader/api.h>

# include <cstdint>
# include <variant>

namespace m03gt1djvvy5atia5evkbg6rqy_software_shader {

namespace shader = m03gsy25j4v7nccgmsdov9ioft_shader;

// One complete, inline shader value, shared by IO, constants, and execution slots.
using value_t = std::variant<
    bool,
    std::int32_t,
    std::uint32_t,
    float,
    shader::vector_t<bool, 2>,
    shader::vector_t<bool, 3>,
    shader::vector_t<bool, 4>,
    shader::vector_t<std::int32_t, 2>,
    shader::vector_t<std::int32_t, 3>,
    shader::vector_t<std::int32_t, 4>,
    shader::vector_t<std::uint32_t, 2>,
    shader::vector_t<std::uint32_t, 3>,
    shader::vector_t<std::uint32_t, 4>,
    shader::vector_t<float, 2>,
    shader::vector_t<float, 3>,
    shader::vector_t<float, 4>,
    shader::matrix_t<float, 2, 2>,
    shader::matrix_t<float, 2, 3>,
    shader::matrix_t<float, 2, 4>,
    shader::matrix_t<float, 3, 2>,
    shader::matrix_t<float, 3, 3>,
    shader::matrix_t<float, 3, 4>,
    shader::matrix_t<float, 4, 2>,
    shader::matrix_t<float, 4, 3>,
    shader::matrix_t<float, 4, 4>
>;

} // namespace m03gt1djvvy5atia5evkbg6rqy_software_shader

#endif // M03GT1DJVVY5ATIA5EVKBG6RQY_SOFTWARE_SHADER_VALUE_H
