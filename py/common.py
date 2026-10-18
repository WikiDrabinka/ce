# Argv should be as follows: argv[1] - data file, argv[2] - potential destination plot file

import matplotlib.pyplot as plt
from sys import argv

def read_file(file_path: str = None) -> tuple[int, int, int]:
    # x y weight
    res = []
    with open(file_path or argv[1]) as file:
        for row in file:
            res.append(tuple(map(int, row.strip().split(';'))))
    return res

def get_matrix(data):
    # Cost of the destination is added to the connection
    res = [[0 for _ in range(len(data))] for _ in range(len(data))]
    for iidx, i in enumerate(data):
        for jidx, j in enumerate(data):
            ## "The distances between nodes are calculated as Euclidean distances rounded **mathematically** to integer values."
            # Python rounding is messed up, so we are using method mentioned in the labs
            raw_distance = ((i[0] - j[0])**2 + (i[1] - j[1])**2)**0.5
            distance_rounded = int(raw_distance + 0.5)
            res[iidx][jidx] = distance_rounded
    return res

def get_costs(data):
    res = [j[2] for j in data]
    return res


def evaluate(matrix, costs, solution) -> float:
    # Solution should contain indices of the distance matrix
    from itertools import pairwise
    cost = sum(costs[node] for node in solution)
    for a, b in pairwise([*solution, solution[0]]):
        cost += matrix[a][b]
    return cost

def plot(solution, file_path: str = None, output_path: str = None):
    from sys import argv
    solution = solution + [solution[0]]
    data = read_file(file_path)
    chosen = [data[i] for i in solution]
    figure = plt.figure()

    x, y, c = zip(*data)
    c = [p for p in c]
    plt.scatter(x, y, c=c, cmap='coolwarm', zorder=2)

    x, y, _ = zip(*chosen)
    plt.plot(x, y, color='gray', linewidth=1, zorder=1)

    plt.colorbar()
    save_path = output_path
    if save_path is None and len(argv) >= 3:
        save_path = argv[2]

    if save_path is None:
        plt.show()
    else:
        plt.savefig(save_path, dpi=300)
    plt.close(figure)
