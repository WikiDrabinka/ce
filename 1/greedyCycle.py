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
        possible_next_node = (i for i in indices if i not in sol)
        best, best_eval = None, 1e100
        for p in possible_next_node:
            for insert_idx in range(len(sol)):
                potential_solution = sol[:insert_idx] + [p] + sol[insert_idx:]
                local_eval = evaluate(data, potential_solution)
                if local_eval < best_eval:
                    best, best_eval = potential_solution, local_eval
        sol = best
        print(len(sol))
    res.append(sol)

run_time = time() - run_time

scores = [evaluate(data, f) for f in res]

print(f'Min: {min(scores)}\nAvg: {sum(scores)/len(scores)}\nMax: {max(scores)}')
best_sol = min(res, key=lambda x: evaluate(data, x))
print('Best solution:', best_sol)
print(f'Run time {round(run_time * 1000, 2)}ms')

plot(best_sol)
