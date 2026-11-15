#include "../common.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>

/**
 * Represents a candidate or cached improvement move for the local search.
 * Supports both intra-route (2-opt) and inter-route (node exchange) moves.
 */
struct Move {
    int type;      // 0: 2-opt (intra-route), 1: node exchange (inter-route)
    double delta;  // Cost change resulting from the move (negative indicates improvement)
    int u1, u2;    // For 2-opt: Edge 1 (u1 -> u2). For node exchange: predecessor, removed node (w)
    int v1, v2;    // For 2-opt: Edge 2 (v1 -> v2). For node exchange: removed node (w), successor
    int v_in;      // For node exchange: unselected node being inserted into the route

    // Comparison operator to sort LM from best (most negative delta) to worst
    bool operator<(const Move& other) const {
        return delta < other.delta;
    }
};

/**
 * Generates a random initial solution of size m by sampling from n available nodes.
 */
inline std::vector<int> generate_random_solution(int n, int m, std::mt19937& rng) {
    std::vector<int> all_nodes(n);
    std::iota(all_nodes.begin(), all_nodes.end(), 0);
    std::shuffle(all_nodes.begin(), all_nodes.end(), rng);
    return std::vector<int>(all_nodes.begin(), all_nodes.begin() + m);
}

/**
 * Safely reverses a circular path segment between indices `from` and `to`.
 */
inline void reverse_segment(std::vector<int>& solution, int from, int to, int m) {
    int len = (to - from + m) % m + 1;
    for (int k = 0; k < len / 2; ++k) {
        int l = (from + k) % m;
        int r = (to - k + m) % m;
        std::swap(solution[l], solution[r]);
    }
}

/**
 * Applies a move to the current solution and updates the node position lookup map.
 */
inline void apply_move(std::vector<int>& solution, const Move& mv, std::vector<int>& pos_in_sol, int m) {
    if (mv.type == 0) {
        // Intra-route 2-opt move
        int idx_u1 = pos_in_sol[mv.u1];
        int idx_u2 = pos_in_sol[mv.u2];
        int idx_v1 = pos_in_sol[mv.v1];
        int idx_v2 = pos_in_sol[mv.v2];

        if (idx_u1 == -1 || idx_u2 == -1 || idx_v1 == -1 || idx_v2 == -1) return;

        if ((idx_u1 + 1) % m == idx_u2 && (idx_v1 + 1) % m == idx_v2) {
            reverse_segment(solution, idx_u2, idx_v1, m);
        } else if ((idx_u2 + 1) % m == idx_u1 && (idx_v2 + 1) % m == idx_v1) {
            reverse_segment(solution, idx_u1, idx_v2, m);
        }
    } else {
        // Inter-route node exchange move: replace node w (mv.u2) with v_in
        int idx = pos_in_sol[mv.u2];
        if (idx != -1) {
            pos_in_sol[mv.u2] = -1;
            solution[idx] = mv.v_in;
            pos_in_sol[mv.v_in] = idx;
        }
    }

    // Refresh position lookup array for all nodes currently in the route
    for (int i = 0; i < m; ++i) {
        pos_in_sol[solution[i]] = i;
    }
}

