#ifndef M03GUBNEVCA0U4AQLFBTUZ06IX_LISP_RUNTIME_RUNTIME_H
# define M03GUBNEVCA0U4AQLFBTUZ06IX_LISP_RUNTIME_RUNTIME_H

# include <m03gagbhsqfsqblhwvelrou7nc_json/json.hpp>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

# include <m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h>

# include <format>
# include <optional>
# include <stdexcept>
# include <string>
# include <string_view>
# include <utility>
# include <vector>

namespace m03gubnevca0u4aqlfbtuz06ix_lisp_runtime {

using invocation_context_t = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context_t;

invocation_context_t invocation_context();

/** @brief A typed Lisp value serialized as `type_module` and `data`. */
struct value_t {
    m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t type_module;
    nlohmann::json data;
};

using record_field_t = std::pair<std::string, value_t>;

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t string_type_module();
m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t path_type_module();
m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t unit_type_module();
m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t bool_type_module();
m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t list_type_module();
m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t record_type_module();
m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t capability_type_module();

value_t string_value(std::string_view value);
value_t path_value(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& value);
value_t unit_value();
value_t bool_value(bool value);
value_t list_value(const std::vector<value_t>& values);
value_t record_value(const std::vector<record_field_t>& fields);
value_t capability_value(const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& module, std::string_view name);

bool is_string(const value_t& value);
bool is_path(const value_t& value);
bool is_unit(const value_t& value);
bool is_bool(const value_t& value);
bool is_list(const value_t& value);
bool is_record(const value_t& value);
bool is_capability(const value_t& value);

std::string as_string(const value_t& value);
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t as_path(const value_t& value);
bool as_bool(const value_t& value);
std::vector<value_t> as_list(const value_t& value);
std::vector<record_field_t> as_record(const value_t& value);
value_t record_field(const value_t& value, std::string_view field);
m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t capability_module(const value_t& value);
std::string capability_name(const value_t& value);

nlohmann::json serialize_value(const value_t& value);
value_t deserialize_value(const nlohmann::json& value);

/** @brief Owns a temporary result file shared with a child through its environment. */
class result_channel_t {
public:
    result_channel_t();
    result_channel_t(const result_channel_t& other) = delete;
    result_channel_t& operator=(const result_channel_t& other) = delete;
    result_channel_t(result_channel_t&& other);
    ~result_channel_t();

    m03gagbhsvr0m5w15urj0o291m_process::environment_variable_t environment() const;
    value_t read() const;

private:
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_path;
    bool m_active;
};

result_channel_t result_channel();
void publish_result(const value_t& result);
void publish_string_result(std::string_view value);
void publish_path_result(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& value);

} // namespace m03gubnevca0u4aqlfbtuz06ix_lisp_runtime

namespace std {

template <> struct formatter<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>;
template <> struct formatter<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::result_channel_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t> : formatter<std::string> {
    auto format(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value, auto& ctx) const {
        auto out = ctx.out();
        std::string text;
        if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
            text = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value);
        } else if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_path(value)) {
            text = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value).string();
        } else if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_unit(value)) {
            text = "unit";
        } else if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_bool(value)) {
            text = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_bool(value) ? "true" : "false";
        } else {
            text = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::serialize_value(value).dump();
        }
        out = formatter<std::string>::format(text, ctx);
        return out;
    }
};

template <> struct formatter<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::result_channel_t> : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

} // namespace std

#endif // M03GUBNEVCA0U4AQLFBTUZ06IX_LISP_RUNTIME_RUNTIME_H
