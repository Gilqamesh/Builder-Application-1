#include <iostream>
#include <vector>

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n;
        std::cin >> n;
        std::vector<int> a(n);
        for (int j = 0; j < n; ++j) {
            std::cin >> a[j];
            if (0 < j && j + 1 < n && a[j] == -1) {
                a[j] = 0;
            }
        }

        if (a[0] == -1) {
            if (a[n - 1] == -1) {
                a[0] = 0;
            } else {
                a[0] = a[n - 1];
            }
        }

        if (a[n - 1] == -1) {
            a[n - 1] = a[0];
        }

        int sum = 0;
        for (int j = 1; j < n; ++j) {
            sum += a[j] - a[j - 1];
        }
        sum = std::abs(sum);
        std::cout << sum << std::endl;
        for (int j : a) {
            std::cout << j << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
