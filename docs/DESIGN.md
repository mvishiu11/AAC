# Multigraph Subgraph and Minimal Extension Problem — Design Document

## 1. Problem Statement

We are given two (multi)graphs:

\[
G = (V, E), \quad H = (U, F)
\]

where:
- \( V, U \) — finite sets of vertices.
- \( E, F \) — sets of edges between vertices (with multiplicity for multigraphs).
- \( m_G(u, v) \) and \( m_H(x, y) \) denote edge multiplicities in \( G \) and \( H \), respectively.

We assume that \( |V| \le |U| \).

The task consists of several related subtasks:

1. **Define the size of a graph.**
2. **Define a distance metric between two graphs.**
3. **Check if \( G \) is a subgraph of \( H \).**
4. **If not, find the minimal extension \( H' \supseteq H \) such that \( G \subseteq H' \).**
5. **For multigraphs:** enumerate or count all possible subgraph mappings.
6. **If exact solutions are exponential, propose heuristics or approximations.**

---

## 2. Mathematical Foundations

### 2.1 Graph and Multigraph Definitions

A **graph** \( G = (V, E) \) is defined by:
- A finite vertex set \( V = \{v_1, v_2, \dots, v_n\} \),
- An edge set \( E \subseteq V \times V \).

A **multigraph** extends this by allowing multiple edges between vertex pairs, captured by a **multiplicity function**:

\[
m_G : V \times V \rightarrow \mathbb{Z}_{\ge0}
\]

For directed graphs, \( m_G(u, v) \) and \( m_G(v, u) \) are independent.

---

### 2.2 Size of a Graph

We define the **size** of a (multi)graph as:

\[
|G| = |V(G)| + |E(G)|
\]
where
\[
|E(G)| = \sum_{u, v \in V} m_G(u, v)
\]

Rationale:
- The number of vertices measures structural scope.
- The number of edges (with multiplicity) measures connectivity.
- The sum reflects the total “complexity” or information content of the graph.

---

### 2.3 Distance Between Graphs

We define a **graph distance** \( d(G, H) \) as a form of **Graph Edit Distance (GED)**:

\[
d(G, H) = \min_{\phi} \sum_{u, v \in V(G)} |m_G(u, v) - m_H(\phi(u), \phi(v))|
\]

where \( \phi : V(G) \to V(H) \) is an injective mapping between vertices of \( G \) and \( H \).

Interpretation:
- The distance counts the total number of edge additions/removals (including multiplicity adjustments) needed to make \( G \) match \( H \).
- If vertex additions or deletions are allowed, these can be incorporated as additional unit-cost operations.

---

### 2.4 Subgraph Relation

We say \( G \) is a **subgraph** of \( H \) if there exists an injective mapping \( \phi : V(G) \to V(H) \) such that:

\[
m_G(u, v) \le m_H(\phi(u), \phi(v)) \quad \forall u, v \in V(G)
\]

If such a mapping exists, \( G \subseteq H \).

If not, we can quantify the **deficit** of a mapping:

\[
\text{deficit}(\phi) = \sum_{u, v \in V(G)} \max(0, m_G(u, v) - m_H(\phi(u), \phi(v)))
\]

The minimal deficit gives the smallest number of edge insertions required for \( G \) to fit inside \( H \).

---

### 2.5 Minimal Extension

