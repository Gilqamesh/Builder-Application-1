#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

namespace m03ge9ij4lbns2mq6722cd8654_function_visualizer {

extern "C" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {
    phase->install_source_tree();
}

extern "C" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {
}

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
}

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    const auto cli = phase->build_cli(
        {
            phase->build(sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("function_visualizer.cpp"))
        },
        {}
    );
    phase->install_cli(cli);
}

} // namespace m03ge9ij4lbns2mq6722cd8654_function_visualizer
