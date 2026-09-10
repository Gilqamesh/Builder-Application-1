#ifndef M03GE9IJ47SUME9P7PG7NBVU88_FUNCTION_IR_BINARY_FUNCTION_IR_BINARY_H
# define M03GE9IJ47SUME9P7PG7NBVU88_FUNCTION_IR_BINARY_FUNCTION_IR_BINARY_H

# include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
# include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>

# include <cstddef>
# include <cstdint>
# include <vector>

namespace m03ge9ij47sume9p7pg7nbvu88_function_ir_binary {

/**
 * @brief Owns an encoded function description and decodes its records into an independent IR.
 *
 * The current format has no magic, version, counts or end marker. It starts with
 * the enclosing function ID, then records extend to the end of the byte vector.
 * Encoding writes all child records in vector order, then all connection records
 * in vector order. Decoding accepts either record opcode at each record boundary.
 * All multibyte integers are big-endian (most significant byte first).
 *
 * | Element | Byte layout |
 * | --- | --- |
 * | Function ID | Namespace bytes, zero byte, name bytes, zero byte, 8-byte unsigned creation time |
 * | Child record | Opcode 0, function ID, four signed 16-bit two's-complement coordinates: left, right, top, bottom |
 * | Connection record | Opcode 1, four unsigned 8-bit fields: from function, from argument, to function, to argument |
 *
 * ID strings are stored byte-for-byte with no character encoding conversion and
 * must not contain embedded zero bytes. Creation times store whole seconds since
 * the system_clock epoch, truncating subseconds toward zero. Use nonnegative
 * seconds representable by system_clock; decoding does not check clock overflow.
 * Child coordinates must be in [-32768, 32767]. Function-index bytes 0..254 refer
 * to zero-based children; byte 255 maps to the IR's enclosing-function sentinel
 * 65535. Argument indices retain their 0..255 values. Child count is not checked,
 * but connections cannot address children beyond index 254 in this format.
 *
 * @note The current encoder omits the enclosing function's left/right/top/bottom,
 * and decoding leaves them zero. Combined with timestamp truncation, this means
 * arbitrary IR values do not round-trip exactly.
 *
 * @todo Decide whether the enclosing function's rectangle must survive binary
 * and file-repository round trips. Child rectangles are already encoded. The
 * binary test expects zero enclosing coordinates, while the file-repository test
 * expects the original coordinates. Reconcile these tests and, if preservation
 * is required, the encoding and compatibility behavior before promising an exact
 * rectangle round trip. The current omission does not establish that callers
 * should discard the enclosing rectangle.
 *
 * @code{.cpp}
 * #include <m03ge9ij47sume9p7pg7nbvu88_function_ir_binary/function_ir_binary.h>
 * #include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
 * #include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>
 *
 * #include <cassert>
 * #include <cstdint>
 * #include <utility>
 * #include <vector>
 *
 * int main() {
 *     using m03ge9ij47sume9p7pg7nbvu88_function_ir_binary::function_ir_binary_t;
 *     m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t function_ir {};
 *     function_ir.function_id =
 *         m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t::from_string("graph::root@42");
 *     const function_ir_binary_t encoded(function_ir);
 *     std::vector<std::uint8_t> stored_bytes = encoded.bytes(); // Own a copy.
 *     const function_ir_binary_t loaded(std::move(stored_bytes)); // No validation yet.
 *     const auto decoded_ir = loaded.function_ir(); // Decode and check record structure.
 *     assert(decoded_ir.function_id == function_ir.function_id);
 * }
 * @endcode
 */
struct function_ir_binary_t {
public:
    /**
     * @brief Takes ownership of raw bytes without decoding or validating them.
     * An lvalue argument is copied; an rvalue can transfer its storage. Empty or
     * malformed vectors are accepted here and diagnosed only by function_ir().
     */
    function_ir_binary_t(std::vector<uint8_t> bytes);
    /**
     * @brief Encodes an IR into owned bytes, checking the format's representation limits.
     * The IR is borrowed only for this call. No IDs are resolved and no checks
     * establish child existence, port availability, rectangle ordering or ID truth.
     * @throws std::invalid_argument If an ID string contains a zero byte, a child
     * coordinate exceeds the signed 16-bit range, or a function index is neither
     * 0..254 nor the enclosing-function sentinel 65535.
     */
    function_ir_binary_t(const m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t& ir);

    /**
     * @brief Borrows the owned byte vector, which may still contain unvalidated data.
     * The reference requires this object to remain alive. Assignment or moving
     * from this object may replace its contents and invalidate element references.
     */
    const std::vector<uint8_t>& bytes() const;
    /**
     * @brief Decodes a fresh IR while checking byte boundaries and record opcodes.
     * The stored bytes remain unchanged on success or failure, and the returned
     * IR owns its strings and vectors independently. This is structural decoding,
     * not validation of ID truth, child existence, ports or rectangle ordering;
     * see m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t for runtime checks.
     * Truncation that removes whole trailing records is undetectable: the format
     * stores neither an expected record count nor an expected total byte size.
     * @throws std::runtime_error For an unterminated namespace/name, truncated
     * timestamp, child coordinates or connection payload, or an unknown opcode.
     */
    m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t function_ir() const;

private:
    void serialize_function_id(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& function_id);
    m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t deserialize_function_id(size_t& offset) const;

private:
    std::vector<uint8_t> m_bytes;
};

} // namespace m03ge9ij47sume9p7pg7nbvu88_function_ir_binary

#endif // M03GE9IJ47SUME9P7PG7NBVU88_FUNCTION_IR_BINARY_FUNCTION_IR_BINARY_H
