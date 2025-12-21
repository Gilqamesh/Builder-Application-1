#include <modules/builder/builder_plugin.h>
#include <modules/builder/builder.h>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    builder_t::lib(ctx, api, { "curl.cpp" }, {}, false);
    builder_t::lib(ctx, api, { "curl.cpp" }, {}, true);
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
    builder_t::binary(
        ctx,
        api,
        { "cli.cpp" },
        {},
        "cli",
        { static_libs },
        { "-lcurl" }
    );
}
