#include <builder/builder_plugin.h>
#include <builder/compiler.h>

#include <format>

BUILDER_EXTERN void builder__export_libraries(builder_ctx_t* ctx, const builder_api_t* api, bundle_type_t bundle_type) {
    const auto system_curl = std::filesystem::path("/usr/lib64/libcurl.so");
    if (!std::filesystem::exists(system_curl)) {
        throw std::runtime_error(std::format("system curl library '{}' does not exist, TODO: extend logic to install it", system_curl.string()));
    }

    compiler_t::create_shared_library(ctx, api, { "curl.cpp" }, {}, "curl");
    compiler_t::reference_shared_library(ctx, api, system_curl, "curl-ref.so");
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api) {
    compiler_t::create_binary(
        ctx, api,
        { "cli.cpp" },
        {},
        BUNDLE_TYPE_SHARED,
        "cli"
    );
}
