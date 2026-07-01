#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

namespace m03ge9ij4hzaaehqgc59cj8f9q_rl_imgui {

extern "C" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {
    phase->install_source_tree();
}

extern "C" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    phase->install_interface(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(sources.root(), m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("rlImGui.h")));
    phase->install_interface(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(sources.root(), m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("imgui_impl_raylib.h")));
    phase->install_interface(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(sources.root(), m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("rlImGuiColors.h")));
    phase->install_interface(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(sources.root(), m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("extras/IconsFontAwesome6.h")));
    phase->install_interface(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(sources.root(), m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("extras/FA6FreeSolidFontData.h")));
}

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    const auto library = phase->build_library(
        {
            phase->build(sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("rlImGui.cpp"))
        },
        {}
    );
    phase->install_library(library);
}

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t* phase) {
}

} // namespace m03ge9ij4hzaaehqgc59cj8f9q_rl_imgui
