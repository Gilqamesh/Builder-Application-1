#include <iostream>

int main() {
    int t;
    std::cin >> t;

    for (int i = 0; i < t; ++i) {
        int n;
        std::cin >> n;
        int result = 0;
        for (int ch = 0; ch <= n / 2; ++ch) {
            if ((n - ch * 2) % 4 == 0) {
                ++result;
            }
        }
        std::cout << result << std::endl;
    }

    return 0;
}
