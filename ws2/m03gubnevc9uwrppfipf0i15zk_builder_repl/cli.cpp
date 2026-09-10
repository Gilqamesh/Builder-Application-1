#include <m03gubnevc9uwrppfipf0i15zk_builder_repl/builder_repl.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char**) {
    if (argc != 1) {
        std::cerr << "usage: builder_repl\n";
        return 1;
    }

    try {
        const auto result = m03gubnevc9uwrppfipf0i15zk_builder_repl::apply({});
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::publish_result(result);
    } catch (const std::exception& e) {
        std::cerr << std::format("builder_repl: {}", e.what()) << std::endl;
        return 1;
    }

    return 0;
}
