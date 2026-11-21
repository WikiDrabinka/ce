#include "../common.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>
#include <limits>

// Global stopping time budget per run (in milliseconds)
double MSLS_AVG_TIME_MS = 1000.0;

enum class DestroyType {
    RANDOM_NODES = 0,
    SINGLE_SUBPATH = 1,
    MULTI_SUBPATH = 2
};

struct Move {
    int type; // 0: 2-opt (intra-route), 1: node exchange (inter-route)
    double delta;
    int u1, u2; // For 2-opt: Edge 1 (u1 -> u2). For exchange: pred, w
    int v1, v2; // For 2-opt: Edge 2 (v1 -> v2). For exchange: w, succ
    int v_in; // For exchange: unselected node to insert

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

inline void reverse_segment(std::vector<int>& solution, int from, int to, int m) {
    if (m <= 1) return;
    int len = (to - from + m) % m + 1;
    for (int k = 0; k < len / 2; ++k) {
        int l = (from + k) % m;
        int r = (to - k + m) % m;
        std::swap(solution[l], solution[r]);
    }
}

inline void apply_move(std::vector<int>& solution, const Move& mv, std::vector<int>& pos_in_sol, int m) {
    if (mv.type == 0) {
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
        int idx = pos_in_sol[mv.u2];
        if (idx != -1) {
            pos_in_sol[mv.u2] = -1;
            solution[idx] = mv.v_in;
            pos_in_sol[mv.v_in] = idx;
        }
    }

    std::fill(pos_in_sol.begin(), pos_in_sol.end(), -1);
    for (int i = 0; i < m; ++i) {
        pos_in_sol[solution[i]] = i;
    }
}

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
    if (m < 2) return;

    std::fill(is_selected.begin(), is_selected.end(), false);
    for (int node : solution) is_selected[node] = true;

    // 1. Intra-route 2-opt moves
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

    // 2. Inter-route node exchange moves
    for (int i = 0; i < m; ++i) {
        int w = solution[i];
        int pred = solution[(i - 1 + m) % m];
        int succ = solution[(i + 1) % m];

        for (int v = 0; v < n; ++v) {
            if (is_selected[v]) continue;

            double delta = (costs[v] + matrix[pred][v] + matrix[v][succ]) -
                           (costs[w] + matrix[pred][w] + matrix[w][succ]);
            if (delta < -1e-9) {
                lm.push_back({1, delta, pred, w, w, succ, v});
            }
        }
    }

    std::sort(lm.begin(), lm.end());
}

// Best Local Search engine: Steepest Local Search with List of Improving Moves (LM)
std::vector<int> lm_steepest_local_search(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    const std::vector<int>& initial_solution,
    int m
) {
    std::vector<int> solution = initial_solution;
    int n = static_cast<int>(matrix.size());
    if (m < 2) return solution;

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
                bool e1_fwd = (succ[mv.u1] == mv.u2);
                bool e1_rev = (succ[mv.u2] == mv.u1);
                bool e2_fwd = (succ[mv.v1] == mv.v2);
                bool e2_rev = (succ[mv.v2] == mv.v1);

                if (!e1_fwd && !e1_rev) continue;
                if (!e2_fwd && !e2_rev) continue;

                if ((e1_fwd && e2_fwd) || (e1_rev && e2_rev)) {
                    apply_move(solution, mv, pos_in_sol, m);
                    applied = true;
                    continue;
                }

                LM[write_idx++] = mv;
            } else {
                int w = mv.u2;
                int pred = mv.u1;
                int s_node = mv.v2;

                bool e1_fwd = (succ[pred] == w);
                bool e1_rev = (succ[w] == pred);
                bool e2_fwd = (succ[w] == s_node);
                bool e2_rev = (succ[s_node] == w);

                if (pos_in_sol[w] == -1 || pos_in_sol[mv.v_in] != -1) continue;
                if (!e1_fwd && !e1_rev) continue;
                if (!e2_fwd && !e2_rev) continue;

                if ((e1_fwd && e2_fwd) || (e1_rev && e2_rev)) {
                    apply_move(solution, mv, pos_in_sol, m);
                    applied = true;
                    continue;
                }

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

// Destroy operator: removes ~30% (k) of nodes from the current solution
std::vector<int> destroy_solution(
    const std::vector<int>& solution,
    int m,
    DestroyType dtype,
    double destroy_ratio,
    std::mt19937& rng
) {
    int k = std::max(1, std::min(m - 1, static_cast<int>(std::round(destroy_ratio * m))));
    std::vector<bool> removed(m, false);

    if (dtype == DestroyType::RANDOM_NODES) {
        // 1. Removal of randomly selected nodes
        std::vector<int> indices(m);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);
        for (int i = 0; i < k; ++i) {
            removed[indices[i]] = true;
        }
    } else if (dtype == DestroyType::SINGLE_SUBPATH) {
        // 2. Removal of a single random subpath of length k
        std::uniform_int_distribution<int> dist_start(0, m - 1);
        int start = dist_start(rng);
        for (int step = 0; step < k; ++step) {
            removed[(start + step) % m] = true;
        }
    } else if (dtype == DestroyType::MULTI_SUBPATH) {
        // 3. Removal of 3-5 randomly selected subpaths totaling k nodes
        int num_paths = std::min(k, std::uniform_int_distribution<int>(3, 5)(rng));
        std::vector<int> lengths(num_paths, 1);
        int remaining = k - num_paths;

        std::uniform_int_distribution<int> dist_path(0, num_paths - 1);
        while (remaining > 0) {
            lengths[dist_path(rng)]++;
            remaining--;
        }

        // Pick disjoint starting positions across the circular tour
        std::vector<int> starts(num_paths);
        std::uniform_int_distribution<int> dist_m(0, m - 1);
        for (int p = 0; p < num_paths; ++p) {
            int attempts = 0;
            int s = dist_m(rng);
            while (attempts < m) {
                bool conflict = false;
                for (int step = 0; step < lengths[p]; ++step) {
                    if (removed[(s + step) % m]) {
                        conflict = true;
                        break;
                    }
                }
                if (!conflict) break;
                s = (s + 1) % m;
                attempts++;
            }
            starts[p] = s;
            for (int step = 0; step < lengths[p]; ++step) {
                removed[(s + step) % m] = true;
            }
        }
    }

    std::vector<int> partial;
    partial.reserve(m - k);
    for (int i = 0; i < m; ++i) {
        if (!removed[i]) {
            partial.push_back(solution[i]);
        }
    }
    return partial;
}

