#include <iostream>

#include <fstream>
#include <format>
#include <string>
#include <vector>

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
    std::vector<int> arr;
    while (std::getline(ifs, line) && !line.empty()) {
        arr.push_back(std::atoi(line.c_str()));
    }

    const int window_width = 3;
    if (arr.size() <= window_width) {
        std::cout << 0 << std::endl;
        return 0;
    }
    
    int sum = 0;
    for (int i = 0; i < window_width && i < arr.size(); ++i) {
        sum += arr[i];
    }

    int prev_sum = sum;
    int result = 0;
    for (int i = 1; i + window_width <= arr.size(); ++i) {
        sum -= arr[i - 1];
        sum += arr[i + window_width - 1];
        if (prev_sum < sum) {
            ++result;
        }
        prev_sum = sum;
    }

    std::cout << result << std::endl;

    return 0;
}
