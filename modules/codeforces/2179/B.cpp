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
        }

        int sum = 0;
        int k = 0;
        int max_k = std::abs(a[1] - a[0]);
        for (int j = 1; j + 1 < n; ++j) {
            sum += std::abs(a[j] - a[j - 1]);
            int sub_k = std::abs(a[j] - a[j - 1]) + std::abs(a[j + 1] - a[j]) - std::abs(a[j + 1] - a[j - 1]);
            if (max_k < sub_k) {
                k = j;
                max_k = sub_k;
            }
        }
        sum += std::abs(a[n - 1] - a[n - 2]);
        int sub_k = std::abs(a[n - 1] - a[n - 2]);
        if (max_k < sub_k) {
            max_k = sub_k;
            k = n - 1;
        }
        std::cout << sum - max_k << std::endl;
    }

    return 0;
}
