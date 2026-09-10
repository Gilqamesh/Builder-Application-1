#include "builder_read.h"

#include <m03gubnevca18el75zd2vx8qxh_language/language.h>

#include <format>
#include <stdexcept>

namespace m03gubnevca0fyv87f6svumxgw_builder_read {

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 1) {
        throw std::runtime_error(std::format("m03gubnevca0fyv87f6svumxgw_builder_read::apply: expected 1 argument, got {}", args.size()));
    }

    return m03gubnevca18el75zd2vx8qxh_language::syntax_value(m03gubnevca18el75zd2vx8qxh_language::read(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0])));
}

} // namespace m03gubnevca0fyv87f6svumxgw_builder_read
