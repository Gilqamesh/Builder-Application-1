#include <m03ge9ij4d9wyfmdsr8oyhkktr_function_compound/function_compound.h>

namespace m03ge9ij4d9wyfmdsr8oyhkktr_function_compound {

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_compound_t::function(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem, m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t function_ir) {
    return new m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t(
        typesystem,
        function_ir,
        [](m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t& function, uint8_t argument_index) {
            function.expand();
            function.send(argument_index);
        }
    );
}

} // namespace m03ge9ij4d9wyfmdsr8oyhkktr_function_compound
