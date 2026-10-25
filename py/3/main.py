from pathlib import Path
from py.common import read_file, get_costs, get_matrix, plot
from .nnfr_2r import find

from random import sample, shuffle
from math import ceil
from itertools import combinations
from time import time
from sys import argv


data = read_file()
costs = get_costs(data)
matrix = get_matrix(data)

N = len(matrix)
K = ceil(N / 2)
EPS = 1e-9

def get_random_sol(matrix):
    return sample(range(len(matrix)), K)


def objective(route, matrix, costs):
    node_cost = sum(costs[node] for node in route)
    if not route:
        return node_cost

    route_cost = sum(
        matrix[route[i]][route[(i + 1) % len(route)]]
        for i in range(len(route))
    )
    return node_cost + route_cost


def neighbors(route, move_type, all_nodes):
    size = len(route)
    positions = list(range(size))
    shuffle(positions)

    if move_type == "nodes":
        for i, j in combinations(positions, 2):
            neighbor = list(route)
            neighbor[i], neighbor[j] = neighbor[j], neighbor[i]
            yield neighbor
    elif move_type == "edges":
        for first, second in combinations(positions, 2):
            i, j = sorted((first, second))
            if j == i + 1 or (i == 0 and j == size - 1):
                continue
            neighbor = list(route)
            neighbor[i:j + 1] = reversed(neighbor[i:j + 1])
            yield neighbor
    else:
        raise ValueError("move_type must be 'nodes' or 'edges'")

    selected = set(route)
    unselected = [node for node in all_nodes if node not in selected]
    shuffle(unselected)
    for position in positions:
        for node in unselected:
            neighbor = list(route)
            neighbor[position] = node
            yield neighbor


def local_search(start_route, matrix, costs, move_type, strategy):
    route = list(start_route)
    all_nodes = list(range(len(matrix)))

    while True:
        current_score = objective(route, matrix, costs)

        if strategy == "greedy":
            improved_route = None
            for neighbor in neighbors(route, move_type, all_nodes):
                if objective(neighbor, matrix, costs) < current_score - EPS:
                    improved_route = neighbor
                    break

            if improved_route is None:
                return route
            route = improved_route

        elif strategy == "steepest":
            best_route = route
            best_score = current_score

            for neighbor in neighbors(route, move_type, all_nodes):
                neighbor_score = objective(neighbor, matrix, costs)
                if neighbor_score < best_score - EPS:
                    best_route = neighbor
                    best_score = neighbor_score

            if best_route is route:
                return route
            route = best_route

        else:
            raise ValueError("strategy must be 'steepest' or 'greedy'")



def get_greedy_sol(matrix, costs, start_idx):
    result = find(matrix, costs, start_idx)

    if isinstance(result, tuple) and len(result) == 2:
        result = result[0]

    return list(result)



def run_experiments(runs=200):
    dataset_path = Path(argv[1])
    image_dir = Path(__file__).with_name("images")
    image_dir.mkdir(exist_ok=True)

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

            final_route = local_search(start, matrix, costs, move_type, strategy)
            final_value = objective(final_route, matrix, costs)

            run_duration = time() - t_start
            run_times.append(run_duration)
            final_values.append(final_value)

            if final_value < best_final_val:
                best_final_val = final_value
                best_route = list(final_route)

        avg_value = sum(final_values) / runs
        min_value = min(final_values)
        max_value = max(final_values)
        min_time = min(run_times)
        avg_time = sum(run_times) / runs
        max_time = max(run_times)

        print(f"Results Summary:")
        print(f" - Best: {min_value}")
        print(f" - Worst: {max_value}")
        print(f" - Average: {avg_value:.2f}")
        print(f" - Minimum run time: {min_time:.4f}s")
        print(f" - Average run time: {avg_time:.4f}s")
        print(f" - Maximum run time: {max_time:.4f}s")
        print(f" - Best route: {best_route}")

        image_path = image_dir / (
            f"{dataset_path.stem}_{strategy}_{move_type}_{start_type}.png"
        )
        plot(best_route, str(dataset_path), str(image_path))
        print(f" - Best route plot: {image_path}")


if __name__ == "__main__":
    print(f"Dataset: {argv[1]}")
    run_experiments()
