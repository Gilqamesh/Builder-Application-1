#include <builder/cpp_module.h>
#include <builder/compiler.h>

BUILDER_EXTERN void c_module__build(const c_module_t* builder) {
    cpp_module_t cpp_module = cpp_module_t::from_c_module(*builder);
}
