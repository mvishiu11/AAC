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

## 7. Expected Outcomes

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
