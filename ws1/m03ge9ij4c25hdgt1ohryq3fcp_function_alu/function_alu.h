#ifndef M03GE9IJ4C25HDGT1OHRYQ3FCP_FUNCTION_ALU_FUNCTION_ALU_H
# define M03GE9IJ4C25HDGT1OHRYQ3FCP_FUNCTION_ALU_FUNCTION_ALU_H

# include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>

namespace m03ge9ij4c25hdgt1ohryq3fcp_function_alu {

/**
 * @brief Creates primitive nodes for integer arithmetic, selection and data forwarding.
 *
 * Every factory returns a fresh caller-owned node borrowing typesystem; adopt
 * it in std::unique_ptr and keep the registry alive at the same address through
 * node use. Nodes initially have no ports: resize arguments() to cover the table
 * before writing, connecting or calling. Port numbers are zero-based.
 *
 * | Factory | Input ports and read types | Output ports and stored types |
 * | --- | --- | --- |
 * | add | 0: int a; 1: int b | 2: int, a + b |
 * | sub | 0: int a; 1: int b | 2: int, a - b |
 * | mul | 0: int a; 1: int b | 2: int, a * b |
 * | div | 0: int a; 1: int b | 2: int, a / b, truncated toward zero |
 * | cond | 0: bool; 1: true branch; 2: false branch | 3: selected branch, same stored type |
 * | is_zero | 0: int | 1: bool, input == 0 |
 * | integer | none | 0: int, always zero |
 * | logger | 0: int; 1: optional borrowed std::ostream* | none; writes integer and newline |
 * | pin | triggering port: stored data | every other connected port: same stored type |
 *
 * Reads use function_t::read() coercions: register the requested read type and
 * any required coercion before calling. write<T>() registers its stored type.
 * The runtime header owns byte-storage restrictions and connection lifetimes.
 * All callbacks except pin ignore the triggering index. They evaluate immediately
 * with the current inputs; there is no wait for all inputs to arrive. Missing
 * required input data or an unavailable coercion throws std::runtime_error.
 * Writes and copies synchronously notify connected outputs, whose exceptions
 * can propagate. Serialize access to connected nodes and their shared registry.
 *
 * Arithmetic uses unchecked signed int operations: sums, differences and products
 * must be representable. For div, b must be nonzero and a / b representable
 * (exclude INT_MIN / -1). The current zero check occurs after division, so a
 * division-by-zero exception cannot be relied upon.
 *
 * @code{.cpp}
 * #include <m03ge9ij4c25hdgt1ohryq3fcp_function_alu/function_alu.h>
 * #include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>
 * #include <m03ge9ij43jyxy821pda20jhwh_typesystem/typesystem.h>
 *
 * #include <cassert>
 * #include <memory>
 *
 * int main() {
 *     using m03ge9ij4c25hdgt1ohryq3fcp_function_alu::function_alu_t;
 *     using m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t;
 *     m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t typesystem;
 *     std::unique_ptr<function_t> function(function_alu_t::add(typesystem));
 *     function->arguments().resize(3);
 *     function->write(0, 19);
 *     function->write(1, 23);
 *     function->call(0); // Both inputs are ready; neither write calls this node.
 *     const int result = function->read(2);
 *     assert(result == 42);
 * } // The node is destroyed before typesystem.
 * @endcode
 */
class function_alu_t {
public:
    /// @brief Allocates a node adding int inputs 0 and 1 into output 2.
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* add(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    /// @brief Allocates a node subtracting int input 1 from input 0 into output 2.
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* sub(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    /// @brief Allocates a node multiplying int inputs 0 and 1 into output 2.
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* mul(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    /// @brief Allocates a node dividing int input 0 by nonzero input 1 into output 2.
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* div(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);

    /**
     * @brief Allocates a node copying port 1 or 2 to port 3 according to bool port 0.
     *
     * A true condition selects port 1. An empty selected branch leaves the old
     * output unchanged and does not notify it; the unselected branch is not read.
     */
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* cond(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    /// @brief Allocates a node testing int port 0 for zero and writing bool port 1.
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* is_zero(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);

    // todo: move these somewhere
    /// @brief Allocates a node overwriting port 0 with int zero on every call.
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* integer(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    /**
     * @brief Allocates a node printing int port 0 and a newline to a stream or std::cout.
     *
     * Port 1 may contain a non-null std::ostream* whose stream must remain alive
     * during calls. An exception while reading that port or writing to its stream
     * triggers a write to std::cout; a null pointer is not a fallback request.
     * Missing/invalid port 0 data fails before the fallback handler.
     */
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* logger(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
    /**
     * @brief Allocates a node forwarding the triggering port to all other connected ports.
     *
     * Copies in increasing port order, retaining the source's stored type. Empty
     * source data sends nothing; unconnected ports are left unchanged. Supply a
     * valid triggering index and at most 256 ports, as indices use uint8_t.
     */
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* pin(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem);
};

} // namespace m03ge9ij4c25hdgt1ohryq3fcp_function_alu

#endif // M03GE9IJ4C25HDGT1OHRYQ3FCP_FUNCTION_ALU_FUNCTION_ALU_H
