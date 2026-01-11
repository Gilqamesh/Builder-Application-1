#include <iostream>
#include <string>
#include <vector>

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n, x, y;
        std::cin >> n >> x >> y;
        std::string s;
        std::cin >> s;
        std::vector<int> p(n);
        std::vector<int> a(n);
        std::vector<int> b(n);
        for (int j = 0; j < n; ++j) {
            std::cin >> p[j];
            const int inc = p[j] / 2 + 1;
            if (s[j] == '0') {
                if (x < inc) {
                    x = -1;
                    break ;
                }
                x -= inc;
                a[j] = inc;
                b[j] = 0;
                p[j] -= inc;
            } else {
                if (y < inc) {
                    y = -1;
                    break ;
                }
                y -= inc;
                a[j] = 0;
                b[j] = inc;
                p[j] -= inc;
            }
        }
        if (x < 0 || y < 0) {
            std::cout << "NO" << std::endl;
            continue ;
        }

        bool no = false;
        for (int j = 0; j < n; ++j) {
            if (0 < x) {
                int to_dec = std::min(p[j], x);
                p[j] -= to_dec;
                a[j] += to_dec;
                x -= to_dec;
            }
            if (0 < y) {
                int to_dec = std::min(p[j], y);
                p[j] -= to_dec;
                b[j] += to_dec;
                y -= to_dec;
            }

            if (0 < p[j]) {
                std::cout << "NO" << std::endl;
                no = true;
                break ;
            }
        }
        if (no) {
            continue ;
        }

        for (int j = 0; j < n; ++j) {
            if (s[j] == '0') {
                a[j] += x;
                x = 0;
                const int max_y_to_dump = std::min(y, a[j] - b[j] - 1);
                b[j] += max_y_to_dump;
                y -= max_y_to_dump;
            } else {
                b[j] += y;
                y = 0;
                const int max_x_to_dump = std::min(x, b[j] - a[j] - 1);
                a[j] += max_x_to_dump;
                x -= max_x_to_dump;
            }
        }

        if (0 < x || 0 < y) {
            std::cout << "NO" << std::endl;
        } else {
            std::cout << "YES" << std::endl;
        }
    }

    return 0;
}
