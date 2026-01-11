#include <iostream>

int f(int k, int x) {
    return k * x + 1;
}

int main() {
    int t;
    std::cin >> t;
    for (int i = 1; i <= t; ++i) {
        int k, x;
        std::cin >> k >> x;
        int result = f(k, x);
        std::cout << result << std::endl;
    }

    return 0;
}