/**
 * Evaluates all applicable improving moves (both 2-opt and node exchanges) 
 * and populates/sorts the List of Improving Moves (LM).
 */
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

    // 1. Evaluate Intra-route 2-opt moves
    for (int i = 0; i < m; ++i) {
        for (int j = i + 1; j < m; ++j) {
            if (j == (i + 1) % m || i == (j + 1) % m) continue; // Skip adjacent edges

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

    // 2. Evaluate Inter-route Node Exchange moves
    for (int i = 0; i < m; ++i) {
        int w = solution[i];
        int pred = solution[(i - 1 + m) % m];
        int succ = solution[(i + 1) % m];

        for (int v = 0; v < n; ++v) {
            if (is_selected[v]) continue; // Only consider unselected nodes

            double delta = (costs[v] + matrix[pred][v] + matrix[v][succ]) - (costs[w] + matrix[pred][w] + matrix[w][succ]);

            if (delta < -1e-9) {
                lm.push_back({1, delta, pred, w, w, succ, v});
            }
        }
    }

    // Sort LM from best (most negative delta) to worst
    std::sort(lm.begin(), lm.end());
}

/**
 * Steepest Local Search engine utilizing the List of Improving Moves (LM) mechanism.
 * Implements the 3 browsing rules to filter, defer, or apply moves efficiently.
 */
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

    // Initial full neighborhood evaluation
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

        // Iterate through LM and evaluate applicability according to the 3 rules
        for (size_t read_idx = 0; read_idx < LM.size(); ++read_idx) {
            const Move& mv = LM[read_idx];

            if (applied) {
                // If a move was already applied in this step, preserve remaining moves
                LM[write_idx++] = mv;
                continue;
            }

            if (mv.type == 0) {
                // --- 2-Opt Move Applicability Checks ---
                bool e1_fwd = (succ[mv.u1] == mv.u2);
                bool e1_rev = (succ[mv.u2] == mv.u1);
                bool e2_fwd = (succ[mv.v1] == mv.v2);
                bool e2_rev = (succ[mv.v2] == mv.v1);

                // Rule 1: Removed edge no longer exists -> drop move
                if (!e1_fwd && !e1_rev) continue;
                if (!e2_fwd && !e2_rev) continue;

                // Rule 3: Same relative direction -> apply move and remove from LM
                if ((e1_fwd && e2_fwd) || (e1_rev && e2_rev)) {
                    apply_move(solution, mv, pos_in_sol, m);
                    applied = true;
                    continue;
                }

                // Rule 2: Inverted relative direction -> defer move in LM
                LM[write_idx++] = mv;
            } else {
                // --- Node Exchange Move Applicability Checks ---
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

                // Rule 3: Same relative direction -> apply move and remove from LM
                if ((e1_fwd && e2_fwd) || (e1_rev && e2_rev)) {
                    apply_move(solution, mv, pos_in_sol, m);
                    applied = true;
                    continue;
                }

                // Rule 2: Inverted relative direction -> defer move in LM
                LM[write_idx++] = mv;
            }
        }

        // Compact LM in-place to remove discarded moves
        LM.resize(write_idx);

        // Fallback: If no applicable move found in LM, re-evaluate entire neighborhood
        if (!applied) {
            evaluate_all_moves_into(LM, matrix, costs, solution, m, n, is_selected);
            if (LM.empty()) break;
        }
    }

    return solution;
}

/**
 * Perturbation function for Iterated Local Search (ILS).
 * Applies k_intra random 2-opt swaps and k_inter node exchanges to knock the solution out of local optima.
 */
std::vector<int> perturb(
    const std::vector<int>& solution,
    int n,
    int m,
    int k_intra,
    int k_inter,
    std::mt19937& rng
) {
    std::vector<int> perturbed = solution;
    std::uniform_int_distribution<int> dist_m(0, m - 1);

    // 1. Intra-route perturbation: k_intra random 2-opt segment reversals
    for (int step = 0; step < k_intra; ++step) {
        int i = dist_m(rng);
        int j = dist_m(rng);
        if (i == j || j == (i + 1) % m || i == (j + 1) % m) continue;
        if (i > j) std::swap(i, j);
        reverse_segment(perturbed, (i + 1) % m, j, m);
    }

    // 2. Inter-route perturbation: k_inter node swaps with unselected pool
    std::vector<bool> is_selected(n, false);
    for (int node : perturbed) is_selected[node] = true;

    std::vector<int> unselected;
    unselected.reserve(n - m);
    for (int v = 0; v < n; ++v) {
        if (!is_selected[v]) unselected.push_back(v);
    }

    if (!unselected.empty()) {
        std::shuffle(unselected.begin(), unselected.end(), rng);
        int swaps = std::min({k_inter, m, (int)unselected.size()});
        
        std::vector<int> positions(m);
        std::iota(positions.begin(), positions.end(), 0);
        std::shuffle(positions.begin(), positions.end(), rng);

        for (int s = 0; s < swaps; ++s) {
            perturbed[positions[s]] = unselected[s];
        }
    }

    return perturbed;
}

