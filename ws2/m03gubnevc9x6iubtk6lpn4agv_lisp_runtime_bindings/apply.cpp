#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevc9x6iubtk6lpn4agv_lisp_runtime_bindings {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t runtime_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("runtime"),
        name
    );
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t predicate_value(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    bool (*predicate)(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t&),
    std::string_view name
) {
    require_arg_count(args, 1, name);
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::bool_value(predicate(args[0]));
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "bool?", runtime_capability("is_bool") },
        { "capability?", runtime_capability("is_capability") },
        { "list?", runtime_capability("is_list") },
        { "path?", runtime_capability("is_path") },
        { "record?", runtime_capability("is_record") },
        { "string?", runtime_capability("is_string") },
        { "unit?", runtime_capability("is_unit") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__is_bool(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return predicate_value(args, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_bool, "is_bool");
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__is_capability(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return predicate_value(args, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_capability, "is_capability");
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__is_list(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return predicate_value(args, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_list, "is_list");
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__is_path(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return predicate_value(args, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_path, "is_path");
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__is_record(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return predicate_value(args, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_record, "is_record");
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__is_string(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return predicate_value(args, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string, "is_string");
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__is_unit(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return predicate_value(args, m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_unit, "is_unit");
}

} // namespace m03gubnevc9x6iubtk6lpn4agv_lisp_runtime_bindings
