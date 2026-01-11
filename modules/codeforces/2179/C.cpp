#include <iostream>
#include <climits>

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n;
        std::cin >> n;
        // m1 <= m2
        int m1 = INT_MAX;
        int m2 = INT_MAX;
        for (int j = 0; j < n; ++j) {
            int a;
            std::cin >> a;
            if (a <= m1) {
                m2 = m1;
                m1 = a;
            } else if (a < m2) {
                m2 = a;
            }
        }
        std::cout << std::max(m1, m2 - m1) << std::endl;
    }
}