/**
 * Executes Multiple Start Local Search (MSLS) over a specified number of random iterations.
 */
std::vector<int> run_msls(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    int n,
    int m,
    int iterations,
    std::mt19937& rng
) {
    std::vector<int> best_solution;
    double best_cost = 1e18;

    for (int iter = 0; iter < iterations; ++iter) {
        auto init_sol = generate_random_solution(n, m, rng);
        auto local_opt = lm_steepest_local_search(matrix, costs, init_sol, m);
        double cost = evaluate(matrix, costs, local_opt);

        if (cost < best_cost) {
            best_cost = cost;
            best_solution = std::move(local_opt);
        }
    }
    return best_solution;
}

/**
 * Executes Iterated Local Search (ILS) under a given wall-clock time limit.
 * Returns the best solution found and the total number of local search executions.
 */
std::pair<std::vector<int>, int> run_ils(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    int n,
    int m,
    double max_time_ms,
    std::mt19937& rng
) {
    auto start_time = current_time_ms();

    // Start with a random solution optimized by local search
    auto init_sol = generate_random_solution(n, m, rng);
    std::vector<int> current_x = lm_steepest_local_search(matrix, costs, init_sol, m);
    double current_cost = evaluate(matrix, costs, current_x);

    std::vector<int> best_x = current_x;
    double best_cost = current_cost;
    int ls_runs = 1;

    int k_intra = 2; // Perturbation strength (2-opt cuts)
    int k_inter = 2; // Perturbation strength (node swaps)

    while (true) {
        auto now = current_time_ms();
        double elapsed_ms = now - start_time;
        if (elapsed_ms >= max_time_ms) {
            break; // Stop when time limit is reached
        }

        // Perturb current local optimum
        std::vector<int> y = perturb(current_x, n, m, k_intra, k_inter, rng);
        
        // Apply local search to perturbed solution
        y = lm_steepest_local_search(matrix, costs, y, m);
        double y_cost = evaluate(matrix, costs, y);
        ls_runs++;

        // Acceptance criterion: accept strictly better local optima
        if (y_cost < current_cost - 1e-9) {
            current_x = y;
            current_cost = y_cost;

            if (current_cost < best_cost - 1e-9) {
                best_cost = current_cost;
                best_x = current_x;
            }
        }
    }

    return {best_x, ls_runs};
}

/**
 * Helper function to print a route vector cleanly.
 */
