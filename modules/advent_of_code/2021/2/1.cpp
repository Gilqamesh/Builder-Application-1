#include <iostream>
#include <fstream>
#include <format>
#include <sstream>

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

    int x = 0;
    int y = 0;
    std::string line;
    while (std::getline(ifs, line) && !line.empty()) {
        std::istringstream iss(line);
        std::string direction;
        iss >> direction;
        int amount;
        iss >> amount;

        if (iss.peek() != EOF) {
            throw std::runtime_error(std::format("unexpected input at line: '{}'", line));
        }

        if (direction == "forward") {
            x += amount;
        } else if (direction == "down") {
            y += amount;
        } else if (direction == "up") {
            y -= amount;
        } else {
            throw std::runtime_error(std::format("wrong direction input: {}", direction));
        }
    }

    if (std::numeric_limits<int>::max() / x < y) {
        throw std::runtime_error(std::format("overflow, x: {}, y: {}", x, y));
    }
    std::cout << x * y << std::endl;

    return 0;
}
