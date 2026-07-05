#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbht7wqhtdg9hwdpmfn5o_download/download.h>

namespace m03gagbhsqfsqblhwvelrou7nc_json {

static constexpr auto JSON_SOURCE_URL = "https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp";
static constexpr auto JSON_SOURCE_SHA256 = "aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63";

extern "C" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {
    phase->install_source_tree();

    const auto json_hpp = m03gagbht7wqhtdg9hwdpmfn5o_download::fetch(
        m03gagbht7wqhtdg9hwdpmfn5o_download::source_lock_t {
            .url = JSON_SOURCE_URL,
            .sha256 = JSON_SOURCE_SHA256
        },
        phase->build_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("json.hpp")
    );
    phase->install_source(json_hpp);
}

extern "C" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {
    phase->install_headers_from_source();
}

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t*) {
}

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t* phase) {
    phase->install_cli({});
}

} // namespace m03gagbhsqfsqblhwvelrou7nc_json
