#include <modules/builder/builder.h>
#include <modules/builder/compiler/cpp_compiler.h>
#include <modules/builder/filesystem/filesystem.h>

#include <iostream>
#include <format>

BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type) {
}

BUILDER_EXTERN void builder__import_libraries(const builder_t* builder) {
    const auto src_files = filesystem_t::find(builder, filesystem_t::cpp_file, filesystem_t::descend_all);
    const auto input_files = filesystem_t::find(builder, filesystem_t::filename("input"), filesystem_t::descend_all);

    for (const auto& src_file : src_files) {
        auto rel = builder->source_dir().relative(src_file);
        rel.extension("");
        cpp_compiler_t::create_binary(builder, { src_file }, {}, LIBRARY_TYPE_STATIC, rel);
    }

    for (const auto& input_file : input_files) {
        const auto dst = builder->import_dir() / builder->source_dir().relative(input_file);
        const auto parent = dst.parent();
        if (!filesystem_t::exists(parent)) {
            filesystem_t::create_directories(parent);
        }
        filesystem_t::copy(input_file, dst);
    }
}
