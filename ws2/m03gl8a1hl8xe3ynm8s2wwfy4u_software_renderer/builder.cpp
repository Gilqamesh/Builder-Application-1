#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t* phase) {
    phase->install_binary("cli", {phase->source("cli.cpp")});
    phase->install_binary("benchmark", {phase->source("benchmark.cpp")});
}
