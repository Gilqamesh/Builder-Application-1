#include <builder/builder_plugin.h>
#include <builder/compiler.h>

BUILDER_EXTERN void builder__export_libraries(builder_ctx_t* ctx, const builder_api_t* api, bundle_type_t bundle_type) {
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api) {
    const auto source_dir = api->source_dir(ctx);
    for (const auto& year_dir : std::filesystem::directory_iterator(source_dir)) {
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

                const auto relative_cpp_file = cpp_file_path.lexically_relative(source_dir);
                const auto cpp_stem = cpp_file_path.stem();
                const auto binary_name = year_dir_stem.string() + "_" + problem_dir_stem.string() + "_" + cpp_stem.string();
                compiler_t::create_binary(ctx, api, { relative_cpp_file }, {}, BUNDLE_TYPE_STATIC, binary_name);
            }
        }
    }
}
