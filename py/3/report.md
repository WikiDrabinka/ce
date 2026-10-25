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

| Method / Run type | TSPA: average (best - worst) | TSPB: average (best - worst) |
| --- | --- | --- |
| steepest, nodes, random | 89872.35 (82701 - 99314) | 65720.85 (59912 - 71828) |
| steepest, nodes, greedy | 78856.01 (76095 - 82627) | 54019.39 (51282 - 57036) |
| steepest, edges, random | 77409.99 (74115 - 80128) | **53467.88** (**51120** - 56622) |
| steepest, edges, greedy | 78037.45 (75630 - 81437) | 53587.86 (51609 - 55751) |
| greedy, nodes, random | 88302.77 (82739 - 94657) | 64167.08 (56771 - 71837) |
| greedy, nodes, greedy | 79041.41 (76289 - 83240) | 53984.46 (51425 - 57782) |
| greedy, edges, random | 77506.33 (74842 - 80921) | 53726.51 (51027 - 56907) |
| greedy, edges, greedy | 77535.99 (75061 - 80483) | 53569.45 (51618 - **55518**) |
| Lab 1: Nearest Neighbor | 87476.63 (84362 - 90075) | 59397.33 (56007 - 63166) |
| Lab 1: Nearest Neighbor (all positions) | 74346.96 (72328 - 77022) | 59475.5 (52915 - 66164) |
| Lab 1: Greedy Cycle | **73793.4** (**72590** - **75281**) | 59649.91 (55655 - 63734) |
| Lab 2: Nearest Neighbor Full Route (2-Regret) | 121360.29 (110950 - 135771) | 73649.84 (68761 - 79756) |
| Lab 2: Nearest Neighbor Full Route (Weighted) | 75542.49 (72419 - 78504) | 55694.15 (50485 - 61406) |
| Lab 2: Greedy Cycle (2-Regret) | 120502.05 (111748 - 129972) | 72327.22 (66167 - 77274) |
| Lab 2: Greedy Cycle (Weighted) | 73339.43 (72011 - 76027) | 57902.08 (52860 - 63406) |

### Lab 3 Runtime

| Method / Run type | TSPA: average (min - max) | TSPB: average (min - max) |
| --- | --- | --- |
| steepest, nodes, random | 0.0869s (0.0619 - 0.1277) | 0.0942s (0.0631 - 0.2104) |
| steepest, nodes, greedy | 0.0623s (0.0431 - 0.1221) | 0.0586s (0.0371 - 0.1388) |
| steepest, edges, random | 0.0822s (0.0359 - 0.1431) | 0.0596s (0.0433 - 0.0937) |
| steepest, edges, greedy | 0.0739s (0.0332 - 0.1274) | 0.0449s (0.0285 - 0.0630) |
| greedy, nodes, random | 0.0894s (0.0516 - 0.2246) | 0.0806s (0.0592 - 0.1414) |
| greedy, nodes, greedy | 0.0554s (0.0318 - 0.1303) | 0.0465s (0.0264 - 0.0752) |
| greedy, edges, random | 0.1091s (0.0534 - 0.1999) | 0.0772s (0.0579 - 0.1216) |
| greedy, edges, greedy | 0.0839s (0.0346 - 0.1356) | 0.0431s (0.0259 - 0.0772) |

## Instance A

## Instance B

## Conclusions

I am going to bed