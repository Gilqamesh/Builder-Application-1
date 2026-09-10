#include <m03gubnevca0fyv87f6svumxgw_builder_read/builder_read.h>

#include <m03gubnevca18el75zd2vx8qxh_language/language.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <expression>\n";
        return 1;
    }

    try {
        const auto result = m03gubnevca0fyv87f6svumxgw_builder_read::apply({ m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(argv[1]) });
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result(result);
        std::cout << m03gubnevca18el75zd2vx8qxh_language::print(result) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
