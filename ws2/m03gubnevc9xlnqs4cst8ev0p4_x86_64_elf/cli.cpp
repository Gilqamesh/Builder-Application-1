#include <m03gubnevc9suwm9ce9dty0f1w_lisp_x86_64_elf/apply.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 1) {
        std::cerr << "usage: " << argv[0] << "\n";
        return 1;
    }

    try {
        const auto result = m03gubnevc9suwm9ce9dty0f1w_lisp_x86_64_elf::apply({});
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result(result);
        std::cout << "x86_64-elf toolchain: " << m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(result).string() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
