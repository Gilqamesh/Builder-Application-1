#include "x86_64_elf.h"

#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>
#include <string>

#ifndef M03GUBNEVC9XLNQS4CST8EV0P4_X86_64_ELF_CXX_PATH
# error M03GUBNEVC9XLNQS4CST8EV0P4_X86_64_ELF_CXX_PATH must be defined by the owning builder
#endif

namespace m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf {

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t cxx_path() {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GUBNEVC9XLNQS4CST8EV0P4_X86_64_ELF_CXX_PATH);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("x86_64_elf: C++ compiler '{}' does not exist or is not a regular file", result));
    }

    return result;
}

static void throw_process_error(
    const std::string& operation,
    int process_result
) {
    if (0 < process_result) {
        throw std::runtime_error(std::format("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::{}: command failed with exit code {}", operation, process_result));
    } else if (process_result < 0) {
        throw std::runtime_error(std::format("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::{}: command terminated by signal {}", operation, -process_result));
    }
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t compile_cpp(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source_file,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& object_file,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs
) {
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(source_file) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(source_file)) {
        throw std::runtime_error(std::format("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::compile_cpp: source file '{}' does not exist or is not a regular file", source_file));
    }

    for (const auto& include_dir : include_dirs) {
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(include_dir) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_directory(include_dir)) {
            throw std::runtime_error(std::format("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::compile_cpp: include dir '{}' does not exist or is not a directory", include_dir));
        }
    }

    const auto object_dir = object_file.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(object_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(object_dir);
    }

    std::vector<std::string> process_args;
    process_args.push_back(cxx_path().string());
    process_args.push_back("--target=x86_64-elf");
    process_args.push_back("-std=c++23");
    process_args.push_back("-ffreestanding");
    process_args.push_back("-fno-exceptions");
    process_args.push_back("-fno-rtti");
    process_args.push_back("-fno-stack-protector");
    process_args.push_back("-fno-pic");
    process_args.push_back("-mno-red-zone");
    process_args.push_back("-nostdlib");
    for (const auto& include_dir : include_dirs) {
        process_args.push_back(std::format("-I{}", include_dir));
    }
    process_args.push_back("-c");
    process_args.push_back(source_file.string());
    process_args.push_back("-o");
    process_args.push_back(object_file.string());

    throw_process_error("compile_cpp", m03gagbhsvr0m5w15urj0o291m_process::create_and_wait(m03gagbhsvr0m5w15urj0o291m_process::command_t { process_args }));

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(object_file)) {
        throw std::runtime_error(std::format("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::compile_cpp: expected output object '{}' to exist but it does not", object_file));
    }

    return object_file;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t link_elf(
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& object_files,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& linker_script,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_elf
) {
    if (object_files.empty()) {
        throw std::runtime_error("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::link_elf: at least one object file is required");
    }

    for (const auto& object_file : object_files) {
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(object_file) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(object_file)) {
            throw std::runtime_error(std::format("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::link_elf: object file '{}' does not exist or is not a regular file", object_file));
        }
    }

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(linker_script) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(linker_script)) {
        throw std::runtime_error(std::format("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::link_elf: linker script '{}' does not exist or is not a regular file", linker_script));
    }

    const auto output_dir = output_elf.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(output_dir);
    }

    std::vector<std::string> process_args;
    process_args.push_back(cxx_path().string());
    process_args.push_back("--target=x86_64-elf");
    process_args.push_back("-nostdlib");
    process_args.push_back(std::format("-Wl,-T,{}", linker_script));
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file.string());
    }
    process_args.push_back("-o");
    process_args.push_back(output_elf.string());

    throw_process_error("link_elf", m03gagbhsvr0m5w15urj0o291m_process::create_and_wait(m03gagbhsvr0m5w15urj0o291m_process::command_t { process_args }));

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_elf)) {
        throw std::runtime_error(std::format("m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf::link_elf: expected output ELF '{}' to exist but it does not", output_elf));
    }

    return output_elf;
}

} // namespace m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf
