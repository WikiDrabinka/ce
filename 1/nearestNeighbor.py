from random import choice
from time import time
from ce.common import read_file, get_matrix, plot, evaluate
from math import ceil

TOTAL_SOLUTIONS = 200

data = read_file()
data = get_matrix(data)
to_find = ceil(len(data) / 2)
indices = [idx for idx, _ in enumerate(data)]

res = []

run_time = time()

for _ in range(TOTAL_SOLUTIONS):
    sol = [choice(indices)]
    while len(sol) < to_find:
        possible_next = (i for i in indices if i not in sol)
        next = min(possible_next, key=lambda x: data[sol[-1]][x])
        sol.append(next)
    res.append(sol)

run_time = time() - run_time

scores = [evaluate(data, f) for f in res]

print(f'Min: {min(scores)}\nAvg: {sum(scores)/len(scores)}\nMax: {max(scores)}')
best_sol = min(res, key=lambda x: evaluate(data, x))
print('Best solution:', best_sol)
print(f'Run time {round(run_time * 1000, 2)}ms')

plot(best_sol)
