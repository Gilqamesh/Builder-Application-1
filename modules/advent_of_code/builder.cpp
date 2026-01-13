#include <builder/builder.h>
#include <builder/compiler/cpp_compiler.h>
#include <builder/filesystem/filesystem.h>

#include <iostream>
#include <format>

BUILDER_EXTERN void builder__export_interface(const builder_t* builder, library_type_t library_type) {
}

BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type) {
}

BUILDER_EXTERN void builder__import_libraries(const builder_t* builder) {
    for (const auto& src_file : filesystem_t::find(builder, filesystem_t::cpp_file, filesystem_t::descend_all)) {
        auto relative_binary_path = builder->source_dir().relative(src_file);
        relative_binary_path.extension("");
        const auto binary = cpp_compiler_t::create_binary(builder, { src_file }, {}, library_type_t::STATIC, relative_binary_path);
        builder->install_import(binary, relative_binary_path);
    }

    for (const auto& input_file : filesystem_t::find(builder, filesystem_t::filename("input"), filesystem_t::descend_all)) {
        const auto relative_input_file = builder->source_dir().relative(input_file);
        builder->install_import(input_file, relative_input_file);
    }
}
