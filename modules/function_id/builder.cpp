#include <modules/builder/builder.h>
#include <modules/builder/compiler/cpp_compiler.h>
#include <modules/builder/filesystem/filesystem.h>


BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type) {
    cpp_compiler_t::create_library(
        builder,
        filesystem_t::find(builder, filesystem_t::filename("function_id.cpp"), filesystem_t::descend_none),
        {},
        relative_path_t("function_id"),
        library_type
    );
}

BUILDER_EXTERN void builder__import_libraries(const builder_t* builder) {
    cpp_compiler_t::create_binary(
        builder,
        filesystem_t::find(builder, filesystem_t::filename("test.cpp"), filesystem_t::descend_none),
        {},
        LIBRARY_TYPE_STATIC,
        relative_path_t("test")
    );
}
