#include <iostream>
#include <string>
#include <vector>
#include <format>

void visit(int n, char previous_color, const std::vector<std::vector<int>>& neighbors_by_vertex, std::vector<char>& colors) {
    if (colors[n] != '\0') {
        return ;
    }

    // 'r' -> 'g', 'g' -> 'b', 'b' -> 'r'
    colors[n] = previous_color == 'r' ? 'g' : previous_color == 'g' ? 'b' : 'r';
    for (int neighbor : neighbors_by_vertex[n]) {
        visit(neighbor, colors[n], neighbors_by_vertex, colors);
    }
}

int main() {
    std::string s;
    std::cin >> s;
    int t;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int n, m;
        std::cin >> n >> m;
        std::vector<std::vector<int>> neighbors_by_vertex(n + 1);
        for (int j = 0; j < m; ++j) {
            int a, b;
            std::cin >> a >> b;
            neighbors_by_vertex[a].push_back(b);
            neighbors_by_vertex[b].push_back(a);
        }
        std::vector<char> colors(n + 1, '\0');
        for (int j = 1; j <= n; ++j) {
            visit(j, 'r', neighbors_by_vertex, colors);
        }
        for (int j = 1; j <= n; ++j) {
            std::cout << colors[j];
        }
        std::cout << std::endl;
    }

    std::cin >> s;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        int q;
        std::cin >> q;
        for (int j = 0; j < q; ++j) {
            int d;
            std::cin >> d;
            std::string c;
            std::cin >> c;
            
        }
    }

    return 0;
}
