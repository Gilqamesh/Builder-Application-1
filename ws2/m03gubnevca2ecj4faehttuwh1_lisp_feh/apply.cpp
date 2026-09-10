#include "apply.h"

#include <m03gubnevca36dpf4iab4p14ah_feh/feh.h>

#include <format>
#include <stdexcept>

namespace m03gubnevca2ecj4faehttuwh1_lisp_feh {


using namespace m03gubnevca36dpf4iab4p14ah_feh;

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 1) {
        throw std::runtime_error(std::format("m03gubnevca36dpf4iab4p14ah_feh::apply: expected 1 argument, got {}", args.size()));
    }

    view(path_arg(args[0]));
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::unit_value();
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevca2ecj4faehttuwh1_lisp_feh::apply(args);
}

} // namespace m03gubnevca2ecj4faehttuwh1_lisp_feh
