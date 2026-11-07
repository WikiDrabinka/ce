#include "../common.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>

struct Move {
    int type;      // 0: 2-opt (intra-route), 1: node exchange (inter-route)
    double delta;
    int u1, u2;    // For 2-opt: Edge 1 (u1 -> u2). For exchange: pred, w (removed node)
    int v1, v2;    // For 2-opt: Edge 2 (v1 -> v2). For exchange: w, succ
    int v_in;      // For exchange: unselected node to insert

    bool operator<(const Move& other) const {
        return delta < other.delta;
    }
};

inline std::vector<int> generate_random_solution(int n, int m, std::mt19937& rng) {
    std::vector<int> all_nodes(n);
    std::iota(all_nodes.begin(), all_nodes.end(), 0);
    std::shuffle(all_nodes.begin(), all_nodes.end(), rng);
    return std::vector<int>(all_nodes.begin(), all_nodes.begin() + m);
}

// Safely reverse a segment between index `from` and `to` circularly
inline void reverse_segment(std::vector<int>& solution, int from, int to, int m) {
    int len = (to - from + m) % m + 1;
    for (int k = 0; k < len / 2; ++k) {
        int l = (from + k) % m;
        int r = (to - k + m) % m;
        std::swap(solution[l], solution[r]);
    }
}

// Apply move and update lookup positions
inline void apply_move(std::vector<int>& solution, const Move& mv, std::vector<int>& pos_in_sol, int m) {
    if (mv.type == 0) {
        int idx_u1 = pos_in_sol[mv.u1];
        int idx_u2 = pos_in_sol[mv.u2];
        int idx_v1 = pos_in_sol[mv.v1];
        int idx_v2 = pos_in_sol[mv.v2];

        if (idx_u1 == -1 || idx_u2 == -1 || idx_v1 == -1 || idx_v2 == -1) return;

        // Ensure we reverse the segment inside the 2-opt cut
        if ((idx_u1 + 1) % m == idx_u2 && (idx_v1 + 1) % m == idx_v2) {
            reverse_segment(solution, idx_u2, idx_v1, m);
        } else if ((idx_u2 + 1) % m == idx_u1 && (idx_v2 + 1) % m == idx_v1) {
            reverse_segment(solution, idx_u1, idx_v2, m);
        }
    } else {
        // Exchange w (mv.u2) with v_in
        int idx = pos_in_sol[mv.u2];
        if (idx != -1) {
            pos_in_sol[mv.u2] = -1;
            solution[idx] = mv.v_in;
            pos_in_sol[mv.v_in] = idx;
        }
    }

    // Refresh pos_in_sol array to ensure strict consistency
    for (int i = 0; i < m; ++i) {
        pos_in_sol[solution[i]] = i;
    }
}

// Evaluate all improving moves
inline void evaluate_all_moves_into(
    std::vector<Move>& lm,
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    const std::vector<int>& solution,
    int m,
    int n,
    std::vector<bool>& is_selected
) {
    lm.clear();
    std::fill(is_selected.begin(), is_selected.end(), false);
    for (int node : solution) is_selected[node] = true;

    // 1. Intra-route 2-opt
    for (int i = 0; i < m; ++i) {
        for (int j = i + 1; j < m; ++j) {
            if (j == (i + 1) % m || i == (j + 1) % m) continue;

            int u1 = solution[i];
            int u2 = solution[(i + 1) % m];
            int v1 = solution[j];
            int v2 = solution[(j + 1) % m];

            double delta = (matrix[u1][v1] + matrix[u2][v2]) - (matrix[u1][u2] + matrix[v1][v2]);

            if (delta < -1e-9) {
                lm.push_back({0, delta, u1, u2, v1, v2, -1});
            }
        }
    }

    // 2. Inter-route Node Exchange
    for (int i = 0; i < m; ++i) {
        int w = solution[i];
        int pred = solution[(i - 1 + m) % m];
        int succ = solution[(i + 1) % m];

        for (int v = 0; v < n; ++v) {
            if (is_selected[v]) continue;

            double delta = (costs[v] + matrix[pred][v] + matrix[v][succ]) - (costs[w] + matrix[pred][w] + matrix[w][succ]);

            if (delta < -1e-9) {
                lm.push_back({1, delta, pred, w, w, succ, v});
            }
        }
    }

    std::sort(lm.begin(), lm.end());
}

