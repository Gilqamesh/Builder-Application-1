#include <modules/builder/builder_plugin.h>
#include <modules/builder/builder.h>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    builder_t::lib(ctx, api, {}, {}, false);
    builder_t::lib(ctx, api, {}, {}, true);
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
    const auto src_dir = api->src_dir(ctx);
    for (const auto& year_dir : std::filesystem::directory_iterator(src_dir)) {
        if (!year_dir.is_directory()) {
            continue ;
        }

        const auto year_dir_stem = year_dir.path().stem();
        for (const auto& problem_dir : std::filesystem::directory_iterator(year_dir)) {
            const auto problem_dir_stem = problem_dir.path().stem();
            for (const auto& entry : std::filesystem::directory_iterator(problem_dir)) {
                const auto cpp_file_path = entry.path();
                if (cpp_file_path.extension() != ".cpp") {
                    continue ;
                }

                const auto cpp_stem = cpp_file_path.stem();
                const auto binary_name = year_dir_stem.string() + "_" + problem_dir_stem.string() + "_" + cpp_stem.string();
                builder_t::binary(ctx, api, { cpp_file_path }, {}, { binary_name }, { static_libs });
            }
        }
    }
}
