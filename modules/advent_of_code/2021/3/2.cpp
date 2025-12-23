#include <iostream>
#include <fstream>
#include <format>
#include <string>
#include <vector>
#include <cstdint>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << std::format("usage: {} <inpu_file>", argv[0]) << std::endl;
        return 1;
    }

    const std::string filename = argv[1];
    std::ifstream ifs(filename);
    if (!ifs) {
        std::cerr << std::format("could not open file '{}'", filename) << std::endl;
        return 1;
    }

    std::vector<int> zeros;
    std::vector<int> ones;
    std::string line;
    while (std::getline(ifs, line) && !line.empty()) {
        if (zeros.empty()) {
            zeros = std::vector<int>(line.size(), 0);
            ones = std::vector<int>(line.size(), 0);
        }

        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '1') {
                ++ones[i];
            } else {
                ++zeros[i];
            }
        }
    }

    uint64_t gamma_rate = 0;
    uint64_t epsilon_rate = 0;
    for (size_t i = 0; i < ones.size(); ++i) {
        const int n_ones = ones[i];
        const int n_zeros = zeros[i];
        gamma_rate = (gamma_rate << 1) | (n_ones < n_zeros ? 1 : 0);
        epsilon_rate = (epsilon_rate << 1) | (n_ones < n_zeros ? 0 : 1);
    }

    std::cout << gamma_rate * epsilon_rate << std::endl;

    return 0;
}
