#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdint>

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n;
        std::cin >> n;
        std::vector<int> a(n);
        for (int j = 0; j < n; ++j) {
            std::cin >> a[j];
        }
        std::sort(a.begin(), a.end());
        std::vector<int> odds;
        uint64_t l = 0;
        for (int j = 0; j < n; ++j) {
            int start = j;
            while (j < n && a[j] == a[start]) {
                ++j;
            }
            int how_many = j - start;
            l += how_many / 2 * (uint64_t)a[start];
            if (how_many % 2 == 1) {
                odds.push_back(a[start]);
            }
            --j;
        }

        // see if we can choose up to 2 odds
        int valid_odd_length = 0;
        for (int j = (int)odds.size() - 1; 0 <= j; --j) {
            int next_biggest = 0;
            if (0 < j) {
                next_biggest = odds[j - 1];
            }
            if (odds[j] - next_biggest < 2 * l) {
                valid_odd_length = odds[j] + next_biggest;
                break ;
            }
        }

        if (n - odds.size() <= 2 && valid_odd_length == 0) {
            std::cout << 0 << std::endl;
        } else {
            std::cout << l * 2 + valid_odd_length << std::endl;
        }
    }

    return 0;
}
