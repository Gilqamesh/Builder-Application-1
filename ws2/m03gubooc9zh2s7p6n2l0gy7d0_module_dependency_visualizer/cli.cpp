#include "api.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    try {
        return m03gubooc9zh2s7p6n2l0gy7d0_module_dependency_visualizer::run(argc, argv);
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
