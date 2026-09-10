#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <functional>
#include <stdexcept>
#include <string>

namespace runtime = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime;
namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace process = m03gagbhsvr0m5w15urj0o291m_process;

static void expect(bool condition) {
    if (!condition) { throw std::runtime_error("Lisp runtime contract check failed"); }
}

static void expect_failure(const std::function<void()>& operation) {
    try { operation(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Lisp runtime accepted an invalid value");
}

int main() {
    const auto capability = runtime::capability_value(filesystem::relative_path_t("filesystem"), "exists");
    const auto record = runtime::record_value({{"items", runtime::list_value({runtime::string_value("hello"), runtime::bool_value(true), capability})}});
    const auto restored = runtime::deserialize_value(runtime::serialize_value(record));
    expect(runtime::serialize_value(restored) == runtime::serialize_value(record));
    const auto items = runtime::as_list(runtime::record_field(restored, "items"));
    expect(runtime::as_string(items[0]) == "hello");
    expect(runtime::as_bool(items[1]));
    expect(runtime::capability_module(items[2]).string() == "filesystem");
    expect(runtime::capability_name(items[2]) == "exists");
    expect_failure([&] { runtime::as_string(runtime::unit_value()); });
    expect_failure([&] { runtime::record_value({{"same", runtime::unit_value()}, {"same", runtime::unit_value()}}); });
    expect_failure([&] { runtime::record_field(record, "missing"); });
    expect_failure([&] { runtime::deserialize_value(nlohmann::json::object()); });
    std::string result_path;
    {
        auto channel = runtime::result_channel();
        result_path = channel.environment().value();
        expect_failure([&] { channel.read(); });
        process::create_and_wait_checked(process::command_t({"/bin/sh", "-c", R"(printf '%s' '{"type_module":"string","data":"child"}' > "$BUILDER_RESULT_PATH")"}, std::nullopt, {channel.environment()}));
        expect(runtime::as_string(channel.read()) == "child");
    }
    expect(!filesystem::exists(filesystem::path_t(result_path)));
    return 0;
}
