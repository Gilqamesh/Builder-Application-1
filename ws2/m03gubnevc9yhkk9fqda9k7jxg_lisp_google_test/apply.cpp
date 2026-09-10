#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevc9yhkk9fqda9k7jxg_lisp_google_test {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gagbhszuoyfx4uxgozn8yrr_google_test::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t google_test_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("google_test"), name);
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "library_stems", google_test_capability("library_stems") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__library_stems(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "library_stems");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::list_value({
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("gtest"),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("gmock")
    });
}

} // namespace m03gubnevc9yhkk9fqda9k7jxg_lisp_google_test
