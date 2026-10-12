## Problem description

We are given three columns of integers with a row for each node. The first two columns contain x
and y coordinates of the node positions in a plane. The third column contains node costs. The goal is
to select exactly 50% of the nodes (if the number of nodes is odd we round the number of nodes to
be selected up) and form a Hamiltonian cycle (closed path) through this set of nodes such that the
sum of the total length of the path plus the total cost of the selected nodes is minimized.

The distances between nodes are calculated as Euclidean distances rounded mathematically to
integer values. The distance matrix should be calculated just after reading an instance and then only
the distance matrix (no nodes coordinates) should be accessed by optimization methods to allow
instances defined only by distance matrices.

## Algorithms

### Nearest Neighbor

```
Input: 
matrix - matrix of distances between nodes
costs - array of node costs

to_find = ⌈len(matrix) / 2⌉
indices = [i for i∊ℕ and i<len(matrix)]

solution = [random.choice(indices)]
while len(solution) < to_find:
    possible_next = indices\solution
    next = argmin[x∊possible_next](matrix[solution[-1]][x] + costs[x])
    solution.append(next)
```

### Nearest Neighbor considering adding the node at all possible positions

```
Input: 
matrix - matrix of distances between nodes
costs - array of node costs

to_find = ⌈len(matrix) / 2⌉
indices = [i for i∊ℕ and i<len(matrix)]

function get_insert_cost(solution, node, idx):
    if idx == 0:
        return matrix[node][solution[0]] + costs[node]
    if len(solution) == idx:
        return matrix[solution[-1]][node] + costs[node]
    return costs[node] + matrix[solution[idx-1]][node] + matrix[node][solution[idx]] - matrix[solution[idx-1]][solution[idx]]

solution = [random.choice(indices)]
while len(solution) < to_find:
    possible_next_node = indices\solution
    possible_inserts = [(node, index) for node in indices for index∊ℕ and index<len(solution) and index∉solution]
    # All inserts after idx
    next = argmin[x∊possible_inserts](get_insert_costs(solution, x[0], x[1]))
    # Insert at the beginning
    beginning_next = argmin[x∊possible_next_node](cost[x] + matrix[x][solution[0]])
    
    if get_insert_cost(solution, next[0], next[1]) < costs[beginning_next] + matrix[beginning_next][solution[0]]:
            solution.insert(next[1] + 1, next[0])
        else:
            solution.insert(0, beginning_next)
```

### Greedy cycle

```
Input: 
matrix - matrix of distances between nodes
costs - array of node costs

to_find = ⌈len(matrix) / 2⌉
indices = [i for i∊ℕ and i<len(matrix)]

function get_insert_cost(solution, node, idx):
    if idx == 0 or len(solution) == idx:
        return matrix[solution[-1]][node] + costs[node] + matrix[node][solution[0]] - matrix[solution[-1]][solution[0]] 
    return costs[node] + matrix[solution[idx - 1]][node] + matrix[node][solution[idx]] - matrix[solution[idx - 1]][solution[idx]]

solution = [random.choice(indices)]
while len(solution) < to_find:
    possible_inserts = [(node, index) for node in indices for index∊ℕ and index<=len(solution) and index∉solution]
    next = argmin[x∊possible_inserts](get_insert_costs(solution, x[0], x[1]))
    solution.insert(next[1], next[0])
```


## Results

|  | Random solutions | Nearest neighbor | Nearest neighbor (all positions) | Greedy cycle |
| --- | --- | --- | --- | --- |
| **Instance A** |
| Min | 225267 | 84362 | 72328 | 72590 |
| Avg | 261519.96 | 87457.88 | 74322.08 | 73781.12 |
| Max | 290575 | 89933 | 77022 | 75281 |
| *Execution time* | 2.93ms | 947.68ms | 151516.94ms | 141700.29ms |
| **Instance B** |
| Min | 178305 | 56007 | 54139 | 55655 |
| Avg | 207024.785 | 59485.015 | 59401.325 | 59135.765 |
| Max | 231261 | 63166 | 64061 | 63734 |
| *Execution time* | 3.09ms | 949.7ms | 162898.49ms | 158964.65ms |

