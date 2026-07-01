#ifndef M03GE9IJ4C25HDGT1OHRYQ3FCP_FUNCTION_ALU_FUNCTION_ALU_H
# define M03GE9IJ4C25HDGT1OHRYQ3FCP_FUNCTION_ALU_FUNCTION_ALU_H

# include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>

namespace m03ge9ij4c25hdgt1ohryq3fcp_function_alu {

class function_alu_t {
public:
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* add(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* sub(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* mul(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* div(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);

    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* cond(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* is_zero(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);

    // todo: move these somewhere
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* integer(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* logger(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* pin(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
};

} // namespace m03ge9ij4c25hdgt1ohryq3fcp_function_alu

#endif // M03GE9IJ4C25HDGT1OHRYQ3FCP_FUNCTION_ALU_FUNCTION_ALU_H
