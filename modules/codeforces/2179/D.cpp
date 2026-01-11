#include <iostream>

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n;
        std::cin >> n;
        for (int j = n; 0 <= j; --j) {
            for (int k = (1 << j) - 1; k < (1 << n); k += (1 << (j + 1))) {
                std::cout << k << " ";
            }
        }
        std::cout << std::endl;
    }

    return 0;
}
