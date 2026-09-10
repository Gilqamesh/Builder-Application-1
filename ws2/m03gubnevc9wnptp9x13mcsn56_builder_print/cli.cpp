#include <m03gubnevc9wnptp9x13mcsn56_builder_print/builder_print.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <value>\n";
        return 1;
    }

    try {
        const auto result = m03gubnevc9wnptp9x13mcsn56_builder_print::apply({ m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(argv[1]) });
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result(result);
        std::cout << m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(result) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
