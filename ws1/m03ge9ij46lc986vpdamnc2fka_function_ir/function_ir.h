#ifndef M03GE9IJ46LC986VPDAMNC2FKA_FUNCTION_IR_FUNCTION_IR_H
# define M03GE9IJ46LC986VPDAMNC2FKA_FUNCTION_IR_FUNCTION_IR_H

# include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>

# include <vector>

namespace m03ge9ij46lc986vpdamnc2fka_function_ir {

/**
 * @brief Describes a function's identity, layout, ordered child instances and argument connections.
 *
 * This mutable aggregate owns its IDs and vectors, not executable function nodes.
 * Value-initialize with `{}` to zero scalar fields. Construction and mutation do
 * not validate IDs, rectangles, child references or argument availability.
 * Reordering or erasing children requires updating connection indices; references
 * into children and connections follow std::vector invalidation rules.
 *
 * left/right are horizontal edges and top/bottom are vertical edges. This IR
 * imposes no physical unit or pixel scale. Child rectangles place instances in
 * the enclosing function's internal coordinate system; the runtime owns scaling
 * and coordinate conversion. The enclosing rectangle is layout metadata and is
 * not applied to a runtime node's separate rectangle by its constructor.
 *
 * Runtime function_t::expand() resolves child IDs through its typesystem, checks
 * child references, and applies child rectangles through finalize_dimensions(),
 * which requires positive width and height. connect() checks argument availability.
 * Keep coordinate differences representable as int for those runtime operations.
 * Binary encoding separately checks representation limits, not graph validity;
 * see m03ge9ij47sume9p7pg7nbvu88_function_ir_binary::function_ir_binary_t.
 *
 * This example describes a connection from enclosing argument 0 to child 0's
 * argument 1; executing it also requires a child factory and those runtime ports.
 * @code{.cpp}
 * #include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
 * #include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>
 *
 * #include <cassert>
 * #include <cstdint>
 * #include <limits>
 *
 * int main() {
 *     using m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t;
 *     using m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t;
 *     function_ir_t function_ir {};
 *     function_ir.function_id = function_id_t::from_string("graph::root@42");
 *     function_ir.children.push_back(function_ir_t::child_t {
 *         .function_id = function_id_t::from_string("math::add@41"),
 *         .left = -20, .right = 20, .top = -10, .bottom = 10
 *     });
 *     function_ir.connections.push_back(function_ir_t::connection_info_t {
 *         .from_function_index = std::numeric_limits<std::uint16_t>::max(),
 *         .from_argument_index = 0,
 *         .to_function_index = 0,
 *         .to_argument_index = 1
 *     });
 *     assert(function_ir.connections.front().to_function_index < function_ir.children.size());
 * }
 * @endcode
 */
struct function_ir_t {
    /// @brief Identity of the enclosing function version.
    m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t function_id;
    /// @brief Horizontal left edge of the enclosing function's layout rectangle.
    int left;
    /// @brief Horizontal right edge of the enclosing function's layout rectangle.
    int right;
    /// @brief Vertical top edge of the enclosing function's layout rectangle.
    int top;
    /// @brief Vertical bottom edge of the enclosing function's layout rectangle.
    int bottom;

    /**
     * @brief Places one child function instance by ID and rectangle within the enclosing function.
     * Repeated IDs may describe distinct instances; the position in children
     * identifies the instance used by connections. Rectangle semantics are those
     * described by function_ir_t, with no checks performed by this aggregate.
     */
    struct child_t {
        /// @brief Function version to instantiate for this child.
        m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t function_id;
        /// @brief Left edge in the enclosing function's internal coordinates.
        int left;
        /// @brief Right edge in the enclosing function's internal coordinates.
        int right;
        /// @brief Top edge in the enclosing function's internal coordinates.
        int top;
        /// @brief Bottom edge in the enclosing function's internal coordinates.
        int bottom;
    };
    /// @brief Child instances in the zero-based order used by connection_info_t.
    std::vector<child_t> children;

    /**
     * @brief Describes a directed connection from one function argument to another.
     * Function indices are zero-based positions in children, except 65535
     * (`std::numeric_limits<std::uint16_t>::max()`), which denotes the enclosing
     * function at either endpoint. Child indices therefore range from 0 to 65534
     * and must also be less than children.size(). Argument indices are zero-based
     * ports of the selected runtime function, range 0..255, and carry no type or
     * input/output classification here. A reverse connection is a separate record.
     */
    struct connection_info_t {
        /// @brief Source child index or the enclosing-function sentinel.
        uint16_t from_function_index;
        /// @brief Source argument on the selected function.
        uint8_t from_argument_index;
        /// @brief Destination child index or the enclosing-function sentinel.
        uint16_t to_function_index;
        /// @brief Destination argument on the selected function.
        uint8_t to_argument_index;
    };
    /// @brief Directed connections, applied by runtime expansion in vector order.
    std::vector<connection_info_t> connections;
};

} // namespace m03ge9ij46lc986vpdamnc2fka_function_ir

#endif // M03GE9IJ46LC986VPDAMNC2FKA_FUNCTION_IR_FUNCTION_IR_H
