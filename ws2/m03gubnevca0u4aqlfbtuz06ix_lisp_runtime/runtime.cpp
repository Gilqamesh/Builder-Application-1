#include "runtime.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace m03gubnevca0u4aqlfbtuz06ix_lisp_runtime {

static constexpr const char* RESULT_CHANNEL_ENV = "BUILDER_RESULT_PATH";

static void close_fd(int fd) {
    if (fd != -1) {
        close(fd);
    }
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t create_result_path() {
    std::string template_path = "/tmp/builder-result.XXXXXX";
    const int fd = mkstemp(template_path.data());
    if (fd == -1) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::result_channel: mkstemp failed: {}", std::strerror(errno)));
    }
    close_fd(fd);
    if (unlink(template_path.c_str()) == -1) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::result_channel: unlink failed for '{}': {}", template_path, std::strerror(errno)));
    }
    return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(template_path);
}

static std::string read_file(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) {
    std::ifstream ifs(path.string(), std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::result_channel_t::read: module CLI did not publish a result value");
    }

    std::string result;
    char buffer[4096];
    while (ifs.read(buffer, sizeof(buffer)) || ifs.gcount() != 0) {
        result.append(buffer, static_cast<std::size_t>(ifs.gcount()));
    }
    if (!ifs.eof()) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::result_channel_t::read: failed to read result channel '{}'", path));
    }

    return result;
}

static value_t parse_result_value(const std::string& result_contents) {
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(result_contents);
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::result_channel_t::read: failed to parse result value: {}", e.what()));
    }

    try {
        return deserialize_value(json);
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::result_channel_t::read: invalid result value: {}", e.what()));
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::result_channel_t::read: invalid result value: {}", e.what()));
    }
}

invocation_context_t invocation_context() {
    return m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t string_type_module() {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("string");
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t path_type_module() {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("path");
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t unit_type_module() {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("unit");
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t bool_type_module() {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("bool");
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t list_type_module() {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("list");
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t record_type_module() {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("record");
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t capability_type_module() {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("capability");
}

value_t string_value(std::string_view value) {
    return value_t {
        .type_module = string_type_module(),
        .data = std::string(value)
    };
}

value_t path_value(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& value) {
    return value_t {
        .type_module = path_type_module(),
        .data = value.string()
    };
}

value_t unit_value() {
    return value_t {
        .type_module = unit_type_module(),
        .data = nlohmann::json::object()
    };
}

value_t bool_value(bool value) {
    return value_t {
        .type_module = bool_type_module(),
        .data = value
    };
}

value_t list_value(const std::vector<value_t>& values) {
    nlohmann::json data = nlohmann::json::array();
    for (const auto& value : values) {
        data.push_back(serialize_value(value));
    }

    return value_t {
        .type_module = list_type_module(),
        .data = std::move(data)
    };
}

value_t record_value(const std::vector<record_field_t>& fields) {
    nlohmann::json data = nlohmann::json::object();
    for (const auto& [field, value] : fields) {
        if (field.empty()) {
            throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value: record field name must not be empty");
        }
        if (data.contains(field)) {
            throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value: duplicate record field '{}'", field));
        }
        data[field] = serialize_value(value);
    }

    return value_t {
        .type_module = record_type_module(),
        .data = std::move(data)
    };
}

value_t capability_value(const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& module, std::string_view name) {
    if (name.empty()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value: capability name must not be empty");
    }

    return value_t {
        .type_module = capability_type_module(),
        .data = nlohmann::json {
            { "module", module.string() },
            { "name", std::string(name) }
        }
    };
}

bool is_string(const value_t& value) {
    return value.type_module == string_type_module();
}

bool is_path(const value_t& value) {
    return value.type_module == path_type_module();
}

bool is_unit(const value_t& value) {
    return value.type_module == unit_type_module();
}

bool is_bool(const value_t& value) {
    return value.type_module == bool_type_module();
}

bool is_list(const value_t& value) {
    return value.type_module == list_type_module();
}

bool is_record(const value_t& value) {
    return value.type_module == record_type_module();
}

bool is_capability(const value_t& value) {
    return value.type_module == capability_type_module();
}

std::string as_string(const value_t& value) {
    if (!is_string(value)) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string: value type '{}' is not '{}'", value.type_module, string_type_module()));
    }
    if (!value.data.is_string()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string: string value data must be a JSON string");
    }

    return value.data.get<std::string>();
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t as_path(const value_t& value) {
    if (!is_path(value)) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path: value type '{}' is not '{}'", value.type_module, path_type_module()));
    }
    if (!value.data.is_string()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path: path value data must be a JSON string");
    }

    return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(value.data.get<std::string>());
}

bool as_bool(const value_t& value) {
    if (!is_bool(value)) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_bool: value type '{}' is not '{}'", value.type_module, bool_type_module()));
    }
    if (!value.data.is_boolean()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_bool: bool value data must be a JSON boolean");
    }

    return value.data.get<bool>();
}

std::vector<value_t> as_list(const value_t& value) {
    if (!is_list(value)) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_list: value type '{}' is not '{}'", value.type_module, list_type_module()));
    }
    if (!value.data.is_array()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_list: list value data must be a JSON array");
    }

    std::vector<value_t> result;
    for (const auto& element : value.data) {
        result.push_back(deserialize_value(element));
    }

    return result;
}

std::vector<record_field_t> as_record(const value_t& value) {
    if (!is_record(value)) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_record: value type '{}' is not '{}'", value.type_module, record_type_module()));
    }
    if (!value.data.is_object()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_record: record value data must be a JSON object");
    }

    std::vector<record_field_t> result;
    for (const auto& [field, field_value] : value.data.items()) {
        result.push_back({ field, deserialize_value(field_value) });
    }

    return result;
}

value_t record_field(const value_t& value, std::string_view field) {
    if (!is_record(value)) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_field: value type '{}' is not '{}'", value.type_module, record_type_module()));
    }
    if (!value.data.is_object()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_field: record value data must be a JSON object");
    }
    const auto field_name = std::string(field);
    if (!value.data.contains(field_name)) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_field: record has no field '{}'", field_name));
    }

    return deserialize_value(value.data.at(field_name));
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t capability_module(const value_t& value) {
    if (!is_capability(value)) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_module: value type '{}' is not '{}'", value.type_module, capability_type_module()));
    }
    if (!value.data.is_object() || !value.data.contains("module") || !value.data.at("module").is_string()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_module: capability value must contain string field 'module'");
    }

    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(value.data.at("module").get<std::string>());
}

std::string capability_name(const value_t& value) {
    if (!is_capability(value)) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_name: value type '{}' is not '{}'", value.type_module, capability_type_module()));
    }
    if (!value.data.is_object() || !value.data.contains("name") || !value.data.at("name").is_string()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_name: capability value must contain string field 'name'");
    }

    return value.data.at("name").get<std::string>();
}

