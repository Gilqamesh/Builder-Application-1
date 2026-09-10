#include <m03gubnevc9vfwh0ka3iz3mjuf_builder_eval/builder_eval.h>

#include <m03gubnevca18el75zd2vx8qxh_language/language.h>

#include <exception>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void publish_and_print(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& result) {
    m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result(result);
    std::cout << m03gubnevca18el75zd2vx8qxh_language::print(result) << std::endl;
}

void print_usage(const char* argv0) {
    std::cerr << "usage: " << argv0 << " <expression>\n";
    std::cerr << "       " << argv0 << " --file <path>\n";
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t file_arg(std::string_view value) {
    if (!value.empty() && value.front() == '/') {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(std::string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::invocation_context().workspace_root
        / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::string(value));
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        if (argc == 3) {
            if (std::string_view(argv[1]) != "--file") {
                print_usage(argv[0]);
                return 1;
            }

            publish_and_print(m03gubnevc9vfwh0ka3iz3mjuf_builder_eval::evaluate_file(file_arg(argv[2])));
        } else {
            publish_and_print(m03gubnevc9vfwh0ka3iz3mjuf_builder_eval::apply({
                m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(argv[1])
            }));
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