The **minimal extension** \( H' \) of \( H \) is defined as the smallest multigraph such that:

\[
H' = (U, F \cup F_{\text{added}}), \quad \text{and} \quad G \subseteq H'
\]
with
\[
|F_{\text{added}}| = \min_{\phi} \text{deficit}(\phi)
\]

That is, \( H' \) is obtained by adding the minimal number of missing edges required under the optimal vertex mapping.

---

## 3. Algorithmic Framework

### 3.1 Overview

The problem will be approached in stages:

| Stage | Objective | Description |
|--------|------------|-------------|
| 1 | Graph representation | Implement data structures for multigraphs with multiplicities. |
| 2 | Graph size and distance | Implement size and graph edit distance computation. |
| 3 | Subgraph detection | Implement an extended **Ullmann Subgraph Isomorphism** algorithm. |
| 4 | Minimal extension | Modify Ullmann’s recursion to track and minimize edge deficits. |
| 5 | Multigraph enumeration | Extend recursion to count all valid subgraph embeddings. |
| 6 | Approximation | Design heuristic/greedy algorithms for large graphs. |

---

## 4. Ullmann’s Algorithm and Its Adaptation

### 4.1 Classic Ullmann Algorithm

Ullmann’s algorithm is a backtracking-based method for subgraph isomorphism.

It operates on a **candidate matrix** \( M \), where:
\[
M[i][j] = 
\begin{cases}
1, & \text{if } v_i \text{ (in } G) \text{ can map to } u_j \text{ (in } H) \\
0, & \text{otherwise}
\end{cases}
\]

The algorithm proceeds recursively:
1. Select an unmapped vertex \( v_i \) in \( G \).
2. Try mapping it to a feasible \( u_j \) in \( H \).
3. Prune mappings where adjacency constraints fail.
4. Continue until a complete mapping is found or all options are exhausted.

This guarantees detection of a subgraph if one exists.

---

### 4.2 Extension for Multigraphs

For multigraphs, we modify adjacency constraints:

\[
\forall v_p, v_q \in V(G): \quad 
m_G(v_p, v_q) \le m_H(\phi(v_p), \phi(v_q))
\]

Feasibility checks now consider **edge multiplicities** instead of binary adjacency.

---

### 4.3 Extension for Minimal Extensions

We extend Ullmann’s recursion to **track the deficit** during mapping:

\[
\text{deficit}(\phi_{\text{partial}}) =
\sum_{(v_i, v_k)\ \text{mapped}} 
\max(0, m_G(v_i, v_k) - m_H(\phi(v_i), \phi(v_k)))
\]

We keep a global best (minimum deficit found so far) and prune branches where the current deficit already exceeds it.

This transforms Ullmann’s algorithm into a **branch-and-bound optimizer**:
- If a mapping with deficit = 0 is found → \( G \subseteq H \).
- Otherwise → minimal deficit mapping gives the required extension \( H' \).

---

### 4.4 Counting Subgraphs

To count all valid subgraph mappings:
- Continue exploring even after one valid mapping is found.
- Increment a counter for every injective mapping satisfying multiplicity constraints.

This count gives the number of distinct embeddings of \( G \) in \( H \).

---

## 5. Complexity and Approximation

### 5.1 Computational Complexity

- Subgraph isomorphism is **NP-complete**.
- Counting all subgraphs is **#P-complete**.
- The minimal extension problem (optimization form) is also **NP-hard**.

Thus, exact algorithms are feasible only for small graphs (typically \( n \le 15–20 \)).

### 5.2 Heuristics and Approximations

For larger graphs, we propose:
- **Greedy Mapping:** Match vertices by degree/multiplicity similarity.
- **Local Search:** Iteratively swap vertex mappings to reduce deficit.
- **Assignment Relaxation:** Solve a linear assignment minimizing local edge mismatch.
- **Randomized Algorithms:** Monte Carlo sampling of mappings for approximate counts.

---

## 6. Implementation Plan

### 6.1 Data Structures
- Use adjacency matrices or dictionaries with multiplicity values.
- Support both directed and undirected multigraphs.
- Provide convenience functions:
  - `size(G)`
  - `distance(G, H)`
  - `is_subgraph(G, H)`
  - `minimal_extension(G, H)`
  - `count_subgraphs(G, H)`

### 6.2 Algorithmic Modules
| Module | Purpose |
|---------|----------|
| `graph_utils.py` | Graph representation and basic operations. |
| `ullmann.py` | Implementation of Ullmann’s algorithm (simple graphs). |
| `ullmann_multigraph.py` | Extended algorithm for multigraphs with multiplicity checks. |
| `extension_solver.py` | Branch-and-bound deficit minimization for minimal extensions. |
| `heuristics.py` | Greedy and probabilistic approximations. |

---

## 7.Pseudocode

### 1) Define the size of a graph

```text
FUNCTION graph_size(G):
  # G is (V, E) with multiplicity m_G(u,v) >= 0  (0 if no edge)

  n_vertices ← |V(G)|
  n_edges_with_multiplicity ← 0
  FOR each unordered pair {u,v} in V(G):
      n_edges_with_multiplicity += m_G(u,v)
  FOR each vertex u in V(G):               # include self-loops once
      n_edges_with_multiplicity += max(0, m_G(u,u))

  RETURN n_vertices + n_edges_with_multiplicity
```

Notes:

* For simple graphs, multiplicity is 0/1 and this reduces to `|V| + |E|`.
* You can choose a different weighting (e.g., `α|V| + β|E|`) if desired.

---

### 2) Define a distance metric between two graphs

```text
# Asymmetric, injective edge-edit distance (only insertions in H)
FUNCTION distance_injective_insertions(G, H):
  IF |V(G)| == 0: RETURN 0
  IF |V(G)| > |V(H)|: RETURN +∞   # unless vertex insertions are allowed

  # Over all injective mappings φ : V(G) -> V(H),
  # take minimal number of missing multiplicities
  best ← +∞
  FOR each injective mapping φ:
      deficit ← 0
      FOR each unordered pair {u,v} in V(G) including self-loops:
          need ← m_G(u,v)
          have ← m_H(φ(u), φ(v))
          deficit += max(0, need - have)
      best ← min(best, deficit)

  RETURN best
```

Notes:

* This equals the **minimal extension cost** of making H contain G by adding edges.
* If you allow deletions/labels, extend the cost terms accordingly (GED).

---

### 3) Check if G is a subgraph of H

```text
FUNCTION is_subgraph(G, H):
  IF |V(G)| == 0: RETURN True
  IF |V(G)| > |V(H)|: RETURN False

  # Build candidate matrix M[i][j] = "v_i in G can map to u_j in H"
  M ← initial_candidates_by_degree_and_labels(G, H)

  # Refine candidates by local feasibility (Ullmann-style)
  IF NOT refine_candidates_multiplicity(G, H, M):
      RETURN False

  # Backtrack over injective mappings
  order ← vertices_of_G_sorted_by_desc_degree()
  used_H ← empty_set
  mapping ← empty_map

  FUNCTION dfs(t):
      IF t == |V(G)|:
          RETURN True    # found a full embedding
      i ← order[t]
      FOR each j in V(H) WHERE M[i][j] AND j ∉ used_H:
          IF consistent_multiplicities_with_partial(mapping ∪ {i→j}):
              used_H.add(j); mapping[i] ← j
              IF dfs(t+1): RETURN True
              used_H.remove(j); remove mapping[i]
      RETURN False

  RETURN dfs(0)
```

---

### 4) If not, find the minimal extension H′ ⊇ H such that G ⊆ H′

```text
FUNCTION minimal_extension(G, H):
  # Branch-and-bound over injective mappings to minimize "deficit" (missing edges)
  best_cost ← +∞
  best_map ← None

  order ← vertices_of_G_sorted_by_desc_degree()
  used_H ← empty_set
  mapping ← empty_map

  FUNCTION incremental_deficit(i→j, mapping):
      add ← 0
      # count self-loop at i
      add += max(0, m_G(i,i) - m_H(j,j))
      # count edges to already-mapped vertices
      FOR each a in mapping.keys():
          ja ← mapping[a]
          add += max(0, m_G(i,a) - m_H(j,ja))
      RETURN add

  FUNCTION full_deficit(mapping):
      total ← 0
      FOR each unordered pair {a,b} in V(G) (include a=b for self-loops):
          ja ← mapping[a]; jb ← mapping[b]
          total += max(0, m_G(a,b) - m_H(ja,jb))
      RETURN total

  FUNCTION dfs(t, cost_so_far):
      IF cost_so_far ≥ best_cost: RETURN
      IF t == |V(G)|:
          total ← full_deficit(mapping)
          IF total < best_cost:
              best_cost ← total; best_map ← mapping.copy()
          RETURN

      i ← order[t]
      FOR each j in V(H) WHERE j ∉ used_H:
          add ← incremental_deficit(i→j, mapping)
          new_cost ← cost_so_far + add
          IF new_cost ≥ best_cost: CONTINUE
          mapping[i] ← j; used_H.add(j)
          dfs(t+1, new_cost)
          used_H.remove(j); remove mapping[i]

  dfs(0, 0)

  # Construct H′ change list from best_map
  missing_edges ← aggregate_all_pairs_missing_from(best_map)
  RETURN (best_cost, best_map, missing_edges)
```

Notes:

* `missing_edges` lists `(u_idx, v_idx, k)` telling how many parallel edges to add between those `H` vertices.
* If `best_cost == 0` then G is already a subgraph of H.

---

### 5) For multigraphs: enumerate or count all possible subgraph mappings

```text
FUNCTION enumerate_subgraphs(G, H):
  IF |V(G)| == 0 OR |V(G)| > |V(H)|: RETURN

  M ← initial_candidates_by_degree_and_labels(G, H)
  IF NOT refine_candidates_multiplicity(G, H, M): RETURN

  order ← vertices_of_G_sorted_by_desc_degree()
  used_H ← empty_set
  mapping ← empty_map

  FUNCTION dfs(t):
      IF t == |V(G)|:
          YIELD mapping.copy()
          RETURN
      i ← order[t]
      FOR each j in V(H) WHERE M[i][j] AND j ∉ used_H:
          IF consistent_multiplicities_with_partial(mapping ∪ {i→j}):
              mapping[i] ← j; used_H.add(j)
              dfs(t+1)
              used_H.remove(j); remove mapping[i]

  dfs(0)

FUNCTION count_subgraphs(G, H):
  count ← 0
  FOR each mapping IN enumerate_subgraphs(G, H):
      count += 1
  RETURN count
```

Notes:

* For unlabeled counting where automorphisms matter, postprocess to factor out symmetries if needed.
* This is **#P-complete**; feasible only for small graphs.

---

### 6) If exact is exponential, propose heuristics / approximations

```text
FUNCTION heuristic_min_extension(G, H, time_budget):
  # 1) Build scores for mapping v_i → u_j (degree, loop presence, neighbor histograms)
  S ← score_matrix(G, H)

  # 2) Get an injective seed mapping via linear assignment (Hungarian) on -S
  φ ← assignment_minimize(-S)

  # 3) Local search: iteratively improve φ by swapping targets to reduce deficit
  best ← φ; best_cost ← full_deficit(φ)
  REPEAT until no improvement or time_budget exceeded:
      improved ← False
      FOR each pair (i, k) in V(G):
          φ' ← φ with u targets for i and k swapped
          c' ← full_deficit(φ')
          IF c' < best_cost:
              best ← φ'; best_cost ← c'; improved ← True
              BREAK
      φ ← best
  RETURN (best_cost, best)

FUNCTION sampling_count(G, H, samples):
  # Monte Carlo estimate of count: sample random injective φ and test feasibility
  success ← 0
  FOR s in 1..samples:
      φ ← random_injective_mapping(V(G) → V(H))
      IF preserves_multiplicities(G, H, φ):
          success += 1
  estimate ← success * P(|V(H)|, |V(G)|) / samples
  RETURN estimate
```

Other options:

* **Seed-and-extend (VF2-like)** with a cost bound (branch-and-bound, stop early).
* **LP/MILP relaxation + rounding** for moderate sizes.
* **Graph embeddings (node2vec/GNN)** to filter candidates before search.

---

## 8. Expected Outcomes

By following this plan, we will achieve:
- A theoretical and computational foundation for comparing and embedding multigraphs.
- An extendable algorithmic framework integrating subgraph detection, optimization, and counting.
- A modular implementation suitable for research, education, or algorithmic benchmarking.

---

## 8. References

- J. R. Ullmann, “An Algorithm for Subgraph Isomorphism,” *Journal of the ACM*, vol. 23, no. 1, 1976.
- Bunke, H. (1997). “On a Relation Between Graph Edit Distance and Maximum Common Subgraph.” *Pattern Recognition Letters*.
- Cordella et al., “A (Sub)Graph Isomorphism Algorithm for Matching Large Graphs,” *IEEE Trans. PAMI*, 2004.
- De Santo, A., “Graph Edit Distance: Algorithms and Applications,” *Pattern Recognition and Image Analysis*, 2020.
