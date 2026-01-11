#include <iostream>
#include <string>
#include <cassert>

bool f(int p, std::string& s) {
    if (s[p] != '0') {
        return true;
    }

    if (0 < p && s[p - 1] == '1' && (p == 1 || s[p - 2] == '1')) {
        if (s[p] == 'l') {
            return false;
        }
        s[p] = 'r';
        if (p + 1 < s.size() && s[p + 1] == '1') {
            if (p + 2 < s.size()) {
                if (s[p + 2] == '0' || s[p + 2] == 'l') {
                    s[p + 2] = 'l';
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
    }

    if (p + 1 < s.size() && s[p + 1] == '1' && (p + 2 == s.size() || s[p + 2] == '1')) {
        if (s[p] == 'r') {
            return false;
        }
        s[p] = 'l';
        if (0 < p && s[p - 1] == '1') {
            if (1 < p) {
                if (s[p - 2] == '0' || s[p - 2] == 'r') {
                    s[p - 2] = 'r';
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
    }

    return true;
}

int main() {
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n;
        std::cin >> n;
        std::string s;
        std::cin >> s;
        bool r = true;
        for (int j = 0; j < n; ++j) {
            r &= f(j, s);
        }
        for (int j = n - 1; 0 <= j; --j) {
            r &= f(j, s);
        }
        std::cout << (r ? "YES" : "NO") << std::endl;
    }

    return 0;
}
