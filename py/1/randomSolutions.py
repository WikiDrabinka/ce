from common import read_file, evaluate, get_matrix, get_costs, plot
import random as rnd
from math import ceil
from time import time

TOTAL_SOLUTIONS = 200

time_limit = input('Time limit in ms (press enter to skip): ').strip()
if time_limit == '':
    print('Skipped, running', TOTAL_SOLUTIONS, 'iterations.')
    time_limit = None
else:
    time_limit = int(time_limit) / 1000
    print(f'Running for {round(time_limit // 60)}min {round(time_limit % 60)}s.')

data = read_file()
matrix = get_matrix(data)
costs = get_costs(data)
to_find = ceil(len(matrix) / 2)
indices = [idx for idx, _ in enumerate(matrix)]

to_find = ceil(len(matrix) / 2)
indices = [idx for idx, _ in enumerate(matrix)]
run_time = time()
found = []
scores = []
if time_limit is None:
    found = [rnd.choices(indices, k=to_find) for _ in range(TOTAL_SOLUTIONS)]
    for f in found:
        rnd.shuffle(f)
    run_time = time() - run_time
    scores = [evaluate(matrix, costs, f) for f in found]
else:
    while time() - run_time < time_limit:
        f = rnd.choices(indices, k=to_find)
        rnd.shuffle(f) 
        found.append(f)
        scores.append(evaluate(matrix, costs, f))
    run_time = time() - run_time


if time_limit is not None:
    print(f'In {time_limit * 1000}ms, a total of {len(scores)} has been evaluated, {len(scores) / time_limit} per second.')
print(f'Min: {min(scores)}\nAvg: {sum(scores)/len(scores)}\nMax: {max(scores)}')
best_sol = min(found, key=lambda x: evaluate(matrix, costs, x))
print('Best solution:', best_sol)
print(f'Run time {round(run_time * 1000, 2)}ms')

plot(best_sol)
