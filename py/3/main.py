from py.common import read_file, get_costs, get_matrix
from .nnfr_2r import find

from random import sample, shuffle
from math import ceil
from itertools import combinations
from time import time


data = read_file()
costs = get_costs(data)
matrix = get_matrix(data)

N = len(matrix)
K = ceil(N / 2)
EPS = 1e-9

def get_random_sol(matrix):
    return sample(range(len(matrix)), K)


def objective(route, matrix):
    if len(route) <= 1:
        return 0
    return sum(
        matrix[route[i]][route[(i + 1) % len(route)]]
        for i in range(len(route))
    )



def swap_delta(route, i, j, matrix):
    n = len(route)
    if i == j or n < 2:
        return 0

    affected = {
        (i - 1) % n, i,
        (j - 1) % n, j,
    }

    old_cost = 0
    new_cost = 0

    for edge_idx in affected:
        a = route[edge_idx]
        b = route[(edge_idx + 1) % n]
        old_cost += matrix[a][b]

        if edge_idx == i:
            a_new = route[j]
        elif edge_idx == j:
            a_new = route[i]
        else:
            a_new = a

        next_idx = (edge_idx + 1) % n
        if next_idx == i:
            b_new = route[j]
        elif next_idx == j:
            b_new = route[i]
        else:
            b_new = b

        new_cost += matrix[a_new][b_new]

    return new_cost - old_cost


def reverse_delta(route, i, j, matrix):
    n = len(route)
    if n < 3 or i == j:
        return 0

    if i > j:
        i, j = j, i

    a = route[(i - 1) % n]
    b = route[i]
    c = route[j]
    d = route[(j + 1) % n]

    delta = (
        matrix[a][c] + matrix[b][d]
        - matrix[a][b] - matrix[c][d]
    )

    for p in range(i, j):
        u = route[p]
        v = route[p + 1]
        delta += matrix[v][u] - matrix[u][v]

    return delta


def replace_delta(route, pos, new_node, matrix):
    n = len(route)
    if n <= 1:
        return 0

    prev_node = route[(pos - 1) % n]
    old_node = route[pos]
    next_node = route[(pos + 1) % n]

    return (
        matrix[prev_node][new_node]
        + matrix[new_node][next_node]
        - matrix[prev_node][old_node]
        - matrix[old_node][next_node]
    )



def intra_moves(route, move_type):
    n = len(route)

    if move_type == "nodes":
        for i, j in combinations(range(n), 2):
            yield ("swap", i, j)

    elif move_type == "edges":
        for i in range(n):
            for j in range(i + 2, n):
                if i == 0 and j == n - 1:
                    continue
                yield ("reverse", i, j)

    else:
        raise ValueError("move_type must be 'nodes' or 'edges'")


def inter_moves(route, all_nodes):
    selected = set(route)
    unselected = [node for node in all_nodes if node not in selected]

    for pos in range(len(route)):
        for node in unselected:
            yield ("replace", pos, node)


def delta_for_move(route, move, matrix):
    kind, a, b = move

    if kind == "swap":
        return swap_delta(route, a, b, matrix)
    if kind == "reverse":
        return reverse_delta(route, a, b, matrix)
    if kind == "replace":
        return replace_delta(route, a, b, matrix)

    raise ValueError(f"Unknown move: {kind}")


def apply_move(route, move):
    kind, a, b = move

    if kind == "swap":
        route[a], route[b] = route[b], route[a]
    elif kind == "reverse":
        route[a:b + 1] = reversed(route[a:b + 1])
    elif kind == "replace":
        route[a] = b
    else:
        raise ValueError(f"Unknown move: {kind}")



def local_search(start_route, matrix, move_type, strategy):
    route = list(start_route)
    all_nodes = list(range(len(matrix)))

    while True:
        if strategy == "greedy":
            moves = list(intra_moves(route, move_type))
            moves.extend(inter_moves(route, all_nodes))
            shuffle(moves)

            improved = False
            for move in moves:
                delta = delta_for_move(route, move, matrix)
                if delta < -EPS:
                    apply_move(route, move)
                    improved = True
                    break

            if not improved:
                return route

        elif strategy == "steepest":
            best_move = None
            best_delta = -EPS

            for move in intra_moves(route, move_type):
                delta = delta_for_move(route, move, matrix)
                if delta < best_delta:
                    best_delta = delta
                    best_move = move

            for move in inter_moves(route, all_nodes):
                delta = delta_for_move(route, move, matrix)
                if delta < best_delta:
                    best_delta = delta
                    best_move = move

            if best_move is None:
                return route

            apply_move(route, best_move)

        else:
            raise ValueError("strategy must be 'steepest' or 'greedy'")



def get_greedy_sol(matrix, costs, start_idx):
    result = find(matrix, costs, start_idx)

    if isinstance(result, tuple) and len(result) == 2:
        result = result[0]

    return list(result)



def run_experiments(runs=200):
    configurations = [
        (strategy, move_type, start_type)
        for strategy in ("steepest", "greedy")
        for move_type in ("nodes", "edges")
        for start_type in ("random", "greedy")
    ]

    for idx, (strategy, move_type, start_type) in enumerate(configurations):
        print(f"Running configuration {idx + 1}/8:")
        print(f"  Strategy: {strategy}")
        print(f"  Move Type: {move_type}")
        print(f"  Start Type: {start_type}")

        final_values = []
        run_times = []
        best_final_val = float('inf')
        best_route = None

        for run_idx in range(runs):
            t_start = time()

            if start_type == "random":
                start = get_random_sol(matrix)
            else:
                start = get_greedy_sol(matrix, costs, run_idx)

            final_route = local_search(start, matrix, move_type, strategy)
            final_value = objective(final_route, matrix)

            run_duration = time() - t_start
            run_times.append(run_duration)
            final_values.append(final_value)

            if final_value < best_final_val:
                best_final_val = final_value
                best_route = list(final_route)

        avg_value = sum(final_values) / runs
        min_value = min(final_values)
        max_value = max(final_values)
        avg_time = sum(run_times) / runs

        print(f"Results Summary:")
        print(f" - Best: {min_value}")
        print(f" - Worst: {max_value}")
        print(f" - Average: {avg_value:.2f}")
        print(f" - Average run time: {avg_time:.4f}s")
        print(f" - Best route: {best_route}")


if __name__ == "__main__":
    run_experiments()
