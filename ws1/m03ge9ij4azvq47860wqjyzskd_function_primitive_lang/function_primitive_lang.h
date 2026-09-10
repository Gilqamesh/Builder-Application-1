#ifndef M03GE9IJ4AZVQ47860WQJYZSKD_FUNCTION_PRIMITIVE_LANG_FUNCTION_PRIMITIVE_LANG_H
# define M03GE9IJ4AZVQ47860WQJYZSKD_FUNCTION_PRIMITIVE_LANG_FUNCTION_PRIMITIVE_LANG_H

# include <string>
# include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>

namespace m03ge9ij4azvq47860wqjyzskd_function_primitive_lang {

/**
 * @brief Creates runtime nodes whose behavior is supplied by a C++ callback.
 *
 * @code{.cpp}
 * #include <m03ge9ij4azvq47860wqjyzskd_function_primitive_lang/function_primitive_lang.h>
 * #include <m03ge9ij49xkr5obofujoj7ltw_function_runtime/function.h>
 * #include <m03ge9ij43jyxy821pda20jhwh_typesystem/typesystem.h>
 *
 * #include <cassert>
 * #include <cstdint>
 * #include <memory>
 *
 * int main() {
 *     using m03ge9ij4azvq47860wqjyzskd_function_primitive_lang::function_primitive_lang_t;
 *     using m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t;
 *     m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t typesystem;
 *     std::unique_ptr<function_t> function(function_primitive_lang_t::function(
 *         typesystem, "example", "copy", +[](function_t& function, std::uint8_t argument_index) {
 *             if (argument_index == 0) {
 *                 function.copy(0, 1);
 *             }
 *         }
 *     ));
 *     function->arguments().resize(2);
 *     function->write(0, 42); // Stores input; this does not call the node itself.
 *     function->call(0);
 *     const int result = function->read(1);
 *     assert(result == 42);
 * } // The node is destroyed before its borrowed typesystem.
 * @endcode
 */
class function_primitive_lang_t {
public:
    /**
     * @brief Allocates an empty runtime node with the supplied identity components and callback.
     * @return A fresh node owned by the caller; adopt it in std::unique_ptr or delete it.
     *
     * The node borrows typesystem, which must remain alive at the same address
     * throughout use; see function_t for port storage and connection lifetimes.
     * ns and name are stored by value without validation; the version timestamp
     * is system_clock::now(). No arguments, children or connections are created.
     * Resize arguments() before using ports, and keep function_call non-null.
     *
     * The callback receives the called node and its triggering argument index:
     * call(i) passes i unchanged; a connected send passes the destination's index.
     * It runs synchronously, defines which indices it accepts, and may propagate
     * exceptions. Writing an unconnected port does not invoke this callback.
     * @throws std::invalid_argument If function_call is null.
     */
    static m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t* function(m03ge9ij43jyxy821pda20jhwh_typesystem::typesystem_t& typesystem, std::string ns, std::string name, void (*function_call)(m03ge9ij49xkr5obofujoj7ltw_function_runtime::function_t&, uint8_t));
};

} // namespace m03ge9ij4azvq47860wqjyzskd_function_primitive_lang

#endif // M03GE9IJ4AZVQ47860WQJYZSKD_FUNCTION_PRIMITIVE_LANG_FUNCTION_PRIMITIVE_LANG_H
