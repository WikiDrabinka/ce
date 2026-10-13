# Nearst Neighbor Full Route - 2-Regret

from time import time
from math import ceil
from py.common import read_file, get_matrix, get_costs, plot, evaluate

data = read_file()
matrix = get_matrix(data)
costs = get_costs(data)

to_find = ceil(len(matrix) / 2)
indices = list(range(len(matrix)))

res = []

run_time = time()


def get_insert_cost(solution, node, idx):
    if idx == 0:
        return matrix[node][solution[0]] + costs[node]

    if idx == len(solution):
        return matrix[solution[-1]][node] + costs[node]

    return (
        costs[node]
        + matrix[solution[idx - 1]][node]
        + matrix[node][solution[idx]]
        - matrix[solution[idx - 1]][solution[idx]]
    )


for start_idx in indices:
    sol = [start_idx]

    while len(sol) < to_find:
        best_candidate = None

        for node in indices:
            if node in sol:
                continue

            insertions = sorted([
                (get_insert_cost(sol, node, idx), idx)
                for idx in range(len(sol) + 1)
            ])

            best_cost, best_idx = insertions[0]
            second_cost, _ = insertions[1]
            regret = second_cost - best_cost
            candidate = (regret, -best_cost, node, best_idx)
            if best_candidate is None or candidate[:2] > best_candidate[:2]:
                best_candidate = candidate

        _, _, node, idx = best_candidate
        sol.insert(idx, node)

    res.append(sol)

run_time = time() - run_time

scores = [evaluate(matrix, costs, solution) for solution in res]

print(f'Min: {min(scores)}\nAvg: {sum(scores) / len(scores)}\nMax: {max(scores)}')

best_sol = min(res, key=lambda x: evaluate(matrix, costs, x))

print('Best solution:', best_sol)
print(f'Run time {round(run_time * 1000, 2)}ms')

plot(best_sol)
