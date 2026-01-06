#include <modules/builder/builder.h>
#include <modules/builder/compiler/cpp_compiler.h>

#include <iostream>
#include <format>

BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type) {
}

BUILDER_EXTERN void builder__import_libraries(const builder_t* builder) {
    const auto source_dir = builder->src_dir();
    for (const auto& year_dir : std::filesystem::directory_iterator(source_dir)) {
        if (!year_dir.is_directory()) {
            continue ;
        }

        const auto year_dir_stem = year_dir.path().stem();
        for (const auto& problem_dir : std::filesystem::directory_iterator(year_dir)) {
            const auto problem_dir_stem = problem_dir.path().stem();
            const auto binary_prefix = year_dir_stem / problem_dir_stem;
            const auto binary_dir = builder->import_dir() / binary_prefix;
            if (!std::filesystem::exists(binary_dir)) {
                std::error_code ec;
                std::filesystem::create_directories(binary_dir, ec);
                if (ec) {
                    throw std::runtime_error(std::format("builder__import_libraries: failed to create binary directory '{}': {}", binary_dir.string(), ec.message()));
                }
            }

            for (const auto& entry : std::filesystem::directory_iterator(problem_dir)) {
                const auto path = entry.path();

                const auto filename = path.filename();
                if (filename == "input") {
                    std::cout << std::format("cp {} {}", path.string(), (binary_dir / path.filename()).string()) << std::endl;
                    std::error_code ec;
                    std::filesystem::copy(path, binary_dir / filename, ec);
                    if (ec) {
                        throw std::runtime_error(std::format("builder__import_libraries: failed to copy input file '{}' to '{}': {}", path.string(), (binary_dir / path.filename()).string(), ec.message()));
                    }
                    continue ;
                }

                if (filename.extension() != ".cpp") {
                    continue ;
                }

                cpp_compiler_t::create_binary(builder, { path }, {}, LIBRARY_TYPE_STATIC, binary_prefix / filename.stem());
            }
        }
    }
}
