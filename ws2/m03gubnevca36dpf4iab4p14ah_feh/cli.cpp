#include <m03gubnevca2ecj4faehttuwh1_lisp_feh/apply.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <path>\n";
        return 1;
    }

    try {
        const auto result = m03gubnevca2ecj4faehttuwh1_lisp_feh::apply({
            m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(argv[1]))
        });
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result(result);
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