// Repair operator: Greedy Cycle with Weighted 2-Regret
std::vector<int> repair_greedy_cycle_weighted_2regret(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    const std::vector<int>& partial_solution,
    int m,
    int n,
    std::mt19937& rng,
    double w_regret = 1.0,
    double w_cost = 0.5
) {
    std::vector<int> tour = partial_solution;
    std::vector<bool> in_tour(n, false);
    for (int v : tour) {
        if (v >= 0 && v < n) in_tour[v] = true;
    }

    // If fewer than 2 nodes remain, seed with lowest-cost / random nodes to form a valid cycle
    if (tour.empty()) {
        int best_node = 0;
        for (int v = 1; v < n; ++v) {
            if (costs[v] < costs[best_node]) best_node = v;
        }
        tour.push_back(best_node);
        in_tour[best_node] = true;
    }

    if (tour.size() == 1 && m >= 2) {
        int a = tour[0];
        int best_v = -1;
        double best_pair_cost = std::numeric_limits<double>::infinity();
        for (int v = 0; v < n; ++v) {
            if (in_tour[v]) continue;
            double c = costs[v] + matrix[a][v] + matrix[v][a];
            if (c < best_pair_cost) {
                best_pair_cost = c;
                best_v = v;
            }
        }
        if (best_v != -1) {
            tour.push_back(best_v);
            in_tour[best_v] = true;
        }
    }

    // Iteratively insert remaining nodes until tour reaches size m
    while (static_cast<int>(tour.size()) < m) {
        int sz = static_cast<int>(tour.size());
        double best_weighted_score = -std::numeric_limits<double>::infinity();
        int chosen_node = -1;
        int chosen_insert_pos = 1;

        for (int v = 0; v < n; ++v) {
            if (in_tour[v]) continue;

            double best_ins = std::numeric_limits<double>::infinity();
            double second_ins = std::numeric_limits<double>::infinity();
            int best_pos = 1;

            for (int idx = 0; idx < sz; ++idx) {
                int a = tour[idx];
                int b = tour[(idx + 1) % sz];

                // Cycle edge insertion delta: add cost[v] + d(a,v) + d(v,b), remove d(a,b)
                double ins_delta = static_cast<double>(costs[v]) +
                                   static_cast<double>(matrix[a][v]) +
                                   static_cast<double>(matrix[v][b]) -
                                   static_cast<double>(matrix[a][b]);

                if (ins_delta < best_ins) {
                    second_ins = best_ins;
                    best_ins = ins_delta;
                    best_pos = idx + 1;
                } else if (ins_delta < second_ins) {
                    second_ins = ins_delta;
                }
            }

            if (!std::isfinite(second_ins)) {
                second_ins = best_ins;
            }

            double regret2 = second_ins - best_ins;
            double weighted_score = w_regret * regret2 - w_cost * best_ins;

            if (weighted_score > best_weighted_score) {
                best_weighted_score = weighted_score;
                chosen_node = v;
                chosen_insert_pos = best_pos;
            }
        }

        if (chosen_node == -1) break;
        tour.insert(tour.begin() + chosen_insert_pos, chosen_node);
        in_tour[chosen_node] = true;
    }

    // Fallback safety if tour is ever shorter than m
    if (static_cast<int>(tour.size()) < m) {
        std::vector<int> rest;
        for (int v = 0; v < n; ++v) {
            if (!in_tour[v]) rest.push_back(v);
        }
        std::shuffle(rest.begin(), rest.end(), rng);
        for (int v : rest) {
            if (static_cast<int>(tour.size()) == m) break;
            tour.push_back(v);
        }
    }

    return tour;
}