## Instance A

### Random

Min: 225267

Avg: 261519.96

Max: 290575

Best solution: [12, 141, 90, 162, 44, 148, 7, 93, 79, 107, 116, 58, 136, 181, 23, 130, 78, 13, 129, 112, 110, 141, 66, 160, 184, 140, 22, 106, 123, 71, 76, 7, 34, 124, 187, 145, 72, 78, 156, 156, 30, 186, 84, 141, 152, 9, 62, 51, 92, 157, 183, 132, 196, 84, 23, 112, 120, 97, 175, 137, 13, 4, 185, 27, 192, 64, 153, 32, 192, 178, 0, 26, 140, 84, 58, 86, 80, 115, 165, 109, 184, 48, 65, 88, 3, 26, 8, 87, 191, 52, 197, 178, 6, 101, 180, 144, 160, 156, 84, 72]

![random](./images/randomA.png)

(*In all images, the color represents node cost*)

Run time: 2.93ms

### NN

Min: 84362

Avg: 87457.88

Max: 89933

Best solution: [54, 9, 33, 36, 177, 67, 144, 53, 135, 58, 134, 176, 174, 106, 89, 107, 57, 182, 151, 93, 50, 3, 1, 164, 178, 29, 136, 186, 184, 47, 159, 130, 153, 78, 88, 193, 170, 146, 75, 64, 150, 42, 113, 43, 85, 142, 8, 131, 84, 158, 192, 181, 114, 141, 180, 132, 76, 80, 48, 31, 63, 149, 168, 97, 127, 187, 30, 123, 152, 108, 60, 116, 49, 11, 119, 160, 83, 90, 51, 16, 35, 87, 45, 140, 52, 179, 185, 28, 102, 156, 157, 71, 91, 38, 100, 4, 112, 139, 115, 5]\

![nn](./images/nnA.png)

Run time 947.68ms

### Nearest neighbor considering adding the node at all possible position

Min: 72328

Avg: 74322.08

Max: 77022

Best solution: [104, 129, 69, 127, 97, 168, 116, 149, 63, 11, 49, 160, 83, 123, 100, 152, 108, 60, 31, 48, 80, 91, 157, 71, 4, 76, 132, 180, 28, 102, 185, 35, 156, 51, 16, 67, 144, 140, 53, 179, 52, 177, 36, 33, 9, 54, 131, 174, 8, 164, 186, 136, 163, 29, 101, 178, 1, 169, 3, 50, 147, 47, 93, 106, 42, 89, 113, 130, 159, 146, 170, 153, 193, 78, 88, 107, 64, 57, 182, 151, 150, 176, 72, 134, 84, 58, 158, 192, 181, 114, 135, 73, 45, 87, 141, 187, 30, 189, 2, 171]

![nnfull](./images/nnfullA.png)

Run time 151516.94ms

### Greedy cycle

Min: 72590

Avg: 73781.12

Max: 75281

Best solution: [80, 91, 157, 71, 4, 76, 132, 180, 28, 102, 185, 35, 156, 51, 16, 67, 144, 140, 53, 179, 52, 177, 36, 33, 9, 54, 131, 174, 8, 142, 164, 186, 136, 163, 29, 101, 178, 1, 169, 3, 50, 147, 47, 93, 106, 42, 89, 113, 130, 159, 146, 170, 153, 193, 78, 88, 107, 64, 57, 182, 151, 150, 176, 72, 134, 84, 58, 158, 192, 181, 114, 135, 73, 45, 87, 141, 187, 30, 189, 2, 171, 69, 127, 97, 160, 83, 123, 90, 100, 152, 108, 60, 49, 11, 63, 168, 149, 116, 31, 48]

![greedycycle](./images/greedycycleA.png)

Run time 141700.29ms

## Instance B

### Random

Min: 178305

Avg: 207024.785

Max: 231261

