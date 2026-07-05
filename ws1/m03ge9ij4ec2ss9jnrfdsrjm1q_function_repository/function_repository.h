#ifndef M03GE9IJ4EC2SS9JNRFDSRJM1Q_FUNCTION_REPOSITORY_FUNCTION_REPOSITORY_H
# define M03GE9IJ4EC2SS9JNRFDSRJM1Q_FUNCTION_REPOSITORY_FUNCTION_REPOSITORY_H

# include <unordered_map>
# include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>
# include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
# include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>

namespace m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository {

struct function_repository_t {
public:
    struct entry_t {
        void (*call)(m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t&, uint8_t);
        m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t ir;
    };

public:
    void save(m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t ir, void (*call)(m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t&, uint8_t));

    entry_t load(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& id);

private:
    std::unordered_map<m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t, entry_t> m_functions;
};

} // namespace m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository

#endif // M03GE9IJ4EC2SS9JNRFDSRJM1Q_FUNCTION_REPOSITORY_FUNCTION_REPOSITORY_H
