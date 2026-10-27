#include "../common.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>
#include <climits>

struct Move {
    int type; 
    int i, j; 
    double delta;
};

std::vector<int> generate_random_solution(int n, int m, std::mt19937& rng) {
    std::vector<int> all_nodes(n);
    std::iota(all_nodes.begin(), all_nodes.end(), 0);
    std::shuffle(all_nodes.begin(), all_nodes.end(), rng);
    std::vector<int> solution(all_nodes.begin(), all_nodes.begin() + m);
    return solution;
}

std::vector<std::vector<int>> precompute_nearest(const std::vector<std::vector<int>>& matrix, const std::vector<int>& costs, int k) {
    int n = matrix.size();
    std::vector<std::vector<int>> nearest(n);
    for (int i = 0; i < n; ++i) {
        std::vector<std::pair<int, int>> scored_neighbors;
        scored_neighbors.reserve(n - 1);
        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            int score = matrix[i][j] + costs[j];
            scored_neighbors.push_back({score, j});
        }
        std::sort(scored_neighbors.begin(), scored_neighbors.end());
        int limit = std::min(k, (int)scored_neighbors.size());
        nearest[i].reserve(limit);
        for (int l = 0; l < limit; ++l) {
            nearest[i].push_back(scored_neighbors[l].second);
        }
    }
    return nearest;
}

std::vector<int> baseline_steepest_local_search(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    const std::vector<int>& initial_solution,
    int m
) {
    std::vector<int> solution = initial_solution;
    int n = matrix.size();
    
    bool improved = true;
    while (improved) {
        improved = false;
        double best_delta = 0.0;
        Move best_move = {-1, -1, -1, 0.0};
        
        for (int i = 0; i < m; ++i) {
            for (int j = i + 1; j < m; ++j) {
                if (j == (i + 1) % m || i == (j + 1) % m) continue;
                
                int u1 = solution[i];
                int u2 = solution[(i + 1) % m];
                int v1 = solution[j];
                int v2 = solution[(j + 1) % m];
                
                double old_edges = matrix[u1][u2] + matrix[v1][v2];
                double new_edges = matrix[u1][v1] + matrix[u2][v2];
                double delta = new_edges - old_edges;
                
                if (delta < best_delta) {
                    best_delta = delta;
                    best_move = {0, i, j, delta};
                }
            }
        }
        
        std::vector<bool> is_selected(n, false);
        for (int node : solution) is_selected[node] = true;
        
        for (int i = 0; i < m; ++i) {
            int w = solution[i];
            int pred = solution[(i - 1 + m) % m];
            int succ = solution[(i + 1) % m];
            
            for (int v = 0; v < n; ++v) {
                if (is_selected[v]) continue;
                
                double old_cost = costs[w] + matrix[pred][w] + matrix[w][succ];
                double new_cost = costs[v] + matrix[pred][v] + matrix[v][succ];
                double delta = new_cost - old_cost;
                
                if (delta < best_delta) {
                    best_delta = delta;
                    best_move = {1, i, v, delta};
                }
            }
        }
        
        if (best_delta < -1e-9) {
            improved = true;
            if (best_move.type == 0) {
                int i = best_move.i;
                int j = best_move.j;
                int left = (i + 1) % m;
                int right = j;
                while (left != right && left != (right + 1) % m) {
                    std::swap(solution[left], solution[right]);
                    left = (left + 1) % m;
                    right = (right - 1 + m) % m;
                }
            } else if (best_move.type == 1) {
                solution[best_move.i] = best_move.j;
            }
        }
    }
    return solution;
}