Best solution: [15, 86, 123, 139, 73, 21, 136, 31, 84, 167, 89, 99, 112, 178, 102, 115, 153, 151, 59, 49, 137, 141, 187, 81, 62, 7, 29, 172, 134, 160, 146, 6, 131, 44, 35, 31, 133, 128, 106, 87, 167, 5, 149, 7, 8, 126, 130, 92, 189, 126, 181, 1, 177, 197, 133, 89, 162, 114, 196, 188, 45, 14, 12, 46, 104, 176, 63, 82, 73, 49, 185, 184, 197, 168, 48, 25, 98, 192, 168, 142, 134, 173, 166, 195, 148, 191, 6, 154, 87, 195, 178, 136, 2, 64, 39, 38, 136, 182, 76, 14]

![random](./images/randomB.png)

Run time 3.09ms

### NN

Min: 56007

Avg: 59485.015

Max: 63166

Best solution: [32, 89, 57, 76, 54, 127, 129, 90, 153, 20, 14, 2, 110, 118, 56, 114, 196, 10, 148, 143, 78, 192, 162, 104, 185, 184, 3, 151, 69, 172, 133, 120, 96, 121, 198, 48, 72, 46, 86, 117, 160, 136, 8, 21, 154, 140, 47, 170, 125, 177, 6, 132, 29, 197, 175, 134, 45, 60, 40, 71, 155, 97, 115, 180, 65, 28, 149, 145, 158, 116, 147, 42, 50, 130, 30, 183, 38, 64, 85, 59, 167, 51, 87, 179, 82, 88, 171, 99, 67, 24, 34, 181, 152, 73, 98, 27, 135, 75, 79, 156]

![nn](./images/nnB.png)

Run time 949.7ms

### Nearest neighbor considering adding the node at all possible position

Min: 54139

Avg: 59401.325

Max: 64061

Best solution: [121, 198, 96, 48, 43, 156, 89, 57, 104, 54, 90, 14, 2, 74, 20, 153, 182, 52, 129, 127, 80, 192, 162, 185, 76, 135, 27, 111, 98, 151, 122, 173, 3, 184, 86, 46, 72, 117, 8, 120, 133, 136, 172, 160, 69, 168, 189, 21, 183, 130, 191, 30, 176, 38, 47, 140, 154, 97, 26, 125, 177, 170, 115, 180, 6, 103, 178, 132, 29, 197, 36, 62, 175, 134, 45, 157, 141, 155, 71, 60, 40, 33, 64, 85, 59, 73, 51, 167, 181, 152, 34, 24, 11, 66, 67, 31, 88, 179, 87, 82]

![nnfull](./images/nnfullB.png)

Run time 162898.49ms

### Greedy cycle

Min: 55655

Avg: 59135.765

Max: 63734

Best solution: [36, 197, 6, 180, 115, 170, 97, 140, 47, 155, 71, 175, 134, 45, 157, 60, 141, 40, 33, 64, 21, 189, 168, 69, 160, 172, 136, 133, 120, 8, 117, 72, 46, 86, 184, 3, 173, 122, 151, 98, 111, 27, 135, 76, 185, 162, 192, 80, 52, 182, 20, 74, 2, 93, 110, 118, 14, 153, 90, 129, 127, 54, 104, 57, 89, 156, 43, 48, 96, 198, 121, 63, 130, 191, 183, 30, 176, 38, 154, 26, 125, 177, 138, 158, 145, 149, 147, 42, 116, 37, 146, 13, 28, 65, 178, 103, 29, 132, 1, 174]

![greedycycle](./images/greedycycleB.png)

Run time 158964.65ms

## Conclusions

Out of the three algorithms, regular Nearest Neighbor finds good solutions the quickest, but Nearest Neighbor considering all positions and Greedy Cycle beat it by the quality of solutions, while suffering in execution time. In both instances, Greedy Cycle finds a slightly worse best solution than Nearest Neighbor considering all positions, but it finds better solutions on average.


*All solutions have been verified with the solution checker*

## Source code

[https://github.com/WikiDrabinka/ce/tree/main/1](https://github.com/WikiDrabinka/ce/tree/main/1)