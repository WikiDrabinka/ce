# Lab 3 Report

Wiktoria Drabińska 160294, Wojciech Dudziński 160293

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


## Algorithm

```
1. Main Experiment Runner
Plaintext

Algorithm: RunExperiments(matrix, costs, configurations, runs=200)
    For each (strategy, move_type, start_type) in configurations:
        Initialize list of final_values, run_times
        BestFinalValue ← Infinity
        BestRoute ← None

        For run_idx from 0 to runs - 1:
            StartTime ← CurrentTime()

            // 1. Generate starting solution
            If start_type == "random" then
                start_route ← GetRandomSolution(matrix, K)
            Else
                start_idx ← run_idx mod N
                start_route ← GetGreedySolution(matrix, costs, start_idx)
            End If

            // 2. Run Local Search
            final_route ← LocalSearch(start_route, matrix, move_type, strategy)
            final_value ← Objective(final_route, matrix, costs)

            RunDuration ← CurrentTime() - StartTime
            Store final_value and RunDuration

            If final_value < BestFinalValue then
                BestFinalValue ← final_value
                BestRoute ← final_route
            End If

        End For

        Print score min/max/average, runtime min/max/average, and BestRoute
    End For

Algorithm: Objective(route, matrix, costs)
    node_cost ← sum(costs[node] for node in route)
    route_cost ← sum(matrix[route[i]][route[(i + 1) mod len(route)]] for i in range(len(route)))
    Return node_cost + route_cost
```

```
2. Local Search Core
Plaintext

Algorithm: LocalSearch(start_route, matrix, costs, move_type, strategy)
    route ← Copy(start_route)
    
    While True:
        If strategy == "greedy" then
            // Generate all intra-route and inter-route moves
            moves ← Combine(GetIntraMoves(route, move_type), GetInterMoves(route))
            Shuffle(moves)  // Randomized order for greedy search
            
            improved ← False
            For each move in moves:
                delta ← DeltaForMove(route, move, matrix, costs)
                If delta < -EPS then
                    ApplyMove(route, move)
                    improved ← True
                    Break // Stop at the first improving move
                End If
            End For
            
            If not improved then
                Return route
            End If

        Else if strategy == "steepest" then
            best_move ← None
            best_delta ← -EPS

            // Evaluate entire neighborhood (both intra and inter-route)
            For each move in Combine(GetIntraMoves(route, move_type), GetInterMoves(route)):
                delta ← DeltaForMove(route, move, matrix, costs)
                If delta < best_delta then
                    best_delta ← delta
                    best_move ← move
                End If
            End For

            If best_move is None then
                Return route
            End If

            ApplyMove(route, best_move)
        End If
    End While
```

```
3. Neighborhood & Delta Evaluation
Plaintext

Algorithm: GetIntraMoves(route, move_type)
    If move_type == "nodes" then
        // Two-nodes exchange (swap positions i and j)
        Return { ("swap", i, j) for all pairs (i, j) }
    Else if move_type == "edges" then
        // Two-edges exchange (2-opt reverse segment, excluding adjacent edges)
        Return { ("reverse", i, j) for valid segments [i, j] }
    End If

Algorithm: GetInterMoves(route)
    // Exchange one selected node with an unselected node
    unselected_nodes ← AllNodes \ route
    Return { ("replace", pos, node) for each pos in route, node in unselected_nodes }

Algorithm: DeltaForMove(route, move, matrix, costs)
    // Recompute this move's delta from the current route; do not carry deltas across iterations
    Kind, a, b ← move
    If kind == "swap" then Return SwapDelta(route, a, b, matrix)
    If kind == "reverse" then Return ReverseDelta(route, a, b, matrix)
    If kind == "replace" then
        prev ← route[(a - 1) mod len(route)]
        old ← route[a]
        next ← route[(a + 1) mod len(route)]
        Return costs[b] - costs[old]
             + matrix[prev][b] + matrix[b][next]
             - matrix[prev][old] - matrix[old][next]
```

## Results

