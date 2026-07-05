#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell {

extern "C" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {
    phase->install_source_tree();
}

extern "C" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    phase->install_interface(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(sources.root(), m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("module_shell.h")));
}

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    const auto library = phase->build_library(
        { phase->build(sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("module_shell.cpp")) },
        {}
    );
    phase->install_library(library);
    
    const auto readline_so_as = m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("libreadline.so");
    const auto readline_so_path = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t("/usr/lib64") / readline_so_as;
    const auto readline_so = phase->build(readline_so_path, readline_so_as);
    phase->install_library(readline_so);

    const auto history_so_as = m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("libhistory.so");
    const auto history_so_path = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t("/usr/lib64") / history_so_as;
    const auto history_so = phase->build(history_so_path, history_so_as);
    phase->install_library(history_so);
}

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t* phase) {
    phase->install_cli({});
}

} // namespace m03gf09la5rvbh6kk4vvt1qawv_module_shell
