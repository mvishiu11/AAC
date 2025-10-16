# 🧮 Multigraph Subgraph Matching & Minimal Extension

This project implements **subgraph isomorphism** and **graph extension analysis** for **multigraphs**, with support for:

* Undirected edges with **multiplicities** (parallel edges),
* Checking whether one multigraph $G$ is a **subgraph** of another $H$,
* Computing the **minimal extension** $H'$ of $H$ such that $`G \subseteq H'`$,
* Estimating the **distance** between graphs,
* Counting or enumerating possible **injective embeddings** $`\phi: V(G) \to V(H)`$.

It is written in **Python** with clear modular structure and test coverage, designed to make algorithmic experimentation easy (e.g., testing heuristics or MILP-based approximations later).

---

## 📘 Mathematical Background

Given two undirected **multigraphs**:

* $`G = (V, E)`$
* $`H = (U, F)`$

where:

* $V$, $U$ are finite sets of vertices,
* $E$, $F$ are multisets of unordered pairs representing edges (with multiplicities).

We assume $`|V| \leq |U|`$.

### 1. Graph Size

The **size** of a graph is defined as:
$`|G| = |V| + |E|`$
where $`|E|`$ counts edges with multiplicity (e.g., two parallel edges count as two).
This gives a simple scalar measure of graph “complexity” (useful in normalization of distance).

### 2. Subgraph Relation

We say that $G$ is a **subgraph** of $H$
if there exists an **injective mapping**:
$`\phi: V(G) \to V(H)`$
such that for every pair $`(v_i, v_j) \in V(G)`$:
$`m_G(v_i, v_j) \leq m_H(\phi(v_i), \phi(v_j))`$
where $m_G$ and $m_H$ denote edge multiplicities.

This is equivalent to finding an **injective adjacency-preserving embedding** under multigraph multiplicities.

### 3. Minimal Extension (H')

If $G$ is **not** a subgraph of $H$, we define $H'$ as the **minimal extension** of $H$ for which the subgraph condition holds.
$H'$ is obtained by **adding edges (possibly multiple)** so that:
$G$ is a subgraph $H'$
and
$`|H' \setminus H|`$ is minimal.
We measure this cost as the number of new parallel edges (sum of multiplicity increases).

---

## 🧩 Algorithmic Design

### Overview

This repository contains a small but complete experimental implementation:

1. A **`MultiGraph`** class for undirected multigraphs,
2. A **subgraph isomorphism algorithm** adapted from **Ullmann’s algorithm**,
3. A **branch-and-bound minimal extension** procedure,
4. A **distance metric** based on required edge insertions.

All components are self-contained and easily replaceable by more sophisticated algorithms later (e.g., VF2, A*, or ILP).

---

### 1. Multigraph Representation (`graph.py`)

* Vertices are stored in a list for stable indexing.
* Adjacency is a nested dictionary:

  ```python
  _adj[i][j] = multiplicity
  ```
* Symmetric and supports self-loops.
* `edge_count()` returns the total number of edges counting multiplicities.
* `size()` returns `|V| + |E|`.

Example:

```python
G = MultiGraph.from_edges([
    ("a", "b", 2),
    ("b", "c", 1),
    ("a", "c", 1)
])
```

---

### 2. Subgraph Isomorphism (`ullmann.py`)

#### Purpose

Check if $`G \subseteq H$.

#### Basis

Adapted from **Ullmann’s algorithm** (1976), one of the classic backtracking algorithms for subgraph isomorphism.

#### Steps

1. **Initial candidate matrix** `M[i][j]`:

   * Vertex $v_i$ in $G$ can map to $u_j$ in $H$ only if $`deg_G(i) <= deg_H(j)`$.

2. **Refinement phase**:

   * For every possible mapping $`v_i \to u_j`$, ensure that for each neighbor $`v_k`$, there exists a feasible $`u_l`$ such that
     $`m_H(j, l) \geq m_G(i, k)`$.
   * This prunes inconsistent candidates early.

3. **Backtracking phase**:

   * Vertices are mapped in descending degree order.
   * For each partial mapping, verify that multiplicity constraints are satisfied for already-mapped pairs.
   * If all vertices can be assigned injectively → success.

#### Output

`is_subgraph(G, H)` returns `True` or `False`.

---

### 3. Minimal Extension (`extension.py`)

#### Goal

Find the **smallest number of parallel edges** that must be added to $H$ so that $`G \subseteq H'`$.

#### Method

Branch-and-bound search over injective mappings:

1. For each vertex $v_i \in G$, try to assign a vertex $u_j \in H$.
2. Compute the **deficit**: how many edge multiplicities are missing given the current partial mapping.
3. Recursively explore mappings in degree-descending order.
4. Maintain `best_cost` (current best found); prune branches exceeding it.

