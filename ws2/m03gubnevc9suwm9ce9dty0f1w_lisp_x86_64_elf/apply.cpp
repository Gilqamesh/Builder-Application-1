#include "apply.h"

#include <m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf/x86_64_elf.h>

#include <format>
#include <stdexcept>

namespace m03gubnevc9suwm9ce9dty0f1w_lisp_x86_64_elf {


using namespace m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf;

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (!args.empty()) {
        throw std::runtime_error(std::format("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::apply: expected 0 arguments, got {}", args.size()));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(cxx_path());
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevc9suwm9ce9dty0f1w_lisp_x86_64_elf::apply(args);
}

} // namespace m03gubnevc9suwm9ce9dty0f1w_lisp_x86_64_elf