std::vector<int> candidate_steepest_local_search(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    const std::vector<int>& initial_solution,
    int m,
    const std::vector<std::vector<int>>& nearest
) {
    std::vector<int> solution = initial_solution;
    int n = matrix.size();
    
    bool improved = true;
    while (improved) {
        improved = false;
        double best_delta = 0.0;
        Move best_move = {-1, -1, -1, 0.0};
        
        std::vector<bool> is_selected(n, false);
        std::vector<int> pos_in_solution(n, -1);
        for (int i = 0; i < m; ++i) {
            is_selected[solution[i]] = true;
            pos_in_solution[solution[i]] = i;
        }
        
        for (int i = 0; i < m; ++i) {
            int u = solution[i];
            for (int v : nearest[u]) {
                if (is_selected[v]) {
                    int j = pos_in_solution[v];
                    if (i == j || j == (i + 1) % m || i == (j + 1) % m) continue;
                    
                    int u1 = solution[i];
                    int u2 = solution[(i + 1) % m];
                    int v1 = solution[j];
                    int v2 = solution[(j + 1) % m];
                    
                    double old_edges = matrix[u1][u2] + matrix[v1][v2];
                    double new_edges = matrix[u1][v1] + matrix[u2][v2];
                    double delta = new_edges - old_edges;
                    
                    if (delta < best_delta) {
                        best_delta = delta;
                        best_move = {0, std::min(i, j), std::max(i, j), delta};
                    }
                } else {
                    int succ_i = (i + 1) % m;
                    int w = solution[succ_i];
                    int next_node = solution[(succ_i + 1) % m];
                    
                    double old_cost = costs[w] + matrix[u][w] + matrix[w][next_node];
                    double new_cost = costs[v] + matrix[u][v] + matrix[v][next_node];
                    double delta = new_cost - old_cost;
                    
                    if (delta < best_delta) {
                        best_delta = delta;
                        best_move = {1, succ_i, v, delta};
                    }
                    
                    int pred_i = (i - 1 + m) % m;
                    int w_pred = solution[pred_i];
                    int prev_w = solution[(pred_i - 1 + m) % m];
                    
                    double old_cost_p = costs[w_pred] + matrix[prev_w][w_pred] + matrix[w_pred][u];
                    double new_cost_p = costs[v] + matrix[prev_w][v] + matrix[v][u];
                    double delta_p = new_cost_p - old_cost_p;
                    
                    if (delta_p < best_delta) {
                        best_delta = delta_p;
                        best_move = {1, pred_i, v, delta_p};
                    }
                }
            }
        }
        
        if (best_delta < -1e-9) {
            improved = true;
            if (best_move.type == 0) {
                int i = best_move.i;
                int j = best_move.j;
                int left = (i + 1) % m;
                int right = j;
                while (left != right && left != (right + 1) % m) {
                    std::swap(solution[left], solution[right]);
                    left = (left + 1) % m;
                    right = (right - 1 + m) % m;
                }
            } else if (best_move.type == 1) {
                solution[best_move.i] = best_move.j;
            }
        }
    }
    return solution;
}

int main(int argc, char* argv[]) {
    std::string filename = "instance.txt";
    if (argc > 1) {
        filename = argv[1];
    }
    auto data = read_file(filename);
    auto matrix = get_matrix(data);
    auto costs = get_costs(data);
    
    int n = data.size();
    int m = n / 2;
    
    auto nearest = precompute_nearest(matrix, costs, 10);
    
    int runs = 200;
    
    double baseline_total_cost = 0.0;
    double baseline_best_cost = 1e9;
    double baseline_worst_cost = -1.0;
    long long baseline_total_time_ms = 0;
    
    double candidate_total_cost = 0.0;
    double candidate_best_cost = 1e9;
    double candidate_worst_cost = -1.0;
    long long candidate_total_time_ms = 0;
    
    std::mt19937 rng(42);
    
    for (int r = 0; r < runs; ++r) {
        auto init_sol = generate_random_solution(n, m, rng);
        
        auto start_b = std::chrono::high_resolution_clock::now();
        auto sol_b = baseline_steepest_local_search(matrix, costs, init_sol, m);
        auto end_b = std::chrono::high_resolution_clock::now();
        double cost_b = evaluate(matrix, costs, sol_b);
        
        baseline_total_cost += cost_b;
        baseline_best_cost = std::min(baseline_best_cost, cost_b);
        baseline_worst_cost = std::max(baseline_worst_cost, cost_b);
        baseline_total_time_ms += std::chrono::duration_cast<std::chrono::milliseconds>(end_b - start_b).count();
        
        auto start_c = std::chrono::high_resolution_clock::now();
        auto sol_c = candidate_steepest_local_search(matrix, costs, init_sol, m, nearest);
        auto end_c = std::chrono::high_resolution_clock::now();
        double cost_c = evaluate(matrix, costs, sol_c);
        
        candidate_total_cost += cost_c;
        candidate_best_cost = std::min(candidate_best_cost, cost_c);
        candidate_worst_cost = std::max(candidate_worst_cost, cost_c);
        candidate_total_time_ms += std::chrono::duration_cast<std::chrono::milliseconds>(end_c - start_c).count();
    }
    
    std::cout << "Method 1: Baseline Steepest Local Search\n";
    std::cout << "  Average Cost: " << baseline_total_cost / runs << "\n";
    std::cout << "  Best Cost:    " << baseline_best_cost << "\n";
    std::cout << "  Worst Cost:   " << baseline_worst_cost << "\n";
    std::cout << "  Avg Time:     " << (double)baseline_total_time_ms / runs << " ms\n";
    std::cout << "\n";
    std::cout << "Method 2: Candidate-based Steepest Local Search (k=10)\n";
    std::cout << "  Average Cost: " << candidate_total_cost / runs << "\n";
    std::cout << "  Best Cost:    " << candidate_best_cost << "\n";
    std::cout << "  Worst Cost:   " << candidate_worst_cost << "\n";
    std::cout << "  Avg Time:     " << (double)candidate_total_time_ms / runs << " ms\n";
    return 0;
}
