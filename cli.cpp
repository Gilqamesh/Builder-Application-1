#include <format>
#include <iostream>
#include <string>
#include <filesystem>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << std::format("usage: {} <module-name> [binary-relative-to-module]", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto builder_dir = std::filesystem::path("builder");
        const auto modules_dir = std::filesystem::path("modules");
        const auto module_dir = std::filesystem::path(argv[1]);
        const auto artifacts_dir = std::filesystem::path("artifacts");
        const auto builder_cli_path = std::filesystem::path("builder_cli");

        if (!std::filesystem::exists(builder_cli_path)) {
            const auto compile_cli_command = std::format("clang++ -std=c++23 {}/*.cpp -o {}", builder_dir.string(), builder_cli_path.string());
            std::cout << compile_cli_command << std::endl;
            const int compile_cli_command_result = std::system(compile_cli_command.c_str());
            if (compile_cli_command_result) {
                throw std::runtime_error(std::format("failed to compile '{}', command exited with code '{}'", builder_cli_path.string(), compile_cli_command_result));
            }
        }

        if (!std::filesystem::exists(builder_cli_path)) {
            throw std::runtime_error(std::format("expected '{}' to exist but it does not", builder_cli_path.string()));
        }

        const auto run_builder_cli_command = std::format("./{} {} {} {} {}", builder_cli_path.string(), builder_dir.string(), modules_dir.string(), module_dir.string(), artifacts_dir.string());
        std::cout << run_builder_cli_command << std::endl;
        const int run_builder_cli_command_result = std::system(run_builder_cli_command.c_str());
        if (run_builder_cli_command_result) {
            throw std::runtime_error(std::format("failed to run '{}', command exited with code '{}'", builder_cli_path.string(), run_builder_cli_command_result));
        }

        if (3 <= argc) {
            const auto binary_relative_to_module = argv[2];
            const auto module_artifact_dir = artifacts_dir / module_dir;
            std::filesystem::path module_artifact_latest_versioned_dir;
            for (const auto& entry : std::filesystem::directory_iterator(module_artifact_dir)) {
                if (!entry.is_directory()) {
                    continue ;
                }
                const auto module_artifact_versioned_dir = entry.path();
                if (module_artifact_latest_versioned_dir < module_artifact_versioned_dir) {
                    module_artifact_latest_versioned_dir = module_artifact_versioned_dir;
                }
            }
            if (module_artifact_latest_versioned_dir.empty()) {
                throw std::runtime_error(std::format("expected at least one versioned artifact directory in '{}' but found none", module_artifact_dir.string()));
            }

            const auto module_binaries_path = module_artifact_latest_versioned_dir / "module";
            const auto binary_path = module_binaries_path / binary_relative_to_module;
            const auto binary = std::format("./{}", binary_path.string());
            if (!std::filesystem::exists(binary)) {
                throw std::runtime_error(std::format("binary '{}' does not exist", binary));
            }

            std::cout << binary << std::endl;
            std::system(binary.c_str());
        }
    } catch (std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
