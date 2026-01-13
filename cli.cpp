#include <format>
#include <iostream>
#include <string>
#include <filesystem>
#include <vector>

#include <unistd.h>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << std::format("usage: {} <target-module-name> [builder-cli-args...]", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto modules_dir = std::filesystem::absolute("modules").lexically_normal();
        const auto target_module_name = std::filesystem::path(argv[1]);
        const auto artifacts_dir = std::filesystem::absolute("artifacts").lexically_normal();
        const auto root_dir = std::filesystem::absolute(modules_dir.parent_path()).lexically_normal();
        const auto builder_module_name = "builder";
        const auto build_relative_dir = "build";
        const auto install_relative_dir = "install";
        const auto builder_library_type = "shared";

        uint64_t latest_builder_version = 0;
        const auto builder_artifacts_dir = artifacts_dir / builder_module_name;
        if (std::filesystem::exists(builder_artifacts_dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(builder_artifacts_dir)) {
                if (!entry.is_directory()) {
                    continue ;
                }
                const auto path = entry.path();
                const auto filename = path.filename();
                const auto at_pos = filename.string().find('@');
                if (at_pos == std::string::npos) {
                    continue ;
                }

                const auto version_str = filename.string().substr(at_pos + 1);
                const uint64_t version = std::stoull(version_str);
                latest_builder_version = std::max(latest_builder_version, version);
            }
        }

        const auto builder_artifact_dir = std::filesystem::absolute(artifacts_dir / builder_module_name / std::format("{}@{}", builder_module_name, latest_builder_version)).lexically_normal();
        const auto builder_source_dir = std::filesystem::absolute(modules_dir / builder_module_name).lexically_normal();
        const auto builder_interface_dir = std::filesystem::absolute(builder_artifact_dir / "interface").lexically_normal();
        const auto builder_interface_build_dir = std::filesystem::absolute(builder_interface_dir / build_relative_dir).lexically_normal();
        const auto builder_interface_install_dir = std::filesystem::absolute(builder_interface_dir / install_relative_dir).lexically_normal();
        const auto builder_libraries_dir = std::filesystem::absolute(builder_artifact_dir / "libraries").lexically_normal();
        const auto builder_libraries_build_dir = std::filesystem::absolute(builder_libraries_dir / builder_library_type / build_relative_dir).lexically_normal();
        const auto builder_libraries_install_dir = std::filesystem::absolute(builder_libraries_dir / builder_library_type / install_relative_dir).lexically_normal();
        const auto builder_import_dir = std::filesystem::absolute(builder_artifact_dir / "import").lexically_normal();
        const auto builder_import_build_dir = std::filesystem::absolute(builder_import_dir / build_relative_dir).lexically_normal();
        const auto builder_import_install_dir = std::filesystem::absolute(builder_import_dir / install_relative_dir).lexically_normal();
        const auto builder_cli_src_path = std::filesystem::absolute(builder_source_dir / "cli.cpp").lexically_normal();
        const auto builder_cli_path = std::filesystem::absolute(builder_import_install_dir / "cli").lexically_normal();

        {
            const auto cli = std::filesystem::canonical("/proc/self/exe");
            const auto cli_src = root_dir / "cli.cpp";

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
                if (execv(exec_args[0], exec_args.data()) == -1) {
                    throw std::runtime_error(std::format("failed to execv updated '{}': {}", cli.string(), std::strerror(errno)));
                }
            }
        }

        if (!std::filesystem::exists(builder_cli_path) || (std::filesystem::last_write_time(builder_cli_path) < std::filesystem::last_write_time(builder_cli_src_path))) {
            const auto make_target = "import_libraries";
            const auto compile_cli_command = std::format(
                "make -C \"{}\" {}"
                " SOURCE_DIR=\"{}\""
                " LIBRARY_TYPE=\"{}\""
                " INTERFACE_BUILD_DIR=\"{}\""
                " INTERFACE_INSTALL_DIR=\"{}\""
                " LIBRARIES_BUILD_DIR=\"{}\""
                " LIBRARIES_INSTALL_DIR=\"{}\""
                " IMPORT_BUILD_DIR=\"{}\""
                " IMPORT_INSTALL_DIR=\"{}\"",
                builder_source_dir.string(), make_target,
                builder_source_dir.string(),
                builder_library_type,
                builder_interface_build_dir.string(),
                builder_interface_install_dir.string(),
                builder_libraries_build_dir.string(),
                builder_libraries_install_dir.string(),
                builder_import_build_dir.string(),
                builder_import_install_dir.string()
            );
            std::cout << compile_cli_command << std::endl;
            const int compile_cli_command_result = std::system(compile_cli_command.c_str());
            if (compile_cli_command_result) {
                throw std::runtime_error(std::format("failed to compile '{}', command exited with code '{}'", builder_cli_path.string(), compile_cli_command_result));
            }
        }

        if (!std::filesystem::exists(builder_cli_path)) {
            throw std::runtime_error(std::format("expected '{}' to exist but it does not", builder_cli_path.string()));
        }

        std::string exec_command;
        std::vector<std::string> exec_string_args;
        exec_string_args.push_back(builder_cli_path.string());
        exec_string_args.push_back(modules_dir.string());
        exec_string_args.push_back(target_module_name.string());
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
        if (execv(exec_args[0], exec_args.data()) == -1) {
            throw std::runtime_error(std::format("failed to execv '{}': {}", builder_cli_path.string(), std::strerror(errno)));
        }
    } catch (std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
