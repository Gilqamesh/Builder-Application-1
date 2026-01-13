#include <builder/builder.h>
#include <builder/curl/curl.h>
#include <builder/gzip/gzip.h>
#include <builder/tar/tar.h>
#include <builder/cmake/cmake.h>

#include <format>

BUILDER_EXTERN void builder__export_interface(const builder_t* builder, library_type_t library_type) {
    const auto tar_gz_path = curl_t::download("https://github.com/google/googletest/archive/refs/tags/v1.15.0.tar.gz", builder->interface_build_dir(library_type) / relative_path_t("googletest.tar.gz"));
    const auto tar_path = gzip_t::ungzip(tar_gz_path, builder->interface_build_dir(library_type) / relative_path_t("googletest.tar"));
    const auto googletest_path = tar_t::untar(tar_path, builder->interface_build_dir(library_type) / relative_path_t("googletest"));
    const auto googletest_cmake_dir_path = googletest_path / relative_path_t("googletest-1.15.0");

    cmake_t::configure(
        googletest_cmake_dir_path,
        builder->interface_build_dir(library_type),
        {
            { "BUILD_SHARED_LIBS", library_type == library_type_t::STATIC ? "OFF" : "ON" },
            { "INSTALL_GTEST", "ON" },
            { "CMAKE_INSTALL_INCLUDEDIR", builder->interface_install_dir(library_type).string() },
            { "CMAKE_INSTALL_LIBDIR", builder->libraries_build_dir(library_type).string() }
        }
    );

    builder->install_interface(builder->source_dir() / relative_path_t("google_test.h"), library_type);
}

BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type) {
    cmake_t::build(builder->interface_build_dir(library_type), std::nullopt);
    cmake_t::install(builder->interface_build_dir(library_type));

    switch (library_type) {
        case library_type_t::STATIC: {
            const auto gtest_static_libs = {
                relative_path_t("libgmock.a"),
                relative_path_t("libgmock_main.a"),
                relative_path_t("libgtest.a"),
                relative_path_t("libgtest_main.a")
            };
            for (const auto& gtest_static_lib : gtest_static_libs) {
                builder->install_library(builder->libraries_build_dir(library_type) / gtest_static_lib, library_type);
            }
        } break ;
        case library_type_t::SHARED: {
            const auto gtest_shared_libs = {
                relative_path_t("libgmock_main.so"),
                relative_path_t("libgmock_main.so.1.15.0"),
                relative_path_t("libgmock.so"),
                relative_path_t("libgmock.so.1.15.0"),
                relative_path_t("libgtest_main.so"),
                relative_path_t("libgtest_main.so.1.15.0"),
                relative_path_t("libgtest.so"),
                relative_path_t("libgtest.so.1.15.0")
            };
            for (const auto& gtest_shared_lib : gtest_shared_libs) {
                builder->install_library(builder->libraries_build_dir(library_type) / gtest_shared_lib, library_type);
            }
        } break ;
        default: throw std::runtime_error(std::format("builder__export_libraries: unknown library_type {}", static_cast<int>(library_type)));
    }
}

BUILDER_EXTERN void builder__import_libraries(const builder_t* builder) {
}
