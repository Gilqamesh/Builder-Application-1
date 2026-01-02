#include <iostream>
#include <fstream>
#include <format>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_set>
#include <cassert>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << std::format("usage: {} <input_file>", argv[0]) << std::endl;
        return 1;
    }

    const std::string filename = argv[1];
    std::ifstream ifs(filename);
    if (!ifs) {
        std::cerr << std::format("could not open file '{}'", filename) << std::endl;
        return 1;
    }

    std::unordered_set<uint64_t> most_commons;
    std::unordered_set<uint64_t> least_commons;
    std::string line;
    size_t cols = 0;
    while (std::getline(ifs, line) && !line.empty()) {
        if (cols == 0) {
            cols = line.size();
        }
        uint64_t n = 0;
        for (char c : line) {
            n = (n << 1) | (c == '1' ? 1 : 0);
        }
        most_commons.insert(n);
        least_commons.insert(n);
    }

    for (size_t col = 0; col < cols && (1 < most_commons.size() || 1 < least_commons.size()); ++col) {
        const uint64_t one_mask = 1 << (cols - col - 1);
        if (1 < most_commons.size()) {
            int ones_minus_zeros = 0;
            for (uint64_t n : most_commons) {
                ones_minus_zeros += (n & one_mask) == 0 ? -1 : 1;
            }
            for (auto it = most_commons.begin(); it != most_commons.end();) {
                if (0 <= ones_minus_zeros) {
                    if (((*it) & one_mask) == 0) {
                        it = most_commons.erase(it);
                    } else {
                        ++it;
                    }
                } else {
                    if (((*it) & one_mask) == 0) {
                        ++it;
                    } else {
                        it = most_commons.erase(it);
                    }
                }
            }
        }
        if (1 < least_commons.size()) {
            int ones_minus_zeros = 0;
            for (uint64_t n : least_commons) {
                ones_minus_zeros += (n & one_mask) == 0 ? -1 : 1;
            }
            for (auto it = least_commons.begin(); it != least_commons.end();) {
                if (ones_minus_zeros < 0) {
                    if (((*it) & one_mask) == 0) {
                        it = least_commons.erase(it);
                    } else {
                        ++it;
                    }
                } else {
                    if (((*it) & one_mask) == 0) {
                        ++it;
                    } else {
                        it = least_commons.erase(it);
                    }
                }
            }
        }
    }

    assert(most_commons.size() == 1);
    assert(least_commons.size() == 1);

    auto it1 = most_commons.begin();
    auto it2 = least_commons.begin();
    std::cout << (*it1) * (*it2) << std::endl;

    return 0;
}
