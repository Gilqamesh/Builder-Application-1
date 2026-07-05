#include <m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository/function_repository.h>
#include <chrono>
#include <stdexcept>

namespace m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository {

void function_repository_t::save(m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t ir, void (*call)(m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t&, uint8_t)) {
    m_functions.emplace(
        ir.function_id,
        entry_t {
            .call = call,
            .ir = ir
        }
    );
}

function_repository_t::entry_t function_repository_t::load(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& id) {
    auto it = m_functions.find(id);
    if (it == m_functions.end()) {
        throw std::runtime_error("function not found in repository");
    }

    return it->second;
}

} // namespace m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository
