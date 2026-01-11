#include <iostream>
#include <vector>
#include <algorithm>

#include <climits>
#include <cstdint>

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n;
        std::cin >> n;
        std::vector<std::vector<int>> a(2, std::vector<int>(n));
        std::vector<std::pair<int, int>> intervals(n);
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < n; ++k) {
                std::cin >> a[j][k];
            }
        }

        for (int row_switch = 0; row_switch < n; ++row_switch) {
            int col = 0;
            int l = INT_MAX;
            int r = INT_MIN;
            while (col <= row_switch) {
                l = std::min(l, a[0][col]);
                r = std::max(r, a[0][col]);
                ++col;
            }
            --col;
            while (col < n) {
                l = std::min(l, a[1][col]);
                r = std::max(r, a[1][col]);
                ++col;
            }
            intervals[row_switch] = { l, r };
        }

        std::sort(intervals.begin(), intervals.end(), [](const std::pair<int, int>& p1, const std::pair<int, int>& p2) {
            return p1.first < p2.first || (p1.first == p2.first && p1.second < p2.second);
        });

        uint64_t res = intervals[0].first * (2 * n - (uint64_t)intervals[0].second + 1);
        for (size_t j = 1; j < intervals.size(); ++j) {
            res += (intervals[j].first - intervals[j - 1].first) * (2 * n - (uint64_t)intervals[j].second + 1);
        }

        std::cout << res << std::endl;
    }

    return 0;
}
