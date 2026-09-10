#ifndef M03GUBNEVC9XLNQS4CST8EV0P4_X86_64_ELF_X86_64_ELF_H
# define M03GUBNEVC9XLNQS4CST8EV0P4_X86_64_ELF_X86_64_ELF_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <vector>


namespace m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf {

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t cxx_path();

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t compile_cpp(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source_file,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& object_file,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs
);

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t link_elf(
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& object_files,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& linker_script,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_elf
);

} // namespace m03gubnevc9xlnqs4cst8ev0p4_x86_64_elf

#endif // M03GUBNEVC9XLNQS4CST8EV0P4_X86_64_ELF_X86_64_ELF_H
