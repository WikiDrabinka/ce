#include "../common.h"
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>

std::vector<int> find_greedy(const std::vector<std::vector<int>>& matrix, const std::vector<int>& costs, int start_idx) {
    int n = matrix.size();
    int to_find = (n + 1) / 2; 
    std::vector<int> sol = {start_idx};
    const double WEIGHT = 0.5;
    
    while (sol.size() < static_cast<size_t>(to_find)) {
        double best_score = -1e18;
        int best_node = -1;
        int best_idx = -1;

        for (int node = 0; node < n; ++node) {
            if (std::find(sol.begin(), sol.end(), node) != sol.end()) {
                continue;
            }

            struct Insertion {
                double cost;
                int idx;
            };
            std::vector<Insertion> insertions;
            int sol_size = sol.size();
            insertions.reserve(sol_size + 1);

            for (int idx = 0; idx <= sol_size; ++idx) {
                double ins_cost = 0;
                if (idx == 0 || idx == sol_size) {
                    ins_cost = matrix[sol.back()][node]
                                + costs[node]
                                + matrix[node][sol[0]]
                                - matrix[sol.back()][sol[0]];
                } else {
                    ins_cost = costs[node] 
                             + matrix[sol[idx - 1]][node] 
                             + matrix[node][sol[idx]] 
                             - matrix[sol[idx - 1]][sol[idx]];
                }
                insertions.push_back({ins_cost, idx});
            }

            std::sort(insertions.begin(), insertions.end(), [](const Insertion& a, const Insertion& b) {
                return a.cost < b.cost;
            });

            double best_cost = insertions[0].cost;
            int b_idx = insertions[0].idx;
            double second_cost = insertions[1].cost;
            double regret = second_cost - best_cost;

            double score = WEIGHT * regret - (1.0 - WEIGHT) * best_cost;

            if (best_node == -1 || score > best_score) {
                best_score = score;
                best_node = node;
                best_idx = b_idx;
            }
        }

        sol.insert(sol.begin() + best_idx, best_node);
    }

    return sol;
}

inline double swap_delta(const std::vector<int>& route, int i, int j, const std::vector<std::vector<int>>& matrix) {
    int n = route.size();
    if (i == j || n < 2) return 0.0;

    int affected[4] = {(i - 1 + n) % n, i, (j - 1 + n) % n, j};
    std::sort(affected, affected + 4);
    int unique_count = std::unique(affected, affected + 4) - affected;

    double old_cost = 0;
    double new_cost = 0;

    for (int k = 0; k < unique_count; ++k) {
        int edge_idx = affected[k];
        int a = route[edge_idx];
        int b = route[(edge_idx + 1) % n];
        old_cost += matrix[a][b];

        int a_new = a;
        if (edge_idx == i) a_new = route[j];
        else if (edge_idx == j) a_new = route[i];

        int next_idx = (edge_idx + 1) % n;
        int b_new = b;
        if (next_idx == i) b_new = route[j];
        else if (next_idx == j) b_new = route[i];

        new_cost += matrix[a_new][b_new];
    }

    return new_cost - old_cost;
}

inline double reverse_delta(const std::vector<int>& route, int i, int j, const std::vector<std::vector<int>>& matrix) {
    int n = route.size();
    if (n < 3 || i == j) return 0.0;

    if (i > j) std::swap(i, j);

    int a = route[(i - 1 + n) % n];
    int b = route[i];
    int c = route[j];
    int d = route[(j + 1) % n];

    double delta = matrix[a][c] + matrix[b][d] - matrix[a][b] - matrix[c][d];

    for (int p = i; p < j; ++p) {
        int u = route[p];
        int v = route[p + 1];
        delta += matrix[v][u] - matrix[u][v];
    }

    return delta;
}

inline double replace_delta(const std::vector<int>& route, int pos, int new_node, const std::vector<int>& costs, const std::vector<std::vector<int>>& matrix) {
    int n = route.size();
    if (n <= 1) return 0.0;

    int prev_node = route[(pos - 1 + n) % n];
    int old_node = route[pos];
    int next_node = route[(pos + 1) % n];

    double edge_delta = matrix[prev_node][new_node]
                      + matrix[new_node][next_node]
                      - matrix[prev_node][old_node]
                      - matrix[old_node][next_node];
    
    double cost_delta = costs[new_node] - costs[old_node];

    return edge_delta + cost_delta;
}