#### Output

`minimal_extension(G, H)` → `(cost, mapping, missing_edges)`
where:

* `cost`: minimal total added multiplicity,
* `mapping`: chosen injective vertex assignment,
* `missing_edges`: list of `(i, j, k)` edges to add (indices in $H$).

#### Example

```text
Minimal extension cost: 2
Mapping (G->H indices): {0: 0, 1: 1, 2: 2}
Missing edges: [(0, 1, 1), (0, 2, 1)]
```

Meaning: add one more edge between H[0]-H[1], and one between H[0]-H[2].

---

### 4. Graph Distance (`distance.py`)

Defines a simple **asymmetric injective edge-edit distance**:

$d(G,H) = \text{minimal number of edge insertions needed in } H \text{ to contain } G$

This is equal to the minimal extension cost.

---

## ✅ Verification & Testing

* **Round-trip test:** Verify that extending $H$ by the returned missing edges makes $G$ a subgraph of the new graph.
* **Brute-force oracle:** Enumerate all injective mappings for small graphs to validate the algorithm’s result.
* **Pytest suite:** Confirms correctness for various corner cases.

---

## ⚙️ Running

### Example 1 — Demo (simple triangle vs path)

```bash
python examples/demo.py
```

Output:

```
G: MultiGraph(n=3, m=4) size: 7
H: MultiGraph(n=3, m=2) size: 5
Is G subgraph of H? False
Minimal extension cost: 2
Mapping (G->H indices): {0: 0, 1: 1, 2: 2}
Missing edges: [(0, 1, 1), (0, 2, 1)]
Injective edge-edit distance: 2
```

### Example 2 — Harder case (5 vs 6 vertices)

```bash
python examples/harder_demo.py
```

You’ll see a comparison between the algorithm and a brute-force oracle.
For small graphs (<8 vertices) both match exactly.

---

## 📊 Complexity & Future Work

| Stage               | Description                    | Complexity          | Comments                  |
| ------------------- | ------------------------------ | ------------------- | ------------------------- |
| Candidate filtering | Degree-based pruning           | $`O(\|V_G\| \|V_H\|)`$    | Fast                      |
| Refinement          | Local consistency checks       | $`O(\|V_G\|^2 \|V_H\|^2)`$| OK for small graphs       |
| Backtracking        | Enumerates injective mappings  | $`O(P(\|V_H\|,\|V_G\|))`$ | Exponential worst case    |
| Minimal extension   | Branch-and-bound over mappings | Exponential, pruned | Good up to ~8–10 vertices |
| Distance            | Calls extension                | Same as above       |                           |

### Planned / Possible extensions:

* Implement **heuristic mapping initialization** (Hungarian assignment or greedy degree matching).
* Add **approximation mode** with relaxed pruning for large graphs.
* Integrate **VF2** or **ILP/MILP** solver for better scaling.
* Support **directed multigraphs**.
* Add **counting of all valid subgraphs** (for the full 5.0-level task).

---

## 🎯 Relation to the Task Requirements

| Task Element                                   | Implemented?   | Explanation                                                                        |
| ---------------------------------------------- | -------------- | ---------------------------------------------------------------------------------- |
| Definition of graph size                       | ✅             | Implemented as $`\|V\| + \|E\|`$, justified by simplicity and comparability.         |
| Distance between graphs                        | ✅             | Injective edge-edit distance via minimal extension.                                |
| Subgraph check ($G \subseteq H$)                         | ✅             | Full implementation with multiplicity-aware Ullmann algorithm.                     |
| Minimal extension ($H'$)                         | ✅             | Exact branch-and-bound solution returning added edges.                             |
| Handle multigraphs                             | ✅             | Fully multiplicity-aware, including self-loops.                                    |
| Find all subgraphs                             | ⚙️ *Partially* | Enumeration function implemented but not optimized for counting all subgraphs yet. |
| Approximation/heuristics for exponential cases | 🚧             | Current version exact; heuristics planned.                                         |

So far, the project **fully covers the 3.5-level requirements** (graphs, single matching)
and **mostly covers the 5.0-level** (multigraphs, matching count) — missing only a fast subgraph enumeration and approximation mode.

---

## 🧠 Summary

This repository provides a **correct, verifiable, and extendable foundation** for subgraph-based analysis of multigraphs.

It includes:

* A **clear mathematical formulation**,
* **Exact algorithms** for all key operations,
* Verification via **brute-force oracle**,
* Modular structure ready for **heuristic extensions**.

In short — it’s the **research-grade baseline** implementation from which both efficient and approximate versions can be built.
