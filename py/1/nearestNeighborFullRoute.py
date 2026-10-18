from random import choice
from itertools import pairwise
from time import time
from common import read_file, get_matrix, get_costs, plot, evaluate
from math import ceil

def find(matrix, costs):
    to_find = ceil(len(matrix) / 2)
    indices = [idx for idx, _ in enumerate(matrix)]

    res = []

    run_time = time()

    def get_insert_cost(solution, node, idx):
        if idx == 0:
            return matrix[node][solution[0]] + costs[node]
        if len(solution) == idx:
            return matrix[solution[-1]][node] + costs[node]
        return costs[node] + matrix[solution[idx-1]][node] + matrix[node][solution[idx]] - matrix[solution[idx-1]][solution[idx]]

    for start_idx in indices:
        sol = [start_idx]
        while len(sol) < to_find:
            possible_inserts = ((i, idx) for i in indices for idx in range(len(sol) + 1) if i not in sol)
            next = min(possible_inserts, key=lambda x: get_insert_cost(sol, x[0], x[1]))
            sol.insert(next[1], next[0])
        res.append(sol)

    run_time = time() - run_time

    scores = [evaluate(matrix, costs, f) for f in res]

    return scores, run_time, res

if __name__=="__main__":
    data = read_file()
    matrix = get_matrix(data)
    costs = get_costs(data)

    scores, run_time, res = find(matrix, costs)
    print(f'Min: {min(scores)}\nAvg: {sum(scores)/len(scores)}\nMax: {max(scores)}')
    best_sol = min(res, key=lambda x: evaluate(matrix, costs, x))
    print('Best solution:', best_sol)
    print(f'Run time {round(run_time * 1000, 2)}ms')

    plot(best_sol)
