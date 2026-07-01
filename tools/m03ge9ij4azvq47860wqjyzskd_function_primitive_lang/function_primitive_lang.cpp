#include <m03ge9ij4azvq47860wqjyzskd_function_primitive_lang/function_primitive_lang.h>
#include <chrono>
#include <utility>

namespace m03ge9ij4azvq47860wqjyzskd_function_primitive_lang {

m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function_primitive_lang_t::function(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem, std::string ns, std::string name, void (*function_call)(m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t&, uint8_t)) {
    return new m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t(
        typesystem,
        m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t {
            .function_id = m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t {
                .ns = std::move(ns),
                .name = std::move(name),
                .creation_time = std::chrono::system_clock::now()
            },
            .left = {},
            .right = {},
            .top = {},
            .bottom = {},
            .children = {},
            .connections = {}
        },
        std::move(function_call)
    );
}

} // namespace m03ge9ij4azvq47860wqjyzskd_function_primitive_lang
