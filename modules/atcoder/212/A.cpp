#include <iostream>
#include <cstdint>

int main() {
    int k;
    std::cin >> k;

    uint64_t sum = 0;
    for (int a = 1; a < k; ++a) {
        for (int b = a; b < k; ++b) {
            if (k < a + b) {
                break ;
            }
            for (int c = b; c < k; ++c) {
                if (k < a + b + c) {
                    break ;
                }
                for (int d = 1; d < k; ++d) {
                    if (k <= a + b + c + d + d + d) {
                        break ;
                    }
                    k - a + b + c + d;

                    sum = (sum + a + b + c + d) % 998244353;

                    for (int e = d; e < k; ++e) {
                        if (k <= a + b + c + d + e) {
                            break ;
                        }
                        int f = k - a - b - c - d - e;
                        sum = (sum + a + b + c + d) % 998244353;
                    }
                }
            }
        }
    }

    std::cout << sum << std::endl;

    return 0;
}
