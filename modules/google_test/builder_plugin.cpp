#include <modules/builder/builder.h>
#include <modules/builder/curl/curl.h>
#include <modules/builder/zip/zip.h>

BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type) {
    const auto tar_gz_path = curl_t::download("https://github.com/google/googletest/archive/refs/tags/v1.15.0.tar.gz", builder->build_dir(library_type) / "googletest.tar.gz");
    const auto tar_path = zip_t::unzip(tar_gz_path, builder->build_dir(library_type) / "googletest.tar");
}

BUILDER_EXTERN void builder__import_libraries(const builder_t* builder) {
}
