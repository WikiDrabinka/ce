from ce.common import read_file, evaluate, get_matrix, get_costs, plot
import random as rnd
from math import ceil
from time import time

TOTAL_SOLUTIONS = 200

data = read_file()
matrix = get_matrix(data)
costs = get_costs(data)
to_find = ceil(len(matrix) / 2)
indices = [idx for idx, _ in enumerate(matrix)]

to_find = ceil(len(matrix) / 2)
indices = [idx for idx, _ in enumerate(matrix)]
run_time = time()
found = [rnd.choices(indices, k=to_find) for _ in range(TOTAL_SOLUTIONS)]
for f in found:
    rnd.shuffle(f)
run_time = time() - run_time

scores = [evaluate(matrix, costs, f) for f in found]

print(f'Min: {min(scores)}\nAvg: {sum(scores)/len(scores)}\nMax: {max(scores)}')
best_sol = min(found, key=lambda x: evaluate(matrix, costs, x))
print('Best solution:', best_sol)
print(f'Run time {round(run_time * 1000, 2)}ms')

plot(best_sol)