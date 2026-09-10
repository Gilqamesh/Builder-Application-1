#include "builder_print.h"

#include <m03gubnevca18el75zd2vx8qxh_language/language.h>

#include <format>
#include <stdexcept>

namespace m03gubnevc9wnptp9x13mcsn56_builder_print {

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 1) {
        throw std::runtime_error(std::format("m03gubnevc9wnptp9x13mcsn56_builder_print::apply: expected 1 argument, got {}", args.size()));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(m03gubnevca18el75zd2vx8qxh_language::print(args[0]));
}

} // namespace m03gubnevc9wnptp9x13mcsn56_builder_print
