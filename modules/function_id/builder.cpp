#include <builder/builder.h>
#include <builder/compiler/cpp_compiler.h>
#include <builder/filesystem/filesystem.h>

BUILDER_EXTERN void builder__export_interface(const builder_t* builder, library_type_t library_type) {
    const auto interface_relative_path = relative_path_t("function_id.h");
    builder->install_interface(
        builder->source_dir() / interface_relative_path,
        interface_relative_path,
        library_type
    );
}

BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type) {
    const auto function_id_library = cpp_compiler_t::create_library(
        builder,
        filesystem_t::find(builder, filesystem_t::filename("function_id.cpp"), filesystem_t::descend_none),
        {},
        relative_path_t("function_id"),
        library_type
    );
    builder->install_library(function_id_library, relative_path_t("function_id.lib"), library_type);
}

BUILDER_EXTERN void builder__import_libraries(const builder_t* builder) {
    const auto binary_relative_path = relative_path_t("test");
    const auto test_binary = cpp_compiler_t::create_binary(
        builder,
        filesystem_t::find(builder, filesystem_t::filename("test.cpp"), filesystem_t::descend_none),
        {},
        library_type_t::SHARED,
        binary_relative_path
    );
    builder->install_import(test_binary, binary_relative_path);
}
