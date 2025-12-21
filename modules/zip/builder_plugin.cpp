#include <modules/builder/builder_plugin.h>
#include <modules/builder/builder.h>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    const auto minic_c = "external/miniz.c";
    const auto zip_cpp = "zip.cpp";
    builder_t::lib(ctx, api, { minic_c, zip_cpp }, {}, false);
    builder_t::lib(ctx, api, { minic_c, zip_cpp }, {}, true);
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
    builder_t::binary(
        ctx,
        api,
        { "cli.cpp" },
        {},
        "cli",
        { static_libs },
        {}
    );
}