enum class MoveKind : uint8_t { SWAP, REVERSE, REPLACE };

struct Move {
    MoveKind kind;
    int a;
    int b;
};

inline void generate_intra_moves(int k_size, const std::string& move_type, std::vector<Move>& moves) {
    if (move_type == "nodes") {
        for (int i = 0; i < k_size; ++i) {
            for (int j = i + 1; j < k_size; ++j) {
                moves.push_back({MoveKind::SWAP, i, j});
            }
        }
    } else if (move_type == "edges") {
        for (int i = 0; i < k_size; ++i) {
            for (int j = i + 2; j < k_size; ++j) {
                if (i == 0 && j == k_size - 1) continue;
                moves.push_back({MoveKind::REVERSE, i, j});
            }
        }
    }
}

inline double delta_for_move(const std::vector<int>& route, const Move& move, const std::vector<int>& costs, const std::vector<std::vector<int>>& matrix) {
    switch (move.kind) {
        case MoveKind::SWAP:    return swap_delta(route, move.a, move.b, matrix);
        case MoveKind::REVERSE: return reverse_delta(route, move.a, move.b, matrix);
        case MoveKind::REPLACE: return replace_delta(route, move.a, move.b, costs, matrix);
    }
    return 0.0;
}

inline void apply_move(std::vector<int>& route, const Move& move) {
    switch (move.kind) {
        case MoveKind::SWAP:
            std::swap(route[move.a], route[move.b]);
            break;
        case MoveKind::REVERSE:
            std::reverse(route.begin() + move.a, route.begin() + move.b + 1);
            break;
        case MoveKind::REPLACE:
            route[move.a] = move.b;
            break;
    }
}

std::vector<int> local_search(
    const std::vector<int>& start_route, 
    const std::vector<std::vector<int>>& matrix, 
    const std::vector<int>& costs, 
    const std::string& move_type, 
    const std::string& strategy, 
    std::mt19937& g
) {
    std::vector<int> route = start_route;
    int total_nodes = matrix.size();
    int k_size = route.size();
    const double EPS = 1e-9;

    std::vector<Move> static_intra_moves;
    generate_intra_moves(k_size, move_type, static_intra_moves);

    int intra_count = static_intra_moves.size();
    int inter_count = k_size * (total_nodes - k_size);
    int total_moves = intra_count + inter_count;

    std::vector<bool> selected_mask(total_nodes, false);
    std::vector<int> unselected_nodes;
    unselected_nodes.reserve(total_nodes - k_size);

    std::vector<int> move_indices(total_moves);
    std::iota(move_indices.begin(), move_indices.end(), 0);

    auto get_move_by_index = [&](int idx) -> Move {
        if (idx < intra_count) {
            return static_intra_moves[idx];
        } else {
            int inter_idx = idx - intra_count;
            int pos = inter_idx / unselected_nodes.size();
            int node_idx = inter_idx % unselected_nodes.size();
            return {MoveKind::REPLACE, pos, unselected_nodes[node_idx]};
        }
    };

    while (true) {
        std::fill(selected_mask.begin(), selected_mask.end(), false);
        for (int node : route) selected_mask[node] = true;
        unselected_nodes.clear();
        for (int i = 0; i < total_nodes; ++i) {
            if (!selected_mask[i]) unselected_nodes.push_back(i);
        }

        if (strategy == "greedy") {
            bool improved = false;
            
            for (int i = 0; i < total_moves; ++i) {
                std::uniform_int_distribution<int> dist(i, total_moves - 1);
                int pick = dist(g);
                std::swap(move_indices[i], move_indices[pick]);

                Move move = get_move_by_index(move_indices[i]);
                double delta = delta_for_move(route, move, costs, matrix);

                if (delta < -EPS) {
                    apply_move(route, move);
                    improved = true;
                    break; 
                }
            }

            if (!improved) return route;

        } else if (strategy == "steepest") {
            Move best_move = {MoveKind::SWAP, -1, -1};
            double best_delta = -EPS;
            bool found = false;

            for (const auto& move : static_intra_moves) {
                double delta = delta_for_move(route, move, costs, matrix);
                if (delta < best_delta) {
                    best_delta = delta;
                    best_move = move;
                    found = true;
                }
            }

            for (int pos = 0; pos < k_size; ++pos) {
                for (int node : unselected_nodes) {
                    Move move = {MoveKind::REPLACE, pos, node};
                    double delta = delta_for_move(route, move, costs, matrix);
                    if (delta < best_delta) {
                        best_delta = delta;
                        best_move = move;
                        found = true;
                    }
                }
            }

            if (!found) return route;
            apply_move(route, best_move);
        } else {
            break;
        }
    }
    return route;
}

