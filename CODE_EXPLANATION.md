# Code explanation

This document explains the code of **Tower Defence: Supply Lines**.
Line numbers refer to the files **in this repository**, which contain no comments, so the numbers match exactly what you see.

* Algorithm files (`core/`, A*, min-heap, `tests/test_runner.cpp`) are explained **line by line**. Lines that belong together are explained as one range, e.g. `29–32`.
* Game and UI files (`App/`) are explained **function by function**.

Contents

1. [Every file at a glance](#1-every-file-at-a-glance)
2. [Which algorithm is used where and why](#2-which-algorithm-is-used-where-and-why)
3. [Flow: from program start to the end of one wave](#3-flow-from-program-start-to-the-end-of-one-wave)
4. [Line by line: algorithm files](#4-line-by-line-algorithm-files)
5. [Function by function: game and UI files](#5-function-by-function-game-and-ui-files)

---

## 1. Every file at a glance

| File | Folder | What it does | Algorithm |
|---|---|---|---|
| `dataset.hpp` | core | Outpost, Road, Convoy, SupplyItem, Dataset structs | Data for all |
| `dataset_loader.hpp/.cpp` | core | Reads and validates a data file; removes outposts for the sensitivity test | Input validation |
| `disjoint_set.hpp/.cpp` | core | Union–find with path compression and union by rank | **Disjoint Set** |
| `mst_planner.hpp/.cpp` | core | Kruskal with the tie-break; incremental update after a destroyed road; component labelling | **Kruskal**, **Incremental repair**, **BFS** |
| `knapsack_allocator.hpp/.cpp` | core | Multiple 0/1 knapsack DP with backtracking; brute force for checking | **Multiple Knapsack DP**, **Brute force** |
| `pipeline.hpp/.cpp` | core | Kruskal → filter cut-off requests → DP; printing | Kruskal + DP (Part C) |
| `supply_network.hpp/.cpp` | core | The game's interface: raid target, incremental repair vs rebuild, supply phase, tower effects | Incremental repair, Kruskal, DP |
| `algorithm_trace.hpp/.cpp` | core | Step records for the three views | Kruskal, Disjoint Set, BFS, DP backtracking |
| `node.hpp/.cpp` | App | One grid tile with A* data | **A*** |
| `min_heap.hpp/.cpp` | App | Binary min-heap (A*'s open set) | **Min-Heap** |
| `a_star_path_finder.hpp/.cpp` | App | A* shortest path with Manhattan heuristic | **A*** |
| `grid.hpp/.cpp` | App | The 20×15 tile grid, 4-neighbours, frost costs, drawing | A* (data), UI |
| `enemy_manager.hpp/.cpp` | App | Spawns waves, keeps paths, reroutes enemies | A* (caller), game |
| `tower_manager.hpp/.cpp` | App | Places towers, refuses ones that block the path | A* (caller), game |
| `Enemy.hpp/.cpp` | App | Enemy types, movement along a path, health/shield bars | Game only |
| `tower.hpp/.cpp`, `gatling_tower.*`, `artillery_tower.*`, `frost_tower.*`, `barrier_tower.*` | App | The four towers | Game only |
| `projectile.*`, `projectile_manager.*` | App | Bullets, shells, splash damage | Game only |
| `game_manager.hpp/.cpp` | App | Main loop, input, waves, raids, supply phase, tooltips | Game (calls Kruskal / repair / DP) |
| `supply_overlay.hpp/.cpp` | App | Draws the simple supply map, the raid flash and the bomb | UI (shows Kruskal / repair) |
| `algorithm_views.hpp/.cpp` | App | Plan Network, Raid Repair, Load Convoys views | UI (shows Kruskal, repair, DP) |
| `hud.hpp/.cpp` | App | Top bar, sidebar, Supply box, toasts, tooltips, help | UI only |
| `menus.hpp/.cpp` | App | Main menu (drawn in code), how to play, pause, game over | UI only |
| `effects.hpp/.cpp` | App | Flash, explosions, death ring | UI only |
| `sound_manager.hpp/.cpp` | App | Sounds generated in code | UI only |
| `asset_manager.hpp/.cpp` | App | Loads textures and fonts | UI only |
| `ui_draw.hpp/.cpp` | App | Drawing helpers | UI only |
| `theme.hpp` | App | Layout sizes and colours | UI only |
| `main.cpp` | App | Starts the game | – |
| `test_runner.cpp` | tests | 15 test groups, 69 checks | Tests for all core algorithms |
| `astar_check.cpp` | tests | A* vs Dijkstra on random layouts (optional target) | A* test |
| `save_test_output.cmake` | tests | Saves the test output to a file (`cmake --build build --target test_output`) | – |
| `data/*.txt` | data | Worked example, disconnected, over-demand, too-heavy datasets | Input |
| `CMakeLists.txt` | root | Builds `test_runner` (no SFML) and `tower_defense` (SFML), copies DLLs | – |

---

## 2. Which algorithm is used where and why

| Algorithm | Where (file → function) | Requirement it meets | Why this choice |
|---|---|---|---|
| **Kruskal MST** | `mst_planner.cpp` → `MstPlanner::build` (lines 22–56) | Part A: reconnect every outpost to the Castle at **minimum total repair cost**, skip destroyed roads, handle a disconnected network. | Destroyed roads can split the graph. Kruskal naturally gives a **minimum spanning forest** (one tree per part), while Prim grows one tree from one start and would have to restart for each part. The tie-break is simply the sort order. The sorted list is **reused** by the incremental update. The graph is sparse (n = 12, m = 22), so O(m log n) is ideal; Prim's array version, O(n²), only wins on dense graphs. |
| **Tie-break** | `MstPlanner::comesBefore` (10–20) | A deterministic, explainable rule for equal costs. | Equal-cost roads give the same total cost, so the rule costs nothing. It links the most severe outposts first, and the id makes the tree unique, so results can be compared road by road. |
| **Disjoint Set** | `disjoint_set.cpp`; used in `build` line 46 and `traceKruskal` | Fast cycle test inside Kruskal. | Path compression + union by rank make each operation almost constant time (O(α(n))). Checking for a cycle with a graph search would cost O(n) per road. |
| **BFS** | `MstPlanner::labelComponents` (146–173), `destroyRoad` (74–92), `traceRaid` (58–72) | Report which outposts are cut off; find the two sides of a split tree. | Visits each tree road once, O(n). A tree has no cycles, so BFS is all that is needed. |
| **Incremental repair** | `MstPlanner::destroyRoad` (58–117), called by `SupplyNetwork::destroyRoad` | Roads are destroyed **during** a wave and the network must stay optimal immediately. | Removing a tree road splits one tree in two. By the **cut property**, the cheapest road crossing that cut completes the new optimal tree. Scanning the list **already sorted** in `build` finds it in O(n + m) with no new sort; a road outside the tree costs O(1). Every raid is also compared with a full rebuild (`SupplyNetwork::destroyRoad` lines 75–80): same cost every time, 18–67 ops vs 46–202. |
| **Multiple Knapsack DP** | `knapsack_allocator.cpp` → `packConvoysDP` (19–84) | Part B: load **3 convoys** with the requested items for **maximum total value**, each item at most once, respecting every capacity. | A **joint** state dp[i][c1][c2][c3] lets the DP trade items between convoys. Running a single knapsack 3 times is wrong: on the full list each convoy could take the same item (double counting); one convoy after another is valid but **not optimal** (example: capacities 4 and 2, items A(2,3), B(2,3), C(3,5): sequential = 6, joint DP = 8). Time O(N·4·P), where P = 21·16·11 = 3 696 for this game. |
| **Backtracking** | `packConvoysDP` (63–79) | Report **which** items go on which convoy. | Walks one path through the table, O(N). |
| **Brute force** | `packConvoysBruteForce` (86–130) | Proof that the DP is optimal (tests only). | Tries all 4^N assignments, so it is certainly right; at N = 12 the DP is about 950× faster. |
| **Pipeline (Part C)** | `pipeline.cpp` → `runPipeline`, `allocateSupplies` | Use Part A's result in Part B. | Requests of outposts not connected to the Castle are removed **before** the DP, because no convoy can reach them. |
| **A*** | `a_star_path_finder.cpp` → `findPath` | Enemy movement (supporting algorithm, from the DSA game). | Shortest path on a grid with frost costs. The Manhattan heuristic is admissible for 4-direction moves with cost ≥ 1, so the path is optimal (checked against Dijkstra in `astar_check.cpp`). |
| **Min-Heap** | `min_heap.cpp` | Open set of A*. | Gives the node with the smallest f in O(log n). Entries store a snapshot of (f, h), so changing a node's cost never breaks the heap order. |

---

## 3. Flow: from program start to the end of one wave

1. `main` → `GameManager::GameManager`
   * `AssetManager::loadAllAssets` loads textures and fonts.
   * `SupplyNetwork::load("../data/worked_example.txt")` → `loadDataset` → `parseDataset` (validation) → `SupplyNetwork::reset`:
     * `MstPlanner::build` (**Kruskal** with `DisjointSet`, then BFS `labelComponents`) → tree cost **45**;
     * `runSupplyPhase` → `allocateSupplies` (filter) → `packConvoysDP` (**DP**) → value **269**; outpost effects set.
   * `Grid::setStartEnd`, `EnemyManager::recalculatePaths` → `AStarPathfinder::findPath` (**A\*** with `MinHeap`).
2. `GameManager::run` loop starts: `handleInput` → `updateUi` → `update` (fixed steps) → `render`.
3. Main menu → **Play** → `restartGame` (resets everything, including `SupplyNetwork::reset`).
4. Player places towers → `tryPlaceTower` → `TowerManager::placeTower` → `Grid::setObstacle` + **A\*** check → `EnemyManager::rerouteAffected` → `applySupplyEffects` (`supplyEffectAt`: green dot if near a supplied outpost).
5. **Space** → `startNextWave` → `wavePlan(1)` → `EnemyManager::spawnWave` (8 enemies, mostly soldiers), +$100, banner.
6. Each step, `update`:
   * `TowerManager::update` (towers shoot) → `ProjectileManager::update` (hits, splash) → `EnemyManager::update` (spawn on game time, move along the A* path) → kills give money.
   * 1 s before each raid: `throwBomb` – the enemy closest to the target road throws a bomb (drawn by `drawBomb`; picture only).
   * Every 4 s of game time (3 times): `bombLands` (small explosion) and `raidRoad`
     1. `nextRaidTarget` → `SupplyNetwork::nextRoadToDestroy` (tier, distance to the A* path, id) → e.g. R4;
     2. `traceRaid` (record for view V);
     3. `SupplyNetwork::destroyRoad` → `MstPlanner::destroyRoad` (**incremental repair**: BFS sides, scan sorted list → R1) + a fresh `MstPlanner::build` for the "vs rebuild" number;
     4. `SupplyOverlay::startRaidAnimation` (red flash, green road, 0.35× slow motion), toast "Repaired: 21 ops vs 202".
   * `checkLivesLost`, `checkWinLoss`.
7. No enemies left → `onWaveEnded`:
   * `SupplyNetwork::runSupplyPhase` → `allocateSupplies` → `packConvoysDP` (**DP**, 269) → outpost effects;
   * `applySupplyEffects` (tower dots), console printout, toast "Supplies delivered: 269 – press L";
   * 6 s countdown → `startNextWave` for wave 2.
8. Any time: **P / V / L** → `openView` → `AlgorithmViews::open` (`traceKruskal`, `traceConvoyLoading`; V uses the stored raid trace). The game pauses while the view is open.

---

## 4. Line by line: algorithm files

### 4.1 `core/dataset.hpp` – the data the algorithms work on (all algorithms)

| Lines | What the code does | Why |
|---|---|---|
| 1 | `#pragma once` | The header is included by many files; this stops it being read twice in one file. |
| 2–3 | Includes `<string>` and `<vector>`. | Names are strings; lists are vectors. |
| 5 | `CONVOY_COUNT = 3` | The brief fixes **three** convoys. The DP table has exactly three capacity dimensions, so this constant is used everywhere instead of the number 3. |
| 7–13 | `struct Outpost`: id, name, grid position, severity (0–10), population. | One node of the graph. The grid position puts it on the battlefield; severity is used by the Kruskal tie-break. |
| 15–20 | `struct Road`: id, the two outposts it joins (`from`, `to`), repair `cost`, `blocked` flag. | One edge of the graph. `blocked = true` means destroyed: Kruskal must ignore it. |
| 22–26 | `struct Convoy`: id, name, capacity. | One knapsack (Wagon 20, Cart 15, Mule 10). |
| 28–34 | `struct SupplyItem`: id, name, weight, value, requesting outpost. | One item for the DP. The outpost is needed to drop items of cut-off outposts. |
| 36–41 | `struct Dataset`: lists of outposts, roads, convoys, items and the index of the Castle. | Everything read from one data file, passed as one object. |
| 43–45 | `roadSeverity`: severity of both ends added. | Used by the tie-break and printed in the tests. |

### 4.2 `core/dataset_loader.hpp` / `.cpp` – reading and checking a data file (input for all algorithms)

`dataset_loader.hpp`

| Lines | What | Why |
|---|---|---|
| 7 | `loadDataset(path, out, error)` | Opens a file and parses it. Returns `false` with a message on any problem, so bad input never reaches the algorithms. |
| 8 | `parseDataset(stream, ...)` | Parses from any stream (file or test string), so the tests can feed text directly. |
| 10 | `withoutOutposts(data, removed)` | Builds a copy of the data without some outposts. Used for the sensitivity test (removing Eaglecrest and Ravenwatch). |

`dataset_loader.cpp`

| Lines | What | Why |
|---|---|---|
| 7–13 | `cleanLine`: cuts everything after `#`, trims spaces/tabs/`\r`. | Data files may contain `#` notes and Windows line endings; after cleaning, an empty result means "skip this line". |
| 15–18 | `fail`: writes "line N: message" into `error` and returns `false`. | One short call per check, and every error message names the line. |
| 20–27 | `loadDataset`: opens the file; if it can't, reports "cannot open"; otherwise calls `parseDataset`. | Separates file handling from parsing. |
| 29–34 | Start of `parseDataset`: clears `out`; `outpostIndex` maps file ids → positions in the vector; sets of used ids catch duplicates; `roadLines`/`itemLines` remember line numbers for later error messages; Castle id and line. | The file uses its own ids; the algorithms use 0..n−1 positions (fast array access). |
| 36–41 | Read the file line by line, count lines, clean each line, skip empty ones. | Simple line-based format. |
| 43–46 | A line `OUTPOSTS` / `ROADS` / `CONVOYS` / `ITEMS` starts a section. | Tells the parser what the next lines mean. |
| 48–54 | `CASTLE <id>` line: read the id, remember the line. | The Castle can be declared anywhere in the file. |
| 56–65 | Outpost line: read 6 fields; reject wrong format, duplicate id, severity outside 0–10, negative population; store its position in `outpostIndex`. | Validation required by the brief (bad data must be rejected with a message, not crash). |
| 66–77 | Road line: read 5 fields; reject duplicate id, negative cost, `blocked` not 0/1; store it and its line number. | Endpoints are checked later (lines 101–111) because outposts may appear after roads. |
| 78–85 | Convoy line: id, name, capacity > 0, no duplicates. | A capacity of 0 would make the DP meaningless. |
| 86–95 | Item line: id, name, weight > 0, value ≥ 0, requesting outpost; no duplicates. | Weight 0 would let the knapsack take infinitely many "free" items. |
| 96–98 | A data line before any section is an error. | Catches broken files. |
| 101–111 | For every road: both outpost ids must exist and differ; replace file ids with vector positions. | A road to an unknown outpost or a self-loop is invalid input. |
| 112–117 | Same for items: the requesting outpost must exist; convert to position. | |
| 119–127 | The Castle line must exist, refer to a real outpost, and the Castle's severity must be 0. | Severity 0 makes sure the tie-break never favours the Castle's own roads. |
| 129–132 | Exactly 3 convoys are required. | The DP is written for `CONVOY_COUNT` dimensions. |
| 133 | All checks passed. | |
| 136–138 | `withoutOutposts`: mark the removed outposts. | |
| 140–146 | Copy the remaining outposts and remember each one's new position (`-1` = removed). | Positions must stay 0..n−1 without gaps. |
| 148–153 | Keep only roads whose both ends remain, with renumbered ends. | |
| 155–159 | Keep only items of remaining outposts. | |
| 161–163 | Convoys unchanged; renumber the Castle. | |

### 4.3 `core/disjoint_set.hpp` / `.cpp` – Disjoint Set (union–find), used by Kruskal

`disjoint_set.hpp`

| Lines | What | Why |
|---|---|---|
| 4–6 | Class with a constructor for `n` elements. | One element per outpost. |
| 8 | `find(x)`: the representative (root) of x's group. | Two outposts are connected exactly when they have the same root. |
| 9 | `unite(a, b)`: join the two groups; returns `false` if they were already one group. | Kruskal uses the `false` answer as "this road would make a cycle". |
| 11 | `getSteps()` | Counts work, so the tests can report operation counts. |
| 14–16 | `parent`, `rank`, `steps`. | `parent[i]` points up the tree; `rank` is an upper bound of a tree's height. |

`disjoint_set.cpp`

| Lines | What | Why |
|---|---|---|
| 4–7 | Every element starts as its own parent (its own group), rank 0. | Before Kruskal starts, no outposts are connected. |
| 9–14 | `find`: count a step; if x is not a root, call `find` on its parent and set `parent[x]` straight to the root (**path compression**); return the root. | Path compression flattens the tree, so later `find` calls are almost O(1). |
| 16–19 | `unite`: find both roots; if equal, return `false` (same group). | This is the cycle test. |
| 21–22 | **Union by rank**: hang the lower-rank root under the higher-rank root. | Keeps trees shallow. With path compression, m operations cost O(m·α(n)), where α(n) ≤ 4 in practice. |
| 23 | If both ranks were equal, the new root's rank grows by 1. | Only then can the height grow. |
| 24 | Return `true`: two groups were joined. | |

### 4.4 `core/mst_planner.hpp` / `.cpp` – Kruskal MST + incremental repair + BFS

`mst_planner.hpp`

| Lines | What | Why |
|---|---|---|
| 5–12 | `MstResult`: chosen tree roads, total cost, number of components (trees in the forest), component label of every outpost, list of outposts not connected to the Castle, operation count. | Everything the game, the tests and the DP need about the current network. |
| 14–19 | `RoadDestroyedReport`: which road, was it in the tree, which road replaced it (`-1` = none), operation count. | The result of one incremental update. |
| 23 | Constructor takes the dataset. | The planner keeps its **own copy** of the roads, because raids change their `blocked` flags. |
| 25 | `build()` – Kruskal. | |
| 27 | `destroyRoad(road)` – incremental repair. | |
| 29–34 | Read-only getters: result, roads, sorted list, number of roads examined, "is this road in the tree?", "is this outpost connected to the Castle?". | Used by the game, the traces and the tests. |
| 37–45 | Private data: outposts, roads, Castle index, the sorted road list (kept after `build()`), examined count, `inTree` flag per road, `treeAdj` (for each outpost, the tree roads touching it), current result. | `sortedRoads` is stored so `destroyRoad` never has to sort again. `treeAdj` makes the BFS fast. |
| 47–52 | Private helpers (explained below). | |

`mst_planner.cpp`

| Lines | What | Why |
|---|---|---|
| 6–8 | Constructor copies outposts, roads and the Castle index. | |
| 10–20 | `comesBefore(a, b)` – the **sort order / tie-break**: (1) lower cost first; (2) if costs are equal, higher combined severity of the two ends first; (3) if still equal, lower road id first. | (1) is what makes it Kruskal. (2) Equal-cost roads give the same total cost, so this free choice is used to link the most threatened outposts first. (3) Makes the order strict, so the tree is always the same. That lets the incremental result be compared road by road with a rebuild. |
| 22–26 | `build()` starts: n outposts, m roads, empty result, operation counter. | |
| 28–32 | Put every road that is **not blocked** into `sortedRoads`. | Destroyed roads are removed **before sorting** (brief requirement); they can never be chosen. |
| 34–37 | `std::sort` with `comesBefore`, counting each comparison. | O(m log m) = O(m log n). This sort dominates Kruskal's running time. |
| 39–42 | Fresh disjoint set; no road in the tree; empty `treeAdj`; examined = 0. | |
| 43–50 | **Kruskal's loop**: take roads cheapest first; if `unite` joins two different groups the road is safe, so add it (`addToTree`); stop as soon as the tree has n − 1 roads. | Greedy choice justified by the **cut property**: the cheapest road joining two groups is always in some minimum spanning tree. Stopping at n − 1 saves time (12 of 20 roads examined in the example). If roads are missing, the loop runs to the end and gives a **minimum spanning forest**. |
| 51 | Add the disjoint-set steps to the operation count. | |
| 53 | `labelComponents`: BFS over the tree to label every tree (component). | Needed to know who is connected to the Castle. |
| 54–55 | Store the operation count and return the result. | |
| 58–61 | `destroyRoad` starts: report for this road; 1 operation. | |
| 63–64 | Mark the road blocked; remember whether it was a tree road. | |
| 66–69 | **Case 1 – not in the tree**: nothing changes; return after O(1). | The tree never used that road, so it is still optimal. |
| 71–72 | **Case 2 – tree road**: remember its component label, remove it from the tree. | Removing it splits that tree into two parts, A and B. |
| 74–92 | **BFS** from one end of the removed road over tree roads only; every outpost reached is on **side A** (`sideA[]` = true, also listed in `sideAList`). | Tells for every outpost which side of the cut it is on. O(n). |
| 94–105 | Scan the road list **sorted once in `build()`**: skip tree roads and blocked roads; a road qualifies if both its ends were in the same old tree and they are on **different sides** (`sideA[a] != sideA[b]`). The **first** one found is the cheapest crossing road: add it to the tree and stop. | **Cut property** again: the cheapest road crossing the cut belongs to the new minimum tree. No new sort is needed, so the update is O(n + m) instead of O(m log n) for a rebuild. |
| 106–113 | No crossing road: the tree stays split. Side A gets a new component label, and the list of outposts cut off from the Castle is recomputed. | This is the "disconnected network" case: outposts that lost their link are reported. |
| 115–116 | Store operations, return the report. | The game shows these operations next to a full rebuild's (e.g. 21 vs 202). |
| 119–121 | `isConnectedToCastle`: same component label as the Castle. | O(1) check used by the DP filter and the UI. |
| 123–125 | `otherEnd`: given a road and one end, return the other end. | Used by both BFS loops. |
| 127–133 | `addToTree`: set the flag, add the road to both ends' adjacency lists and to the result, add its cost. | Keeps all tree data in sync. |
| 135–144 | `removeFromTree`: the reverse; `std::remove` + `erase` deletes the road from the lists; subtract its cost. | |
| 146–149 | `labelComponents` starts: every outpost unlabelled (`-1`), 0 components. | |
| 151–170 | For every unlabelled outpost start a new label and **BFS** over tree roads, giving every reached outpost that label. | Standard BFS component labelling, O(n + tree roads). After this, two outposts are connected exactly when they have the same label. |
| 172 | Recompute the cut-off list. | |
| 175–182 | `updateUnreachable`: every outpost whose label differs from the Castle's is listed. | This is the list shown as "Cut off" and used to drop requests before the DP. |

### 4.5 `core/knapsack_allocator.hpp` / `.cpp` – Multiple 0/1 Knapsack DP + Brute force

`knapsack_allocator.hpp`

| Lines | What | Why |
|---|---|---|
| 6–15 | `PackingResult`: `ok` + `error`, best total value, `assignment[i]` (convoy 0–2 for item i, or −1 = left behind), load and value per convoy, number of DP cells (`states`), operations. | One result type for both the DP and the brute force, so the tests can compare them directly. |
| 17 | `packConvoysDP` | The real algorithm (Part B). |
| 19–20 | `BRUTE_FORCE_MAX_ITEMS = 12`, `packConvoysBruteForce` | Brute force tries 4^N assignments; 4^12 ≈ 16.8 million is the largest size that still runs in seconds. It is only used to **check** the DP. |

`knapsack_allocator.cpp`

| Lines | What | Why |
|---|---|---|
| 4 | `MAX_TABLE_CELLS = 50 000 000` | A safety limit: a bigger table would need gigabytes of memory. The DP refuses with a message instead of crashing (Test 9). |
| 6–17 | `sumConvoyTotals`: from the assignment, add up the weight and value loaded on each convoy. | Shared by DP and brute force for the printed summary. |
| 19–24 | Start of the DP: fail with a message unless there are exactly 3 convoys. | The table below has exactly 3 capacity dimensions. |
| 26–29 | N items; capacities C1, C2, C3. | |
| 31–35 | One "layer" of the table has (C1+1)(C2+1)(C3+1) cells (all remaining-capacity combinations); the whole table has N+1 layers. Refuse if it is too large. | Pseudo-polynomial size: it grows with the **capacities**, not only with N. This is the price of an exact answer for a strongly NP-hard problem. |
| 37–39 | `cell(i, c1, c2, c3)` turns the 4 indices into one position in a flat vector. | A flat `std::vector` is simpler and faster than a 4-level nested vector. |
| 41–42 | The table, filled with 0. Layer N is the **base case**: with no items left, the value is 0. | `dp[i][c1][c2][c3]` = the best value that can be packed from items i..N−1 when the convoys have c1, c2, c3 space left. |
| 44–46 | Fill the layers from the last item back to the first; w, v = weight and value of item i. | Layer i only needs layer i+1, which is already filled. |
| 47–49 | Every combination of remaining capacities. | |
| 50 | Option 1: **skip** item i → the value of the rest with the same capacities. | Always possible, so the DP never fails, even when demand exceeds capacity. |
| 51–53 | Options 2–4: put item i on convoy 1, 2 or 3 **if it fits** → v + the best value of the rest with that convoy's space reduced by w. | One item goes on **at most one** convoy, because it is decided once, in layer i. A single shared state for all three capacities lets the DP trade items between convoys, which is why it beats packing the convoys one after another. An item heavier than every convoy fails all three `if`s and is skipped automatically. |
| 54 | Store the best of the four options. | **Recurrence**: dp[i][c] = max(skip, v + dp[i+1][c with w taken from convoy j]). |
| 55 | Count 4 operations (one per option). | Gives N·4·P operations (266 112 in the example). |
| 61 | Answer: `dp[0][C1][C2][C3]` (all items, full capacities). | 269 in the worked example. |
| 63–64 | **Backtracking** starts: everything "left behind", full capacities. | The table gives the best value; this part finds **which** items gave it. |
| 65–67 | For item i: if the value is the same as without the item, the item was skipped. | Ties choose "skip"; the value is the same either way. |
| 69–78 | Otherwise find the convoy j whose option produced this value (`here == v + dp[i+1][... − w on j]`), record it, reduce that convoy's space and go on. | Walks one path through the table, O(N·3). |
| 81–83 | Add up per-convoy totals; success. | |
| 86–97 | `tryAll` (brute force): count the call; when all items are decided, check that no convoy is over capacity and keep the best value found. | Checks every assignment, so its answer is certainly optimal. Used to prove the DP right. |
| 99–100 | Try "item i left behind". | |
| 102–107 | Try item i on each convoy: subtract its weight, recurse, add the weight back (**backtracking**). | 4 choices per item → 4^N leaves. |
| 108 | Reset the choice for item i. | |
| 111–120 | Wrapper: exactly 3 convoys and at most 12 items, otherwise refuse. | |
| 122–129 | Start with full capacities, run `tryAll`, add up the totals. | |

### 4.6 `core/pipeline.hpp` / `.cpp` – Part C: Kruskal → filter cut-off requests → DP

`pipeline.hpp`

| Lines | What | Why |
|---|---|---|
| 9–16 | `PipelineReport`: the planner (with its tree), deliverable items, cut-off items, packing result. | The output of one full run of Part A + Part B. |
| 18 | `runPipeline(data)` | Builds the tree, then packs. |
| 20 | `allocateSupplies(data, report)` | Re-packs using the planner's **current** tree. The game calls it after raids. |
| 22–26 | Printing helpers. | Same text in the tests and in the game's console. |

`pipeline.cpp`

| Lines | What | Why |
|---|---|---|
| 5–10 | `runPipeline`: create the planner, run Kruskal, then the supply allocation. | The brief's order: network first, then supplies. |
| 12–20 | Split the requests: an item goes to `deliverable` if its outpost is connected to the Castle, otherwise to `cutOff`. | Supplies cannot reach a cut-off outpost, so its items must **not** be offered to the DP. The two parts are linked here. |
| 21 | Run the DP on the deliverable items only. | |
| 24–27 | `roadLabel`: e.g. "R4 Ironhold-Millbrook". | |
| 29–69 | `printMst`: counts; blocked roads; every sorted road with cost, severity and status (in tree / skipped / not examined); chosen roads; total cost; number of trees; unreachable outposts. | This is the Kruskal trace printed in `test_output.txt`. A road is "skipped" if it was examined (position < examined count) but not added. |
| 71–111 | `printPacking`: each item with its convoy or "left behind" (and "heavier than every convoy" when true); loads and values per convoy; total demand vs capacity; total value; DP states and operations. | Shows the DP result and why items were left out. |
| 113–124 | `printPipelineReport`: Part A, then the deliverable/cut-off counts, then Part B. | |

### 4.7 `core/supply_network.hpp` / `.cpp` – the game's link to the algorithms (raids, incremental repair, supply phase)

`supply_network.hpp`

| Lines | What | Why |
|---|---|---|
| 9–16 | `RoadDestroyedEvent`: destroyed road, replacement, was it a tree road, incremental ops, ops of a full rebuild, does the result match the rebuild? | Everything the game shows after a raid ("Repaired: 21 ops vs 202"). |
| 20–23 | Effect values (−1 penalty, 0 none, +1 buff) and the effect radius of 3 tiles. | Tower buff/penalty rule. |
| 25–26 | `load`, `isLoaded` | Without a data file the game still runs, just without supply roads. |
| 28 | `reset` | Restart: all roads repaired, new tree and packing. |
| 30–31 | `nextRoadToDestroy`, `raidTier` | Which road the enemies destroy next. |
| 33 | `destroyRoad` | Incremental update, plus a rebuild for comparison. |
| 35 | `runSupplyPhase` | Filter + DP between waves. |
| 37 | `supplyEffectAt(x, y)` | Effect for a tower on that tile. |
| 39–45 | Getters. | Read-only access for the UI. |
| 48–52 | The dataset, the current `PipelineReport` (`std::optional` = maybe not loaded), effect per outpost, last event, number of destroyed roads. | |

`supply_network.cpp`

| Lines | What | Why |
|---|---|---|
| 7–14 | `load`: read the file (on error, nothing is loaded and the message is returned), then `reset`. | |
| 16–22 | `reset`: build a fresh report from the original data (no raids), run Kruskal, clear the last event, run the first supply phase. | So the game starts with tree cost 45 and value 269, and Restart gives exactly that again. |
| 24–31 | `pointToSegment`: shortest distance from a point (a path tile) to a road drawn as a line segment. | Projects the point onto the segment, clamps it to the segment's ends, measures the distance. |
| 33–38 | `raidTier`: 0 = tree road not touching the Castle, 1 = tree road at the Castle, 2 = unused road, 3 = unused road at the Castle. | Raids hit tree roads first (otherwise nothing interesting happens), and **Castle roads are protected** until no other tree road is left. Without this, the Castle was cut off by wave 3. |
| 40–65 | `nextRoadToDestroy`: among roads that are not destroyed, pick the smallest (tier, distance to the enemy path, road id). | Enemies attack what is near them; the id makes the choice deterministic, so tests and demos repeat exactly. |
| 67–73 | `destroyRoad`: call the planner's **incremental** update and copy its results. | |
| 75–80 | Build a **fresh planner** on the same roads and run full Kruskal: store its operation count and check the cost is the same. | Proves on every raid that the incremental result is optimal (it matches a rebuild), and gives the "vs 202" number. |
| 82–84 | Remember the event, count the destroyed road. | |
| 87–88 | `runSupplyPhase`: `allocateSupplies` = filter cut-off requests + DP. | Part C pipeline, run between waves. |
| 90–93 | Every outpost not connected to the Castle gets the **penalty**. | Towers near cut-off outposts fire 25 % slower. |
| 95–98 | Every outpost that received at least one item gets the **buff**. | Towers near supplied outposts fire 25 % faster. |
| 101–113 | `supplyEffectAt`: find the nearest outpost (not the Castle) within 3 tiles of the tile and return its effect. | Only the nearest outpost counts, so a tower never gets two effects. |

### 4.8 `core/algorithm_trace.hpp` / `.cpp` – step records for the three views (Kruskal, Disjoint Set, BFS, DP backtracking)

`algorithm_trace.hpp`

| Lines | What | Why |
|---|---|---|
| 8–13 | `KruskalStep`: road examined, added or skipped, cost so far, group (set root) of every outpost after this step. | The Plan Network view colours outposts by group, so groups visibly merge. |
| 15–21 | `KruskalTrace`: sorted roads, blocked roads, starting groups, the steps, total cost. | |
| 23 | `traceKruskal` | |
| 25–32 | `ScanVerdict`: why a road in the raid scan was not chosen (just destroyed, already in tree, blocked, same side, other tree) or `Chosen`. | Shown next to every road in the Raid Repair view. |
| 34–45 | `RaidScanStep`, `RaidTrace`: destroyed road, was in tree, side of every outpost (0 = A, 1 = B, −1 = other tree), the scan, the replacement. | |
| 47 | `traceRaid` | |
| 49–60 | `LeftReason` and `LoadStep`: each item, its convoy, or why it was left behind (too heavy / no space / cut off). | For the Load Convoys view. |
| 62–64 | `traceConvoyLoading`, `leftReasonText` | |

`algorithm_trace.cpp`

| Lines | What | Why |
|---|---|---|
| 6–10 | Run the real `MstPlanner` on the current roads. | The trace uses the **same** sorted order (same tie-break) as the real algorithm. |
| 12–15 | Copy the sorted list; list the blocked roads. | |
| 17–19 | New `DisjointSet`; every outpost starts in its own group. | The same class Kruskal uses. |
| 21–35 | Replay Kruskal's loop: for each road call `unite`; record added/skipped, running cost and the root of every outpost; stop at n − 1 roads. | Exactly the loop of `MstPlanner::build`, but recording every step. O(m·α(n)) plus O(n) per step for the group list. |
| 36–37 | Total cost; return. | Test 15 checks that it equals the real result (45). |
| 40–49 | `traceRaid` on the planner **before** the update: if the road is not a tree road, there is nothing to show. | The game calls it just before `destroyRoad`, so the tree still contains the road. |
| 51–56 | Adjacency list of the tree **without** the destroyed road. | |
| 58–72 | Two BFS runs, from each end of the destroyed road: everything reached from end 0 is side A (0), from end 1 side B (1). Outposts in other trees stay −1. | The two halves of the split tree. |
| 74–90 | Walk the sorted list exactly like `destroyRoad`: give each road its verdict, stop at the first road whose ends are on different sides (`Chosen`). | Test 15 checks that it chooses the same replacement as `MstPlanner::destroyRoad` in all 20 raids. |
| 94–107 | `traceConvoyLoading`: for each deliverable item (in DP backtracking order) take its convoy from the DP's assignment; if left behind, the reason is "too heavy" when it is heavier than every convoy, otherwise "no space". | It **reads** the DP result; it does not run a second DP. |
| 108–113 | Add the cut-off requests with reason "cut off". | |
| 117–124 | Text for each reason. | |

### 4.9 `App/node.hpp` / `.cpp` – one grid tile (data for A*)

| Lines | What | Why |
|---|---|---|
| node.hpp 6–7 | Tile position and `walkable` (false = a blocking tower stands here). | |
| node.hpp 9–10 | `gCost` = cost from the start so far; `hCost` = estimated cost to the goal. | The two halves of A*'s f = g + h. |
| node.hpp 11–12 | `baseCost` (1) and `slowMultiplier` (> 1 on frost tiles). | Moving onto a frosted tile is "more expensive", so A* may walk around it. |
| node.hpp 13 | `parent` | Where we came from; used to rebuild the path. |
| node.hpp 14–15 | `opened` (has been put into the open set) and `closed` (finished). | An explicit flag replaced an old bug where `gCost == 0` was used to mean "not visited". |
| node.cpp 3–13 | Constructor: costs 0, multiplier 1, no parent, not opened or closed. | |
| node.cpp 15–17 | `getFCost` = g + h. | Priority in the open set. |
| node.cpp 19–21 | `getMovementCost` = base × slow multiplier. | Cost of stepping **onto** this tile. |
| node.cpp 23–25 | Two nodes are equal when their positions are equal. | |
| node.cpp 27–33 | `resetCosts`: clear search data before a new search. | Every A* run starts clean. |

### 4.10 `App/min_heap.hpp` / `.cpp` – Min-Heap (priority queue for A*)

| Lines | What | Why |
|---|---|---|
| min_heap.hpp 5–9 | `HeapEntry`: a **copy** of f and h taken when the node is pushed, plus the node pointer. | Fix of the old "wrong path" bug: the heap used to compare nodes by their **live** costs, and changing a cost while the node sat in the heap broke the heap order. A snapshot can never change. |
| min_heap.hpp 13 | The heap is stored in a vector: children of index i are 2i+1 and 2i+2. | Standard array heap, no pointers needed. |
| min_heap.cpp 4–7 | `comesFirst`: smaller f first; if f is equal, smaller h first. | Ties prefer the node closer to the goal, so A* expands fewer nodes. |
| min_heap.cpp 9–17 | `heapifyUp`: while the entry is better than its parent ((i−1)/2), swap them. | Restores the heap after a push. O(log n). |
| min_heap.cpp 19–34 | `heapifyDown`: find the better of the node and its two children; if a child is better, swap and continue there. | Restores the heap after a pop. O(log n). |
| min_heap.cpp 36–39 | `push`: add the snapshot at the end, then `heapifyUp`. | |
| min_heap.cpp 41–50 | `pop`: take the top (best) node, move the last entry to the top, shrink, `heapifyDown`. | Returns the node with the smallest f. |
| min_heap.cpp 52–58 | `empty`, `clear`. | |

### 4.11 `App/a_star_path_finder.hpp` / `.cpp` – A* (supporting algorithm; enemy movement)

| Lines | What | Why |
|---|---|---|
| .hpp 8–9 | Pointer to the grid; the open set is a `MinHeap`. | The min-heap gives the best node in O(log n). |
| .cpp 7–9 | `calculateHCost`: **Manhattan distance** \|dx\| + \|dy\|. | Enemies move in 4 directions (`Grid::getNeighbors`) and every step costs at least 1, so this estimate never overestimates (it is **admissible**). That guarantees A* finds the shortest path. |
| .cpp 12–14 | No start/end → empty path; start == end → path of one tile. | Edge cases. |
| .cpp 16–17 | Reset all tiles' search data; empty the open set. | |
| .cpp 19–22 | Start: g = 0, h = estimate, mark opened, push. | |
| .cpp 24–28 | Main loop: pop the node with the smallest f. If it is already closed, skip it; otherwise close it. | A node can be in the heap more than once (pushed again with a better cost). The old copy is a "stale duplicate" and is skipped; this was another bug fix. |
| .cpp 30–33 | Reached the goal → rebuild the path and return it. | When the goal is popped, its g is final and minimal. |
| .cpp 35–37 | For each of the 4 neighbours: skip walls (towers) and closed nodes. | |
| .cpp 39–40 | Cost through `current` = g(current) + cost of stepping onto the neighbour (frost makes it larger). | |
| .cpp 42–49 | If the neighbour was never opened, or this way is cheaper: record the parent, the new g and h, mark it opened, and push it (with a fresh snapshot). | **Relaxation**, like Dijkstra. Pushing again instead of "decrease-key" keeps the heap simple. |
| .cpp 53 | Open set empty → no path (empty vector). | `TowerManager` uses this to **refuse** a tower that would block the path completely. |
| .cpp 56–63 | `reconstructPath`: follow `parent` from the goal back to the start, then reverse. | |

Where A* is called: `EnemyManager` (path for new enemies, and rerouting enemies when a tower or frost changes the grid) and `TowerManager::placeTower` (the "would this tower block the path?" check). `tests/astar_check.cpp` compares A* with Dijkstra on many random layouts.

### 4.12 `tests/test_runner.cpp` – the 15 test groups (69 checks) for all algorithms

The program needs no SFML. Run it with `build\test_runner.exe data`.

**Helpers (lines 1–225)**

| Lines | What | Why |
|---|---|---|
| 1–17 | Standard headers and every `core/` header. | The tests call the real algorithm code, not copies of it. |
| 19–21 | Pass/fail counters; the data folder (default `data`, or the first command-line argument). | |
| 23–27 | `check(condition, text)`: prints `[PASS]`/`[FAIL]` with the text and counts it. | Every check is printed, so `test_output.txt` documents itself. |
| 29–35 | `title`, `subtitle`: headings in the output. | |
| 37–45 | `load(file)`: load a data file or stop with the error. | |
| 47–51 | `roadIndexById`: find a road's position from its id (e.g. R5). | Tests talk about road ids, as the report does. |
| 53–56 | `positionInSortedOrder`: where a road is in Kruskal's sorted list. | Used to test the tie-break. |
| 58–61 | `sortedCopy`: a sorted copy of a list. | Compares two trees as sets of roads (order doesn't matter). |
| 63–67 | `unreachableNames`: names of cut-off outposts. | |
| 69–85 | `packingIsValid`: every assignment is −1..2; no convoy over capacity; loads and total value add up to what the result claims. | Checks any DP result **independently**. |
| 87–97 | `totalWeight`, `totalCapacity`. | Over-demand checks. |
| 99–102 | `microsSince`: elapsed microseconds. | Timing tables. |
| 104–106 | `standardConvoys`: Wagon 20, Cart 15, Mule 10. | |
| 108–130 | `randomDataset(n, m, seed)`: Castle = 0; random severities 1–10; the first n−1 roads link each new outpost to an earlier one (so the graph is connected), the rest are random roads with costs 1–50. | Large graphs for the performance test. The fixed seed makes the output identical on every run. |
| 132–141 | `randomItems(count, seed)`: weights 1–10, values 5–50. | DP vs brute-force growth test. |
| 143–151 | `IncrementalRow`: one destroyed road with incremental and rebuild ops, times, costs, and "same tree?". | |
| 153–194 | `compareIncremental`: for each road to destroy, time `destroyRoad` on copies (`timingRuns` times), apply it for real, then rebuild a **fresh** planner on the same roads (timed too), and compare costs, tree roads and unreachable lists. | Proves the incremental update gives the same tree as Kruskal from scratch, and measures the speed difference. |
| 196–219 | `printIncrementalRows`: print each row and the totals with the rebuild/incremental ratio. | |
| 221–225 | `allSame`: every row matched the rebuild. | |

**Test groups**

| Lines | Test | What it checks | Why it matters |
|---|---|---|---|
| 227–242 | **1 Worked example** (Kruskal + DP pipeline) | ≥ 10 outposts and ≥ 20 roads; tree has n − 1 roads; no blocked road in the tree; demand > capacity; packing valid. Output: cost 45, value 269. | The brief's minimum sizes and Part C. |
| 244–271 | **2 Tie-break** (Kruskal) | Castle severity 0; R5 (severity 10) sorted before R9 (7); R5 in tree, R9 skipped; R10 before R13 (equal cost and severity → lower id). | Proves the sort rule works on real roads. |
| 273–294 | **3 Disconnected graph** (Kruskal forest + BFS) | 4 trees; unreachable = Duskmoor, Fenmarsh, Westgate, Eaglecrest; forest cost 14; only connected requests reach the DP. | Edge case from the brief. |
| 296–312 | **4 Over-demand** (DP vs brute force) | Demand > capacity; the DP still returns a value; some items left behind; valid; equals brute force. | Partial packing edge case. |
| 314–340 | **5 Items too heavy** (DP) | Siege_Engine and Ballista left behind; valid; equals brute force; equals the DP run with those items removed by hand. | Shows the recurrence handles them with no special case. |
| 342–384 | **6 DP vs brute force** | First 10 worked-example items: same value; then N = 4, 6, …, 12 random items: same value every time, with ops and µs for both. | Correctness and growth: the DP grows linearly in N, brute force ×4 per item. |
| 386–426 | **7 Incremental vs rebuild** (incremental repair) | 3 tree roads destroyed: same tree as a rebuild, nobody cut off; then a non-tree road (1 op), and R17 + R18 so that Westgate is cut off; then random graphs with n = 200 / m = 1000 and n = 1000 / m = 5000. | Correctness plus speed (126× fewer ops on the large graph). |
| 428–485 | **8 Sensitivity** (Kruskal + DP) | Remove the 2 most severe outposts (`withoutOutposts`), run again, print a before/after table and which tree roads changed. | Discussion in the report: cost 45 → 36, value 269 → 234. |
| 487–500 | **9 Table limit** (DP) | 400/400/400 capacities → refused with "too large", no crash. | Shows the pseudo-polynomial limit. |
| 502–522 | **10 Input validation** | Unknown outpost on a road, Castle severity > 0, only two convoys → all rejected with a line number. | Bad data never reaches the algorithms. |
| 524–538 | Helpers | `loadNetwork` (a `SupplyNetwork` on the worked example); `straightPath` (enemy path along row 7). | |
| 540–554 | Helpers | `distanceToPath` (same geometry as the game, re-written independently); `effectName`. | The test computes the rule itself rather than trusting the code it tests. |
| 556–606 | **11 Raid rule** (incremental repair) | Raids until every road is gone: each one is the lowest (tier, distance, id) of all remaining roads; Castle roads only after the others; every update matches a rebuild; 20 raids. | The order of raids in the game is fair and every repair is optimal. |
| 608–638 | **12 Buff / penalty rule** | Six tiles get the expected effect (inclusive radius, Castle excluded, no outpost nearby); after Fenmarsh is cut off, the buff stays until the next supply phase and then becomes a penalty. | Game rule linked to Kruskal and the DP. |
| 640–669 | **13 Supply-phase filter** (Kruskal → DP) | With Fenmarsh and Westgate cut off, their 4 requests are excluded, every request in the DP is connected, and the value equals the DP on a hand-filtered list. Both outposts give a penalty. | The link between Part A and Part B. |
| 671–700 | **14 Restart** | After 8 raids and `reset()`: cost 45 again, only the 2 file-blocked roads blocked, counters cleared, value 269, same effects as a fresh load. | Restart works. |
| 702–748 | **15 Traces** (views) | The Kruskal trace adds the same roads, cost, number of steps and groups as `build()` (two data files); the raid trace picks the same replacement as `destroyRoad()` in all 20 raids; the convoy trace delivers 269. | The algorithm views show the real algorithms. |
| 750–772 | `main` | Optional data folder argument; run all 15 groups; print "69 checks passed, 0 failed"; exit code 1 if anything failed. | CMake's `test_output` target saves this output. |

---

## 5. Function by function: game and UI files

These files use SFML for windows, drawing, input and sound. They **call** `core/` but contain no Kruskal or DP code. The numbers are the line where each function starts in the clean file.

### `App/main.cpp`
| Line | Block | What it does |
|---|---|---|
| 4 | `main` | Creates the `GameManager` and runs it; any exception is printed and the program exits with code 1. |

### `App/game_manager.hpp` / `.cpp` – the game loop and the link between the game and `core/`
| Line | Function | What it does and why |
|---|---|---|
| 8 | constructor | Opens the 1280×768 window "Tower Defence: Supply Lines" at 60 FPS; the battlefield is a 960×720 view under the 48-px top bar. Loads assets and fonts, creates the HUD, connects towers to effects and sound, **loads `../data/worked_example.txt` into `SupplyNetwork`** (runs Kruskal + DP once: cost 45, value 269), sets grid textures, start (0,7) and goal (19,7), and asks A* for the first path. |
| 52 | `run` | The main loop: input → UI animations → simulation → draw. The simulation uses a **fixed 1/60 s step**, scaled by game speed (1×/2×) and by 0.35 while a raid flash plays. It stops completely while a view, help or pause is open. Prints the FPS every 5 s. |
| 82 | `handleInput` | Reads window events. Ignores a click that only focused the window (old bug: it placed a tower). Then handles each screen: menu (Play / How to Play / Algorithm View / Quit), how-to-play, pause, game over (R = restart) and playing. While a view is open, `D` still raids and opens the Raid view. |
| 164 | `handleGameEvent` | Keys 1–4, Space, Esc, M, D (an enemy bombs the next road; it is raided when the bomb lands 1 s later), P, L, V, H, F, N. Mouse: Algorithms menu, view items, tower cards, speed, mute, wave button; otherwise place a tower on the clicked tile. Esc first closes the Algorithms menu. |
| 217 | `update` | One game step: towers shoot, enemies move, effects; kills give money; **raids**: one every 4 s of game time, 3 per wave (`raidRoad`); 1 s before each raid a bomb is thrown (`throwBomb`) and it lands at the raid (`bombLands`) – only a picture, the rule and timing do not depend on it; when no enemies are left the wave ends (`onWaveEnded`); lives and game over; countdown to the next wave. |
| 258 | `updateUi` | Real-time animations: banner age, raid animation, toast age, views, sliding panels; the D bomb's flight (then `raidRoad`). |
| 285 | `render` | Draws the current screen: menu, how-to-play, an algorithm view, or the battlefield + top bar + sidebar + wave button + banner + toast + tooltip + Algorithms menu + help; then pause or game-over on top. |
| 322 | `renderBattlefield` | Grid, supply map (under towers), towers, enemies, effects, tower dots, bombs in the air; then the range circle and the green/red placement preview for the hovered tile. |
| 366 | `tooltipAt` | Hover text for an enemy (health, shield, speed, reward), a tower (range, rate, exact supply effect), an outpost (name, severity, population, connected?, requests/delivered, effect) or a road (id, cost, severity, status, next target). Over the sidebar it calls `sidebarTooltip`. |
| 435 | `hudState` | Collects money, lives, wave, speed, mute, selected tower, menu state and the wave button text ("Start wave 2: …" or "Wave 2 in 5 s"). |
| 455 | `restartGame` | Resets towers, enemies, grid, path, money ($1000), lives (20), counters, effects, and **`supplyNetwork.reset()`** (cost 45, value 269 again). |
| 492 | `wavePlan` | Wave w: 5 + 3w enemies, about 70 % of one type (soldiers, runners, tanks, shielded in turn), every 5th wave mixed; spawn gap shrinks with w. |
| 504 | `startNextWave` | Only when the field is clear: wave + 1, spawn the plan, +$100, reset raid timer, banner, sound. |
| 522 | `openView` | Opens an algorithm view; the traces are built from the current network at that moment. |
| 526 | `enemyPathTiles` | The tiles of the A* path new enemies follow. |
| 533 | `nextRaidTarget` | Asks `SupplyNetwork::nextRoadToDestroy` with that path. |
| 538 | `raidRoad` | **One raid**: record the raid trace (tree before the update), call `supplyNetwork.destroyRoad` (incremental repair + rebuild comparison), start the short map flash, store the trace for view V, play a sound, show a toast ("Repaired: 21 ops vs 202" / no replacement / unused road), and print the raid in the console. |
| 569 | `roadMiddle` | Pixel middle of a road (where the bomb lands). |
| 575 | `throwBomb` | Picks the enemy closest to the road (`Ui::distanceToSegment`), or the spawn point if no enemy is out; muzzle flash and sound. |
| 599 | `bombLands` | Small explosion and sound on the road's middle. |
| 605 | `onWaveEnded` | **Supply phase**: start the 6 s countdown, `runSupplyPhase()` (filter + DP), update tower effects, print the packing, toast "Supplies delivered: 269 – press L". |
| 623 | `showToast` | Stores the text, colour and duration of the toast. |
| 630 | `sidebarTooltip` | Tower card tooltip (cost, key, description, range) or the Supply box tooltip (tree size/cost, destroyed roads, cut-off names, last raid and its ops, next target, convoy loads, value, items left behind). |
| 684 | `applySupplyEffects` | Each shooting tower gets the effect of the nearest outpost within 3 tiles (`supplyEffectAt`). |
| 695 | `checkLivesLost` | Enemies at the Castle cost lives. |
| 703 | `checkWinLoss` | 0 lives → game over. |
| 710 | `togglePause` | Playing ↔ paused. |
| 715 | `tryPlaceTower` | Enough money? → `TowerManager::placeTower` (A* checks the path is not blocked) → pay, apply supply effect. |
| 727–751 | Small helpers | Window ↔ battlefield coordinates, "is the mouse over the grid?", tile from position, state change, affordability, tower cost. |

### `App/supply_overlay.hpp` / `.cpp` – the simple supply map on the battlefield (shows Kruskal / incremental-repair results)
| Line | Function | What it does |
|---|---|---|
| 7 | `outpostCentre` | Pixel centre of an outpost's tile. |
| 11 | `drawOutpostMarker` | The **detailed** marker with severity number and name, used only inside the algorithm views. |
| 39–55 | `reset`, `startRaidAnimation`, `update` | Start and time the 2-second raid animation. |
| 57 | `drawCastleIcon` | The small gold keep. |
| 65 | `drawMap` | (1) Destroyed roads as faint red dashes; the road destroyed just now flashes red for 0.8 s. (2) **Tree roads only**, in one blue colour; the replacement shows green, then fades to blue. (3) Outposts as small dots: blue = connected, grey = cut off; the Castle keep. Hovered items get a white highlight. |
| 114 | `drawTowerBadges` | Green dot = buff, red dot = penalty, at the tower's corner. |
| 123 | `roadAt` | Which drawn road (tree or destroyed) is under the mouse (within 7 px). |
| 142 | `outpostAt` | Which outpost is under the mouse. |
| 152 | `drawBomb` | The bomb on its arc (straight line lifted by a parabola), a dotted trail, its shadow and a pulsing red ring on the target road. |

### `App/algorithm_views.hpp` / `.cpp` – the three step-by-step views (Kruskal, incremental repair, DP)
| Line | Function | What it does |
|---|---|---|
| 18 | `open` | Builds the **Kruskal trace** (`traceKruskal`) for the current roads and the **convoy trace** (`traceConvoyLoading`) from the last supply phase, then starts playback. |
| 34 | `recordRaid` | Stores the latest raid's trace and event (called from `raidRoad`). |
| 41–72 | `restartPlayback`, `stepCount`, `stepDuration`, `update` | Playback: step k of n, speed from the slider, automatic stepping while playing. Each frame only moves k; nothing is recomputed. |
| 74–90 | layout helpers | Rectangles of tabs, Close, controls, slider, item rows. |
| 91 | `handleEvent` | Space play/pause, ←/→ step, 1–3 tabs, Esc close, clicks on tabs/controls/slider. |
| 128 | `drawFrame` | Background, the three tabs, Close button. |
| 166 | `drawControls` | Step counter, progress bar, Play/Pause, back, forward, Restart, speed slider. |
| 182 | `wrappedText` | Text that wraps at a maximum width. |
| 208 | `draw` | Calls the drawing of the active tab. |
| 240 | `drawKruskal` | **Plan Network**: all usable roads; examined road highlighted; added = green, skipped (cycle) = grey dashed, destroyed = red; outposts coloured by disjoint-set group (groups merge); sorted list with cost, severity and result; question box "are the two ends in different groups?"; counters tree cost / roads / groups. |
| 369 | `scanVerdictText` | Text for each scan verdict. |
| 381 | `drawRaid` | **Raid Repair**: destroyed road, side A (blue) and side B (pink), the scan down the sorted list with a verdict per road, the chosen road in green, and the operation counts of incremental vs rebuild. |
| 525 | `drawItemIcon` | Small drawn icon per item type (ammo, medkit, …). |
| 557 | `buildLoadTargets` | Where each item lands on its truck. |
| 575 | `drawConvoys` | **Load Convoys**: request list in backtracking order; items move onto the Wagon/Cart/Mule; load bars; value so far vs the optimum dp[0][C1][C2][C3]; "left behind" list with reasons. |

### `App/hud.hpp` / `.cpp` – top bar, sidebar, toasts, tooltips, help (UI only)
| Line | Function | What it does |
|---|---|---|
| 10 | `towerInfo` | Name, cost, range, description, hotkey of each tower. |
| 22–53 | `buttonRect`, `cardRect`, `supplyBoxRect`, `buttonAt` | Where everything is, and what is under the mouse (the open Algorithms menu is checked first). |
| 55–105 | `drawTowerIcon`, `coinIcon`, `heartIcon`, `speakerIcon` | Tower pictures from textures; coin, heart and speaker drawn with shapes. |
| 107 | `drawTopBar` | Money, lives, wave; Algorithms button; small 1x/2x and mute buttons. |
| 136 | `drawAlgorithmMenu` | The small menu: Plan Network P, Raid Repair V, Load Convoys L. |
| 151 | `drawSidebar` | Four tower cards (icon, name, cost, hotkey); the **Supply box** with network cost, outposts connected (x/11) and last convoy value; the key reminder. |
| 203 | `drawWaveButton` | Start / countdown button. |
| 208 | `drawWaveBanner` | "WAVE n – type – count" for about 2 s. |
| 224 | `drawToast` | Rounded message at the bottom of the map that fades in and out. |
| 235 | `drawTooltip` | Box with a title line and detail lines, kept on screen. |
| 250–310 | `helpCloseRect`, `drawHelp` | Help panel: all keys and the map legend (including the bomb line). |

### `App/menus.hpp` / `.cpp` – menu, how to play, pause, game over (UI only)
| Line | Function | What it does |
|---|---|---|
| 12–28 | button rectangles | Positions of the menu, pause, back and game-over buttons. |
| 30 | `drawRidge` | A mountain range: a strip whose top is a sum of two sine waves. |
| 41 | `rect` | Small helper for a filled rectangle. |
| 48 | `drawCastle` | Castle silhouette: towers, battlements, lit windows, gate, red flag. |
| 76 | `drawBackground` | The whole menu picture in code: sky gradient, fixed stars, sun, two ridges, Castle, gold supply roads; then a dark shade. |
| 115 | `drawMain` | Title "TOWER DEFENCE: SUPPLY LINES", subtitle "Kruskal MST + Multiple Knapsack DP", tagline, four buttons (Play glows). |
| 155–250 | `clickMain`, `drawHowToPlay`, `clickHowToPlay`, `drawPause`, `clickPause`, `drawGameOver`, `clickGameOver` | The other screens and what their buttons return. How to Play explains the game, Part A and Part B in three boxes. |

### Battlefield objects (game only; A* is used through them)
| File | Main blocks | What they do |
|---|---|---|
| `grid.hpp/.cpp` | `initialize` (10), `getNode` (40), **`getNeighbors` (46)**, `setObstacle` (64), `setStartEnd` (69), `applyFrostEffect` (74), `resetCosts` (89), `draw` (97), `drawIce` (165) | The 20×15 tile grid A* searches. `getNeighbors` returns the 4 neighbours (up, down, left, right). `applyFrostEffect` raises the movement cost of tiles around a Frost tower once, and returns the changed tiles so only affected enemies are rerouted. |
| `enemy_manager.hpp/.cpp` | `update` (22), `spawnWave` (43), `takeDeaths` (52), `spawnEnemy` (59), `clearDeadEnemies` (160), **`recalculatePaths` (178)**, **`rerouteAffected` (216)**, `reset` (264) | Spawns enemies on **game time** (featured type 70 %), keeps the cached A* path, records kills (for money), and reroutes only enemies whose remaining path crosses a changed tile. |
| `Enemy.hpp/.cpp` | constructor (6), direction/animation (22–52), `update` (53), textures (87–133), `getCurrentSpeed` (134), draw + health bars (138–173), `takeDamage` (174), `setPath` (179); `Normal/Fast/Tank/ShieldEnemy` (220–273) | Moves along the path tile by tile (slower on frost), animated sprites per direction, health and shield bars. A new path continues from the closest tile. |
| `tower_manager.hpp/.cpp` | `update` (22), `draw` (29), `isOccupied` (36), **`placeTower` (49)**, `getTowerAt` (127), `reset` (136) | `placeTower` puts a blocking tower down, runs **A\*** from start to goal, and takes it back if no path is left. Frost towers change tile costs instead. Then it reroutes affected enemies. |
| `tower.hpp/.cpp` | constructor (4), `canAttack` (15), textures (24–49), `rotateToward` (50) | Base class: position, range, fire rate (× supply multiplier), textures, turret rotation. |
| `gatling_tower`, `artillery_tower`, `frost_tower`, `barrier_tower` | `update` / `draw` | Gatling: nearest enemy in range, fast bullets. Artillery: slow shells with splash damage. Frost: no shooting; slows a 5×5 area. Barrier: only blocks. |
| `projectile.hpp/.cpp`, `projectile_manager.hpp/.cpp` | `spawnProjectile` (12), `update` (38), `draw` (83), `applyAoEDamage` (113) | Bullets and shells fly, hit enemies, and artillery shells damage everything within their radius, with an explosion effect and sound. |
| `effects.hpp/.cpp` | `muzzleFlash` (16), `explosion` (20), `deathPuff` (31), `update` (35), `draw` (52) | A few short effects: a small flash, explosions, a soft ring when an enemy dies. Particles are drawn as one vertex array. |
| `sound_manager.hpp/.cpp` | constructor (8), `make` (21), `play` (41) | Sounds are **generated in code** (a tone that slides in pitch plus some noise, fading out), so no sound files are needed. |
| `asset_manager.hpp/.cpp` | `loadTexture`, `getTexture`, `loadFont`, `loadAllAssets` (54) | Loads all pictures from `../Assets/` and the Segoe UI font from Windows (pixel font as fallback). |
| `ui_draw.hpp/.cpp` | `roundedRect` (19), `panel`, `text*`, `bar`, `button`, `slider`, **`line`** (106), `circle`, `distanceToSegment` (135) | Shared drawing helpers. `line` draws thick or dashed lines (roads); `distanceToSegment` is used for road hover. |
| `theme.hpp` | constants | Window layout (1280×768, 48 px top bar, 960×720 battlefield, 48 px tiles) and colours: main blue, gold accent, red, green, plus the extra colours of the algorithm views. |
| `tests/astar_check.cpp` | whole file | Optional check: A* path length equals Dijkstra's on many random tower layouts (needs SFML because it uses `Grid`). |