void print_route(const std::vector<int>& route) {
    std::cout << "[";
    for (size_t i = 0; i < route.size(); ++i) {
        std::cout << route[i] << (i + 1 < route.size() ? ", " : "");
    }
    std::cout << "]" << std::endl;
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
        int experiment_runs = 20;
        int msls_iterations = 400; // 400 local search starts per MSLS run

        std::mt19937 rng(42);

        // ==========================================
        // 1. Run MSLS Experiment (20 independent runs)
        // ==========================================
        std::cout << "Running MSLS (" << experiment_runs << " runs x " << msls_iterations << " LM-LS iterations)..." << std::endl;
        double msls_total_cost = 0.0, msls_best_cost = 1e18, msls_worst_cost = -1.0;
        double msls_min_time = 1e18, msls_max_time = -1.0, msls_total_time = 0.0;
        std::vector<int> msls_best_route;

        for (int r = 0; r < experiment_runs; ++r) {
            auto start = current_time_ms();
            auto sol = run_msls(matrix, costs, n, m, msls_iterations, rng);
            auto end = current_time_ms();

            double cost = evaluate(matrix, costs, sol);
            double elapsed_ms = end - start;

            msls_total_cost += cost;
            if (cost < msls_best_cost) {
                msls_best_cost = cost;
                msls_best_route = sol;
            }
            msls_worst_cost = std::max(msls_worst_cost, cost);
            msls_total_time += elapsed_ms;
            msls_min_time = std::min(msls_min_time, elapsed_ms);
            msls_max_time = std::max(msls_max_time, elapsed_ms);
        }

        double avg_msls_time_ms = msls_total_time / experiment_runs;

        // ==========================================
        // 2. Run ILS Experiment (20 independent runs)
        // ==========================================
        std::cout << "Running ILS (" << experiment_runs << " runs, time limit = " << avg_msls_time_ms << " ms)..." << std::endl;
        double ils_total_cost = 0.0, ils_best_cost = 1e18, ils_worst_cost = -1.0;
        double ils_min_time = 1e18, ils_max_time = -1.0, ils_total_time = 0.0;
        long long ils_total_ls_runs = 0;
        int ils_min_ls_runs = 1e9, ils_max_ls_runs = -1;
        std::vector<int> ils_best_route;

        for (int r = 0; r < experiment_runs; ++r) {
            auto start = current_time_ms();
            auto ils_result = run_ils(matrix, costs, n, m, avg_msls_time_ms, rng);
            auto sol = ils_result.first;
            auto ls_runs = ils_result.second;
            // auto [sol, ls_runs] = run_ils(matrix, costs, n, m, avg_msls_time_ms, rng);
            auto end = current_time_ms();

            double cost = evaluate(matrix, costs, sol);
            double elapsed_ms = end - start;

            ils_total_cost += cost;
            if (cost < ils_best_cost) {
                ils_best_cost = cost;
                ils_best_route = sol;
            }
            ils_worst_cost = std::max(ils_worst_cost, cost);
            ils_total_time += elapsed_ms;
            ils_min_time = std::min(ils_min_time, elapsed_ms);
            ils_max_time = std::max(ils_max_time, elapsed_ms);
            ils_total_ls_runs += ls_runs;
            ils_min_ls_runs = std::min(ils_min_ls_runs, ls_runs);
            ils_max_ls_runs = std::max(ils_max_ls_runs, ls_runs);
        }

        // ==========================================
        // Print Final Experimental Results & Summaries
        // ==========================================
        std::cout << "\n=== EXPERIMENTAL RESULTS (" << experiment_runs << " Runs) ===" << std::endl << std::endl;

        std::cout << "Method 1: Multiple Start Local Search (MSLS)" << std::endl;
        std::cout << "  Average Cost: " << msls_total_cost / experiment_runs << std::endl;
        std::cout << "  Best Cost:    " << msls_best_cost << std::endl;
        std::cout << "  Worst Cost:   " << msls_worst_cost << std::endl;
        std::cout << "  Min Time:     " << msls_min_time << " ms" << std::endl;
        std::cout << "  Max Time:     " << msls_max_time << " ms" << std::endl;
        std::cout << "  Avg Time:     " << avg_msls_time_ms << " ms" << std::endl;
        std::cout << "  Best Route:   ";
        print_route(msls_best_route);

        std::cout << std::endl;

        std::cout << "Method 2: Iterated Local Search (ILS)" << std::endl;
        std::cout << "  Average Cost: " << ils_total_cost / experiment_runs << std::endl;
        std::cout << "  Best Cost:    " << ils_best_cost << std::endl;
        std::cout << "  Worst Cost:   " << ils_worst_cost << std::endl;
        std::cout << "  Min Time:     " << ils_min_time << " ms" << std::endl;
        std::cout << "  Max Time:     " << ils_max_time << " ms" << std::endl;
        std::cout << "  Avg Time:     " << ils_total_time / experiment_runs << " ms" << std::endl;
        std::cout << "  LS Runs/Run:  Avg = " << (double)ils_total_ls_runs / experiment_runs
                  << " (Min = " << ils_min_ls_runs << ", Max = " << ils_max_ls_runs << ")" << std::endl;
        std::cout << "  Best Route:   ";
        print_route(ils_best_route);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
