# Nearst Neighbor Full Route - Weighted

from time import time
from math import ceil
from common import read_file, get_matrix, get_costs, plot, evaluate

WEIGHT = 0.5

def find(matrix, costs):
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
            candidates = []

            for node in indices:
                if node in sol:
                    continue

                insertions = [
                    (get_insert_cost(sol, node, idx), idx)
                    for idx in range(len(sol) + 1)
                ]

                insertions.sort()

                best_cost, best_idx = insertions[0]
                second_cost, _ = insertions[1]

                regret = second_cost - best_cost

                score = WEIGHT * regret - (1 - WEIGHT) * best_cost

                candidates.append((score, node, best_idx))

            # Select the candidate with the highest score.
            _, node, idx = max(candidates)

            sol.insert(idx, node)

        res.append(sol)


    run_time = time() - run_time

    scores = [evaluate(matrix, costs, solution) for solution in res]

    return scores, run_time, res

if __name__=="__main__":
    data = read_file()
    matrix = get_matrix(data)
    costs = get_costs(data)


    scores, run_time, res = find(matrix, costs)
    print(f'Min: {min(scores)}\nAvg: {sum(scores) / len(scores)}\nMax: {max(scores)}')

    best_sol = min(res, key=lambda x: evaluate(matrix, costs, x))

    print('Best solution:', best_sol)
    print(f'Run time {round(run_time * 1000, 2)}ms')

    plot(best_sol)
