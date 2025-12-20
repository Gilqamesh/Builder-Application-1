#include <iostream>
#include <fstream>
#include <format>
#include <sstream>
#include <cstdint>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << std::format("usage: {} <input_file>", argv[0]) << std::endl;
        return 1;
    }

    const std::string filename = argv[1];
    std::ifstream ifs(filename);
    if (!ifs) {
        std::cerr << std::format("unable to open file '{}'", filename) << std::endl;
        return 1;
    }

    uint64_t horizontal_position = 0;
    uint64_t depth = 0;
    uint64_t aim = 0;
    std::string line;
    while (std::getline(ifs, line) && !line.empty()) {
        std::istringstream iss(line);
        std::string direction;
        iss >> direction;
        uint64_t amount;
        iss >> amount;

        if (iss.peek() != EOF) {
            throw std::runtime_error(std::format("unexpected input at line: '{}'", line));
        }

        if (direction == "forward") {
            horizontal_position += amount;
            depth += aim * amount;
        } else if (direction == "down") {
            aim += amount;
        } else if (direction == "up") {
            aim -= amount;
        } else {
            throw std::runtime_error(std::format("wrong direction input: {}", direction));
        }
    }

    if (std::numeric_limits<uint64_t>::max() / horizontal_position < depth) {
        throw std::runtime_error(std::format("overflow, horizontal_position: {}, depth: {}", horizontal_position, depth));
    }
    std::cout << horizontal_position * depth << std::endl;

    return 0;
}
