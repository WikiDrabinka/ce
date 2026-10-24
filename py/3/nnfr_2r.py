from time import time
from math import ceil
from py.common import read_file, get_matrix, get_costs, plot, evaluate

def find(matrix, costs, start_idx):
    to_find = ceil(len(matrix) / 2)
    indices = list(range(len(matrix)))

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

    return sol
