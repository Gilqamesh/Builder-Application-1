#ifndef M03GE9IJ47SUME9P7PG7NBVU88_FUNCTION_IR_BINARY_FUNCTION_IR_BINARY_H
# define M03GE9IJ47SUME9P7PG7NBVU88_FUNCTION_IR_BINARY_FUNCTION_IR_BINARY_H

# include <cstddef>
# include <vector>
# include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
# include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>

namespace m03ge9ij47sume9p7pg7nbvu88_function_ir_binary {

struct function_ir_binary_t {
public:
    function_ir_binary_t(std::vector<uint8_t> bytes);
    function_ir_binary_t(const m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t& ir);

    const std::vector<uint8_t>& bytes() const;
    m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t function_ir() const;

private:
    void serialize_function_id(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& function_id);
    m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t deserialize_function_id(size_t& offset) const;

private:
    std::vector<uint8_t> m_bytes;
};

} // namespace m03ge9ij47sume9p7pg7nbvu88_function_ir_binary

#endif // M03GE9IJ47SUME9P7PG7NBVU88_FUNCTION_IR_BINARY_FUNCTION_IR_BINARY_H
