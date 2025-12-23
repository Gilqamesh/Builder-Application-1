#include <modules/builder/builder_plugin.h>
#include <modules/builder/builder_api.h>
#include <modules/builder/builder.h>

#include <format>

BUILDER_EXTERN void builder__export_bundle_static(builder_ctx_t* ctx, const builder_api_t* api) {
    throw std::runtime_error("builder__export_bundle_static isn't implemented");
}

BUILDER_EXTERN void builder__export_bundle_shared(builder_ctx_t* ctx, const builder_api_t* api) {
    const auto system_curl = std::filesystem::path("/usr/lib64/libcurl.so");
    if (!std::filesystem::exists(system_curl)) {
        throw std::runtime_error(std::format("system curl library '{}' does not exist, TODO: extend logic to install it", system_curl.string()));
    }

    builder_t::materialize_shared_library(ctx, api, "curl.so", { "curl.cpp" }, {});
    builder_t::reference_shared_library(ctx, api, system_curl);
}

BUILDER_EXTERN void builder__link_module(builder_ctx_t* ctx, const builder_api_t* api) {
    builder_t::materialize_binary(
        ctx, api, "cli",
        { "cli.cpp" },
        {},
        { api->get_shared_link_command_line(ctx) }
    );
}
