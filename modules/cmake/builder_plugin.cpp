#include <modules/builder/builder_plugin.h>
#include <modules/builder/builder.h>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    builder_t::lib(ctx, api, { }, {}, false);
    builder_t::lib(ctx, api, { }, {}, true);

    // https://github.com/google/googletest/archive/refs/tags/v1.15.0.tar.gz

    // curl -> unzip -> untar
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
}
