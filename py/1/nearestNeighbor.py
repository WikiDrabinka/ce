from random import choice
from time import time
from ce.common import read_file, get_matrix, get_costs, plot, evaluate
from math import ceil

data = read_file()
matrix = get_matrix(data)
costs = get_costs(data)
to_find = ceil(len(matrix) / 2)
indices = [idx for idx, _ in enumerate(matrix)]

res = []

run_time = time()

for start_idx in indices:
    sol = [start_idx]
    while len(sol) < to_find:
        possible_next = (i for i in indices if i not in sol)
        next = min(possible_next, key=lambda x: matrix[sol[-1]][x] + costs[x])
        sol.append(next)
    res.append(sol)

run_time = time() - run_time

scores = [evaluate(matrix, costs, f) for f in res]

print(f'Min: {min(scores)}\nAvg: {sum(scores)/len(scores)}\nMax: {max(scores)}')
best_sol = min(res, key=lambda x: evaluate(matrix, costs, x))
print('Best solution:', best_sol)
print(f'Run time {round(run_time * 1000, 2)}ms')

plot(best_sol)