std::vector<int> get_random_sol(int n, int k, std::mt19937& g) {
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), g);
    indices.resize(k);
    return indices;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <data_file>\n";
        return 1;
    }

    auto data = read_file(argv[1]);
    auto costs = get_costs(data);
    auto matrix = get_matrix(data);
    int n = matrix.size();
    int k = (n + 1) / 2;

    std::vector<std::tuple<std::string, std::string, std::string>> configurations = {
        {"steepest", "nodes", "random"},
        {"steepest", "nodes", "greedy"},
        {"steepest", "edges", "random"},
        {"steepest", "edges", "greedy"},
        {"greedy", "nodes", "random"},
        {"greedy", "nodes", "greedy"},
        {"greedy", "edges", "random"},
        {"greedy", "edges", "greedy"}
    };

    std::random_device rd;
    std::mt19937 g(rd());

    int run_count = 200;

    for (size_t idx = 0; idx < configurations.size(); ++idx) {
        // auto [strategy, move_type, start_type] = configurations[idx];
        std::string strategy = std::get<0>(configurations[idx]);
        std::string move_type = std::get<1>(configurations[idx]);
        std::string start_type = std::get<2>(configurations[idx]);

        std::cout << "Configuration " << (idx + 1) << "/8:\n";
        std::cout << "  Strategy: " << strategy << "\n";
        std::cout << "  Move Type: " << move_type << "\n";
        std::cout << "  Start Type: " << start_type << "\n";

        double best_val = 1e18;
        double worst_val = -1e18;
        double total_val = 0;
        
        double min_time = 1e18;
        double max_time = -1e18;
        double total_time = 0;

        std::vector<int> best_route;

        for (int run = 0; run < run_count; ++run) {
            auto start_time = std::chrono::high_resolution_clock::now();

            std::vector<int> start_route;
            if (start_type == "random") {
                start_route = get_random_sol(n, k, g);
            } else {
                int start_idx = run % n;
                start_route = find_greedy(matrix, costs, start_idx);
            }

            std::vector<int> final_route = local_search(start_route, matrix, costs, move_type, strategy, g);
            double final_val = evaluate(matrix, costs, final_route);

            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end_time - start_time;
            double dur_sec = duration.count();

            total_val += final_val;
            total_time += dur_sec;
            if (dur_sec < min_time) min_time = dur_sec;
            if (dur_sec > max_time) max_time = dur_sec;

            if (final_val < best_val) {
                best_val = final_val;
                best_route = final_route;
            }
            if (final_val > worst_val) {
                worst_val = final_val;
            }
        }

        std::cout << "Results Summary:\n";
        std::cout << "  - Best Final Value (Minimum): " << best_val << "\n";
        std::cout << "  - Worst Final Value (Maximum): " << worst_val << "\n";
        std::cout << "  - Average Final Value: " << (total_val / run_count) << "\n";
        std::cout << "  - Minimum Run Time: " << min_time << " seconds\n";
        std::cout << "  - Maximum Run Time: " << max_time << " seconds\n";
        std::cout << "  - Average Run Time: " << (total_time / run_count) << " seconds\n";
        std::cout << "  - Best Route Found: [";
        for (size_t i = 0; i < best_route.size(); ++i) {
            std::cout << best_route[i] << (i + 1 < best_route.size() ? ", " : "");
        }
        std::cout << "]\n";
    }

    return 0;
}
