#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

struct NodeData {
    int x;
    int y;
    int weight;
};

inline std::vector<NodeData> read_file(const std::string& file_path) {
    std::vector<NodeData> res;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + file_path);
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string segment;
        std::vector<int> tokens;
        while (std::getline(ss, segment, ';')) {
            tokens.push_back(std::stoi(segment));
        }
        if (tokens.size() >= 3) {
            res.push_back({tokens[0], tokens[1], tokens[2]});
        }
    }
    return res;
}

inline std::vector<std::vector<int>> get_matrix(const std::vector<NodeData>& data) {
    int n = data.size();
    std::vector<std::vector<int>> res(n, std::vector<int>(n, 0));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double dx = data[i].x - data[j].x;
            double dy = data[i].y - data[j].y;
            double raw_distance = std::sqrt(dx * dx + dy * dy);
            res[i][j] = static_cast<int>(raw_distance + 0.5);
        }
    }
    return res;
}

inline std::vector<int> get_costs(const std::vector<NodeData>& data) {
    std::vector<int> res;
    res.reserve(data.size());
    for (const auto& node : data) {
        res.push_back(node.weight);
    }
    return res;
}

inline double evaluate(const std::vector<std::vector<int>>& matrix, const std::vector<int>& costs, const std::vector<int>& solution) {
    if (solution.empty()) return 0.0;
    double cost = 0.0;
    for (int node : solution) {
        cost += costs[node];
    }
    int n = solution.size();
    for (int i = 0; i < n; ++i) {
        int a = solution[i];
        int b = solution[(i + 1) % n];
        cost += matrix[a][b];
    }
    return cost;
}

#endif // COMMON_H
