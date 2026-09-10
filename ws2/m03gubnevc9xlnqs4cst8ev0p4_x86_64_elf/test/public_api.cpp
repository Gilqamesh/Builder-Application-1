#include <m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf/x86_64_elf.h>

#include <array>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>

namespace elf = m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf;
namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;

int main() {
    std::string pattern = "/tmp/builder-lisp-elf.XXXXXX";
    if (mkdtemp(pattern.data()) == nullptr) { throw std::runtime_error("Failed to create ELF test directory"); }
    const auto directory = filesystem::path_t(pattern);
    try {
        const auto source = directory / filesystem::relative_path_t("entry.cpp");
        const auto script = directory / filesystem::relative_path_t("linker.ld");
        { std::ofstream output(source.string()); output << "extern \"C\" void _start() { for (;;) {} }\n"; }
        { std::ofstream output(script.string()); output << "ENTRY(_start)\nSECTIONS { . = 1M; .text : { *(.text*) } .data : { *(.data*) } .bss : { *(.bss*) } }\n"; }
        const auto object = elf::compile_cpp(source, directory / filesystem::relative_path_t("entry.o"), {});
        const auto executable = elf::link_elf({object}, script, directory / filesystem::relative_path_t("kernel.elf"));
        std::array<char, 4> magic;
        std::ifstream input(executable.string(), std::ios::binary);
        input.read(magic.data(), magic.size());
        if (!input || magic != std::array<char, 4>{'\x7f', 'E', 'L', 'F'}) { throw std::runtime_error("Freestanding compiler did not produce an ELF file"); }
        filesystem::remove_all(directory);
    } catch (...) {
        filesystem::remove_all(directory);
        throw;
    }
}
