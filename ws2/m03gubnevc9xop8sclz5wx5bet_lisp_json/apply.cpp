#include <m03gagbhsqfsqblhwvelrou7nc_json/json.hpp>
#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevc9xop8sclz5wx5bet_lisp_json {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "nlohmann::json::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t json_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("json"), name);
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "deserialize_value", json_capability("deserialize_value") },
        { "serialize_value", json_capability("serialize_value") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__deserialize_value(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "deserialize_value");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::deserialize_value(nlohmann::json::parse(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0])));
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__serialize_value(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 1, "serialize_value");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::serialize_value(args[0]).dump());
}

} // namespace m03gubnevc9xop8sclz5wx5bet_lisp_json
