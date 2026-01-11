#include <iostream>
#include <vector>

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n;
        std::cin >> n;
        std::vector<int> a(n);
        int first_diff = -1;
        bool start_with_r = false;
        bool impossible = false;
        for (int j = 0; j < n; ++j) {
            std::cin >> a[j];
            
            if (0 < j && a[j] != a[j - 1] && first_diff == -1) {
                first_diff = j;
                if (1 < std::abs(a[j] - a[j - 1])) {
                    impossible = true;
                }
                start_with_r = ((j % 2 == 1) && a[j - 1] == a[j] + 1) || ((j % 2 == 0) && a[j - 1] + 1 == a[j]);
            }
        }
        if (impossible) {
            std::cout << 0 << std::endl;
            continue ;
        }

        if (first_diff == -1) {
            if (n % 2 == 1) {
                if (n / 2 + 1 == a[0]) {
                    std::cout << 2 << std::endl;
                } else {
                    std::cout << 0 << std::endl;
                }
            } else {
                if (n / 2 == a[0] || n / 2 + 1 == a[0]) {
                    std::cout << 1 << std::endl;
                } else {
                    std::cout << 0 << std::endl;
                }
            }
        } else {
            int ls = start_with_r ? 0 : 1;
            bool is_cur_r = start_with_r;
            std::vector<int> b(n, 1);
            for (int j = 1; j < n; ++j) {
                b[j] += ls;

                if (a[j] == a[j - 1]) {
                    is_cur_r = !is_cur_r;
                }

                if (!is_cur_r) {
                    ++ls;
                }
            }

            int rs = is_cur_r ? 1 : 0;
            for (int j = n - 2; 0 <= j; --j) {
                b[j] += rs;

                if (a[j] == a[j + 1]) {
                    is_cur_r = !is_cur_r;
                }

                if (is_cur_r) {
                    ++rs;
                }
            }

            if (b == a) {
                std::cout << 1 << std::endl;
            } else {
                std::cout << 0 << std::endl;
            }
        }
    }
}
