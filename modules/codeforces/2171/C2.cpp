#include <iostream>
#include <vector>

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n;
        std::cin >> n;
        std::vector<int> a(n);
        std::vector<int> b(n);
        int xors = 0;
        for (int j = 0; j < n; ++j) {
            std::cin >> a[j];
            xors ^= a[j];
        }
        for (int j = 0; j < n; ++j) {
            std::cin >> b[j];
            xors ^= b[j];
        }

        if (xors == 0) {
            std::cout << "Tie" << std::endl;
            continue ;
        }


        int mask = xors;
        while (xors) {
            mask = xors;
            xors &= xors - 1;
        }

        for (int l = n - 1; 0 <= l; --l) {
            if ((a[l] & mask) != (b[l] & mask)) {
                if (l % 2 == 0) {
                    std::cout << "Ajisai" << std::endl;
                } else {
                    std::cout << "Mai" << std::endl;
                }
                break ;
            }
        }
    }

    return 0;
}
