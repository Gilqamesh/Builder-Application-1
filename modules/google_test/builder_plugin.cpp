#include <modules/builder/builder_plugin.h>
#include <modules/builder/builder.h>

#include <modules/curl/curl.h>
#include <modules/zip/zip.h>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    builder_t::lib(ctx, api, { }, {}, false);
    builder_t::lib(ctx, api, { }, {}, true);

    const auto tar_gz_path = curl_t::download("https://github.com/google/googletest/archive/refs/tags/v1.15.0.tar.gz", std::filesystem::path(api->artifact_dir(ctx)) / "googletest.tar.gz");
    const auto tar_path = zip_t::unzip(tar_gz_path, std::filesystem::path(api->artifact_dir(ctx)) / "googletest.tar");
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
}
