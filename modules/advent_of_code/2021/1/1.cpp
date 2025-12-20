#include <iostream>

#include <fstream>
#include <format>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << std::format("usage: {} <input_file>", argv[0]) << std::endl;
        return 1;
    }

    const std::string filename = argv[1];
    std::ifstream ifs(filename);
    if (!ifs) {
        std::cerr << std::format("could not file '{}'", filename) << std::endl;
        return 1;
    }

    std::string line;
    int prev = std::numeric_limits<int>::max();
    int result = 0;
    while (std::getline(ifs, line) && !line.empty()) {
        const int cur = std::atoi(line.c_str());
        if (prev < cur) {
            ++result;
        }
        prev = cur;
    }

    std::cout << result << std::endl;

    return 0;
}