nlohmann::json serialize_value(const value_t& value) {
    return nlohmann::json {
        { "type_module", value.type_module.string() },
        { "data", value.data }
    };
}

value_t deserialize_value(const nlohmann::json& value) {
    if (!value.is_object()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::deserialize_value: value must be a JSON object");
    }
    if (!value.contains("type_module") || !value.at("type_module").is_string()) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::deserialize_value: field 'type_module' must be a string");
    }
    if (!value.contains("data")) {
        throw std::runtime_error("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::deserialize_value: field 'data' is required");
    }

    return value_t {
        .type_module = m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(value.at("type_module").get<std::string>()),
        .data = value.at("data")
    };
}

result_channel_t::result_channel_t():
    m_path(create_result_path()),
    m_active(true)
{
}

result_channel_t::result_channel_t(result_channel_t&& other):
    m_path(other.m_path),
    m_active(other.m_active)
{
    other.m_active = false;
}

result_channel_t::~result_channel_t() {
    if (!m_active) {
        return ;
    }

    try {
        if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(m_path)) {
            m03gagbhsnusi43zogoacgj2ez_filesystem::remove(m_path);
        }
    } catch (...) {
    }
}

m03gagbhsvr0m5w15urj0o291m_process::environment_variable_t result_channel_t::environment() const {
    return m03gagbhsvr0m5w15urj0o291m_process::environment_variable_t(RESULT_CHANNEL_ENV, m_path.string());
}

value_t result_channel_t::read() const {
    return parse_result_value(read_file(m_path));
}

result_channel_t result_channel() {
    return result_channel_t();
}

void publish_result(const value_t& result) {
    const char* result_path = std::getenv(RESULT_CHANNEL_ENV);
    if (result_path == nullptr) {
        return ;
    }
    if (*result_path == '\0') {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result: {} must not be empty", RESULT_CHANNEL_ENV));
    }

    std::ofstream ofs(result_path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result: failed to open result channel '{}'", result_path));
    }

    ofs << serialize_value(result).dump() << '\n';
    if (!ofs) {
        throw std::runtime_error(std::format("m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result: failed to write result channel '{}'", result_path));
    }
}

void publish_string_result(std::string_view value) {
    publish_result(string_value(value));
}

void publish_path_result(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& value) {
    publish_result(path_value(value));
}

} // namespace m03gubnevca0u4aqlfbtuz06ix_lisp_runtime
