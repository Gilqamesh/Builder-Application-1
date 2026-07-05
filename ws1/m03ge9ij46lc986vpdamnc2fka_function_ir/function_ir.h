#ifndef M03GE9IJ46LC986VPDAMNC2FKA_FUNCTION_IR_FUNCTION_IR_H
# define M03GE9IJ46LC986VPDAMNC2FKA_FUNCTION_IR_FUNCTION_IR_H

# include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>

# include <vector>

namespace m03ge9ij46lc986vpdamnc2fka_function_ir {

struct function_ir_t {
    m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t function_id;
    int left;
    int right;
    int top;
    int bottom;

    struct child_t {
        m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t function_id;
        int left;
        int right;
        int top;
        int bottom;
    };
    std::vector<child_t> children;

    struct connection_info_t {
        uint16_t from_function_index;
        uint8_t from_argument_index;
        uint16_t to_function_index;
        uint8_t to_argument_index;
    };
    std::vector<connection_info_t> connections;
};

} // namespace m03ge9ij46lc986vpdamnc2fka_function_ir

#endif // M03GE9IJ46LC986VPDAMNC2FKA_FUNCTION_IR_FUNCTION_IR_H
