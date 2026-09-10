#ifndef M03GE9IJ4EC2SS9JNRFDSRJM1Q_FUNCTION_REPOSITORY_FUNCTION_REPOSITORY_H
# define M03GE9IJ4EC2SS9JNRFDSRJM1Q_FUNCTION_REPOSITORY_FUNCTION_REPOSITORY_H

# include <unordered_map>
# include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>
# include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
# include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>

namespace m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository {

/**
 * @brief Stores the first saved IR and callback for each exact function ID in memory.
 *
 * Entries own their IR values; callback pointers are copied without owning any
 * external state they use. The repository does not create runtime nodes or
 * invoke callbacks. IDs use function_id_t equality, including the full timestamp,
 * rather than their potentially lower-precision text representation. Synchronize
 * access if save() can run concurrently with another operation on this repository.
 *
 * @code{.cpp}
 * #include <m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository/function_repository.h>
 * #include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>
 * #include <m03ge9ij46lc986vpdamnc2fka_function_ir/function_ir.h>
 * #include <m03ge9ij45dcznrmna12qow5r5_function_id/function_id.h>
 * #include <m03ge9ij43jyxy821pda20jhwh_typesystem/typesystem.h>
 *
 * #include <cassert>
 * #include <cstdint>
 * #include <stdexcept>
 *
 * int main() {
 *     using m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository::function_repository_t;
 *     using m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t;
 *     using m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t;
 *     using m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t;
 *     function_repository_t function_repository;
 *     function_ir_t function_ir {};
 *     function_ir.function_id = function_id_t::from_string("example::constant@1");
 *     auto first_call = +[](function_t& function, std::uint8_t) { function.write(0, 42); };
 *     function_repository.save(function_ir, first_call);
 *     function_ir.left = 99;
 *     function_repository.save(function_ir, +[](function_t&, std::uint8_t) {});
 *     auto entry = function_repository.load(function_ir.function_id);
 *     assert(entry.ir.left == 0 && entry.call == first_call); // Duplicate retained the first entry.
 *     entry.ir.left = 7;
 *     assert(function_repository.load(function_ir.function_id).ir.left == 0); // load returns a copy.
 *     m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t typesystem;
 *     function_t function(typesystem, entry.ir, entry.call);
 *     function.arguments().resize(1);
 *     function.call(0);
 *     const int result = function.read(0);
 *     assert(result == 42);
 *     bool missing = false;
 *     try {
 *         function_repository.load(function_id_t::from_string("example::missing@1"));
 *     } catch (const std::runtime_error&) {
 *         missing = true;
 *     }
 *     assert(missing);
 * }
 * @endcode
 */
struct function_repository_t {
public:
    /**
     * @brief Pairs an owned IR value with the callback to use when constructing its runtime node.
     *
     * A loaded entry is independent of repository lifetime and later saves.
     * Callback code and any state it accesses must remain valid during invocation.
     * Value-initialize with {} when creating an empty entry; no validity is enforced.
     */
    struct entry_t {
        /// @brief Stored callback pointer, which may be null and must be checked before use.
        void (*call)(m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t&, uint8_t);
        /// @brief Owned function description, independent of the saved source IR.
        m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t ir;
    };

public:
    /**
     * @brief Saves an IR/callback pair only if its exact IR function ID is not already present.
     *
     * Copies both inputs into the entry. A duplicate keeps the original IR and
     * callback and returns normally without reporting insertion status. IDs, IR
     * structure and callback are not validated; even an empty ID or null callback
     * can be stored. Runtime function_t construction separately rejects null
     * callbacks. Allocation and value-copy failures propagate.
     */
    void save(m03ge9ij46lc986vpdamnc2fka_function_ir::function_ir_t ir, void (*call)(m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t&, uint8_t));

    /**
     * @brief Returns an independent entry copy for an exact function ID.
     *
     * Mutating the result cannot change the repository; its callback pointer is
     * still the same function pointer. No latest-version search is performed.
     * @throws std::runtime_error If the ID is absent.
     *
     * Allocation and value-copy failures propagate.
     */
    entry_t load(const m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t& id);

private:
    std::unordered_map<m03ge9ij45dcznrmna12qow5r5_function_id::function_id_t, entry_t> m_functions;
};

} // namespace m03ge9ij4ec2ss9jnrfdsrjm1q_function_repository

#endif // M03GE9IJ4EC2SS9JNRFDSRJM1Q_FUNCTION_REPOSITORY_FUNCTION_REPOSITORY_H