| Method / Run type | Instance A: average (best - worst) | Instance B: average (best - worst) |
| --- | --- | --- |
| steepest, nodes, random | 89975.6 (83270 - 96771) | 65898.8 (58765 - 74064) |
| steepest, nodes, greedy | 74666.5 (72089 - 76893) | 54288.8 (49978 - 61265) |
| steepest, edges, random | 77397.5 (75016 - 80970) | 53573 (51443 - **56060**) |
| steepest, edges, greedy | 73894.9 (72104 - 75652) | **53220.6** (49676 - 60362) |
| greedy, nodes, random | 87647.3 (80713 - 95146) | 63668.8 (55856 - 70342) |
| greedy, nodes, greedy | 74685.3 (72257 - 76909) | 54265.5 (49978 - 61265) |
| greedy, edges, random | 77345.5 (75064 - 80622) | 53695.7 (51641 - 56949) |
| greedy, edges, greedy | 74027.8 (72097 - 76385) | 53300.9 (**49570** - 60547) |
| Lab 1: Nearest Neighbor | 87476.63 (84362 - 90075) | 59397.33 (56007 - 63166) |
| Lab 1: Nearest Neighbor (all positions) | 74346.96 (72328 - 77022) | 59475.5 (52915 - 66164) |
| Lab 1: Greedy Cycle | 73793.4 (72590 - **75281**) | 59649.91 (55655 - 63734) |
| Lab 2: Nearest Neighbor Full Route (2-Regret) | 121360.29 (110950 - 135771) | 73649.84 (68761 - 79756) |
| Lab 2: Nearest Neighbor Full Route (Weighted) | 75542.49 (72419 - 78504) | 55694.15 (50485 - 61406) |
| Lab 2: Greedy Cycle (2-Regret) | 120502.05 (111748 - 129972) | 72327.22 (66167 - 77274) |
| Lab 2: Greedy Cycle (Weighted) | **73339.43** (**72011** - 76027) | 57902.08 (52860 - 63406) |

### Lab 3 Runtime

| Method / Run type | Instance A: average (min - max) | Instance B: average (min - max) |
| --- | --- | --- |
| steepest, nodes, random | 0.243417s (0.19872 - 0.347455) | 0.243801s (0.197221 - 0.317648) |
| steepest, nodes, greedy | 0.151723s (0.142109 - 0.170276) | 0.159687s (0.138963 - 0.182584) |
| steepest, edges, random | 0.410427s (0.346246 - 0.47343) | 0.41292s (0.366126 - 0.469225) |
| steepest, edges, greedy | 0.166165s (0.14206 - 0.201614) | 0.181376s (0.148795 - 0.229889) |
| greedy, nodes, random | **0.0308224s** (**0.0164679** - 0.0529609) | **0.028847s** (**0.0183508** - **0.0511948)** |
| greedy, nodes, greedy | 0.148551s (0.140379 - 0.160058) | 0.150827s (0.138765 - 0.165104) |
| greedy, edges, random | 0.0326536s (0.0204918 - **0.0528527**) | 0.033262s (0.0214708 - 0.0536183) |
| greedy, edges, greedy | 0.154173s (0.143242 - 0.178303) | 0.158293s (0.142438 - 0.176939) |

## Instance A

### Steepest, nodes, random

![image](./images/TSPA_steepest_nodes_random.png)

### Steepest, nodes, greedy

![image](./images/TSPA_steepest_nodes_greedy.png)

### Steepest, edges, random

![image](./images/TSPA_steepest_edges_random.png)

### Steepest, edges, greedy

![image](./images/TSPA_steepest_edges_greedy.png)

### Greedy, nodes, random

![image](./images/TSPA_greedy_nodes_random.png)

### Greedy, nodes, greedy

![image](./images/TSPA_greedy_nodes_greedy.png)

### Greedy, edges, random

![image](./images/TSPA_greedy_edges_random.png)

### Greedy, edges, greedy

![image](./images/TSPA_greedy_edges_greedy.png)

## Instance B

### Steepest, nodes, random

![image](./images/TSPB_steepest_nodes_random.png)

### Steepest, nodes, greedy

![image](./images/TSPB_steepest_nodes_greedy.png)

### Steepest, edges, random

![image](./images/TSPB_steepest_edges_random.png)

### Steepest, edges, greedy

![image](./images/TSPB_steepest_edges_greedy.png)

### Greedy, nodes, random

![image](./images/TSPB_greedy_nodes_random.png)

### Greedy, nodes, greedy

![image](./images/TSPB_greedy_nodes_greedy.png)

### Greedy, edges, random

![image](./images/TSPB_greedy_edges_random.png)

### Greedy, edges, greedy

![image](./images/TSPB_greedy_edges_greedy.png)

## Conclusions

I am going to bed