// Baseline Steepest Local Search
std::vector<int> baseline_steepest_local_search(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    const std::vector<int>& initial_solution,
    int m
) {
    std::vector<int> solution = initial_solution;
    int n = matrix.size();
    bool improved = true;

    std::vector<int> pos_in_sol(n, -1);
    std::vector<bool> is_selected(n, false);

    while (improved) {
        improved = false;
        double best_delta = 0.0;
        Move best_move = {-1, 0.0, -1, -1, -1, -1, -1};

        for (int i = 0; i < m; ++i) {
            pos_in_sol[solution[i]] = i;
        }

        // 2-opt evaluation
        for (int i = 0; i < m; ++i) {
            for (int j = i + 1; j < m; ++j) {
                if (j == (i + 1) % m || i == (j + 1) % m) continue;
                int u1 = solution[i], u2 = solution[(i + 1) % m];
                int v1 = solution[j], v2 = solution[(j + 1) % m];
                double delta = (matrix[u1][v1] + matrix[u2][v2]) - (matrix[u1][u2] + matrix[v1][v2]);
                if (delta < best_delta) {
                    best_delta = delta;
                    best_move = {0, delta, u1, u2, v1, v2, -1};
                }
            }
        }

        // Exchange evaluation
        std::fill(is_selected.begin(), is_selected.end(), false);
        for (int node : solution) is_selected[node] = true;

        for (int i = 0; i < m; ++i) {
            int w = solution[i];
            int pred = solution[(i - 1 + m) % m];
            int succ = solution[(i + 1) % m];
            for (int v = 0; v < n; ++v) {
                if (is_selected[v]) continue;
                double delta = (costs[v] + matrix[pred][v] + matrix[v][succ]) - (costs[w] + matrix[pred][w] + matrix[w][succ]);
                if (delta < best_delta) {
                    best_delta = delta;
                    best_move = {1, delta, pred, w, w, succ, v};
                }
            }
        }

        if (best_delta < -1e-9) {
            improved = true;
            apply_move(solution, best_move, pos_in_sol, m);
        }
    }
    return solution;
}

// Steepest Local Search with List of Improving Moves (LM)
std::vector<int> lm_steepest_local_search(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    const std::vector<int>& initial_solution,
    int m
) {
    std::vector<int> solution = initial_solution;
    int n = matrix.size();

    std::vector<int> succ(n, -1);
    std::vector<int> pos_in_sol(n, -1);
    std::vector<bool> is_selected(n, false);

    std::vector<Move> LM;
    LM.reserve(m * m + m * n);

    evaluate_all_moves_into(LM, matrix, costs, solution, m, n, is_selected);

    while (!LM.empty()) {
        std::fill(succ.begin(), succ.end(), -1);
        std::fill(pos_in_sol.begin(), pos_in_sol.end(), -1);

        for (int i = 0; i < m; ++i) {
            succ[solution[i]] = solution[(i + 1) % m];
            pos_in_sol[solution[i]] = i;
        }

        bool applied = false;
        size_t write_idx = 0;

        for (size_t read_idx = 0; read_idx < LM.size(); ++read_idx) {
            const Move& mv = LM[read_idx];

            if (applied) {
                LM[write_idx++] = mv;
                continue;
            }

            if (mv.type == 0) {
                // 2-Opt Edge Checks
                bool e1_fwd = (succ[mv.u1] == mv.u2);
                bool e1_rev = (succ[mv.u2] == mv.u1);
                bool e2_fwd = (succ[mv.v1] == mv.v2);
                bool e2_rev = (succ[mv.v2] == mv.v1);

                // Rule 1: Removed edge missing -> drop move
                if (!e1_fwd && !e1_rev) continue;
                if (!e2_fwd && !e2_rev) continue;

                // Rule 3: Same direction -> apply move
                if ((e1_fwd && e2_fwd) || (e1_rev && e2_rev)) {
                    apply_move(solution, mv, pos_in_sol, m);
                    applied = true;
                    continue;
                }

                // Rule 2: Inverted direction -> defer move
                LM[write_idx++] = mv;
            } else {
                // Node Exchange Checks: mv.u2 is w (replaced node), mv.v_in is candidate node
                int w = mv.u2;
                int pred = mv.u1;
                int s_node = mv.v2;

                bool e1_fwd = (succ[pred] == w);
                bool e1_rev = (succ[w] == pred);
                bool e2_fwd = (succ[w] == s_node);
                bool e2_rev = (succ[s_node] == w);

                // Rule 1: Node w removed or edges broken -> drop move
                if (pos_in_sol[w] == -1 || pos_in_sol[mv.v_in] != -1) continue;
                if (!e1_fwd && !e1_rev) continue;
                if (!e2_fwd && !e2_rev) continue;

                // Rule 3: Same direction -> apply move
                if ((e1_fwd && e2_fwd) || (e1_rev && e2_rev)) {
                    apply_move(solution, mv, pos_in_sol, m);
                    applied = true;
                    continue;
                }

                // Rule 2: Inverted direction -> defer move
                LM[write_idx++] = mv;
            }
        }

        LM.resize(write_idx);

        if (!applied) {
            evaluate_all_moves_into(LM, matrix, costs, solution, m, n, is_selected);
            if (LM.empty()) break;
        }
    }

    return solution;
}

