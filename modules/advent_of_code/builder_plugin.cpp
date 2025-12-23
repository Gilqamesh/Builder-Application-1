#include <modules/builder/builder_plugin.h>
#include <modules/builder/builder.h>

BUILDER_EXTERN void builder__export_bundle_static(builder_ctx_t* ctx, const builder_api_t* api) {
    throw std::runtime_error("builder__export_bundle_static isn't implemented");
}

BUILDER_EXTERN void builder__export_bundle_shared(builder_ctx_t* ctx, const builder_api_t* api) {
    throw std::runtime_error("builder__export_bundle_shared isn't implemented");
}

BUILDER_EXTERN void builder__link_module(builder_ctx_t* ctx, const builder_api_t* api) {
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

                const auto cpp_stem = cpp_file_path.stem();
                const auto binary_name = year_dir_stem.string() + "_" + problem_dir_stem.string() + "_" + cpp_stem.string();
                builder_t::materialize_binary(ctx, api, binary_name, { cpp_file_path }, {}, { api->get_static_link_command_line(ctx) });
            }
        }
    }
}
