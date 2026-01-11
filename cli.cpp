#include <format>
#include <iostream>
#include <string>
#include <filesystem>
#include <source_location>
#include <vector>

#include <unistd.h>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << std::format("usage: {} <module-name> [builder-cli-args...]", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto modules_dir = std::filesystem::path("modules");
        const auto module_dir = std::filesystem::path(argv[1]);
        const auto artifacts_dir = std::filesystem::path("artifacts");

        const auto builder_dir = modules_dir / "builder";

        const auto root_dir = modules_dir.parent_path().empty() ? "." : modules_dir.parent_path();

        const auto builder_cli_src_path = builder_dir / "cli.cpp";

        const auto builder_version_zero_artifact_dir = artifacts_dir / "builder" / "builder@0";
        const auto builder_version_zero_library_type = "shared";
        const auto builder_version_zero_build_dir = builder_version_zero_artifact_dir / "build" / builder_version_zero_library_type;
        const auto builder_version_zero_export_dir = builder_version_zero_artifact_dir / "export" / builder_version_zero_library_type;
        const auto builder_version_zero_import_dir = builder_version_zero_artifact_dir / "import";
        const auto builder_version_zero_cli_path = builder_version_zero_import_dir / "cli";

        {
            const auto cli = std::filesystem::canonical("/proc/self/exe");
            const auto cli_src = root_dir / std::filesystem::path(std::source_location::current().file_name()).filename();

            std::error_code ec;
            const auto cli_last_write_time = std::filesystem::last_write_time(cli, ec);
            if (ec) {
                throw std::runtime_error(std::format("failed to get last write time of cli '{}': {}", cli.string(), ec.message()));
            }

            const auto cli_src_last_write_time = std::filesystem::last_write_time(cli_src, ec);
            if (ec) {
                throw std::runtime_error(std::format("failed to get last write time of cli source '{}': {}", cli_src.string(), ec.message()));
            }

            if (cli_last_write_time < cli_src_last_write_time) {
                const auto command = std::format("clang++ -g -std=c++23 -o {} {}", cli.string(), cli_src.string());
                std::cout << command << std::endl;
                const int command_result = std::system(command.c_str());
                if (command_result != 0) {
                    throw std::runtime_error(std::format("failed to compile updated '{}', command exited with code {}", cli.string(), command_result));
                }

                if (!std::filesystem::exists(cli)) {
                    throw std::runtime_error(std::format("expected updated '{}' to exist but it does not", cli.string()));
                }

                std::string exec_command;
                std::vector<char*> exec_args;
                for (int i = 0; i < argc; ++i) {
                    exec_args.push_back(argv[i]);
                    if (!exec_command.empty()) {
                        exec_command += " ";
                    }
                    exec_command += argv[i];
                }
                exec_args.push_back(nullptr);

                std::cout << exec_command << std::endl;
                if (execv(cli.c_str(), exec_args.data()) == -1) {
                    throw std::runtime_error(std::format("failed to execv updated '{}': {}", cli.string(), std::strerror(errno)));
                }
            }
        }

        if (!std::filesystem::exists(builder_version_zero_cli_path) || std::filesystem::last_write_time(builder_version_zero_cli_path) < std::filesystem::last_write_time(builder_cli_src_path)) {
            const auto compile_cli_command = std::format(
                "make -C \"{}\" BUILD_DIR=\"{}\" EXPORT_DIR=\"{}\" IMPORT_DIR=\"{}\" LIBRARY_TYPE=\"{}\"",
                builder_dir.string(),
                std::filesystem::absolute(builder_version_zero_build_dir).string(),
                std::filesystem::absolute(builder_version_zero_export_dir).string(),
                std::filesystem::absolute(builder_version_zero_import_dir).string(),
                builder_version_zero_library_type
            );
            std::cout << compile_cli_command << std::endl;
            const int compile_cli_command_result = std::system(compile_cli_command.c_str());
            if (compile_cli_command_result) {
                throw std::runtime_error(std::format("failed to compile '{}', command exited with code '{}'", builder_version_zero_cli_path.string(), compile_cli_command_result));
            }
        }

        if (!std::filesystem::exists(builder_version_zero_cli_path)) {
            throw std::runtime_error(std::format("expected '{}' to exist but it does not", builder_version_zero_cli_path.string()));
        }

        std::string exec_command;
        std::vector<std::string> exec_string_args;
        exec_string_args.push_back(builder_version_zero_cli_path.string());
        exec_string_args.push_back(modules_dir.string());
        exec_string_args.push_back(module_dir.string());
        exec_string_args.push_back(artifacts_dir.string());
        for (int i = 2; i < argc; ++i) {
            exec_string_args.push_back(argv[i]);
        }
        std::vector<char*> exec_args;
        for (const auto& exec_string_arg : exec_string_args) {
            exec_args.push_back(const_cast<char*>(exec_string_arg.c_str()));
            if (!exec_command.empty()) {
                exec_command += " ";
            }
            exec_command += exec_string_arg;
        }
        exec_args.push_back(nullptr);

        std::cout << exec_command << std::endl;
        if (execv(builder_version_zero_cli_path.c_str(), exec_args.data()) == -1) {
            throw std::runtime_error(std::format("failed to execv '{}': {}", builder_version_zero_cli_path.string(), std::strerror(errno)));
        }
    } catch (std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