int main(int argc, char* argv[]) {
    std::string filename = "instance.txt";
    if (argc > 1) filename = argv[1];

    try {
        auto data = read_file(filename);
        auto matrix = get_matrix(data);
        auto costs = get_costs(data);

        int n = data.size();
        int m = n / 2;
        int runs = 200;

        double baseline_total_cost = 0.0, baseline_best_cost = 1e9, baseline_worst_cost = -1.0;
        long long baseline_total_time_ms = 0;

        double lm_total_cost = 0.0, lm_best_cost = 1e9, lm_worst_cost = -1.0;
        long long lm_total_time_ms = 0;

        std::mt19937 rng(42);

        for (int r = 0; r < runs; ++r) {
            auto init_sol = generate_random_solution(n, m, rng);

            // 1. Baseline Steepest LS
            auto start_b = std::chrono::high_resolution_clock::now();
            auto sol_b = baseline_steepest_local_search(matrix, costs, init_sol, m);
            auto end_b = std::chrono::high_resolution_clock::now();
            double cost_b = evaluate(matrix, costs, sol_b);

            baseline_total_cost += cost_b;
            baseline_best_cost = std::min(baseline_best_cost, cost_b);
            baseline_worst_cost = std::max(baseline_worst_cost, cost_b);
            baseline_total_time_ms += std::chrono::duration_cast<std::chrono::milliseconds>(end_b - start_b).count();

            // 2. LM Steepest LS
            auto start_lm = std::chrono::high_resolution_clock::now();
            auto sol_lm = lm_steepest_local_search(matrix, costs, init_sol, m);
            auto end_lm = std::chrono::high_resolution_clock::now();
            double cost_lm = evaluate(matrix, costs, sol_lm);

            lm_total_cost += cost_lm;
            lm_best_cost = std::min(lm_best_cost, cost_lm);
            lm_worst_cost = std::max(lm_worst_cost, cost_lm);
            lm_total_time_ms += std::chrono::duration_cast<std::chrono::milliseconds>(end_lm - start_lm).count();
        }

        std::cout << "Method 1: Baseline Steepest Local Search" << std::endl;
        std::cout << "  Average Cost: " << baseline_total_cost / runs << std::endl;
        std::cout << "  Best Cost:    " << baseline_best_cost << std::endl;
        std::cout << "  Worst Cost:   " << baseline_worst_cost << std::endl;
        std::cout << "  Avg Time:     " << (double)baseline_total_time_ms / runs << " ms" << std::endl;
        std::cout << std::endl;
        std::cout << "Method 2: Steepest Local Search with LM (Fixed)" << std::endl;
        std::cout << "  Average Cost: " << lm_total_cost / runs << std::endl;
        std::cout << "  Best Cost:    " << lm_best_cost << std::endl;
        std::cout << "  Worst Cost:   " << lm_worst_cost << std::endl;
        std::cout << "  Avg Time:     " << (double)lm_total_time_ms / runs << " ms" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
