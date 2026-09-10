#ifndef M03GE9IJ4D9WYFMDSR8OYHKKTR_FUNCTION_COMPOUND_FUNCTION_COMPOUND_H
# define M03GE9IJ4D9WYFMDSR8OYHKKTR_FUNCTION_COMPOUND_FUNCTION_COMPOUND_H

# include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>

namespace m03ge9ij4d9wyfmdsr8oyhkktr_function_compound {

/**
 * @brief Creates runtime nodes that lazily expand an IR graph and forward triggering ports.
 *
 * This one-child example supplies a resolver for an externally owned node. The
 * static pointer is only a bridge for the typesystem's capture-free callback;
 * its pointee remains alive until the compound and its outgoing links are gone.
 *
 * @code{.cpp}
 * #include <m03ge9ij4d9wyfmdsr8oyhkktr_function_compound/function_compound.h>
 * #include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>
 * #include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
 * #include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>
 * #include <m03ge9ij43jyxy821pda20jhwh_typesystem/typesystem.h>
 *
 * #include <cassert>
 * #include <cstdint>
 * #include <limits>
 * #include <memory>
 * #include <stdexcept>
 *
 * int main() {
 *     using m03ge9ij4d9wyfmdsr8oyhkktr_function_compound::function_compound_t;
 *     using m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t;
 *     using m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t;
 *     using m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t;
 *     m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t typesystem;
 *     function_ir_t child_ir {};
 *     child_ir.function_id = function_id_t::from_string("example::copy@1");
 *     function_t child(typesystem, child_ir, +[](function_t& function, std::uint8_t index) {
 *         function.copy(index, 1);
 *     });
 *     child.arguments().resize(2);
 *     static function_t* child_function = nullptr;
 *     child_function = &child;
 *     typesystem.register_type<function_id_t>();
 *     typesystem.register_type<function_t*>();
 *     typesystem.register_coercion<function_id_t, function_t*>(+[](function_id_t function_id) -> function_t* {
 *         if (child_function == nullptr || function_id != child_function->function_ir().function_id) {
 *             throw std::runtime_error("example: unknown child function");
 *         }
 *         return child_function;
 *     });
 *     function_ir_t function_ir {};
 *     function_ir.function_id = function_id_t::from_string("example::compound@2");
 *     function_ir.children.push_back(function_ir_t::child_t {
 *         .function_id = child_ir.function_id,
 *         .left = 0, .right = 100, .top = 0, .bottom = 100
 *     });
 *     function_ir.connections.push_back(function_ir_t::connection_info_t {
 *         .from_function_index = std::numeric_limits<std::uint16_t>::max(),
 *         .from_argument_index = 0,
 *         .to_function_index = 0,
 *         .to_argument_index = 0
 *     });
 *     std::unique_ptr<function_t> function(function_compound_t::function(typesystem, function_ir));
 *     function->arguments().resize(1);
 *     assert(function->children().empty());
 *     function->call(0); // Lazily expands; empty input sends nothing.
 *     function->write(0, 42); // The installed link now calls the child synchronously.
 *     const int result = child.read(1);
 *     assert(result == 42);
 *     function.reset(); // Disconnects the compound; it does not delete child.
 *     child_function = nullptr;
 * } // child is destroyed before typesystem.
 * @endcode
 */
class function_compound_t {
public:
    /**
     * @brief Allocates an unexpanded node whose callback expands the IR and sends the triggering port.
     * @return A fresh caller-owned node; adopt it in std::unique_ptr or delete it.
     *
     * The node owns an IR copy and borrows typesystem, which must remain alive at
     * the same address through node use. It starts without ports: allocate the
     * enclosing arguments() before connecting, writing or calling.
     *
     * For each child, register function_id_t and function_t* and a coercion from
     * ID to node pointer in this registry. The resolver must supply non-null,
     * externally managed nodes with referenced ports ready and compatible use
     * of this same registry. Expansion mutates child rectangles and connections;
     * it does not set parent pointers or assume ownership. Deleting the compound
     * does not delete its children. See function_t for graph/child lifetimes and
     * function_ir_t::connection_info_t for connection indices and their sentinel.
     *
     * call(i) runs expand() followed by send(i); a completed expansion is reused.
     * expand() may already send stored input while installing links, so the first
     * call can deliver it again through send(i). Expanding before writing input,
     * as above, avoids this initial duplicate delivery. Writes do not themselves
     * call or expand the compound; once linked they send normally.
     *
     * Coercion, invalid child layout/connection, missing triggering port and
     * downstream callback failures propagate from function_t. Expansion has no
     * rollback, and callbacks during expansion must not reenter that expansion.
     * The runtime's current shrink() limitation also applies to these nodes.
     */
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem, m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t function_ir);
};

} // namespace m03ge9ij4d9wyfmdsr8oyhkktr_function_compound

#endif // M03GE9IJ4D9WYFMDSR8OYHKKTR_FUNCTION_COMPOUND_FUNCTION_COMPOUND_H
