#include <fstream>
#include <format>
#include <string>
#include <cstdint>
#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <cassert>

struct board_t {
    board_t() = default;

    board_t(std::ifstream& ifs) {
        for (int row = 0; row < 5; ++row) {
            std::string line;
            std::getline(ifs, line);
            std::stringstream ss(line);
            for (int col = 0; col < 5; ++col) {
                marked[row][col] = false;
                ss >> numbers[row][col];
            }
        }
    }

    void print() {
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (marked[row][col]) {
                    std::cout << "x";
                } else {
                    std::cout << numbers[row][col];
                }
                std::cout << " ";
            }
            std::cout << std::endl;
        }
    }

    void mark(int n) {
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (numbers[row][col] == n) {
                    marked[row][col] = true;
                }
            }
        }
    }

    bool is_winning() {
        for (int row = 0; row < 5; ++row) {
            bool found_unmarked = false;
            for (int col = 0; col < 5; ++col) {
                if (!marked[row][col]) {
                    found_unmarked = true;
                    break ;
                }
            }
            if (!found_unmarked) {
                return true;
            }
        }

        for (int col = 0; col < 5; ++col) {
            bool found_unmarked = false;
            for (int row = 0; row < 5; ++row) {
                if (!marked[row][col]) {
                    found_unmarked = true;
                    break ;
                }
            }
            if (!found_unmarked) {
                return true;
            }
        }

        return false;
    }

    int sum_unmarked() {
        int sum = 0;
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (!marked[row][col]) {
                    sum += numbers[row][col];
                }
            }
        }
        return sum;
    }

    int numbers[5][5];
    bool marked[5][5];
};

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << std::format("usage: {} <input-file>", argv[0]) << std::endl;
        return 1;
    }

    const std::string filename = argv[1];
    std::ifstream ifs(filename);
    if (!ifs) {
        std::cerr << std::format("{}: could not open '{}'", argv[0], filename) << std::endl;
        return 1;
    }

    std::string line;
    std::getline(ifs, line);
    std::vector<int> winners;
    std::stringstream ss(line);
    while (ss) {
        int winner;
        ss >> winner;
        ss.get(); // skip ','
        winners.push_back(winner);
    }

    std::map<int, board_t> boards;
    int board_index = 0;
    while (std::getline(ifs, line)) {
        board_t board(ifs);
        // board.print();
        // std::cout << std::endl;
        boards[board_index++] = board;
    }

    int winner_index = 0;
    while (winner_index < winners.size()) {
        bool found_last_winning = false;
        for (auto it = boards.begin(); it != boards.end();) {
            it->second.mark(winners[winner_index]);
            if (it->second.is_winning()) {
                if (boards.size() == 1) {
                    found_last_winning = true;
                    break ;
                }
                it = boards.erase(it);
            } else {
                ++it;
            }
        }
        if (found_last_winning) {
            break ;
        }
        ++winner_index;
    }

    assert(boards.size() == 1);
    std::cout << boards.begin()->second.sum_unmarked() * winners[winner_index] << std::endl;

    return 0;
}
