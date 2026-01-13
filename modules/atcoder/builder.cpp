#include <modules/builder/builder.h>
#include <modules/builder/compiler/cpp_compiler.h>

BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type) {
}

BUILDER_EXTERN void builder__import_libraries(const builder_t* builder) {
    for (const auto& entry : std::filesystem::directory_iterator(builder->source_dir())) {
        if (!entry.is_directory()) {
            continue ;
        }

        const auto& problem_number_path = entry.path();
        for (const auto& subproblem_entry : std::filesystem::directory_iterator(problem_number_path)) {
            const auto& subproblem_file = subproblem_entry.path();
            if (subproblem_file.extension() != ".cpp") {
                continue ;
            }


            cpp_compiler_t::create_binary(
                builder,
                { subproblem_file },
                {},
                LIBRARY_TYPE_STATIC,
                problem_number_path.filename() / subproblem_file.stem()
            );
        }
    }
}
