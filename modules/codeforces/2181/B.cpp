#include <iostream>
#include <queue>

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        std::priority_queue<int, std::vector<int>, std::less<int>> a;
        std::priority_queue<int, std::vector<int>, std::less<int>> b;
        int n, m;
        std::cin >> n >> m;
        for (int j = 0; j < n; ++j) {
            int aj;
            std::cin >> aj;
            a.push(aj);
        }
        for (int j = 0; j < m; ++j) {
            int bj;
            std::cin >> bj;
            b.push(bj);
        }

        bool turn_a = true;
        while (!a.empty() && !b.empty()) {
            int amax = a.top();
            int bmax = b.top();
            if (turn_a) {
                b.pop();
                bmax -= amax;
                if (0 < bmax) {
                    b.push(bmax);
                }
            } else {
                a.pop();
                amax -= bmax;
                if (0 < amax) {
                    a.push(amax);
                }
            }

            turn_a = !turn_a;
        }

        if (a.empty()) {
            std::cout << "Bob" << std::endl;
        } else {
            std::cout << "Alice" << std::endl;
        }
    }

    return 0;
}