struct LNSResult {
    std::vector<int> best_solution;
    double best_cost;
    long long iterations;
};

// Large Neighborhood Search (with or without LS after destroy-repair)
LNSResult run_lns(
    const std::vector<std::vector<int>>& matrix,
    const std::vector<int>& costs,
    int n,
    int m,
    DestroyType dtype,
    bool use_ls_after_repair,
    double time_budget_ms,
    std::mt19937& rng
) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // Always start from a random solution and apply local search to initial solution
    std::vector<int> x = generate_random_solution(n, m, rng);
    x = lm_steepest_local_search(matrix, costs, x, m);
    double fx = evaluate(matrix, costs, x);

    std::vector<int> best_x = x;
    double best_f = fx;

    long long iterations = 0;
    const double destroy_ratio = 0.30; // Remove 30% of nodes

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(now - start_time).count();
        if (elapsed_ms >= time_budget_ms) {
            break;
        }

        iterations++;

        // 1. Destroy
        std::vector<int> y = destroy_solution(x, m, dtype, destroy_ratio, rng);

        // 2. Repair (Greedy Cycle Weighted 2-Regret)
        y = repair_greedy_cycle_weighted_2regret(matrix, costs, y, m, n, rng);

        // 3. Optional Local Search after destroy-repair
        if (use_ls_after_repair) {
            y = lm_steepest_local_search(matrix, costs, y, m);
        }

        double fy = evaluate(matrix, costs, y);

        // Acceptance criterion (minimizing total objective cost)
        if (fy < fx - 1e-9) {
            x = std::move(y);
            fx = fy;

            if (fx < best_f - 1e-9) {
                best_f = fx;
                best_x = x;
            }
        }
    }

    return {best_x, best_f, iterations};
}

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

        int n = static_cast<int>(data.size());
        int m = n / 2;
        int runs = 20;

        struct Config {
            std::string name;
            DestroyType dtype;
            bool use_ls;
        };

        std::vector<Config> configs = {
            {"LNS (Random Nodes, No LS)",      DestroyType::RANDOM_NODES,   false},
            {"LNS (Random Nodes, With LS)",     DestroyType::RANDOM_NODES,   true},
            {"LNS (Single Subpath, No LS)",     DestroyType::SINGLE_SUBPATH, false},
            {"LNS (Single Subpath, With LS)",    DestroyType::SINGLE_SUBPATH, true},
            {"LNS (3-5 Subpaths, No LS)",       DestroyType::MULTI_SUBPATH,  false},
            {"LNS (3-5 Subpaths, With LS)",      DestroyType::MULTI_SUBPATH,  true}
        };

        std::mt19937 rng(42);

        std::cout << "LNS EXPERIMENT (" << runs << " runs per method, time limit = "
                  << MSLS_AVG_TIME_MS << " ms)\n\n";

        for (const auto& cfg : configs) {
            double total_cost = 0.0;
            double best_cost = std::numeric_limits<double>::infinity();
            double worst_cost = -std::numeric_limits<double>::infinity();

            double min_time = std::numeric_limits<double>::infinity();
            double max_time = -std::numeric_limits<double>::infinity();
            double total_time = 0.0;

            long long total_iters = 0;
            long long min_iters = std::numeric_limits<long long>::max();
            long long max_iters = 0;

            std::vector<int> best_route;

            for (int r = 0; r < runs; ++r) {
                auto start = std::chrono::high_resolution_clock::now();
                LNSResult res = run_lns(matrix, costs, n, m, cfg.dtype, cfg.use_ls, MSLS_AVG_TIME_MS, rng);
                auto end = std::chrono::high_resolution_clock::now();

                double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

                total_cost += res.best_cost;
                if (res.best_cost < best_cost) {
                    best_cost = res.best_cost;
                    best_route = res.best_solution;
                }
                worst_cost = std::max(worst_cost, res.best_cost);

                total_time += elapsed_ms;
                min_time = std::min(min_time, elapsed_ms);
                max_time = std::max(max_time, elapsed_ms);

                total_iters += res.iterations;
                min_iters = std::min(min_iters, res.iterations);
                max_iters = std::max(max_iters, res.iterations);
            }

            std::cout << "Method: " << cfg.name << std::endl;
            std::cout << "  Average Cost: " << (total_cost / runs) << std::endl;
            std::cout << "  Best Cost:    " << best_cost << std::endl;
            std::cout << "  Worst Cost:   " << worst_cost << std::endl;
            std::cout << "  Min Time:     " << min_time << " ms" << std::endl;
            std::cout << "  Max Time:     " << max_time << " ms" << std::endl;
            std::cout << "  Avg Time:     " << (total_time / runs) << " ms" << std::endl;
            std::cout << "  Iterations:   Avg = " << (static_cast<double>(total_iters) / runs)
                      << " (Min = " << min_iters << ", Max = " << max_iters << ")" << std::endl;
            std::cout << "  Best Route:   ";
            print_route(best_route);
            std::cout << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
